/*
 * P710 native snapshot update: tests/native-core/native_world_node_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual world-node creation/removal functions; authored metadata only.
#include "worlddef.h"
#include "worldman.h"
#include "TileDat.h"
#include "SaveLoadMap.h"
#include <array>
#include <cstdlib>
#include <iostream>

extern BOOLEAN CreateLevelNode(LEVELNODE**);
extern UINT32 guiLevelNodes;

namespace {
unsigned checks = 0;
void expect(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << why << '\n'; std::abort(); }
}
LEVELNODE* node(UINT16 index) {
    LEVELNODE* value = nullptr;
    expect(CreateLevelNode(&value) && value, "actual level-node allocation");
    value->usIndex = index;
    return value;
}
void link(std::initializer_list<LEVELNODE*> nodes, unsigned grid = 0) {
    LEVELNODE** next = &gpWorldLevelData[grid].pStructHead;
    for (auto* value : nodes) { *next = value; next = &value->pNext; }
    *next = nullptr;
}
void retiredTrail() {
    auto* trail = node(99);
    auto* survivor = node(99);
    link({trail, survivor});
    expect(guiLevelNodes == 2, "two same-index nodes exist");
    expect(RemoveStruct(0, 99), "index deletion retires the first trail node");
    expect(gpWorldLevelData[0].pStructHead == survivor && guiLevelNodes == 1,
           "same-index survivor remains after index deletion");
    // Matches the observed UpdateBullets free/use order. No allocation occurs
    // between retirement and the stale-handle call, so reuse cannot hide it.
    expect(!RemoveStructFromLevelNode(0, trail), "retired trail handle is absent");
    expect(gpWorldLevelData[0].pStructHead == survivor && !survivor->pNext && guiLevelNodes == 1,
           "absent removal preserves the live survivor and count");
    expect(RemoveStructFromLevelNode(0, survivor), "live survivor removed by identity");
    expect(!gpWorldLevelData[0].pStructHead && guiLevelNodes == 0, "list and count are empty");
    expect(!RemoveStructFromLevelNode(0, survivor), "repeated retired handle does not free twice");
    expect(!RemoveStructFromLevelNode(0, nullptr), "null handle is absent");
}
void identityRemoval() {
    auto* head = node(99); auto* middle = node(99); auto* tail = node(99);
    auto* foreign = node(99);
    link({head, middle, tail}); link({foreign}, 1);
    expect(!RemoveStructFromLevelNode(0, foreign), "live node in another grid is absent here");
    expect(gpWorldLevelData[1].pStructHead == foreign && guiLevelNodes == 4, "foreign ownership retained");
    expect(!RemoveStructFromLevelNode(0, nullptr), "null does not remove a live node");
    expect(RemoveStructFromLevelNode(0, middle), "remove exact middle among equal indexes");
    expect(gpWorldLevelData[0].pStructHead == head && head->pNext == tail && guiLevelNodes == 3,
           "middle unlink preserves head and tail");
    expect(RemoveStructFromLevelNode(0, tail), "remove exact tail");
    expect(!head->pNext && guiLevelNodes == 2, "tail unlink retains head");
    expect(RemoveStructFromLevelNode(0, head), "remove exact head");
    expect(!gpWorldLevelData[0].pStructHead && guiLevelNodes == 1, "head unlink leaves other grid");
    expect(RemoveStructFromLevelNode(1, foreign), "remove foreign node from its owning grid");
    expect(!gpWorldLevelData[1].pStructHead && guiLevelNodes == 0, "all authored nodes retired");
}
}
int main() {
    std::array<MAP_ELEMENT, 2> world{};
    gpWorldLevelData = world.data();
    guiLevelNodes = 0;
    giNumberOfTiles = 0; // Authored visual nodes have no resource-backed tile metadata.
    ApplyMapChangesToMapTempFile(FALSE);
    retiredTrail(); identityRemoval();
    gpWorldLevelData = nullptr;
    std::cout << checks << " original world-node lifetime assertions passed\n";
}
