/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * GRC service wrapper is based on the SysDVR-family service implementation; credit exelix11/SysDVR.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#include <switch.h>
#include "grcd.h"

Result sdgrcdOpen(Service* out) {
    if (serviceIsActive(out)) return 0;
    Result rc = smGetService(out, "grc:d");
    if (R_FAILED(rc)) serviceClose(out);
    return rc;
}

void sdgrcdClose(Service* svc) {
    serviceClose(svc);
}

Result sdgrcdBegin(Service* svc) {
    return serviceDispatch(svc, 1);
}

Result sdgrcdTransfer(Service* svc, GrcStream stream, void* buffer, size_t size,
                    u32* num_frames, u32* data_size, u64* start_timestamp) {
    struct {
        u32 num_frames;
        u32 data_size;
        u64 start_timestamp;
    } out = {0};

    u32 stream_id = (u32)stream;
    Result rc = serviceDispatchInOut(svc, 2, stream_id, out,
        .buffer_attrs = { SfBufferAttr_HipcMapAlias | SfBufferAttr_Out },
        .buffers = { { buffer, size } },
    );

    if (R_SUCCEEDED(rc)) {
        if (num_frames) *num_frames = out.num_frames;
        if (data_size) *data_size = out.data_size;
        if (start_timestamp) *start_timestamp = out.start_timestamp;
    }
    return rc;
}
