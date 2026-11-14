/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * Local-to-SD recording implementation from the recorder prototype and later NXCR changes.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#include <stdint.h>
#pragma once
#include <stdbool.h>
#include "capture.h"

bool recorderInitialize(void);
void recorderExit(void);
void recorderSetEnabled(bool enabled);
bool recorderIsEnabled(void);
void recorderPushVideo(const VideoFrame* frame);
void recorderPushAudio(const AudioFrame* frame);

/* v0.4.1 bookmark support. timeline_us is the same shared media timeline fed
   to recorderPushVideo/Audio. The sidecar timestamp is made segment-relative. */
bool recorderAddMarker(uint64_t timeline_us, uint64_t* out_rel_ms, unsigned* out_index);
unsigned recorderMarkerCount(void);
