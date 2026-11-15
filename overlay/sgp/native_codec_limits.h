/*
 * P710 native snapshot update: sgp/native_codec_limits.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Shared policy for the approved private FFmpeg SDK used by native audio/video.
#ifndef SGP_NATIVE_CODEC_LIMITS_H
#define SGP_NATIVE_CODEC_LIMITS_H
extern "C" {
#include <libavutil/mem.h>
}
#include <cstddef>
#include <mutex>
namespace sgp {
// Single libavutil allocation ceiling, process-wide for this private SDK.
// 48MiB accommodates the32MiB selected video-packet limit plus FFmpeg input
// padding/geometric packet growth, declared Bink/RGB frame dimensions and the
// existing2MiB audio packets/32768x8 input samples/65536 stereo output samples.
// It covers libavutil allocations DURING probing as well as decoding, including
// skipped streams. It is NOT a total memory bound, a bound on arbitrary C++
// allocations, or a complete opaque codec allocation budget. Multiple admitted
// blocks may coexist. No other SDK consumer may reset the process policy.
inline constexpr int NativeCodecContainerPacketLimit=32*1024*1024;
inline constexpr std::size_t NativeCodecSingleAllocationLimit=48u*1024u*1024u;
inline void ApplyNativeCodecMemoryPolicy() {
    static std::once_flag policy;
    std::call_once(policy,[] { av_max_alloc(NativeCodecSingleAllocationLimit); });
}
}
#endif
