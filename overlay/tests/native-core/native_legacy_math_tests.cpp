/*
 * P710 native snapshot update: tests/native-core/native_legacy_math_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_legacy_math.h"
#include <array>
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;
void check(bool value)
{
    ++checks;
    if (!value) { std::cerr << "fixed-point IMUL contract failure\n"; std::abort(); }
}
// Independent unsigned partial-product/sign-correction reference. All operations
// are defined modulo2^64, including signed32 endpoint input bit patterns.
std::uint32_t reference(std::uint32_t a, std::uint32_t b)
{
    std::uint64_t bits = std::uint64_t(a) * b;
    if (a & UINT32_C(0x80000000)) bits -= std::uint64_t(b) << 32;
    if (b & UINT32_C(0x80000000)) bits -= std::uint64_t(a) << 32;
    return static_cast<std::uint32_t>(bits >> 16);
}
}
int main()
{
    using ja2::native::LegacyFixedMultiply;
    check(LegacyFixedMultiply(0x10000, 0x10000) == 0x10000);
    check(LegacyFixedMultiply(0x18000, 0x24000) == 0x36000);
    check(LegacyFixedMultiply(0xffffffff, 0xffffffff) == 0);
    check(LegacyFixedMultiply(0xffffffff, 0x10000) == 0xffffffff);
    check(LegacyFixedMultiply(0x80000000, 0x10000) == 0x80000000);
    check(LegacyFixedMultiply(0x7fffffff, 0x7fffffff) == 0xffff0000);
    const std::array<std::uint32_t, 10> values{{0,1,0x7fff,0x8000,0xffff,
        0x10000,0xffff0000,0x7fffffff,0x80000000,0xffffffff}};
    for (auto a : values) for (auto b : values)
        check(LegacyFixedMultiply(a,b) == reference(a,b));
    std::uint32_t state = 0x710113;
    for (unsigned i=0; i<10000; ++i) {
        state = state * 1664525u + 1013904223u; const auto a=state;
        state = state * 1664525u + 1013904223u;
        check(LegacyFixedMultiply(a,state) == reference(a,state));
    }
    std::cout << checks << " native fixed-point IMUL checks passed\n";
}
