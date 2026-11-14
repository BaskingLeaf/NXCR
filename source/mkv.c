/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Local-to-SD MKV implementation from the original recorder prototype and later NXCR changes.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "mkv.h"
#include "config.h"

typedef struct {
    uint8_t data[2048];
    size_t len;
    bool failed;
} ByteBuf;

static bool bb_put(ByteBuf* b, const void* p, size_t n) {
    if (b->failed || b->len + n > sizeof(b->data)) {
        b->failed = true;
        return false;
    }
    memcpy(b->data + b->len, p, n);
    b->len += n;
    return true;
}

static bool bb_u8(ByteBuf* b, uint8_t v) { return bb_put(b, &v, 1); }

static size_t id_bytes(uint32_t id) {
    if (id <= 0xffu) return 1;
    if (id <= 0xffffu) return 2;
    if (id <= 0xffffffu) return 3;
    return 4;
}

static bool bb_id(ByteBuf* b, uint32_t id) {
    size_t n = id_bytes(id);
    uint8_t out[4];
    for (size_t i = 0; i < n; ++i) out[n - 1 - i] = (uint8_t)(id >> (i * 8));
    return bb_put(b, out, n);
}

static size_t vint_size_len(uint64_t value) {
    for (size_t n = 1; n <= 8; ++n) {
        uint64_t max = (n == 8) ? ((1ULL << 56) - 2) : ((1ULL << (7 * n)) - 2);
        if (value <= max) return n;
    }
    return 0;
}

static bool bb_size(ByteBuf* b, uint64_t value) {
    size_t n = vint_size_len(value);
    if (!n) return false;
    uint64_t encoded = value | (1ULL << (7 * n));
    uint8_t out[8];
    for (size_t i = 0; i < n; ++i) out[n - 1 - i] = (uint8_t)(encoded >> (i * 8));
    return bb_put(b, out, n);
}

static size_t uint_payload_len(uint64_t value) {
    size_t n = 1;
    while (n < 8 && value >= (1ULL << (n * 8))) ++n;
    return n;
}

static bool bb_uint(ByteBuf* b, uint32_t id, uint64_t value) {
    size_t n = uint_payload_len(value);
    if (!bb_id(b, id) || !bb_size(b, n)) return false;
    uint8_t out[8];
    for (size_t i = 0; i < n; ++i) out[n - 1 - i] = (uint8_t)(value >> (i * 8));
    return bb_put(b, out, n);
}

static bool bb_string(ByteBuf* b, uint32_t id, const char* s) {
    size_t n = strlen(s);
    return bb_id(b, id) && bb_size(b, n) && bb_put(b, s, n);
}

static bool bb_binary(ByteBuf* b, uint32_t id, const void* p, size_t n) {
    return bb_id(b, id) && bb_size(b, n) && bb_put(b, p, n);
}

static bool bb_float64(ByteBuf* b, uint32_t id, double value) {
    union { double d; uint64_t u; } v;
    v.d = value;
    uint8_t out[8];
    for (size_t i = 0; i < 8; ++i) out[7 - i] = (uint8_t)(v.u >> (i * 8));
    return bb_id(b, id) && bb_size(b, 8) && bb_put(b, out, 8);
}

static bool bb_master(ByteBuf* dst, uint32_t id, const ByteBuf* child) {
    return !child->failed && bb_id(dst, id) && bb_size(dst, child->len) && bb_put(dst, child->data, child->len);
}

static bool file_put(MkvWriter* w, const void* p, size_t n) {
    if (!w || !w->fp || w->failed) return false;
    if (n && fwrite(p, 1, n, w->fp) != n) {
        w->failed = true;
        return false;
    }
    w->bytes_since_flush += n;
    if (w->bytes_since_flush >= FLUSH_EVERY_BYTES) return mkvFlush(w);
    return true;
}

static bool file_id(MkvWriter* w, uint32_t id) {
    uint8_t out[4];
    size_t n = id_bytes(id);
    for (size_t i = 0; i < n; ++i) out[n - 1 - i] = (uint8_t)(id >> (i * 8));
    return file_put(w, out, n);
}

static bool file_size(MkvWriter* w, uint64_t value) {
    size_t n = vint_size_len(value);
    if (!n) return false;
    uint64_t encoded = value | (1ULL << (7 * n));
    uint8_t out[8];
    for (size_t i = 0; i < n; ++i) out[n - 1 - i] = (uint8_t)(encoded >> (i * 8));
    return file_put(w, out, n);
}

