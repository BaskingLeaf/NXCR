/*
 * NXCR MODIFICATION NOTICE (2026-09 to 2026-11).
 * NXCR runtime configuration evolved from the recorder prototype.
 * Modifications include microSD recording, recording controls, markers,
 * configuration, capture recovery and NXCR packaging as applicable.
 * Original authors and copyright holders retain their rights.
 * See LICENSE, THIRD_PARTY_NOTICES.md and SOURCE_PROVENANCE.md for
 * the distribution licence and third-party acknowledgements.
 */
#pragma once

#define NXCR_VERSION "NXCR-0.4.1-public-candidate"
#define RECORD_DIR "sdmc:/switch/NXCR"
/* Public NXCR video location; keep RECORD_DIR for legacy local controls. */
#define VIDEO_DIR "sdmc:/Videos/NXCR"
#define CONTROL_FILE RECORD_DIR "/recording.enabled"
#define ACTIVE_FILE RECORD_DIR "/recording.active"
#define DIAGNOSTICS_FILE RECORD_DIR "/diagnostics.txt"
#define SEGMENT_MINUTES 30u
#define MIN_FREE_MIB 512u
#define FLUSH_EVERY_BYTES (32u * 1024u * 1024u)

#define VIDEO_BUFFER_SIZE 0x54000u
#define AUDIO_CHUNK_SIZE 0x1000u
#define AUDIO_BATCH_CHUNKS 4u
#define AUDIO_BUFFER_SIZE (AUDIO_CHUNK_SIZE * AUDIO_BATCH_CHUNKS)

/* Keep the lite v0.3.1 footprint: about 1.3 MiB of packet queue storage. */
#define VIDEO_QUEUE_SLOTS 5u
#define AUDIO_QUEUE_SLOTS 24u

#define VIDEO_WIDTH 1280u
#define VIDEO_HEIGHT 720u
#define VIDEO_FPS 30u
#define AUDIO_SAMPLE_RATE 48000u
#define AUDIO_CHANNELS 2u
#define AUDIO_BITS_PER_SAMPLE 16u

#define TIMELINE_GAP_THRESHOLD_US 250000ULL
/* HOME placeholder IDRs are only hundreds of bytes; real game IDRs are much larger. */
#define ACTIVE_KEYFRAME_MIN_BYTES 4096u

#define CONFIG_FILE RECORD_DIR "/config.ini"
#define CONFIG_RELOAD_FILE RECORD_DIR "/config.reload"

#define MARKER_REQUEST_FILE RECORD_DIR "/marker.request"
#define MARKER_RESULT_FILE RECORD_DIR "/marker.result"
#define STATUS_REQUEST_FILE RECORD_DIR "/status.request"
#define STATUS_FILE RECORD_DIR "/status.txt"
#define STATUS_TEMP_FILE RECORD_DIR "/status.tmp"
