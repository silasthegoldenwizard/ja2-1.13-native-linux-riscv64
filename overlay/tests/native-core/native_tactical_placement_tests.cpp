/*
 * P710 native snapshot update: tests/native-core/native_tactical_placement_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Synthetic state around the exact extracted tactical-placement production code.
// No map, save, profile, or installed game data is used.
#include <cstdint>
#include <iostream>
#include <stdexcept>

using BOOLEAN = bool;
using INT32 = std::int32_t;
using UINT8 = std::uint8_t;
using UINT16 = std::uint16_t;

constexpr BOOLEAN TRUE = true;
constexpr BOOLEAN FALSE = false;
constexpr UINT8 INSERTION_CODE_NORTH = 1;
constexpr UINT8 INSERTION_CODE_EAST = 2;
constexpr UINT8 INSERTION_CODE_SOUTH = 3;
constexpr UINT8 INSERTION_CODE_WEST = 4;
constexpr UINT8 INSERTION_CODE_CENTER = 5;
constexpr UINT8 INSERTION_CODE_CHOPPER = 6;
constexpr UINT8 INSERTION_CODE_PRIMARY_EDGEINDEX = 7;
constexpr UINT8 INSERTION_CODE_SECONDARY_EDGEINDEX = 8;

constexpr int FONT10ARIALBOLD = 0;
constexpr int FONT_RED = 0;
constexpr int FONT_YELLOW = 0;
constexpr int FONT_MCOLOR_BLACK = 0;
constexpr int LEFT_JUSTIFIED = 0;

struct Soldier
{
    UINT8 ubStrategicInsertionCode = 0;
    UINT16 usStrategicInsertionData = 0;
};

struct MercPlacement
{
    Soldier* pSoldier = nullptr;
    UINT8 ubStrategicInsertionCode = 0;
    BOOLEAN fPlaced = TRUE;
};

struct MapInformation
{
    INT32 sNorthGridNo = -1;
    INT32 sEastGridNo = -1;
    INT32 sSouthGridNo = -1;
    INT32 sWestGridNo = -1;
};

struct RenderArea
{
    INT32 x = 0;
    INT32 y = 0;
};

MapInformation gMapInformation;
RenderArea gOverheadMapRenderArea;
INT32* gps1stNorthEdgepointArray = nullptr;
INT32* gps1stEastEdgepointArray = nullptr;
INT32* gps1stSouthEdgepointArray = nullptr;
INT32* gps1stWestEdgepointArray = nullptr;
BOOLEAN gfChangedEntrySide = FALSE;
MercPlacement placements[1];
MercPlacement* gMercPlacement = placements;
INT32 giPlacements = 0;
Soldier soldier;
Soldier* pSoldier = &soldier;

namespace
{
int assertions = 0;
int cases = 0;
int unexpectedServices = 0;
INT32 edgepoint = 91;

void require(bool value, const char* reason)
{
    ++assertions;
    if (!value)
        throw std::runtime_error(reason);
}

void p710AssertMsg(bool value, const char* reason)
{
    require(value, reason);
}
}

#define AssertMsg(condition, message) p710AssertMsg((condition), (message))

void InvalidateScreen()
{
    ++unexpectedServices;
}

void DrawTextToScreen(const wchar_t*, ...)
{
    ++unexpectedServices;
}

void RefreshScreen(void*)
{
    ++unexpectedServices;
}

void GenerateMapEdgepoints(BOOLEAN)
{
    ++unexpectedServices;
}

#include "tactical_placement_logic.inc"

namespace
{
void configureEdge(bool north, bool east, bool south, bool west)
{
    gMapInformation = {};
    gMapInformation.sNorthGridNo = north ? 101 : -1;
    gMapInformation.sEastGridNo = east ? 102 : -1;
    gMapInformation.sSouthGridNo = south ? 103 : -1;
    gMapInformation.sWestGridNo = west ? 104 : -1;
    gps1stNorthEdgepointArray = north ? &edgepoint : nullptr;
    gps1stEastEdgepointArray = east ? &edgepoint : nullptr;
    gps1stSouthEdgepointArray = south ? &edgepoint : nullptr;
    gps1stWestEdgepointArray = west ? &edgepoint : nullptr;
}

void scenario(UINT8 original, UINT8 insertionData, bool north, bool east,
              bool south, bool west, UINT8 expected, const char* name)
{
    configureEdge(north, east, south, west);
    soldier = {};
    soldier.ubStrategicInsertionCode = original;
    soldier.usStrategicInsertionData = insertionData;
    placements[0] = {};
    placements[0].ubStrategicInsertionCode = 250;
    placements[0].fPlaced = TRUE;
    gfChangedEntrySide = FALSE;
    unexpectedServices = 0;

    P710ApplyTacticalPlacementBlock();

    require(placements[0].pSoldier == &soldier, name);
    require(placements[0].fPlaced == FALSE, name);
    require(soldier.ubStrategicInsertionCode == expected, name);
    require(placements[0].ubStrategicInsertionCode == expected, name);
    require(unexpectedServices == 0, name);
    ++cases;
}
}

int main()
{
    try
    {
        scenario(INSERTION_CODE_EAST, 0, true, false, false, false,
                 INSERTION_CODE_NORTH, "east redirects to north");
        scenario(INSERTION_CODE_WEST, 0, false, false, true, false,
                 INSERTION_CODE_SOUTH, "west redirects to south");
        scenario(INSERTION_CODE_EAST, 0, false, true, false, false,
                 INSERTION_CODE_EAST, "available east remains east");
        scenario(INSERTION_CODE_PRIMARY_EDGEINDEX, INSERTION_CODE_EAST,
                 true, false, false, false, INSERTION_CODE_NORTH,
                 "primary edge index redirects");
        scenario(INSERTION_CODE_SECONDARY_EDGEINDEX, INSERTION_CODE_WEST,
                 false, false, true, false, INSERTION_CODE_SOUTH,
                 "secondary edge index redirects");
        scenario(INSERTION_CODE_CENTER, 0, false, false, false, false,
                 INSERTION_CODE_CENTER, "center remains center");
        scenario(INSERTION_CODE_CHOPPER, 0, false, false, false, false,
                 INSERTION_CODE_CHOPPER, "chopper remains chopper");
        require(cases == 7, "all placement scenarios ran");
        std::cout << "PASS " << assertions << " tactical placement checks; "
                  << cases << " scenarios\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
