/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Background recorder developed from the SysDVR-compatible prototype; see upstream SysDVR credit.
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
#include "capture.h"
#include "recorder.h"
#include "config.h"
#include "runtime_config.h"

u32 __nx_applet_type = AppletType_None;
u32 __nx_fs_num_sessions = 2;
u32 __nx_fsdev_direntry_cache_size = 1;

#define INNER_HEAP_SIZE (384 * 1024)
static char g_heap[INNER_HEAP_SIZE];

void __libnx_initheap(void) {
    extern char* fake_heap_start;
    extern char* fake_heap_end;
    fake_heap_start = g_heap;
    fake_heap_end = g_heap + sizeof(g_heap);
}

#define VIDEO_STACK_SIZE 0x6000
#define AUDIO_STACK_SIZE 0x6000
#define WRITER_STACK_SIZE 0x10000
#define CONTROL_STACK_SIZE 0x3000
#define THREAD_PRIORITY 44
#define WRITER_PRIORITY 43
#define THREAD_CPU 3

typedef struct {
    u64 timestamp_us;
    u32 size;
    u32 generation;
    bool keyframe;
    u8 data[VIDEO_BUFFER_SIZE];
} VideoQueueSlot;

typedef struct {
    u64 timestamp_us;
    u32 size;
    u32 generation;
    u8 data[AUDIO_BUFFER_SIZE];
} AudioQueueSlot;

static Thread g_video_thread;
static Thread g_audio_thread;
static Thread g_writer_thread;
static Thread g_control_thread;
static u8 alignas(0x1000) g_video_stack[VIDEO_STACK_SIZE];
static u8 alignas(0x1000) g_audio_stack[AUDIO_STACK_SIZE];
static u8 alignas(0x1000) g_writer_stack[WRITER_STACK_SIZE];
static u8 alignas(0x1000) g_control_stack[CONTROL_STACK_SIZE];

static VideoQueueSlot alignas(0x1000) g_video_queue[VIDEO_QUEUE_SLOTS];
static AudioQueueSlot alignas(0x1000) g_audio_queue[AUDIO_QUEUE_SLOTS];
static Mutex g_video_q_lock;
static Mutex g_audio_q_lock;
static u32 g_video_q_head = 0, g_video_q_tail = 0, g_video_q_count = 0;
static u32 g_audio_q_head = 0, g_audio_q_tail = 0, g_audio_q_count = 0;
static volatile u32 g_generation = 1;
static volatile bool g_capture_enabled = false;
static volatile bool g_running = true;
static volatile bool g_need_video_keyframe = true;
static bool g_sd_mounted = false;

/* Shared A/V timeline. Video is authoritative across HOME/system-UI gaps.
   While video waits for a clean game IDR, audio is discarded too. When the
   IDR arrives, both streams resume from one common timestamp offset. */
static Mutex g_timeline_lock;
static bool g_timeline_started = false;
static volatile bool g_timeline_rebase_pending = true;
static u64 g_timeline_offset_us = 0;
static u64 g_resume_raw_ts = 0;
static u64 g_last_video_raw_ts = 0;
static u64 g_last_audio_raw_ts = 0;
static u64 g_last_video_out_ts = 0;
static u64 g_last_audio_out_end_ts = 0;
static volatile u64 g_diag_video_timeline_gaps = 0;
static volatile u64 g_diag_audio_timeline_gaps = 0;
static volatile u64 g_diag_timeline_rebases = 0;
static volatile u64 g_diag_timeline_gap_us_removed = 0;
static volatile u64 g_diag_home_placeholder_keyframes = 0;
static volatile u64 g_diag_audio_held_for_video_resync = 0;
static volatile u64 g_diag_markers_added = 0;

/* Per-recording diagnostics. Each counter has one main writer thread except max queue use. */
static volatile u64 g_diag_video_captured = 0;
static volatile u64 g_diag_audio_captured = 0;
static volatile u64 g_diag_video_capture_failures = 0;
static volatile u64 g_diag_audio_capture_failures = 0;
static volatile u64 g_diag_video_queue_overflows = 0;
static volatile u64 g_diag_audio_queue_overflows = 0;
static volatile u64 g_diag_video_resync_events = 0;
static volatile u64 g_diag_video_resync_drops = 0;
static volatile u64 g_diag_video_resync_recoveries = 0;
static volatile u32 g_diag_video_max_queue = 0;
static volatile u32 g_diag_audio_max_queue = 0;

