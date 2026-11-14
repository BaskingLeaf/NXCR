/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Local-to-SD MKV implementation from the original recorder prototype and later NXCR changes.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    FILE* fp;
    size_t bytes_since_flush;
    bool failed;
    uint8_t io_buffer[256 * 1024];
} MkvWriter;

bool mkvOpen(MkvWriter* w, const char* path);
void mkvClose(MkvWriter* w);
bool mkvWriteVideo(MkvWriter* w, uint64_t time_ms, const uint8_t* annexb, size_t size, bool keyframe);
bool mkvWriteAudio(MkvWriter* w, uint64_t time_ms, const uint8_t* pcm, size_t size);
bool mkvFlush(MkvWriter* w);
