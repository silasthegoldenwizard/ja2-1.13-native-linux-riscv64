/*
 * P710 native snapshot update: tests/native-core/native_tile_database_capacity_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "GameSettings.h"
#include "TileDat.h"

#include <iostream>

namespace {
unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void checkConfiguration(UINT8 sheets, bool extendsPastCapacityTable)
{
    gGameExternalOptions.ubNumPItems = sheets;
    SetNumberOfTiles();
    const UINT32 expected = GetTileDatabaseExpectedSize();
    check(expected != 0, "an accepted item-sheet configuration has a usable database size");
    check(expected <= UINT16_MAX, "the effective database size fits its published 16-bit index");
    check(giNumberOfTileTypes == P4ITEMS + sheets - 3,
          "the configured item-sheet count selects the matching tile-type range");
    if (extendsPastCapacityTable)
    {
        check(expected == P11ITEM1,
              "item sheets beyond P10 preserve the existing effective tile capacity");
        check(expected < static_cast<UINT32>(giNumberOfTiles),
              "extended logical tile IDs do not require nonexistent database entries");
    }
    else
    {
        check(expected == static_cast<UINT32>(giNumberOfTiles),
              "listed item sheets retain their complete configured capacity");
    }
}
}

int main()
{
    const UINT8 savedSheets = gGameExternalOptions.ubNumPItems;
    const INT32 savedTiles = giNumberOfTiles;
    const INT32 savedTypes = giNumberOfTileTypes;

    checkConfiguration(3, false);
    checkConfiguration(10, false);
    checkConfiguration(11, true);
    checkConfiguration(12, true);
    checkConfiguration(20, true);

    giNumberOfTileTypes = NUMBEROFTILETYPES + 1;
    check(GetTileDatabaseExpectedSize() == 0,
          "an out-of-range tile-type count is rejected before array access");

    gGameExternalOptions.ubNumPItems = savedSheets;
    giNumberOfTiles = savedTiles;
    giNumberOfTileTypes = savedTypes;

    if (failures != 0)
        return 1;
    std::cout << "PASS: " << checks << " tile database capacity assertions\n";
    return 0;
}