static bool control_file_exists(void) {
    struct stat st;
    return stat(CONTROL_FILE, &st) == 0;
}

static void reset_diagnostics(void) {
    g_diag_video_captured = 0;
    g_diag_audio_captured = 0;
    g_diag_video_capture_failures = 0;
    g_diag_audio_capture_failures = 0;
    g_diag_video_queue_overflows = 0;
    g_diag_audio_queue_overflows = 0;
    g_diag_video_resync_events = 0;
    g_diag_video_resync_drops = 0;
    g_diag_video_resync_recoveries = 0;
    g_diag_video_max_queue = 0;
    g_diag_audio_max_queue = 0;
    g_diag_video_timeline_gaps = 0;
    g_diag_audio_timeline_gaps = 0;
    g_diag_timeline_rebases = 0;
    g_diag_timeline_gap_us_removed = 0;
    g_diag_home_placeholder_keyframes = 0;
    g_diag_audio_held_for_video_resync = 0;
    g_diag_markers_added = 0;
    g_timeline_started = false;
    g_timeline_rebase_pending = true;
    g_timeline_offset_us = 0;
    g_resume_raw_ts = 0;
    g_last_video_raw_ts = 0;
    g_last_audio_raw_ts = 0;
    g_last_video_out_ts = 0;
    g_last_audio_out_end_ts = 0;
}

static void write_diagnostics(void) {
    const unsigned level = runtimeConfigDiagnosticsLevel();
    if (level == 0u) {
        remove(DIAGNOSTICS_FILE);
        return;
    }

    (void)mkdir(RECORD_DIR, 0777);
    FILE* f = fopen(DIAGNOSTICS_FILE, "w");
    if (!f) return;

    fprintf(f, "NXCR diagnostics\n");
    fprintf(f, "version=%s\n", NXCR_VERSION);
    fprintf(f, "diagnostics_level=%u\n", level);
    fprintf(f, "segment_minutes=%u\n", runtimeConfigSegmentMinutes());
    fprintf(f, "min_free_mib=%u\n", runtimeConfigMinFreeMiB());
    fprintf(f, "audio_enabled=%u\n", runtimeConfigAudioEnabled() ? 1u : 0u);

    fprintf(f, "video_capture_failures=%llu\n", (unsigned long long)g_diag_video_capture_failures);
    fprintf(f, "audio_capture_failures=%llu\n", (unsigned long long)g_diag_audio_capture_failures);
    fprintf(f, "video_queue_overflows=%llu\n", (unsigned long long)g_diag_video_queue_overflows);
    fprintf(f, "audio_queue_overflows=%llu\n", (unsigned long long)g_diag_audio_queue_overflows);
    fprintf(f, "video_timeline_gaps=%llu\n", (unsigned long long)g_diag_video_timeline_gaps);
    fprintf(f, "audio_timeline_gaps=%llu\n", (unsigned long long)g_diag_audio_timeline_gaps);
    fprintf(f, "timeline_rebases=%llu\n", (unsigned long long)g_diag_timeline_rebases);
    fprintf(f, "timeline_gap_ms_removed=%llu\n",
            (unsigned long long)(g_diag_timeline_gap_us_removed / 1000ULL));
    fprintf(f, "home_placeholder_keyframes=%llu\n",
            (unsigned long long)g_diag_home_placeholder_keyframes);
    fprintf(f, "markers_added=%llu\n",
            (unsigned long long)g_diag_markers_added);

    if (level >= 2u) {
        fprintf(f, "video_captured=%llu\n", (unsigned long long)g_diag_video_captured);
        fprintf(f, "audio_batches_captured=%llu\n", (unsigned long long)g_diag_audio_captured);
        fprintf(f, "video_resync_events=%llu\n", (unsigned long long)g_diag_video_resync_events);
        fprintf(f, "video_resync_predictive_frames_dropped=%llu\n",
                (unsigned long long)g_diag_video_resync_drops);
        fprintf(f, "video_resync_recoveries=%llu\n",
                (unsigned long long)g_diag_video_resync_recoveries);
        fprintf(f, "video_max_queue_used=%u/%u\n",
                (unsigned)g_diag_video_max_queue, (unsigned)VIDEO_QUEUE_SLOTS);
        fprintf(f, "audio_max_queue_used=%u/%u\n",
                (unsigned)g_diag_audio_max_queue, (unsigned)AUDIO_QUEUE_SLOTS);
        fprintf(f, "audio_batches_held_for_video_resync=%llu\n",
                (unsigned long long)g_diag_audio_held_for_video_resync);
    }

    fprintf(f, "\nInterpretation:\n");
    fprintf(f, "- capture_failures: Nintendo grc:d did not provide a usable packet.\n");
    fprintf(f, "- queue_overflows: the SD writer could not drain RAM fast enough.\n");
    fprintf(f, "- timeline_gap_ms_removed: HOME/system-UI time removed to keep A/V synchronized.\n");
    fclose(f);
}

