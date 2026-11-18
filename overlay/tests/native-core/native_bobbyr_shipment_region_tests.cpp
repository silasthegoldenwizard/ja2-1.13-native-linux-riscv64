/*
 * P710 native snapshot update: tests/native-core/native_bobbyr_shipment_region_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include <array>
#include <cstdint>
#include <iostream>

using UINT16 = std::uint16_t;
using UINT32 = std::uint32_t;
using INT32 = std::int32_t;

constexpr UINT32 BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS = 13;
constexpr UINT32 MAX_SHIPMENTS_THAT_FIT_ON_SCREEN = 13;
constexpr UINT16 BOBBYR_SHIPMENT_ORDER_NUM_START_Y = 144;
constexpr UINT16 BOBBYR_SHIPMENT_DELIVERY_GRID_WIDTH = 183;
constexpr UINT16 BOBBYR_SHIPMENT_ORDER_NUM_X = 116;
constexpr UINT16 BOBBYR_SHIPMENT_GAP_BTN_LINES = 20;
constexpr int BOBBYR_SHIPMENT_STATIC_TEXT_FONT = 0;
constexpr int SHIPMENT_INTRANSIT = 0;
constexpr int MSYS_PRIORITY_HIGH = 0;
constexpr int CURSOR_WWW = 0;
constexpr auto MSYS_NO_CALLBACK = nullptr;

struct MOUSE_REGION
{
    bool defined = false;
    bool added = false;
    bool removed = false;
    INT32 userData = -1;
};

struct PostalService
{
    UINT32 count = 0;
    UINT32 GetShipmentCount(int) const { return count; }
};

PostalService gPostalService;
MOUSE_REGION gSelectedPreviousShipmentsRegion[BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS];
UINT32 guiBobbyRShipmentPreviousRegionCount = 0;

int defineCalls = 0;
int addCalls = 0;
int removeCalls = 0;
int failures = 0;
int checks = 0;

void SelectPreviousShipmentsRegionCallBack(MOUSE_REGION*, INT32) {}
UINT16 GetFontHeight(int) { return 10; }

void MSYS_DefineRegion(MOUSE_REGION* region, ...)
{
    region->defined = true;
    ++defineCalls;
}

void MSYS_AddRegion(MOUSE_REGION* region)
{
    region->added = true;
    ++addCalls;
}

void MSYS_SetRegionUserData(MOUSE_REGION* region, INT32, INT32 value)
{
    region->userData = value;
}

void MSYS_RemoveRegion(MOUSE_REGION* region)
{
    region->removed = true;
    ++removeCalls;
}

#include "create_previous_shipments_regions.inc"
#include "remove_previous_shipments_regions.inc"

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void reset(UINT32 shipmentCount)
{
    gPostalService.count = shipmentCount;
    guiBobbyRShipmentPreviousRegionCount = 0;
    defineCalls = 0;
    addCalls = 0;
    removeCalls = 0;
    for (MOUSE_REGION& region : gSelectedPreviousShipmentsRegion)
    {
        region = {};
    }
}

void checkCreatedPrefix(UINT32 expected)
{
    check(defineCalls == static_cast<int>(expected), "expected define count");
    check(addCalls == static_cast<int>(expected), "expected add count");
    check(guiBobbyRShipmentPreviousRegionCount == expected, "published region count");
    for (UINT32 i = 0; i < BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS; ++i)
    {
        check(gSelectedPreviousShipmentsRegion[i].defined == (i < expected),
              "only the expected region prefix is defined");
        check(gSelectedPreviousShipmentsRegion[i].added == (i < expected),
              "only the expected region prefix is added");
        if (i < expected)
        {
            check(gSelectedPreviousShipmentsRegion[i].userData == static_cast<INT32>(i),
                  "region user data matches its shipment row");
        }
    }
}

void checkRemovedPrefix(UINT32 expected)
{
    check(removeCalls == static_cast<int>(expected), "expected remove count");
    check(guiBobbyRShipmentPreviousRegionCount == 0, "removed region count is cleared");
    for (UINT32 i = 0; i < BOBBYR_SHIPMENT_NUM_PREVIOUS_SHIPMENTS; ++i)
    {
        check(gSelectedPreviousShipmentsRegion[i].removed == (i < expected),
              "only the created region prefix is removed");
    }
}

int main()
{
    reset(14);
    CreatePreviousShipmentsMouseRegions();
    checkCreatedPrefix(13);
    RemovePreviousShipmentsMouseRegions();
    checkRemovedPrefix(13);

    reset(5);
    CreatePreviousShipmentsMouseRegions();
    checkCreatedPrefix(5);
    gPostalService.count = 12;
    RemovePreviousShipmentsMouseRegions();
    checkRemovedPrefix(5);

    reset(12);
    CreatePreviousShipmentsMouseRegions();
    checkCreatedPrefix(12);
    gPostalService.count = 0;
    RemovePreviousShipmentsMouseRegions();
    checkRemovedPrefix(12);

    reset(0);
    CreatePreviousShipmentsMouseRegions();
    checkCreatedPrefix(0);
    RemovePreviousShipmentsMouseRegions();
    checkRemovedPrefix(0);

    std::cout << checks - failures << '/' << checks
              << " shipment-region lifecycle checks passed\n";
    return failures != 0;
}
