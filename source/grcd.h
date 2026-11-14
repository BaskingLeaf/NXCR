/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * GRC service interface originated in the SysDVR-compatible recording prototype.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#pragma once
#include <switch.h>

Result sdgrcdOpen(Service* out);
void sdgrcdClose(Service* svc);
Result sdgrcdBegin(Service* svc);
Result sdgrcdTransfer(Service* svc, GrcStream stream, void* buffer, size_t size,
                    u32* num_frames, u32* data_size, u64* start_timestamp);