static void enter_video_resync(void) {
    if (!g_need_video_keyframe) {
        g_need_video_keyframe = true;
        g_timeline_rebase_pending = true;
        ++g_diag_video_resync_events;
    }
}

static bool enqueue_video(const VideoFrame* frame, u32 generation) {
    bool ok = false;
    mutexLock(&g_video_q_lock);
    if (g_video_q_count < VIDEO_QUEUE_SLOTS) {
        VideoQueueSlot* slot = &g_video_queue[g_video_q_head];
        slot->timestamp_us = frame->timestamp_us;
        slot->size = frame->size;
        slot->generation = generation;
        slot->keyframe = frame->keyframe;
        memcpy(slot->data, frame->data, frame->size);
        g_video_q_head = (g_video_q_head + 1u) % VIDEO_QUEUE_SLOTS;
        ++g_video_q_count;
        if (g_video_q_count > g_diag_video_max_queue) g_diag_video_max_queue = g_video_q_count;
        ok = true;
    }
    mutexUnlock(&g_video_q_lock);
    return ok;
}

static bool enqueue_audio(const AudioFrame* frame, u32 generation) {
    bool ok = false;
    mutexLock(&g_audio_q_lock);
    if (g_audio_q_count < AUDIO_QUEUE_SLOTS) {
        AudioQueueSlot* slot = &g_audio_queue[g_audio_q_head];
        slot->timestamp_us = frame->timestamp_us;
        slot->size = frame->size;
        slot->generation = generation;
        memcpy(slot->data, frame->data, frame->size);
        g_audio_q_head = (g_audio_q_head + 1u) % AUDIO_QUEUE_SLOTS;
        ++g_audio_q_count;
        if (g_audio_q_count > g_diag_audio_max_queue) g_diag_audio_max_queue = g_audio_q_count;
        ok = true;
    }
    mutexUnlock(&g_audio_q_lock);
    return ok;
}

static bool video_tail_info(u64* ts, u32* generation) {
    bool have = false;
    mutexLock(&g_video_q_lock);
    if (g_video_q_count) {
        *ts = g_video_queue[g_video_q_tail].timestamp_us;
        *generation = g_video_queue[g_video_q_tail].generation;
        have = true;
    }
    mutexUnlock(&g_video_q_lock);
    return have;
}

static bool audio_tail_info(u64* ts, u32* generation) {
    bool have = false;
    mutexLock(&g_audio_q_lock);
    if (g_audio_q_count) {
        *ts = g_audio_queue[g_audio_q_tail].timestamp_us;
        *generation = g_audio_queue[g_audio_q_tail].generation;
        have = true;
    }
    mutexUnlock(&g_audio_q_lock);
    return have;
}

