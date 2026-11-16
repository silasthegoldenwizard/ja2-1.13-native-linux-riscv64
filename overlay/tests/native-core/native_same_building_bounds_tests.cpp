/*
 * P710 native snapshot update: tests/native-core/native_same_building_bounds_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise the production SameBuilding boundary before any building-table read.
#include "types.h"
#include "Buildings.h"
#include "Isometric Utils.h"
#include "worlddef.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {
unsigned checks = 0;

void expect(bool value, const char* why) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << why << '\n';
        std::abort();
    }
}
}

int main() {
    guiWorldCols = OLD_WORLD_COLS;
    guiWorldRows = OLD_WORLD_ROWS;

    // Keep readable guard bytes on both sides. Before the repair, matching
    // guard values make each invalid-grid comparison return true without
    // relying on undefined behavior in this regression process.
    std::vector<UINT8> storage(static_cast<std::size_t>(WORLD_MAX) + 2, NO_BUILDING);
    UINT8* const prior = gubBuildingInfo;
    gubBuildingInfo = storage.data() + 1;

    constexpr UINT8 building = 7;
    storage.front() = building;
    storage.back() = building;
    gubBuildingInfo[0] = building;
    gubBuildingInfo[WORLD_MAX - 1] = building;

    expect(!SameBuilding(NOWHERE, 0), "NOWHERE first grid is rejected before indexing");
    expect(!SameBuilding(0, NOWHERE), "NOWHERE second grid is rejected before indexing");
    expect(!SameBuilding(WORLD_MAX, 0), "upper-bound first grid is rejected before indexing");
    expect(!SameBuilding(0, WORLD_MAX), "upper-bound second grid is rejected before indexing");
    expect(SameBuilding(0, WORLD_MAX - 1), "valid matching building IDs remain equal");
    gubBuildingInfo[WORLD_MAX - 1] = building + 1;
    expect(!SameBuilding(0, WORLD_MAX - 1), "valid different building IDs remain unequal");
    gubBuildingInfo[WORLD_MAX - 1] = NO_BUILDING;
    expect(!SameBuilding(0, WORLD_MAX - 1), "no-building tile remains unequal");

    gubBuildingInfo = prior;
    std::cout << checks << " original SameBuilding boundary assertions passed\n";
}
