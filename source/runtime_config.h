/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * NXCR persistent runtime configuration developed in September 2026.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#pragma once
#include <stdbool.h>

bool runtimeConfigInit(void);
bool runtimeConfigReload(void);

unsigned runtimeConfigSegmentMinutes(void);
unsigned runtimeConfigMinFreeMiB(void);
bool runtimeConfigAudioEnabled(void);
unsigned runtimeConfigDiagnosticsLevel(void);
