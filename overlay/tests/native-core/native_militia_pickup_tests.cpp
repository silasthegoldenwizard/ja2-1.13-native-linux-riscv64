/*
 * P710 native snapshot update: tests/native-core/native_militia_pickup_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_militia_pickup_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise sentinel-based militia pickup through the production world-item path.
#include "GameSettings.h"
#include "Handle Items.h"
#include "Items.h"
#include "Overhead.h"
#include "SaveLoadMap.h"
#include "TileDat.h"
#include "World Items.h"
#include "renderworld.h"
#include "strategicmap.h"
#include "worlddef.h"
#include "worldman.h"
#include <array>
#include <cstdlib>
#include <iostream>

extern BOOLEAN CreateLevelNode(LEVELNODE**);
extern UINT32 guiLevelNodes;

namespace {
unsigned checks = 0;
constexpr UINT16 testItem = 500;

void expect(bool value, const char* reason) {
    ++checks;
    if (!value) {
        std::cerr << "FAIL: " << reason << '\n';
        std::abort();
    }
}

OBJECTTYPE* findItem(SOLDIERTYPE& soldier) {
    for (INT8 slot = 0; slot < NUM_INV_SLOTS; ++slot)
        if (soldier.inv[slot].exists() && soldier.inv[slot].usItem == testItem)
            return &soldier.inv[slot];
    return nullptr;
}

void exercise(INT32 action, UINT16 worldFlag, UINT64 objectFlag,
              UINT32 itemClass = IC_MISC, bool throughHandler = false) {
    Item[testItem].usItemClass = itemClass;
    std::array<MAP_ELEMENT, 1> world{};
    gpWorldLevelData = world.data();
    guiLevelNodes = 0;
    giNumberOfTiles = 0;
    ApplyMapChangesToMapTempFile(FALSE);

    gWorldItems.clear();
    gWorldItems.resize(1);
    auto& worldItem = gWorldItems[0];
    worldItem.fExists = TRUE;
    worldItem.sGridNo = 0;
    worldItem.ubLevel = 0;
    worldItem.usFlags = WORLD_ITEM_REACHABLE | worldFlag;
    worldItem.bRenderZHeightAboveLevel = 0;
    worldItem.bVisible = VISIBLE;
    CreateItem(testItem, 100, &worldItem.object);

    LEVELNODE* node = nullptr;
    expect(CreateLevelNode(&node) && node, "create production level node");
    auto* pool = static_cast<ITEM_POOL*>(MemAlloc(sizeof(ITEM_POOL)));
    expect(pool != nullptr, "allocate production item-pool node");
    *pool = {};
    pool->iItemIndex = 0;
    pool->sGridNo = 0;
    pool->ubLevel = 0;
    pool->bVisible = VISIBLE;
    pool->bRenderZHeightAboveLevel = 0;
    pool->pLevelNode = node;
    node->uiFlags = LEVELNODE_ITEM;
    node->pItemPool = pool;
    world[0].pStructHead = node;
    world[0].uiFlags = MAPELEMENT_ITEMPOOL_PRESENT;

    SOLDIERTYPE soldier;
    soldier.initialize();
    soldier.ubID = SoldierID(0);
    soldier.bActive = TRUE;
    soldier.bTeam = MILITIA_TEAM;
    soldier.stats.bLife = 80;
    soldier.sGridNo = 0;
    soldier.pathing.bLevel = 0;

    BOOLEAN selected[1] = {TRUE};
    if (throughHandler) {
        HandleSoldierPickupItem(&soldier, action, 0, 0);
    } else {
        SoldierGetItemFromWorld(&soldier, action, 0, 0,
                                action == ITEM_PICKUP_SELECTION ? selected : nullptr);
    }

    OBJECTTYPE* picked = findItem(soldier);
    expect(picked != nullptr, "militia pickup moves the world item into inventory");
    expect(((*picked)[0]->data.sObjectFlag & objectFlag) != 0,
           "militia pickup applies the expected carried-item flag");
    expect(!gWorldItems[0].fExists, "picked world item is retired");
    expect(world[0].pStructHead == nullptr, "picked item pool is removed from the world");
    expect(guiLevelNodes == 0, "picked item graphic level node is released");
    gpWorldLevelData = nullptr;
}

void exerciseMovedRecipientDuringGive(INT16 structureSoldierId) {
    const auto oldWorldSectorZ = gbWorldSectorZ;
    const auto oldCenterWorldX = gCenterWorldX;
    const auto oldCenterWorldY = gCenterWorldY;
    const auto oldTLX = gsTLX;
    const auto oldTLY = gsTLY;
    const auto oldTRX = gsTRX;
    const auto oldBLY = gsBLY;
    const auto oldCX = gsCX;
    const auto oldCY = gsCY;
    gbWorldSectorZ = 1;
    gCenterWorldX = 0;
    gCenterWorldY = 0;
    gsTLX = 0;
    gsTLY = 0;
    gsTRX = 200;
    gsBLY = 200;
    gsCX = 100;
    gsCY = 100;
    Item[testItem].usItemClass = IC_MISC;
    std::array<MAP_ELEMENT, 1> world{};
    gpWorldLevelData = world.data();
    guiLevelNodes = 0;
    giNumberOfTiles = 0;
    ApplyMapChangesToMapTempFile(FALSE);
    gWorldItems.clear();
    gTempObject.initialize();

    SOLDIERTYPE giver;
    giver.initialize();
    giver.ubID = SoldierID(0);
    giver.bActive = TRUE;
    giver.bInSector = TRUE;
    giver.stats.bLife = 80;
    giver.sGridNo = 0;
    giver.pathing.bLevel = 0;
    giver.bActionPoints = 100;
    giver.flags.uiStatusFlags |= SOLDIER_ENGAGEDINACTION;
    giver.pTempObject = new OBJECTTYPE;
    CreateItem(testItem, 100, giver.pTempObject);
    giver.bPendingActionData5 = NO_SLOT;
    giver.aiData.sPendingActionData2 = 0;
    giver.aiData.bPendingActionData3 = 0;
    giver.aiData.uiPendingActionData4 = 1;

    SOLDIERTYPE originalRecipient;
    originalRecipient.initialize();
    originalRecipient.ubID = SoldierID(1);
    originalRecipient.bActive = FALSE;
    originalRecipient.flags.uiStatusFlags |= SOLDIER_ENGAGEDINACTION;
    SOLDIERTYPE* oldRecipient = MercPtrs[1];
    MercPtrs[1] = &originalRecipient;

    STRUCTURE replacement{};
    if (structureSoldierId >= 0) {
        replacement.fFlags = STRUCTURE_PERSON;
        replacement.usStructureID = static_cast<UINT16>(structureSoldierId);
        replacement.sGridNo = 0;
        replacement.sCubeOffset = 0;
        world[0].pStructureHead = &replacement;
    }

    SoldierGiveItemFromAnimation(&giver);

    expect(giver.pTempObject == nullptr,
           "moved-recipient give releases the temporary object");
    expect((giver.flags.uiStatusFlags & SOLDIER_ENGAGEDINACTION) == 0,
           "moved-recipient give disengages the giver");
    expect((originalRecipient.flags.uiStatusFlags & SOLDIER_ENGAGEDINACTION) == 0,
           "moved-recipient give disengages the original recipient");
    unsigned preservedItems = 0;
    for (const auto& worldItem : gWorldItems)
        if (worldItem.fExists && worldItem.object.exists() &&
            worldItem.object.usItem == testItem)
            ++preservedItems;
    expect(preservedItems == 1,
           "moved-recipient give preserves the item in the world");

    ITEM_POOL* remaining = nullptr;
    while (GetItemPool(0, &remaining, 0))
        expect(RemoveItemFromPool(0, remaining->iItemIndex, 0),
               "remove moved-recipient fallback item");
    expect(world[0].pStructHead == nullptr,
           "moved-recipient fallback item graphic is released");
    expect(guiLevelNodes == 0,
           "moved-recipient fallback releases its level node");
    MercPtrs[1] = oldRecipient;
    gWorldItems.clear();
    gTempObject.initialize();
    gpWorldLevelData = nullptr;
    gbWorldSectorZ = oldWorldSectorZ;
    gCenterWorldX = oldCenterWorldX;
    gCenterWorldY = oldCenterWorldY;
    gsTLX = oldTLX;
    gsTLY = oldTLY;
    gsTRX = oldTRX;
    gsBLY = oldBLY;
    gsCX = oldCX;
    gsCY = oldCY;
}
}

int main() {
    Item[testItem].ubPerPocket = 1;
    gMAXITEMS_READ = testItem + 1;
    const BOOLEAN oldUse = gGameExternalOptions.fMilitiaUseSectorInventory;
    const BOOLEAN oldFace = gGameExternalOptions.fMilitiaUseSectorInventory_Face;
    gGameExternalOptions.fMilitiaUseSectorInventory = TRUE;
    gGameExternalOptions.fMilitiaUseSectorInventory_Face = TRUE;

    exercise(ITEM_PICKUP_ACTION_ALL, WORLD_ITEM_TABOO_FOR_MILITIA_EQ_GREEN,
             TAKEN_BY_MILITIA_TABOO_GREEN);
    exercise(ITEM_PICKUP_SELECTION, WORLD_ITEM_TABOO_FOR_MILITIA_EQ_BLUE,
             TAKEN_BY_MILITIA_TABOO_BLUE);
    exercise(0, 0, TAKEN_BY_MILITIA, IC_FACE);
    exercise(ITEM_PICKUP_ACTION_ALL, WORLD_ITEM_TABOO_FOR_MILITIA_EQ_GREEN,
             TAKEN_BY_MILITIA_TABOO_GREEN, IC_MISC, true);
    exerciseMovedRecipientDuringGive(-1);
    exerciseMovedRecipientDuringGive(2);
    exerciseMovedRecipientDuringGive(1);

    gGameExternalOptions.fMilitiaUseSectorInventory = oldUse;
    gGameExternalOptions.fMilitiaUseSectorInventory_Face = oldFace;
    gWorldItems.clear();
    std::cout << checks << " militia pickup assertions passed\n";
}