static void consume_video(void) {
    VideoQueueSlot* slot;
    mutexLock(&g_video_q_lock);
    if (!g_video_q_count) {
        mutexUnlock(&g_video_q_lock);
        return;
    }
    slot = &g_video_queue[g_video_q_tail];
    mutexUnlock(&g_video_q_lock);

    if (slot->generation == g_generation) {
        VideoFrame frame = {slot->data, slot->size, slot->timestamp_us, slot->keyframe};
        recorderPushVideo(&frame);
    }

    mutexLock(&g_video_q_lock);
    g_video_q_tail = (g_video_q_tail + 1u) % VIDEO_QUEUE_SLOTS;
    --g_video_q_count;
    mutexUnlock(&g_video_q_lock);
}

static void consume_audio(void) {
    AudioQueueSlot* slot;
    mutexLock(&g_audio_q_lock);
    if (!g_audio_q_count) {
        mutexUnlock(&g_audio_q_lock);
        return;
    }
    slot = &g_audio_queue[g_audio_q_tail];
    mutexUnlock(&g_audio_q_lock);

    if (slot->generation == g_generation) {
        AudioFrame frame = {slot->data, slot->size, slot->timestamp_us};
        recorderPushAudio(&frame);
    }

    mutexLock(&g_audio_q_lock);
    g_audio_q_tail = (g_audio_q_tail + 1u) % AUDIO_QUEUE_SLOTS;
    --g_audio_q_count;
    mutexUnlock(&g_audio_q_lock);
}

static u64 audio_duration_us(const AudioFrame* frame) {
    const u64 bytes_per_sec = (u64)AUDIO_SAMPLE_RATE * AUDIO_CHANNELS * (AUDIO_BITS_PER_SAMPLE / 8u);
    return bytes_per_sec ? ((u64)frame->size * 1000000ULL) / bytes_per_sec : 0;
}

static void video_thread(void* arg) {
    (void)arg;
    VideoFrame frame;
    while (g_running) {
        if (captureReadVideo(&frame)) {
            if (!g_capture_enabled) continue;
            ++g_diag_video_captured;

            const u64 raw_ts = frame.timestamp_us;
            bool raw_gap = false;

            mutexLock(&g_timeline_lock);
            if (g_last_video_raw_ts && raw_ts > g_last_video_raw_ts &&
                raw_ts - g_last_video_raw_ts > TIMELINE_GAP_THRESHOLD_US) {
                raw_gap = true;
                ++g_diag_video_timeline_gaps;
            }
            g_last_video_raw_ts = raw_ts;
            mutexUnlock(&g_timeline_lock);

            if (raw_gap) enter_video_resync();

            /* HOME/system UI can emit tiny black IDRs instead of stopping the
               stream entirely. Treat those as a discontinuity, never as video. */
            if (frame.keyframe && frame.size < ACTIVE_KEYFRAME_MIN_BYTES) {
                ++g_diag_home_placeholder_keyframes;
                enter_video_resync();
                ++g_diag_video_resync_drops;
                continue;
            }

            if (g_need_video_keyframe) {
                if (!frame.keyframe || frame.size < ACTIVE_KEYFRAME_MIN_BYTES) {
                    ++g_diag_video_resync_drops;
                    continue;
                }

                mutexLock(&g_timeline_lock);
                u64 target_ts = 0;
                if (!g_timeline_started) {
                    /* First real game frame: recording time starts here. */
                    g_timeline_started = true;
                    target_ts = 0;
                } else {
                    const u64 video_next = g_last_video_out_ts + (1000000ULL / VIDEO_FPS);
                    target_ts = video_next;
                    if (g_last_audio_out_end_ts > target_ts)
                        target_ts = g_last_audio_out_end_ts;

                    /* Measure how much dead HOME/resync time was removed. */
                    if (raw_ts >= g_timeline_offset_us) {
                        const u64 old_projected = raw_ts - g_timeline_offset_us;
                        if (old_projected > target_ts)
                            g_diag_timeline_gap_us_removed += old_projected - target_ts;
                    }
                    ++g_diag_timeline_rebases;
                }

                g_timeline_offset_us = raw_ts >= target_ts ? raw_ts - target_ts : 0;
                g_resume_raw_ts = raw_ts;
                frame.timestamp_us = target_ts;

                const bool ok = enqueue_video(&frame, g_generation);
                if (ok) {
                    g_last_video_out_ts = target_ts;
                    g_need_video_keyframe = false;
                    g_timeline_rebase_pending = false;
                    ++g_diag_video_resync_recoveries;
                }
                mutexUnlock(&g_timeline_lock);

                if (!ok) {
                    ++g_diag_video_queue_overflows;
                    /* Keep waiting for another clean game IDR. */
                }
                continue;
            }

            mutexLock(&g_timeline_lock);
            if (!g_timeline_started) {
                mutexUnlock(&g_timeline_lock);
                continue;
            }

            u64 out_ts = raw_ts >= g_timeline_offset_us ? raw_ts - g_timeline_offset_us
                                                        : g_last_video_out_ts + (1000000ULL / VIDEO_FPS);
            frame.timestamp_us = out_ts;
            const bool ok = enqueue_video(&frame, g_generation);
            if (ok) g_last_video_out_ts = out_ts;
            mutexUnlock(&g_timeline_lock);

            if (!ok) {
                ++g_diag_video_queue_overflows;
                enter_video_resync();
            }
        } else {
            if (g_capture_enabled) {
                ++g_diag_video_capture_failures;
                enter_video_resync();
            }
            svcSleepThread(2 * 1000 * 1000LL);
        }
    }
}