static bool write_segment_unknown_size(MkvWriter* w) {
    static const uint8_t unknown8[8] = {0x01,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    return file_id(w, 0x18538067) && file_put(w, unknown8, sizeof(unknown8));
}

/* H.264 parameter sets compatible with the Switch's 1280x720/30 stream.
   Stored without Annex-B start codes for AVCDecoderConfigurationRecord. */
static const uint8_t kSps[] = {
    0x67,0x64,0x0c,0x20,0xac,0x2b,0x40,0x28,0x02,0xdd,0x35,0x01,
    0x0d,0x01,0xf0,0x00,0x00,0x03,0x00,0x10,0x00,0x00,0x03,0x03,
    0xc8,0xf0,0x88,0x46,0xa0
};
static const uint8_t kPps[] = {0x68,0xee,0x3c,0xb0};

static bool make_avcc(uint8_t* out, size_t cap, size_t* out_len) {
    size_t need = 11 + sizeof(kSps) + sizeof(kPps);
    if (cap < need) return false;
    size_t p = 0;
    out[p++] = 1;
    out[p++] = kSps[1];
    out[p++] = kSps[2];
    out[p++] = kSps[3];
    out[p++] = 0xff; /* 4-byte NAL lengths */
    out[p++] = 0xe1; /* one SPS */
    out[p++] = (uint8_t)(sizeof(kSps) >> 8);
    out[p++] = (uint8_t)sizeof(kSps);
    memcpy(out + p, kSps, sizeof(kSps)); p += sizeof(kSps);
    out[p++] = 1; /* one PPS */
    out[p++] = (uint8_t)(sizeof(kPps) >> 8);
    out[p++] = (uint8_t)sizeof(kPps);
    memcpy(out + p, kPps, sizeof(kPps)); p += sizeof(kPps);
    *out_len = p;
    return true;
}

static bool write_headers(MkvWriter* w) {
    ByteBuf ebml = {0}, ebml_body = {0};
    bb_uint(&ebml_body, 0x4286, 1);
    bb_uint(&ebml_body, 0x42f7, 1);
    bb_uint(&ebml_body, 0x42f2, 4);
    bb_uint(&ebml_body, 0x42f3, 8);
    bb_string(&ebml_body, 0x4282, "matroska");
    bb_uint(&ebml_body, 0x4287, 4);
    bb_uint(&ebml_body, 0x4285, 2);
    bb_master(&ebml, 0x1a45dfa3, &ebml_body);
    if (ebml.failed || !file_put(w, ebml.data, ebml.len)) return false;
    if (!write_segment_unknown_size(w)) return false;

    ByteBuf info = {0}, info_body = {0};
    bb_uint(&info_body, 0x2ad7b1, 1000000); /* 1 ms */
    bb_string(&info_body, 0x4d80, "NXCR");
    bb_string(&info_body, 0x5741, "NXCR " NXCR_VERSION);
    bb_master(&info, 0x1549a966, &info_body);
    if (info.failed || !file_put(w, info.data, info.len)) return false;

    uint8_t avcc[128]; size_t avcc_len = 0;
    if (!make_avcc(avcc, sizeof(avcc), &avcc_len)) return false;

    ByteBuf video_dims = {0};
    bb_uint(&video_dims, 0xb0, VIDEO_WIDTH);
    bb_uint(&video_dims, 0xba, VIDEO_HEIGHT);

    ByteBuf ventry_body = {0}, ventry = {0};
    bb_uint(&ventry_body, 0xd7, 1);
    bb_uint(&ventry_body, 0x73c5, 1);
    bb_uint(&ventry_body, 0x83, 1);
    bb_uint(&ventry_body, 0x9c, 0);
    bb_uint(&ventry_body, 0x23e383, 1000000000ULL / VIDEO_FPS);
    bb_string(&ventry_body, 0x86, "V_MPEG4/ISO/AVC");
    bb_binary(&ventry_body, 0x63a2, avcc, avcc_len);
    bb_master(&ventry_body, 0xe0, &video_dims);
    bb_master(&ventry, 0xae, &ventry_body);

    ByteBuf audio_fmt = {0};
    bb_float64(&audio_fmt, 0xb5, (double)AUDIO_SAMPLE_RATE);
    bb_uint(&audio_fmt, 0x9f, AUDIO_CHANNELS);
    bb_uint(&audio_fmt, 0x6264, AUDIO_BITS_PER_SAMPLE);

    ByteBuf aentry_body = {0}, aentry = {0};
    bb_uint(&aentry_body, 0xd7, 2);
    bb_uint(&aentry_body, 0x73c5, 2);
    bb_uint(&aentry_body, 0x83, 2);
    bb_uint(&aentry_body, 0x9c, 0);
    bb_string(&aentry_body, 0x86, "A_PCM/INT/LIT");
    bb_master(&aentry_body, 0xe1, &audio_fmt);
    bb_master(&aentry, 0xae, &aentry_body);

    ByteBuf tracks_body = {0}, tracks = {0};
    bb_put(&tracks_body, ventry.data, ventry.len);
    bb_put(&tracks_body, aentry.data, aentry.len);
    bb_master(&tracks, 0x1654ae6b, &tracks_body);
    if (tracks.failed || !file_put(w, tracks.data, tracks.len)) return false;
    return mkvFlush(w);
}

static size_t sc_len(const uint8_t* p, size_t n) {
    if (n >= 4 && p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 1) return 4;
    if (n >= 3 && p[0] == 0 && p[1] == 0 && p[2] == 1) return 3;
    return 0;
}

static bool find_sc(const uint8_t* p, size_t n, size_t from, size_t* pos, size_t* len) {
    for (size_t i = from; i + 3 <= n; ++i) {
        size_t s = sc_len(p + i, n - i);
        if (s) { *pos = i; *len = s; return true; }
    }
    return false;
}

static size_t annexb_avcc_size(const uint8_t* p, size_t n) {
    size_t first = 0, first_sc = 0;
    if (!find_sc(p, n, 0, &first, &first_sc)) return n ? n + 4 : 0;
    size_t total = 0;
    size_t cur = first + first_sc;
    while (cur < n) {
        size_t next = 0, next_sc = 0;
        bool has_next = find_sc(p, n, cur, &next, &next_sc);
        size_t end = has_next ? next : n;
        if (end > cur) total += 4 + (end - cur);
        if (!has_next) break;
        cur = next + next_sc;
    }
    return total;
}

static bool write_be32(MkvWriter* w, uint32_t v) {
    uint8_t b[4] = {(uint8_t)(v>>24),(uint8_t)(v>>16),(uint8_t)(v>>8),(uint8_t)v};
    return file_put(w, b, 4);
}

static bool write_annexb_as_avcc(MkvWriter* w, const uint8_t* p, size_t n) {
    size_t first = 0, first_sc = 0;
    if (!find_sc(p, n, 0, &first, &first_sc)) {
        return n <= UINT32_MAX && write_be32(w, (uint32_t)n) && file_put(w, p, n);
    }
    size_t cur = first + first_sc;
    while (cur < n) {
        size_t next = 0, next_sc = 0;
        bool has_next = find_sc(p, n, cur, &next, &next_sc);
        size_t end = has_next ? next : n;
        if (end > cur) {
            size_t nal = end - cur;
            if (nal > UINT32_MAX || !write_be32(w, (uint32_t)nal) || !file_put(w, p + cur, nal)) return false;
        }
        if (!has_next) break;
        cur = next + next_sc;
    }
    return true;
}

static bool write_cluster_prefix(MkvWriter* w, uint64_t time_ms, uint8_t track, uint8_t flags, size_t payload_size) {
    ByteBuf body_prefix = {0};
    bb_uint(&body_prefix, 0xe7, time_ms);
    if (!bb_id(&body_prefix, 0xa3) || !bb_size(&body_prefix, 4 + payload_size)) return false;
    bb_u8(&body_prefix, (uint8_t)(0x80u | track));
    bb_u8(&body_prefix, 0); /* relative timecode = 0 */
    bb_u8(&body_prefix, 0);
    bb_u8(&body_prefix, flags);
    if (body_prefix.failed) return false;
    return file_id(w, 0x1f43b675) && file_size(w, body_prefix.len + payload_size) &&
           file_put(w, body_prefix.data, body_prefix.len);
}

bool mkvOpen(MkvWriter* w, const char* path) {
    if (!w || !path) return false;
    memset(w, 0, sizeof(*w));
    w->fp = fopen(path, "wb");
    if (!w->fp) return false;
    /* Keep tiny EBML/header writes from becoming tiny SD-card writes. */
    setvbuf(w->fp, (char*)w->io_buffer, _IOFBF, sizeof(w->io_buffer));
    if (!write_headers(w)) {
        mkvClose(w);
        return false;
    }
    return true;
}

void mkvClose(MkvWriter* w) {
    if (!w) return;
    if (w->fp) {
        fflush(w->fp);
        fclose(w->fp);
    }
    memset(w, 0, sizeof(*w));
}

bool mkvFlush(MkvWriter* w) {
    if (!w || !w->fp || w->failed) return false;
    if (fflush(w->fp) != 0) {
        w->failed = true;
        return false;
    }
    w->bytes_since_flush = 0;
    return true;
}

bool mkvWriteVideo(MkvWriter* w, uint64_t time_ms, const uint8_t* annexb, size_t size, bool keyframe) {
    if (!w || !annexb || !size) return false;
    size_t payload = annexb_avcc_size(annexb, size);
    if (!payload) return false;
    if (!write_cluster_prefix(w, time_ms, 1, keyframe ? 0x80 : 0x00, payload)) return false;
    return write_annexb_as_avcc(w, annexb, size);
}

bool mkvWriteAudio(MkvWriter* w, uint64_t time_ms, const uint8_t* pcm, size_t size) {
    if (!w || !pcm || !size) return false;
    if (!write_cluster_prefix(w, time_ms, 2, 0x80, size)) return false;
    return file_put(w, pcm, size);
}
