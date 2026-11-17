/*
 * P710 native snapshot update: tests/native-core/native_sector_inventory_sort_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_sector_inventory_sort_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual sector-inventory sorting with authored item metadata and nested lists.
#include "World Items.h"
#include "ArmsDealerInvInit.h"
#include "GameSettings.h"
#include "Items.h"
#include "Map Screen Interface.h"
#include "Map Screen Interface Map.h"
#include "Map Screen Interface Map Inventory.h"
#include "Overhead.h"
#include "Screens.h"
#include "mapscreen.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <vector>

extern void SortSectorInventory(std::vector<WORLDITEM>&, UINT32);
extern INT32 MapScreenSectorInventoryCompare(const void*, const void*);
extern void SortSectorInventoryEmptyLBE();
extern void SortSectorInventoryAmmo(bool);
extern void SortSectorInventorySeparateAttachments();
extern INT32 giDesiredNumMapInventorySlots;
extern OBJECTTYPE* gpItemDescObject;
namespace {
unsigned checks = 0;
void expect(bool value, const char* reason) {
    ++checks;
    if (!value) { std::cerr << "FAIL: " << reason << '\n'; std::abort(); }
}
WORLDITEM authored(unsigned identity, bool occupied, bool attachment) {
    WORLDITEM value;
    value.sGridNo = 1000 + identity;
    value.usFlags = WORLD_ITEM_REACHABLE;
    value.bVisible = 1;
    value.fExists = TRUE; // An emptied sector slot can retain this flag.
    if (occupied) {
        value.object.usItem = 100 + identity % 3;
        value.object.ubNumberOfObjects = 1;
        value.object[0]->data.objectStatus = 100 - identity % 8;
        if (attachment) {
            OBJECTTYPE child;
            child.usItem = 110;
            child.ubNumberOfObjects = 1;
            child[0]->data.objectStatus = 50 + identity;
            value.object[0]->attachments.push_back(child);
        }
    }
    return value;
}
void exercise(const std::vector<unsigned>& order, unsigned occupied, bool attachments, unsigned prefix) {
    std::vector<WORLDITEM> values;
    for (auto identity : order) values.push_back(authored(identity, identity < occupied, attachments && identity % 2));
    const auto original = values;
    // The unmodified function raw-sorts list-owning values. Copy after sorting
    // reproduces the invalid list traversal seen when the real pool is closed.
    SortSectorInventory(values, prefix);
    const auto copied = values;
    expect(copied.size() == original.size(), "sort and copy preserve count");
    std::vector<INT32> originalIds, sortedIds;
    for (const auto& item : original) originalIds.push_back(item.sGridNo);
    for (const auto& item : copied) sortedIds.push_back(item.sGridNo);
    std::sort(originalIds.begin(), originalIds.end());
    std::sort(sortedIds.begin(), sortedIds.end());
    expect(originalIds == sortedIds, "sorting preserves each identity with its exact multiplicity");
    for (const auto& item : copied) {
        const auto found = std::find_if(original.begin(), original.end(), [&](const WORLDITEM& other) {
            return other.sGridNo == item.sGridNo;
        });
        expect(found != original.end(), "all item identities survive sorting");
        expect(item.usFlags == found->usFlags && item.bVisible == found->bVisible && item.fExists == found->fExists,
               "world metadata remains with its item");
        expect(item.object == found->object, "stack data and nested attachments remain with their item");
        OBJECTTYPE poolCopy(item.object);
        expect(poolCopy == item.object, "world-pool copy has valid list ownership");
    }
    for (unsigned i = 0; i < prefix; ++i) {
        for (unsigned j = i + 1; j < prefix; ++j) {
            const auto& left = values[i].object;
            const auto& right = values[j].object;
            if (!left.ubNumberOfObjects) expect(!right.ubNumberOfObjects, "all empty slots follow occupied slots");
            else if (right.ubNumberOfObjects)
                expect(CompareItemsForSorting(left.usItem, right.usItem, left[0]->data.objectStatus,
                                             right[0]->data.objectStatus) <= 0, "original category/name/quality ordering is preserved");
        }
    }
    for (unsigned i = prefix; i < values.size(); ++i)
        expect(values[i].sGridNo == original[i].sGridNo && values[i].object == original[i].object,
               "sort does not change values beyond the requested prefix");
    if (values.size() > 1) {
        for (unsigned i = 0; i < values.size(); ++i) if (values[i].object.ubNumberOfObjects) {
            const auto before = copied[i].object[0]->data.objectStatus;
            values[i].object[0]->data.objectStatus = 1;
            expect(copied[i].object[0]->data.objectStatus == before, "copies own independent list storage");
            break;
        }
    }
}
void prepareFullPool() {
    pInventoryPoolList.clear();
    std::vector<WORLDITEM>(1).swap(pInventoryPoolList);
    MAP_INVENTORY_POOL_SLOT_COUNT = 40;
    giDesiredNumMapInventorySlots = 40;
    iCurrentInventoryPoolPage = iLastInventoryPoolPage = 0;
    guiCurrentScreen = MAP_SCREEN;
    fShowMapInventoryPool = TRUE;
    bSelectedInfoChar = 0;
    gCharactersList[0].fValid = TRUE;
    gCharactersList[0].usSolID = SoldierID(0);
    Menptr[0].initialize();
    Menptr[0].ubID = SoldierID(0);
    Menptr[0].bActive = TRUE;
    Menptr[0].stats.bLife = 80;
    Menptr[0].sSectorX = sSelMapX = 1;
    Menptr[0].sSectorY = sSelMapY = 1;
    Menptr[0].bSectorZ = iCurrentMapSectorZ = 0;
    MercPtrs[0] = &Menptr[0];
    pInventoryPoolList[0].usFlags = WORLD_ITEM_REACHABLE;
    pInventoryPoolList[0].bVisible = TRUE;
    pInventoryPoolList[0].fExists = TRUE;
}
void emptyLBEWithGrowth() {
    prepareFullPool();
    constexpr UINT16 lbe = 120, contents = 121;
    Item[lbe].usItemClass = IC_LBEGEAR;
    Item[contents].usItemClass = IC_MISC;
    Item[contents].ubPerPocket = 1;
    auto& object = pInventoryPoolList[0].object;
    object.usItem = lbe;
    object.ubNumberOfObjects = 2;
    object.objectStack.resize(2);
    LBEArray.clear();
    for (unsigned stack = 0; stack < 2; ++stack) {
        LBEArray.push_back(LBENODE());
        auto& node = LBEArray.back();
        node.uniqueID = 100 + stack;
        node.inv.resize(2);
        object[stack]->data.lbe.bLBE = -1;
        object[stack]->data.lbe.uniqueID = node.uniqueID;
        for (auto& pocket : node.inv) {
            pocket.usItem = contents;
            pocket.ubNumberOfObjects = 1;
            pocket[0]->data.objectStatus = 80;
        }
    }
    const auto capacity = pInventoryPoolList.capacity();
    SortSectorInventoryEmptyLBE();
    expect(pInventoryPoolList.capacity() > capacity, "emptying LBE uses real pool growth");
    expect(LBEArray.empty(), "both emptied LBE nodes are released");
    expect(pInventoryPoolList[0].object.ubNumberOfObjects == 2 &&
           !pInventoryPoolList[0].object.IsActiveLBE(0) &&
           !pInventoryPoolList[0].object.IsActiveLBE(1), "both source LBE stack entries remain valid");
    unsigned count = 0;
    for (const auto& item : pInventoryPoolList)
        if (item.object.usItem == contents) count += item.object.ubNumberOfObjects;
    expect(count == 4, "emptying LBE conserves all pocket objects");
}
void ammoWithGrowth(bool boxes) {
    prepareFullPool();
    constexpr UINT16 ammo = 130, crate = 131;
    gMAXITEMS_READ = 132;
    Item[ammo].usItemClass = Item[crate].usItemClass = IC_AMMO;
    Item[ammo].ubClassIndex = ammo;
    Item[crate].ubClassIndex = crate;
    Item[ammo].ubPerPocket = Item[crate].ubPerPocket = 1;
    Magazine[ammo].ubCalibre = Magazine[crate].ubCalibre = 1;
    Magazine[ammo].ubAmmoType = Magazine[crate].ubAmmoType = 1;
    Magazine[ammo].ubMagType = 0;
    Magazine[crate].ubMagType = boxes ? AMMO_BOX : AMMO_CRATE;
    Magazine[ammo].ubMagSize = 30;
    Magazine[crate].ubMagSize = 100;
    CreateItems(ammo, 100, 4, &pInventoryPoolList[0].object);
    const auto capacity = pInventoryPoolList.capacity();
    SortSectorInventoryAmmo(boxes);
    expect(pInventoryPoolList.capacity() > capacity, "ammo conversion uses real pool growth");
    unsigned rounds = 0, magazines = 0;
    for (const auto& item : pInventoryPoolList) {
        if (item.object.usItem == ammo) magazines += item.object.ubNumberOfObjects;
        if (item.object.usItem == crate)
            for (unsigned stack = 0; stack < item.object.ubNumberOfObjects; ++stack)
                rounds += item.object[stack]->data.ubShotsLeft;
    }
    expect(magazines == 0 && rounds == 120, "ammo conversion removes consumed magazines and conserves rounds");
}
void transformWithGrowth() {
    prepareFullPool();
    constexpr UINT16 source = 140, primary = 141, secondary = 142, ammo = 143, blocker = 144;
    gMAXITEMS_READ = blocker + 1;
    Item[source].usItemClass = IC_GUN;
    Item[source].ubClassIndex = source;
    Item[primary].usItemClass = Item[secondary].usItemClass = Item[blocker].usItemClass = IC_MISC;
    Item[ammo].usItemClass = IC_AMMO;
    Item[ammo].ubClassIndex = ammo;
    Item[source].ubPerPocket = Item[primary].ubPerPocket = Item[secondary].ubPerPocket = 1;
    Item[ammo].ubPerPocket = Item[blocker].ubPerPocket = 1;
    Weapon[source].ubCalibre = 1;
    Weapon[source].ubMagSize = 30;
    Magazine[ammo].ubCalibre = 1;
    Magazine[ammo].ubAmmoType = 1;
    Magazine[ammo].ubMagSize = 30;
    CreateItem(source, 87, &pInventoryPoolList[0].object);
    pInventoryPoolList[0].object[0]->data.gun.usGunAmmoItem = ammo;
    pInventoryPoolList[0].object[0]->data.gun.ubGunAmmoType = 1;
    pInventoryPoolList[0].object[0]->data.gun.ubGunShotsLeft = 5;
    for (INT8 slot = 0; slot < NUM_INV_SLOTS; ++slot)
        CreateItem(blocker, 100, &Menptr[0].inv[slot]);

    TransformInfoStruct transform = {};
    transform.usItem = source;
    transform.usResult[0] = primary;
    transform.usResult[1] = secondary;

    gpItemDescObject = &pInventoryPoolList[0].object;
    const auto capacity = pInventoryPoolList.capacity();
    expect(gpItemDescObject->TransformObject(&Menptr[0], 0, &transform, NULL),
           "sector item transformation succeeds");
    expect(pInventoryPoolList.capacity() > capacity,
           "multi-result transformation uses real pool growth");
    expect(gpItemDescObject == &pInventoryPoolList[0].object,
           "description pointer follows the transformed pool item after growth");
    expect(pInventoryPoolList[0].object.usItem == primary &&
           pInventoryPoolList[0].object.ubNumberOfObjects == 1 &&
           pInventoryPoolList[0].object[0]->data.objectStatus == 87,
           "primary result stays in the original world-item slot");
    unsigned secondaryCount = 0;
    unsigned ammoRounds = 0;
    for (const auto& item : pInventoryPoolList)
    {
        if (item.object.usItem == secondary) secondaryCount += item.object.ubNumberOfObjects;
        if (item.object.usItem == ammo)
            for (unsigned stack = 0; stack < item.object.ubNumberOfObjects; ++stack)
                ammoRounds += item.object[stack]->data.ubShotsLeft;
    }
    expect(secondaryCount == 1, "secondary result survives pool reallocation");
    expect(ammoRounds == 5, "ejected ammunition survives pool reallocation");
    gpItemDescObject = NULL;
}
void separateEmptyAttachmentSlot() {
    prepareFullPool();
    constexpr UINT16 host = 150;
    Item[host].usItemClass = IC_MISC;
    auto& object = pInventoryPoolList[0].object;
    object.usItem = host;
    object.ubNumberOfObjects = 1;
    object[0]->data.objectStatus = 100;
    object[0]->attachments.push_back(OBJECTTYPE());
    SortSectorInventorySeparateAttachments();
    expect(object.exists() && object[0]->attachments.size() == 1 &&
           !object[0]->attachments.front().exists(),
           "separating attachments tolerates an authored empty attachment slot");
}
}
int main() {
    for (unsigned i = 100; i <= 110; ++i) Item[i].usItemClass = IC_MISC;
    Item[102].usItemClass = IC_MEDKIT;
    std::wcscpy(Item[100].szBRName, L"C");
    std::wcscpy(Item[101].szBRName, L"A");
    std::wcscpy(Item[102].szBRName, L"B");
    auto empty1 = authored(0, false, false);
    auto empty2 = authored(1, false, false);
    expect(MapScreenSectorInventoryCompare(&empty1, &empty1) == 0, "empty comparison is irreflexive");
    expect(MapScreenSectorInventoryCompare(&empty1, &empty2) == 0, "two empty slots are equivalent");
    expect(MapScreenSectorInventoryCompare(&empty2, &empty1) == 0, "empty equivalence is symmetric");
    // Nested attachments and empty slots are both part of the real item type.
    exercise({5, 2, 4, 1, 3, 0}, 4, true, 6);
    exercise({}, 0, false, 0);
    exercise({0}, 0, false, 1);
    exercise({0}, 1, true, 1);
    exercise({0, 1, 2, 3, 4, 5}, 0, false, 6);
    exercise({5, 4, 3, 2, 1, 0}, 6, false, 6);
    exercise({5, 4, 3, 2, 1, 0}, 6, true, 4);
    std::vector<unsigned> large;
    for (unsigned i = 64; i > 0; --i) large.push_back(i - 1);
    exercise(large, 48, true, 64); // Partitioning, equal keys, empty slots, nested lists.
    std::array<unsigned, 4> order{0, 1, 2, 3};
    do { exercise(std::vector<unsigned>(order.begin(), order.end()), 3, true, 4); }
    while (std::next_permutation(order.begin(), order.end()));
    emptyLBEWithGrowth();
    ammoWithGrowth(false);
    ammoWithGrowth(true);
    transformWithGrowth();
    separateEmptyAttachmentSlot();
    pInventoryPoolList.clear();
    std::cout << checks << " original sector-inventory sort assertions passed\n";
}