static void audio_thread(void* arg) {
    (void)arg;
    AudioFrame frame;
    while (g_running) {
        if (captureReadAudio(&frame)) {
            if (!g_capture_enabled) continue;
            if (!runtimeConfigAudioEnabled()) continue;
            ++g_diag_audio_captured;

            const u64 raw_ts = frame.timestamp_us;

            /* Audio gaps are diagnostic only. Video owns discontinuity/resume so
               staggered audio return cannot force a second resync after video. */
            mutexLock(&g_timeline_lock);
            if (g_last_audio_raw_ts && raw_ts > g_last_audio_raw_ts &&
                raw_ts - g_last_audio_raw_ts > TIMELINE_GAP_THRESHOLD_US) {
                ++g_diag_audio_timeline_gaps;
            }
            g_last_audio_raw_ts = raw_ts;

            if (!g_timeline_started || g_need_video_keyframe || raw_ts < g_resume_raw_ts) {
                mutexUnlock(&g_timeline_lock);
                ++g_diag_audio_held_for_video_resync;
                continue;
            }

            u64 out_ts = raw_ts >= g_timeline_offset_us ? raw_ts - g_timeline_offset_us : 0;
            if (out_ts < g_last_audio_out_end_ts)
                out_ts = g_last_audio_out_end_ts;

            frame.timestamp_us = out_ts;
            const bool ok = enqueue_audio(&frame, g_generation);
            if (ok)
                g_last_audio_out_end_ts = out_ts + audio_duration_us(&frame);
            mutexUnlock(&g_timeline_lock);

            if (!ok) ++g_diag_audio_queue_overflows;
        } else {
            if (g_capture_enabled) ++g_diag_audio_capture_failures;
            svcSleepThread(2 * 1000 * 1000LL);
        }
    }
}

static void writer_thread(void* arg) {
    (void)arg;
    while (g_running) {
        u64 vts = 0, ats = 0;
        u32 vgen = 0, agen = 0;
        bool hv = video_tail_info(&vts, &vgen);
        bool ha = audio_tail_info(&ats, &agen);
        (void)vgen;
        (void)agen;

        if (hv && ha) {
            if (vts <= ats) consume_video();
            else consume_audio();
        } else if (hv) {
            consume_video();
        } else if (ha) {
            consume_audio();
        } else {
            svcSleepThread(1 * 1000 * 1000LL);
        }
    }

    for (;;) {
        u64 vts = 0, ats = 0;
        u32 vgen = 0, agen = 0;
        bool hv = video_tail_info(&vts, &vgen);
        bool ha = audio_tail_info(&ats, &agen);
        if (!hv && !ha) break;
        if (hv && (!ha || vts <= ats)) consume_video();
        else consume_audio();
    }
}


