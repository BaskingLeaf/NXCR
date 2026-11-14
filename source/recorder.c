/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Local-to-SD recording implementation from the recorder prototype and later NXCR changes.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#include <switch.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "recorder.h"
#include "mkv.h"
#include "config.h"
#include "runtime_config.h"

static Mutex g_lock;
static MkvWriter g_mkv;
static bool g_open = false;
static bool g_enabled = false;
static uint64_t g_base_us = 0;
static uint64_t g_last_space_check_ms = 0;
static unsigned g_next_index = 1;
static char g_current_path[256];
static unsigned g_marker_count = 0;

static void set_active_marker(bool active, const char* path) {
    if (active) {
        FILE* f = fopen(ACTIVE_FILE, "wb");
        if (f) {
            fprintf(f, "path=%s\n", path ? path : "");
            fprintf(f, "start_tick=%llu\n", (unsigned long long)armGetSystemTick());
            fclose(f);
        }
    } else {
        remove(ACTIVE_FILE);
    }
}

static bool enough_space(void) {
    struct statvfs vfs;
    if (statvfs("sdmc:/", &vfs) != 0) return true;
    uint64_t free_bytes = (uint64_t)vfs.f_bavail * (uint64_t)vfs.f_frsize;
    const uint64_t min_bytes = (uint64_t)runtimeConfigMinFreeMiB() * 1024ULL * 1024ULL;
    return free_bytes >= min_bytes;
}

static void ensure_dir(void) {
    mkdir("sdmc:/switch", 0777);
    mkdir(RECORD_DIR, 0777);
    mkdir("sdmc:/Videos", 0777);
    mkdir(VIDEO_DIR, 0777);
}

static unsigned find_next_index(void) {
    char path[256];
    for (unsigned i = 1; i < 1000000u; ++i) {
        snprintf(path, sizeof(path), VIDEO_DIR "/recording_%06u.mkv", i);
        FILE* f = fopen(path, "rb");
        if (!f) return i;
        fclose(f);
    }
    return 1;
}

static bool open_new_locked(uint64_t base_us) {
    if (!g_enabled || !enough_space()) return false;

    snprintf(g_current_path, sizeof(g_current_path), VIDEO_DIR "/recording_%06u.mkv", g_next_index++);
    if (!mkvOpen(&g_mkv, g_current_path)) {
        g_current_path[0] = '\0';
        return false;
    }

    g_base_us = base_us;
    g_last_space_check_ms = 0;
    g_marker_count = 0;
    g_open = true;
    set_active_marker(true, g_current_path);
    return true;
}

static void close_locked(void) {
    if (g_open) mkvClose(&g_mkv);
    g_open = false;
    g_base_us = 0;
    g_current_path[0] = '\0';
    set_active_marker(false, NULL);
}

static bool periodic_space_check_locked(uint64_t rel_ms) {
    if (rel_ms < g_last_space_check_ms || rel_ms - g_last_space_check_ms >= 60000u) {
        g_last_space_check_ms = rel_ms;
        if (!enough_space()) {
            close_locked();
            return false;
        }
    }
    return true;
}

bool recorderInitialize(void) {
    mutexInit(&g_lock);
    ensure_dir();
    remove(ACTIVE_FILE);
    g_next_index = find_next_index();
    g_enabled = false;
    g_current_path[0] = '\0';
    return true;
}

void recorderExit(void) {
    mutexLock(&g_lock);
    g_enabled = false;
    close_locked();
    mutexUnlock(&g_lock);
}

void recorderSetEnabled(bool enabled) {
    mutexLock(&g_lock);
    if (g_enabled != enabled) {
        g_enabled = enabled;
        if (!enabled) close_locked();
    }
    mutexUnlock(&g_lock);
}

bool recorderIsEnabled(void) {
    bool enabled;
    mutexLock(&g_lock);
    enabled = g_enabled;
    mutexUnlock(&g_lock);
    return enabled;
}

