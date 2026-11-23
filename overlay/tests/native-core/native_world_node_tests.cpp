/*
 * P710 native snapshot update: tests/native-core/native_world_node_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_world_node_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_world_node_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_world_node_tests.cpp
 * Native Linux corpse removal and test changes from the November 23, 2026 checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-23.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual world-node creation/removal functions; authored metadata only.
#include "worlddef.h"
#include "worldman.h"
#include "TileDat.h"
#include "SaveLoadMap.h"
#include "structure.h"
#include "Tile Animation.h"
#include "Rotting Corpses.h"
#include "MemMan.h"
#include <array>
#include <cstring>
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
void linkLand(std::initializer_list<LEVELNODE*> nodes, unsigned grid = 0) {
    LEVELNODE* previous = nullptr;
    LEVELNODE** next = &gpWorldLevelData[grid].pLandHead;
    for (auto* value : nodes) {
        *next = value;
        value->pPrevNode = previous;
        previous = value;
        next = &value->pNext;
    }
    *next = nullptr;
}
void linkOnRoof(std::initializer_list<LEVELNODE*> nodes, unsigned grid = 0) {
    LEVELNODE** next = &gpWorldLevelData[grid].pOnRoofHead;
    for (auto* value : nodes) { *next = value; next = &value->pNext; }
    *next = nullptr;
}
struct CorpseStructureFixture {
    DB_STRUCTURE definition{};
    DB_STRUCTURE_TILE tile{};
    std::array<DB_STRUCTURE_TILE*, 1> tiles{&tile};
    DB_STRUCTURE_REF reference{&definition, tiles.data()};

    CorpseStructureFixture() {
        definition.ubHitPoints = 100;
        definition.ubNumberOfTiles = 1;
        definition.fFlags = STRUCTURE_CORPSE;
        definition.ubWallOrientation = NO_ORIENTATION;
        tile.sPosRelToBase = 0;
        tile.fFlags = TILE_PASSABLE;
    }
};
ANITILE* corpseAnimation(LEVELNODE* levelNode, INT32 corpseId, UINT8 levelId, UINT16 tileIndex = 99) {
    auto* animation = static_cast<ANITILE*>(MemAlloc(sizeof(ANITILE)));
    expect(animation != nullptr, "corpse animation allocation");
    std::memset(animation, 0, sizeof(ANITILE));
    animation->pLevelNode = levelNode;
    animation->ubLevelID = levelId;
    animation->sGridNo = 0;
    animation->usTileIndex = tileIndex;
    animation->sCachedTileID = -1;
    animation->uiUserData = static_cast<UINT32>(corpseId);
    levelNode->pAniTile = animation;
    levelNode->uiFlags |= LEVELNODE_ROTTINGCORPSE;
    return animation;
}
void resetCorpse(INT32 corpseId, ANITILE* animation) {
    gRottingCorpse[corpseId] = {};
    gRottingCorpse[corpseId].def.sGridNo = 0;
    gRottingCorpse[corpseId].fActivated = TRUE;
    gRottingCorpse[corpseId].pAniTile = animation;
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
void completeLandDeletion() {
    auto* head = node(42); auto* middle = node(42); auto* tail = node(7);
    linkLand({head, middle, tail});
    gpWorldLevelData[0].pLandStart = middle;
    expect(guiLevelNodes == 3, "three land nodes exist before complete deletion");
    expect(DeleteAllLandLayers(0), "complete land deletion succeeds");
    expect(!gpWorldLevelData[0].pLandHead, "complete land deletion clears the head owner");
    expect(!gpWorldLevelData[0].pLandStart, "complete land deletion clears the full-tile owner");
    expect(guiLevelNodes == 0, "complete land deletion retires every node exactly once");
}
void rejectedLandInsertion() {
    auto* head = node(11);
    linkLand({head});
    const UINT32 liveNodes = guiLevelNodes;
    expect(!InsertLandIndexAtLevel(0, 22, 3), "out-of-range land insertion is rejected");
    expect(guiLevelNodes == liveNodes, "rejected insertion does not leak a level node");
    expect(gpWorldLevelData[0].pLandHead == head && !head->pNext,
           "rejected insertion preserves the existing land list");
    expect(DeleteAllLandLayers(0), "land cleanup succeeds after rejected insertion");
    expect(guiLevelNodes == 0, "rejected-insertion cleanup leaves no live nodes");
}
void malformedLandBackLinkCleanup() {
    auto* head = node(51);
    auto* tail = node(52);
    linkLand({head, tail});
    gpWorldLevelData[0].pLandStart = tail;
    head->pPrevNode = tail; // Simulate malformed map state; forward ownership is still complete.
    expect(guiLevelNodes == 2, "malformed land list owns two nodes");
    expect(DeleteAllLandLayers(0), "complete cleanup tolerates a malformed back-link");
    expect(!gpWorldLevelData[0].pLandHead, "malformed cleanup clears the head owner");
    expect(!gpWorldLevelData[0].pLandStart, "malformed cleanup clears the full-tile owner");
    expect(guiLevelNodes == 0, "malformed cleanup retires every forward-owned node once");
}
void sameGridCorpseRemovalPreservesSurvivor() {
    CorpseStructureFixture fixture;
    auto* removedNode = node(99);
    auto* survivorNode = node(99);
    link({removedNode, survivorNode});
    auto* removedAnimation = corpseAnimation(removedNode, 0, ANI_STRUCT_LEVEL);
    auto* survivorAnimation = corpseAnimation(survivorNode, 1, ANI_STRUCT_LEVEL);
    removedAnimation->pNext = survivorAnimation;
    pAniTileHead = removedAnimation;
    expect(AddStructureToWorld(0, 0, &fixture.reference, removedNode),
           "first same-grid corpse structure is added through production code");
    expect(AddStructureToWorld(0, 0, &fixture.reference, survivorNode),
           "second same-grid corpse structure is added through production code");
    auto* survivorStructure = survivorNode->pStructureData;
    expect(survivorStructure != nullptr && survivorStructure != removedNode->pStructureData,
           "same-grid corpses have distinct structure identities");
    resetCorpse(0, removedAnimation);
    resetCorpse(1, survivorAnimation);
    giNumRottingCorpse = 2;

    RemoveCorpse(0);
    expect(gpWorldLevelData[0].pStructHead == survivorNode && !survivorNode->pNext,
           "removing one ground corpse preserves the surviving level node");
    expect(gpWorldLevelData[0].pStructureHead == survivorStructure &&
               gpWorldLevelData[0].pStructureTail == survivorStructure,
           "removing one ground corpse preserves the surviving structure identity");
    expect(survivorNode->pStructureData == survivorStructure,
           "surviving ground corpse retains its structure binding");
    expect(pAniTileHead == survivorAnimation && !survivorAnimation->pNext,
           "removing one ground corpse preserves the surviving animation");

    RemoveCorpse(1);
    expect(!gpWorldLevelData[0].pStructHead && !gpWorldLevelData[0].pStructureHead &&
               !gpWorldLevelData[0].pStructureTail && !pAniTileHead,
           "ground corpse teardown retires each owned node and structure exactly once");
    expect(guiLevelNodes == 0, "ground corpse teardown leaves no live level nodes");
    gRottingCorpse[0] = {};
    gRottingCorpse[1] = {};
    giNumRottingCorpse = 0;
}
void sameGridRoofCorpseRemovalUsesExactStructure(bool sharedTileIndex = false) {
    CorpseStructureFixture fixture;
    constexpr UINT16 removedTileIndex = 101;
    const UINT16 survivorTileIndex = sharedTileIndex ? removedTileIndex : 102;
    auto* removedNode = node(removedTileIndex);
    auto* survivorNode = node(survivorTileIndex);
    if (sharedTileIndex) {
        // A newer roof animation is inserted at the head, even if it shares imagery.
        linkOnRoof({survivorNode, removedNode});
    } else {
        linkOnRoof({removedNode, survivorNode});
    }
    auto* removedAnimation = corpseAnimation(removedNode, 0, ANI_ONROOF_LEVEL, removedTileIndex);
    auto* survivorAnimation = corpseAnimation(survivorNode, 1, ANI_ONROOF_LEVEL, survivorTileIndex);
    auto* firstAnimation = sharedTileIndex ? survivorAnimation : removedAnimation;
    firstAnimation->pNext = sharedTileIndex ? removedAnimation : survivorAnimation;
    pAniTileHead = firstAnimation;
    expect(AddStructureToWorld(0, 1, &fixture.reference, removedNode),
           "first same-grid roof corpse structure is added through production code");
    expect(AddStructureToWorld(0, 1, &fixture.reference, survivorNode),
           "second same-grid roof corpse structure is added through production code");
    auto* removedStructure = removedNode->pStructureData;
    auto* survivorStructure = survivorNode->pStructureData;
    expect(removedStructure != nullptr && survivorStructure != nullptr &&
               removedStructure != survivorStructure,
           "same-grid roof corpses have distinct structure identities");
    resetCorpse(0, removedAnimation);
    resetCorpse(1, survivorAnimation);
    giNumRottingCorpse = 2;

    RemoveCorpse(0);
    expect(gpWorldLevelData[0].pOnRoofHead == survivorNode && !survivorNode->pNext,
           sharedTileIndex ? "removing one equal-index roof corpse preserves the surviving level node"
                           : "removing one roof corpse preserves the surviving level node");
    expect(gpWorldLevelData[0].pStructureHead == survivorStructure &&
               gpWorldLevelData[0].pStructureTail == survivorStructure,
           "roof corpse cleanup removes its exact structure and preserves the survivor");
    expect(survivorNode->pStructureData == survivorStructure,
           "surviving roof corpse retains its structure binding");
    expect(pAniTileHead == survivorAnimation && !survivorAnimation->pNext,
           "removing one roof corpse preserves the surviving animation");

    RemoveCorpse(1);
    expect(!gpWorldLevelData[0].pOnRoofHead && !gpWorldLevelData[0].pStructureHead &&
               !gpWorldLevelData[0].pStructureTail && !pAniTileHead,
           "roof corpse teardown retires each owned node and structure exactly once");
    expect(guiLevelNodes == 0, "roof corpse teardown leaves no live level nodes");
    gRottingCorpse[0] = {};
    gRottingCorpse[1] = {};
    giNumRottingCorpse = 0;
}
}
int main() {
    std::array<MAP_ELEMENT, 2> world{};
    gpWorldLevelData = world.data();
    guiLevelNodes = 0;
    giNumberOfTiles = 0; // Authored visual nodes have no resource-backed tile metadata.
    ApplyMapChangesToMapTempFile(FALSE);
    retiredTrail(); identityRemoval(); completeLandDeletion(); rejectedLandInsertion();
    malformedLandBackLinkCleanup();
    sameGridCorpseRemovalPreservesSurvivor();
    sameGridRoofCorpseRemovalUsesExactStructure();
    sameGridRoofCorpseRemovalUsesExactStructure(true);
    gpWorldLevelData = nullptr;
    std::cout << checks << " original world-node lifetime assertions passed\n";
}
