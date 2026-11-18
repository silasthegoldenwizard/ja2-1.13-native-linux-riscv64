/*
 * P710 native snapshot update: tests/native-core/native_intel_market_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Intelmarket.h"

#include <cstdint>
#include <iostream>
#include <limits>

namespace
{
int failures = 0;
int checks = 0;

void expect(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

int main()
{
    for (std::int16_t region = 0; region < 16; ++region)
    {
        std::uint32_t regular = ~std::uint32_t{0};
        std::uint32_t advanced = ~std::uint32_t{0};
        const auto low = std::uint32_t{1} << static_cast<unsigned>(region);
        const auto high = std::uint32_t{1} << (16u + static_cast<unsigned>(region));

        expect(Intelmarket::BuildMapIntelFlagMask(region, false, regular),
               "regular region must be accepted");
        expect(regular == low, "regular purchase must set exactly the low region bit");
        expect(Intelmarket::BuildMapIntelFlagMask(region, true, advanced),
               "advanced region must be accepted");
        expect(advanced == (low | high),
               "advanced purchase must set the matching low and high region bits");
    }

    for (const auto region : {std::numeric_limits<std::int16_t>::min(),
                              std::int16_t{-1}, std::int16_t{16},
                              std::numeric_limits<std::int16_t>::max()})
    {
        std::uint32_t flags = ~std::uint32_t{0};
        expect(!Intelmarket::BuildMapIntelFlagMask(region, true, flags),
               "invalid region must be rejected");
        expect(flags == 0, "invalid region must leave a deterministic empty mask");
    }

    if (failures)
    {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " Intel-market assertions failed\n";
        return 1;
    }
    std::cout << "PASS: " << checks << " Intel-market assertions\n";
    return 0;
}
