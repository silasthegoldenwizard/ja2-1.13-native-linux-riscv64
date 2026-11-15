/*
 * P710 native snapshot update: sgp/native_legacy_math.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_LEGACY_MATH_H
#define SGP_NATIVE_LEGACY_MATH_H

#include <cstdint>

namespace ja2::native {
// Original FPMult32 uses signed IMUL, then returns product bits16..47.
// Sign extension is explicit so no out-of-range signed cast/right shift is
// needed, and the final unsigned conversion retains the original bit pattern.
constexpr std::uint32_t LegacyFixedMultiply(std::uint32_t a, std::uint32_t b) noexcept
{
    const auto sa = a <= UINT32_C(0x7fffffff) ? std::int64_t(a)
        : std::int64_t(a) - INT64_C(0x100000000);
    const auto sb = b <= UINT32_C(0x7fffffff) ? std::int64_t(b)
        : std::int64_t(b) - INT64_C(0x100000000);
    return static_cast<std::uint32_t>(static_cast<std::uint64_t>(sa * sb) >> 16);
}
}
#endif