static bool take_request_file(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return false;
    remove(path);
    return true;
}

static void write_marker_result(bool ok, uint64_t rel_ms, unsigned index, const char* reason) {
    FILE* f = fopen(MARKER_RESULT_FILE, "wb");
    if (!f) return;
    fprintf(f, "ok=%u\n", ok ? 1u : 0u);
    fprintf(f, "index=%u\n", index);
    fprintf(f, "time_ms=%llu\n", (unsigned long long)rel_ms);
    fprintf(f, "reason=%s\n", reason ? reason : "");
    fclose(f);
}

static void handle_marker_request(void) {
    uint64_t timeline_us = 0;

    mutexLock(&g_timeline_lock);
    const bool can_mark = g_capture_enabled && g_timeline_started &&
                          !g_need_video_keyframe && g_last_video_out_ts > 0;
    if (can_mark) timeline_us = g_last_video_out_ts;
    mutexUnlock(&g_timeline_lock);

    if (!can_mark) {
        write_marker_result(false, 0, 0, "not-recording");
        return;
    }

    uint64_t rel_ms = 0;
    unsigned index = 0;
    if (recorderAddMarker(timeline_us, &rel_ms, &index)) {
        ++g_diag_markers_added;
        write_marker_result(true, rel_ms, index, "ok");
    } else {
        write_marker_result(false, 0, 0, "no-open-file");
    }
}

static void write_status_snapshot(void) {
    u32 video_q = 0;
    u32 audio_q = 0;

    mutexLock(&g_video_q_lock);
    video_q = g_video_q_count;
    mutexUnlock(&g_video_q_lock);

    mutexLock(&g_audio_q_lock);
    audio_q = g_audio_q_count;
    mutexUnlock(&g_audio_q_lock);

    uint64_t media_us = 0;
    bool timeline_started = false;
    bool recovering = false;
    mutexLock(&g_timeline_lock);
    media_us = g_last_video_out_ts;
    timeline_started = g_timeline_started;
    recovering = g_need_video_keyframe;
    mutexUnlock(&g_timeline_lock);

    struct stat active_st;
    const bool active = stat(ACTIVE_FILE, &active_st) == 0;

    FILE* f = fopen(STATUS_TEMP_FILE, "wb");
    if (!f) return;

    fprintf(f, "version=%s\n", NXCR_VERSION);
    fprintf(f, "state=%s\n",
            active ? (recovering ? "Recovering" : "Recording")
                   : (g_capture_enabled ? "Armed" : "Off"));
    fprintf(f, "media_ms=%llu\n", (unsigned long long)(media_us / 1000ULL));
    fprintf(f, "timeline_started=%u\n", timeline_started ? 1u : 0u);
    fprintf(f, "video_queue=%u\n", (unsigned)video_q);
    fprintf(f, "video_queue_capacity=%u\n", (unsigned)VIDEO_QUEUE_SLOTS);
    fprintf(f, "audio_queue=%u\n", (unsigned)audio_q);
    fprintf(f, "audio_queue_capacity=%u\n", (unsigned)AUDIO_QUEUE_SLOTS);
    fprintf(f, "video_queue_overflows=%llu\n",
            (unsigned long long)g_diag_video_queue_overflows);
    fprintf(f, "audio_queue_overflows=%llu\n",
            (unsigned long long)g_diag_audio_queue_overflows);
    fprintf(f, "video_capture_failures=%llu\n",
            (unsigned long long)g_diag_video_capture_failures);
    fprintf(f, "audio_capture_failures=%llu\n",
            (unsigned long long)g_diag_audio_capture_failures);
    fprintf(f, "resync_events=%llu\n",
            (unsigned long long)g_diag_video_resync_events);
    fprintf(f, "timeline_rebases=%llu\n",
            (unsigned long long)g_diag_timeline_rebases);
    fprintf(f, "home_placeholder_keyframes=%llu\n",
            (unsigned long long)g_diag_home_placeholder_keyframes);
    fprintf(f, "markers=%llu\n",
            (unsigned long long)g_diag_markers_added);
    fclose(f);

    remove(STATUS_FILE);
    rename(STATUS_TEMP_FILE, STATUS_FILE);
}