void recorderPushVideo(const VideoFrame* frame) {
    if (!frame || !frame->data || !frame->size) return;
    mutexLock(&g_lock);

    if (!g_enabled) {
        mutexUnlock(&g_lock);
        return;
    }

    if (!g_open) {
        if (!frame->keyframe || !open_new_locked(frame->timestamp_us)) {
            mutexUnlock(&g_lock);
            return;
        }
    }

    if (frame->timestamp_us < g_base_us) {
        mutexUnlock(&g_lock);
        return;
    }

    uint64_t rel_ms = (frame->timestamp_us - g_base_us) / 1000ULL;
    const unsigned segment_minutes = runtimeConfigSegmentMinutes();

    if (segment_minutes != 0u) {
        const uint64_t rotate_ms = (uint64_t)segment_minutes * 60ULL * 1000ULL;
        if (frame->keyframe && rel_ms >= rotate_ms) {
            close_locked();
            if (!open_new_locked(frame->timestamp_us)) {
                mutexUnlock(&g_lock);
                return;
            }
            rel_ms = 0;
        }
    }

    if (!periodic_space_check_locked(rel_ms)) {
        mutexUnlock(&g_lock);
        return;
    }

    if (!mkvWriteVideo(&g_mkv, rel_ms, frame->data, frame->size, frame->keyframe)) close_locked();
    mutexUnlock(&g_lock);
}

void recorderPushAudio(const AudioFrame* frame) {
    if (!frame || !frame->data || !frame->size) return;
    if (!runtimeConfigAudioEnabled()) return;

    mutexLock(&g_lock);
    if (!g_enabled || !g_open || frame->timestamp_us < g_base_us) {
        mutexUnlock(&g_lock);
        return;
    }

    const uint64_t rel_ms = (frame->timestamp_us - g_base_us) / 1000ULL;
    if (!mkvWriteAudio(&g_mkv, rel_ms, frame->data, frame->size)) close_locked();
    mutexUnlock(&g_lock);
}

static void marker_path_locked(char* out, size_t out_size) {
    if (!out || !out_size) return;
    snprintf(out, out_size, "%s", g_current_path);
    size_t len = strlen(out);
    if (len >= 4 && strcmp(out + len - 4, ".mkv") == 0)
        snprintf(out + len - 4, out_size - (len - 4), ".markers.txt");
    else if (len + 12 < out_size)
        strcat(out, ".markers.txt");
}

bool recorderAddMarker(uint64_t timeline_us, uint64_t* out_rel_ms, unsigned* out_index) {
    bool ok = false;
    mutexLock(&g_lock);

    if (g_open && timeline_us >= g_base_us && g_current_path[0]) {
        const uint64_t rel_ms = (timeline_us - g_base_us) / 1000ULL;
        char path[320];
        marker_path_locked(path, sizeof(path));

        const bool new_file = access(path, F_OK) != 0;
        FILE* f = fopen(path, "ab");
        if (f) {
            if (new_file) {
                fprintf(f, "# NXCR markers\n");
                fprintf(f, "# video=%s\n", g_current_path);
                fprintf(f, "# index,time_ms,time\n");
            }

            const unsigned index = g_marker_count + 1u;
            const uint64_t total_sec = rel_ms / 1000ULL;
            const unsigned ms = (unsigned)(rel_ms % 1000ULL);
            const unsigned sec = (unsigned)(total_sec % 60ULL);
            const unsigned min = (unsigned)((total_sec / 60ULL) % 60ULL);
            const unsigned hour = (unsigned)(total_sec / 3600ULL);

            fprintf(f, "%u,%llu,%02u:%02u:%02u.%03u\n",
                    index,
                    (unsigned long long)rel_ms,
                    hour, min, sec, ms);
            if (fclose(f) == 0) {
                g_marker_count = index;
                if (out_rel_ms) *out_rel_ms = rel_ms;
                if (out_index) *out_index = index;
                ok = true;
            }
        }
    }

    mutexUnlock(&g_lock);
    return ok;
}

unsigned recorderMarkerCount(void) {
    unsigned count;
    mutexLock(&g_lock);
    count = g_marker_count;
    mutexUnlock(&g_lock);
    return count;
}
