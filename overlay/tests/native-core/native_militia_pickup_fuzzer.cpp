/*
 * P710 native snapshot update: tests/native-core/native_militia_pickup_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production tactical pickup pools.
#include "GameSettings.h"
#include "Handle Items.h"
#include "Items.h"
#include "Overhead.h"
#include "SaveLoadMap.h"
#include "TileDat.h"
#include "World Items.h"
#include "worlddef.h"
#include "worldman.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern BOOLEAN CreateLevelNode(LEVELNODE**);
extern UINT32 guiLevelNodes;

namespace {
constexpr UINT16 itemBase = 500;
constexpr std::size_t maxPoolItems = 12;

[[noreturn]] void invariantFailure() {
    std::abort();
}

std::uint8_t inputByte(const std::uint8_t* data, std::size_t size,
                       std::size_t index) {
    return data[index % size];
}

OBJECTTYPE* findCarriedItem(SOLDIERTYPE& soldier, UINT16 item) {
    for (INT8 slot = 0; slot < NUM_INV_SLOTS; ++slot) {
        if (soldier.inv[slot].exists() && soldier.inv[slot].usItem == item) {
            return &soldier.inv[slot];
        }
    }
    return nullptr;
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    if (size < 3) {
        return 0;
    }

    const std::size_t count = 1 + data[0] % maxPoolItems;
    std::array<MAP_ELEMENT, 1> world{};
    gpWorldLevelData = world.data();
    guiLevelNodes = 0;
    giNumberOfTiles = 0;
    ApplyMapChangesToMapTempFile(FALSE);

    gWorldItems.clear();
    gWorldItems.resize(count);
    gMAXITEMS_READ = std::max<UINT32>(gMAXITEMS_READ,
                                     itemBase + static_cast<UINT32>(count));
    std::vector<ITEM_POOL*> pools(count, nullptr);
    std::vector<LEVELNODE*> nodes(count, nullptr);
    std::vector<BOOLEAN> selected(count, FALSE);
    std::vector<BOOLEAN> expectedPickup(count, FALSE);

    for (std::size_t i = 0; i < count; ++i) {
        const auto recipe = inputByte(data, size, 3 + i);
        const UINT16 item = static_cast<UINT16>(itemBase + i);
        Item[item].usItemClass = (recipe & 0x08) ? IC_FACE : IC_MISC;
        Item[item].ubPerPocket = 1;

        auto& worldItem = gWorldItems[i];
        worldItem.fExists = TRUE;
        worldItem.sGridNo = 0;
        worldItem.ubLevel = 0;
        worldItem.usFlags = WORLD_ITEM_REACHABLE;
        if (recipe & 0x10) {
            worldItem.usFlags |= WORLD_ITEM_TABOO_FOR_MILITIA_EQ_GREEN;
        }
        if (recipe & 0x20) {
            worldItem.usFlags |= WORLD_ITEM_TABOO_FOR_MILITIA_EQ_BLUE;
        }
        worldItem.bRenderZHeightAboveLevel = 0;
        worldItem.bVisible = VISIBLE;
        CreateItem(item, static_cast<INT8>(50 + recipe % 51), &worldItem.object);

        if (!CreateLevelNode(&nodes[i]) || nodes[i] == nullptr) {
            invariantFailure();
        }
        nodes[i]->uiFlags = LEVELNODE_ITEM;
        if (i > 0) {
            nodes[i - 1]->pNext = nodes[i];
        }

        pools[i] = static_cast<ITEM_POOL*>(MemAlloc(sizeof(ITEM_POOL)));
        if (pools[i] == nullptr) {
            invariantFailure();
        }
        *pools[i] = {};
        pools[i]->iItemIndex = static_cast<INT32>(i);
        pools[i]->sGridNo = 0;
        pools[i]->ubLevel = 0;
        pools[i]->bVisible = VISIBLE;
        pools[i]->pLevelNode = nodes[i];
        if (i > 0) {
            pools[i - 1]->pNext = pools[i];
            pools[i]->pPrev = pools[i - 1];
        }
        selected[i] = (recipe & 0x01) ? TRUE : FALSE;
    }
    for (auto* node : nodes) {
        node->pItemPool = pools.front();
    }
    world[0].pStructHead = nodes.front();
    world[0].uiFlags = MAPELEMENT_ITEMPOOL_PRESENT;

    SOLDIERTYPE soldier;
    soldier.initialize();
    soldier.ubID = SoldierID(0);
    soldier.bActive = TRUE;
    soldier.bTeam = MILITIA_TEAM;
    soldier.stats.bLife = 80;
    soldier.bActionPoints = 100;
    soldier.sGridNo = 0;
    soldier.pathing.bLevel = 0;

    const BOOLEAN oldUse = gGameExternalOptions.fMilitiaUseSectorInventory;
    const BOOLEAN oldFace = gGameExternalOptions.fMilitiaUseSectorInventory_Face;
    gGameExternalOptions.fMilitiaUseSectorInventory = TRUE;
    gGameExternalOptions.fMilitiaUseSectorInventory_Face = TRUE;

    INT32 action = ITEM_PICKUP_ACTION_ALL;
    BOOLEAN* selection = nullptr;
    bool throughHandler = false;
    switch (data[1] % 4) {
        case 0:
            std::fill(expectedPickup.begin(), expectedPickup.end(), TRUE);
            break;
        case 1:
            action = ITEM_PICKUP_SELECTION;
            expectedPickup = selected;
            selection = selected.data();
            break;
        case 2: {
            const std::size_t item = data[2] % count;
            action = static_cast<INT32>(item);
            expectedPickup[item] = TRUE;
            break;
        }
        default:
            throughHandler = true;
            std::fill(expectedPickup.begin(), expectedPickup.end(), TRUE);
            break;
    }

    if (throughHandler) {
        HandleSoldierPickupItem(&soldier, action, 0, 0);
    } else {
        SoldierGetItemFromWorld(&soldier, action, 0, 0, selection);
    }

    for (std::size_t i = 0; i < count; ++i) {
        const UINT16 item = static_cast<UINT16>(itemBase + i);
        OBJECTTYPE* carried = findCarriedItem(soldier, item);
        if (expectedPickup[i] != (carried != nullptr)) {
            invariantFailure();
        }
        if (expectedPickup[i] == gWorldItems[i].fExists) {
            invariantFailure();
        }
        if (carried != nullptr) {
            const UINT64 objectFlags = (*carried)[0]->data.sObjectFlag;
            if ((Item[item].usItemClass & IC_FACE) &&
                !(objectFlags & TAKEN_BY_MILITIA)) {
                invariantFailure();
            }
            if ((gWorldItems[i].usFlags & WORLD_ITEM_TABOO_FOR_MILITIA_EQ_GREEN) &&
                !(objectFlags & TAKEN_BY_MILITIA_TABOO_GREEN)) {
                invariantFailure();
            }
            if ((gWorldItems[i].usFlags & WORLD_ITEM_TABOO_FOR_MILITIA_EQ_BLUE) &&
                !(objectFlags & TAKEN_BY_MILITIA_TABOO_BLUE)) {
                invariantFailure();
            }
        }
    }

    ITEM_POOL* remaining = nullptr;
    while (GetItemPool(0, &remaining, 0)) {
        if (!RemoveItemFromPool(0, remaining->iItemIndex, 0)) {
            invariantFailure();
        }
    }
    if (world[0].pStructHead != nullptr || guiLevelNodes != 0) {
        invariantFailure();
    }

    gGameExternalOptions.fMilitiaUseSectorInventory = oldUse;
    gGameExternalOptions.fMilitiaUseSectorInventory_Face = oldFace;
    gWorldItems.clear();
    gpWorldLevelData = nullptr;
    return 0;
}