static void control_thread(void* arg) {
    (void)arg;
    bool first = true;
    bool last = false;
    unsigned config_poll_ticks = 0;
    while (g_running) {
        if (take_request_file(MARKER_REQUEST_FILE))
            handle_marker_request();

        if (take_request_file(STATUS_REQUEST_FILE))
            write_status_snapshot();

        if (++config_poll_ticks >= 20u) {
            config_poll_ticks = 0;
            struct stat cfgst;
            if (stat(CONFIG_RELOAD_FILE, &cfgst) == 0) {
                remove(CONFIG_RELOAD_FILE);
                (void)runtimeConfigReload();
            }
        }

        bool enabled = control_file_exists();
        if (first || enabled != last) {
            g_capture_enabled = false;
            ++g_generation;

            if (enabled) {
                reset_diagnostics();
                g_need_video_keyframe = true;
                recorderSetEnabled(true);
                g_capture_enabled = true;
            } else {
                recorderSetEnabled(false);
                if (!first && last) write_diagnostics();
                g_need_video_keyframe = true;
            }

            last = enabled;
            first = false;
        }
        svcSleepThread(100LL * 1000 * 1000);
    }
}

void __appInit(void) {
    svcSleepThread(20LL * 1000 * 1000 * 1000);
    Result rc = smInitialize();
    if (R_FAILED(rc)) fatalThrow(rc);
    rc = fsInitialize();
    if (R_FAILED(rc)) fatalThrow(rc);
    rc = fsdevMountSdmc();
    if (R_FAILED(rc)) fatalThrow(rc);
    g_sd_mounted = true;
}

void __appExit(void) {
    g_running = false;
    recorderExit();
    captureExit();
    if (g_sd_mounted) fsdevUnmountDevice("sdmc");
    fsExit();
    smExit();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    mutexInit(&g_video_q_lock);
    mutexInit(&g_audio_q_lock);
    mutexInit(&g_timeline_lock);
    (void)runtimeConfigInit();
    if (!recorderInitialize()) return 1;
    Result rc = captureInitialize();
    if (R_FAILED(rc)) return 2;

    rc = threadCreate(&g_video_thread, video_thread, NULL, g_video_stack, sizeof(g_video_stack), THREAD_PRIORITY, THREAD_CPU);
    if (R_FAILED(rc)) return 3;
    rc = threadCreate(&g_audio_thread, audio_thread, NULL, g_audio_stack, sizeof(g_audio_stack), THREAD_PRIORITY, THREAD_CPU);
    if (R_FAILED(rc)) {
        threadClose(&g_video_thread);
        return 4;
    }
    rc = threadCreate(&g_writer_thread, writer_thread, NULL, g_writer_stack, sizeof(g_writer_stack), WRITER_PRIORITY, THREAD_CPU);
    if (R_FAILED(rc)) {
        threadClose(&g_audio_thread);
        threadClose(&g_video_thread);
        return 5;
    }
    rc = threadCreate(&g_control_thread, control_thread, NULL, g_control_stack, sizeof(g_control_stack), THREAD_PRIORITY, THREAD_CPU);
    if (R_FAILED(rc)) {
        threadClose(&g_writer_thread);
        threadClose(&g_audio_thread);
        threadClose(&g_video_thread);
        return 6;
    }

    threadStart(&g_video_thread);
    threadStart(&g_audio_thread);
    threadStart(&g_writer_thread);
    threadStart(&g_control_thread);

    while (g_running) svcSleepThread(1000LL * 1000 * 1000);

    threadWaitForExit(&g_control_thread);
    threadWaitForExit(&g_video_thread);
    threadWaitForExit(&g_audio_thread);
    threadWaitForExit(&g_writer_thread);
    threadClose(&g_control_thread);
    threadClose(&g_writer_thread);
    threadClose(&g_audio_thread);
    threadClose(&g_video_thread);
    return 0;
}
