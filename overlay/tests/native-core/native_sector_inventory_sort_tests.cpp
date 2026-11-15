/*
 * P710 native snapshot update: tests/native-core/native_sector_inventory_sort_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual sector-inventory sorting with authored item metadata and nested lists.
#include "World Items.h"
#include "ArmsDealerInvInit.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <vector>

extern void SortSectorInventory(std::vector<WORLDITEM>&, UINT32);
extern INT32 MapScreenSectorInventoryCompare(const void*, const void*);
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
    std::cout << checks << " original sector-inventory sort assertions passed\n";
}
