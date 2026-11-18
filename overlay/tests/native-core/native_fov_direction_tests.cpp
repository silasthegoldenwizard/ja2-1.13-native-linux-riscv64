/*
 * P710 native snapshot update: tests/native-core/native_fov_direction_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "fov.h"
#include <array>
#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;

void check(bool value, const char* reason)
{
    ++checks;
    if (!value)
    {
        std::cerr << "FAIL: " << reason << '\n';
        std::abort();
    }
}

std::array<UINT32, 5> sight(UINT32 direction)
{
    std::array<UINT32, 5> result{UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    BuildSightDir(direction, &result[0], &result[1], &result[2], &result[3], &result[4]);
    return result;
}

void validDirections()
{
    const std::array<std::array<UINT32, 5>, NUM_WORLD_DIRECTIONS> expected{{
        {{NORTHWEST, NORTHEAST, NORTH, WEST, EAST}},
        {{NORTH, EAST, NORTHEAST, NORTHWEST, SOUTHEAST}},
        {{NORTHEAST, SOUTHEAST, EAST, NORTH, SOUTH}},
        {{EAST, SOUTH, SOUTHEAST, NORTHEAST, SOUTHWEST}},
        {{SOUTHEAST, SOUTHWEST, SOUTH, EAST, WEST}},
        {{SOUTH, WEST, SOUTHWEST, SOUTHEAST, NORTHWEST}},
        {{SOUTHWEST, NORTHWEST, WEST, SOUTH, NORTH}},
        {{WEST, NORTH, NORTHWEST, SOUTHWEST, NORTHEAST}},
    }};
    for (UINT32 direction = 0; direction < NUM_WORLD_DIRECTIONS; ++direction)
        check(sight(direction) == expected[direction], "valid direction keeps its authored sight fan");
}

void invalidDirections()
{
    const auto north = sight(NORTH);
    for (UINT32 direction : {UINT32(NUM_WORLD_DIRECTIONS), UINT32(DIRECTION_IRRELEVANT),
                             UINT32(DIRECTION_EXITGRID), UINT32_MAX})
        check(sight(direction) == north, "invalid direction receives a complete north-facing fallback");

    UINT32 untouched = 0x12345678u;
    BuildSightDir(NORTH, nullptr, &untouched, &untouched, &untouched, &untouched);
    check(untouched == 0x12345678u, "null output rejects the sight fan atomically");
}
}

int main()
{
    validDirections();
    invalidDirections();
    std::cout << "PASS: " << checks << " field-of-view direction assertions\n";
}
