/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Capture interface evolved from the SysDVR-compatible recording prototype.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#pragma once
#include <switch.h>
#include <stdbool.h>

typedef struct {
    const u8* data;
    u32 size;
    u64 timestamp_us;
    bool keyframe;
} VideoFrame;

typedef struct {
    const u8* data;
    u32 size;
    u64 timestamp_us;
} AudioFrame;

Result captureInitialize(void);
void captureExit(void);
bool captureReadVideo(VideoFrame* out);
bool captureReadAudio(AudioFrame* out);
