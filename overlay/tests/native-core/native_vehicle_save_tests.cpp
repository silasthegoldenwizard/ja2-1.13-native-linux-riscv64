/*
 * P710 native snapshot update: tests/native-core/native_vehicle_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_vehicle_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production vehicle save-load lifecycle tests with synthetic records only.
#include "BufferMan.h"
#include "MemMan.h"
#include "Soldier Profile.h"
#include "Strategic Pathing.h"
#include "Vehicles.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;
unsigned failures = 0;

void expect(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template<class T>
void append(Bytes& bytes, const T& value) {
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

struct BufferBounds {
    explicit BufferBounds(Bytes& bytes) {
        gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
        giLoadBufferSize = static_cast<INT32>(bytes.size());
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() {
        gpLoadBufferStart = nullptr;
        giLoadBufferSize = 0;
        gfBufferLoadFailed = FALSE;
    }
};

Bytes vehicleRecord(UINT32 pathNodes, bool includePathNodes) {
    Bytes bytes;
    const UINT8 count = 1;
    const BOOLEAN valid = TRUE;
    VEHICLETYPE vehicle{};
    vehicle.fValid = TRUE;
    vehicle.ubProfileID = 0;
    vehicle.ubVehicleType = 0;
    vehicle.pMercPath = reinterpret_cast<PathSt*>(static_cast<std::uintptr_t>(0x1234));
    for (auto& passenger : vehicle.pPassengers) {
        passenger = reinterpret_cast<SOLDIERTYPE*>(static_cast<std::uintptr_t>(NO_PROFILE));
    }
    append(bytes, count);
    append(bytes, valid);
    append(bytes, vehicle);
    append(bytes, pathNodes);
    for (UINT32 index = 0; includePathNodes && index < pathNodes; ++index) {
        PathSt path{};
        path.uiSectorId = 17 + index % 2;
        append(bytes, path);
    }
    return bytes;
}

void publishSentinelVehicle() {
    ClearOutVehicleList();
    ubNumberOfVehicles = 1;
    pVehicleList = static_cast<VEHICLETYPE*>(MemAlloc(sizeof(VEHICLETYPE)));
    std::memset(pVehicleList, 0, sizeof(VEHICLETYPE));
    pVehicleList[0].fValid = TRUE;
    pVehicleList[0].ubProfileID = 3;
    pVehicleList[0].sGridNo = 1234;
    auto* path = static_cast<PathSt*>(MemAlloc(sizeof(PathSt)));
    std::memset(path, 0, sizeof(PathSt));
    path->uiSectorId = 42;
    pVehicleList[0].pMercPath = path;
}

bool sentinelVehicleIsPublished() {
    return ubNumberOfVehicles == 1 && pVehicleList != nullptr &&
           pVehicleList[0].fValid && pVehicleList[0].ubProfileID == 3 &&
           pVehicleList[0].sGridNo == 1234 &&
           pVehicleList[0].pMercPath != nullptr &&
           pVehicleList[0].pMercPath->uiSectorId == 42 &&
           pVehicleList[0].pMercPath->pPrev == nullptr &&
           pVehicleList[0].pMercPath->pNext == nullptr;
}

void publishSentinelMovementGroups() {
    for (std::size_t index = 0; index < gubVehicleMovementGroups.size(); ++index) {
        gubVehicleMovementGroups[index] = static_cast<INT8>(40 + index);
    }
}

bool sentinelMovementGroupsArePublished() {
    if (gubVehicleMovementGroups.size() != MAX_VEHICLES) return false;
    for (std::size_t index = 0; index < gubVehicleMovementGroups.size(); ++index) {
        if (gubVehicleMovementGroups[index] != static_cast<INT8>(40 + index)) return false;
    }
    return true;
}

Bytes movementRecord(std::size_t count) {
    Bytes bytes;
    for (std::size_t index = 0; index < count; ++index) {
        const INT8 group = static_cast<INT8>(70 + index);
        append(bytes, group);
    }
    return bytes;
}

struct StrategicGroupSnapshot {
    GROUP* identity;
    UINT8 groupID;
};

std::vector<StrategicGroupSnapshot> snapshotStrategicGroups() {
    std::vector<StrategicGroupSnapshot> snapshot;
    for (GROUP* group = gpGroupList; group != nullptr; group = group->next) {
        snapshot.push_back({group, group->ubGroupID});
    }
    return snapshot;
}

bool strategicGroupsMatch(const std::vector<StrategicGroupSnapshot>& snapshot) {
    GROUP* group = gpGroupList;
    for (const auto& saved : snapshot) {
        if (group != saved.identity || group->ubGroupID != saved.groupID) return false;
        group = group->next;
    }
    return group == nullptr;
}

void truncatedNewMovementTable() {
    publishSentinelMovementGroups();
    auto bytes = movementRecord(MAX_VEHICLES - 1);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!NewLoadVehicleMovementInfoFromSavedGame(&cursor),
           "truncated current vehicle movement table rejects");
    expect(gfBufferLoadFailed,
           "truncated current vehicle movement table records the bounded read failure");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "truncated current vehicle movement table preserves the caller cursor");
    expect(sentinelMovementGroupsArePublished(),
           "truncated current vehicle movement table preserves the published groups");
}

void truncatedLegacyMovementTable() {
    publishSentinelMovementGroups();
    auto bytes = movementRecord(4);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleMovementInfoFromSavedGame(&cursor),
           "truncated legacy vehicle movement table rejects");
    expect(gfBufferLoadFailed,
           "truncated legacy vehicle movement table records the bounded read failure");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "truncated legacy vehicle movement table preserves the caller cursor");
    expect(sentinelMovementGroupsArePublished(),
           "truncated legacy vehicle movement table preserves the published groups");
}

void validCurrentMovementTable() {
    publishSentinelMovementGroups();
    auto bytes = movementRecord(MAX_VEHICLES);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(NewLoadVehicleMovementInfoFromSavedGame(&cursor),
           "complete current vehicle movement table loads");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data() + bytes.size()),
           "complete current vehicle movement table consumes its exact extent");
    expect(gubVehicleMovementGroups.size() == MAX_VEHICLES,
           "complete current vehicle movement table publishes the fixed runtime size");
    for (std::size_t index = 0; index < gubVehicleMovementGroups.size(); ++index) {
        expect(gubVehicleMovementGroups[index] == static_cast<INT8>(70 + index),
               "complete current vehicle movement table publishes every decoded ID");
    }
}

void validLegacyMovementTable() {
    RemoveAllGroups();
    publishSentinelMovementGroups();
    auto bytes = movementRecord(5);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(LoadVehicleMovementInfoFromSavedGame(&cursor),
           "complete legacy vehicle movement table loads");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data() + bytes.size()),
           "complete legacy vehicle movement table consumes its exact extent");
    for (std::size_t index = 0; index < 5; ++index) {
        expect(gubVehicleMovementGroups[index] == static_cast<INT8>(70 + index),
               "legacy vehicle movement table publishes every saved ID");
    }
    for (std::size_t index = 5; index < MAX_VEHICLES; ++index) {
        GROUP* const group = GetGroup(static_cast<UINT8>(gubVehicleMovementGroups[index]));
        expect(group != nullptr && group->fPersistant,
               "legacy vehicle movement table publishes each added persistent group");
    }
    RemoveAllGroups();
}

void legacyMovementTableWithInsufficientGroupCapacity() {
    RemoveAllGroups();
    for (UINT16 index = 0; index < 251; ++index) {
        if (CreateNewVehicleGroupDepartingFromSector(1, 1, index) == 0) {
            throw std::runtime_error("failed to prepare nearly full strategic group list");
        }
    }

    const auto groupsBeforeLoad = snapshotStrategicGroups();
    publishSentinelMovementGroups();
    auto bytes = movementRecord(5);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());

    expect(!LoadVehicleMovementInfoFromSavedGame(&cursor),
           "legacy vehicle movement table rejects insufficient strategic group capacity");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "insufficient strategic group capacity preserves the caller cursor");
    expect(sentinelMovementGroupsArePublished(),
           "insufficient strategic group capacity preserves the published vehicle groups");
    expect(strategicGroupsMatch(groupsBeforeLoad),
           "insufficient strategic group capacity preserves the strategic group list");
    RemoveAllGroups();
}

void nullMovementCursor() {
    publishSentinelMovementGroups();
    expect(!NewLoadVehicleMovementInfoFromSavedGame(nullptr),
           "null current vehicle movement cursor rejects");
    expect(sentinelMovementGroupsArePublished(),
           "null current vehicle movement cursor preserves the published groups");
    expect(!LoadVehicleMovementInfoFromSavedGame(nullptr),
           "null legacy vehicle movement cursor rejects");
    expect(sentinelMovementGroupsArePublished(),
           "null legacy vehicle movement cursor preserves the published groups");

    CHAR8* cursor = nullptr;
    expect(!NewLoadVehicleMovementInfoFromSavedGame(&cursor),
           "null current vehicle movement buffer rejects");
    expect(sentinelMovementGroupsArePublished(),
           "null current vehicle movement buffer preserves the published groups");
    expect(!LoadVehicleMovementInfoFromSavedGame(&cursor),
           "null legacy vehicle movement buffer rejects");
    expect(sentinelMovementGroupsArePublished(),
           "null legacy vehicle movement buffer preserves the published groups");
}

void emptyList() {
    publishSentinelVehicle();
    Bytes bytes{0};
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(LoadVehicleInformationFromSavedGame(&cursor, 100),
           "zero-vehicle record loads");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data() + bytes.size()),
           "zero-vehicle record consumes its exact extent");
    expect(pVehicleList == nullptr && ubNumberOfVehicles == 0,
           "zero-vehicle record leaves no owned list");
}

void validPath() {
    auto bytes = vehicleRecord(1, true);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(LoadVehicleInformationFromSavedGame(&cursor, 100),
           "one-node vehicle path loads");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data() + bytes.size()),
           "valid vehicle record consumes its exact extent");
    expect(pVehicleList != nullptr && ubNumberOfVehicles == 1 &&
               pVehicleList[0].fValid && pVehicleList[0].pMercPath != nullptr &&
               pVehicleList[0].pMercPath->pPrev == nullptr &&
               pVehicleList[0].pMercPath->pNext == nullptr &&
               pVehicleList[0].pMercPath->uiSectorId == 17,
           "vehicle path ownership and links are rebuilt");
    ClearOutVehicleList();
}

void longValidPath() {
    const UINT32 nodeCount = MAX_PATH_LIST_SIZE + 1;
    auto bytes = vehicleRecord(nodeCount, true);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(LoadVehicleInformationFromSavedGame(&cursor, 100),
           "appended strategic vehicle path longer than the tactical path limit loads");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data() + bytes.size()),
           "long vehicle route consumes its exact extent");
    UINT32 loaded = 0;
    PathSt* previous = nullptr;
    for (PathSt* node = pVehicleList ? pVehicleList[0].pMercPath : nullptr;
         node; node = node->pNext) {
        expect(node->pPrev == previous,
               "long vehicle route rebuilds each backward link");
        previous = node;
        ++loaded;
    }
    expect(loaded == nodeCount,
           "long vehicle route retains every saved strategic node");
    ClearOutVehicleList();
}

void truncatedPath() {
    publishSentinelVehicle();
    auto bytes = vehicleRecord(1, false);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "truncated vehicle path rejects");
    expect(gfBufferLoadFailed,
           "truncated vehicle path records the bounded read failure");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "truncated vehicle path preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "truncated vehicle path preserves the published vehicle state");
    ClearOutVehicleList();
}

void truncatedPathCount() {
    publishSentinelVehicle();
    auto bytes = vehicleRecord(0, false);
    bytes.resize(bytes.size() - sizeof(UINT32));
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "truncated vehicle path count rejects");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "truncated vehicle path count preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "truncated path-count load preserves published pointer ownership");
    ClearOutVehicleList();
}

void excessivePathCount() {
    publishSentinelVehicle();
    auto bytes = vehicleRecord(UINT32_MAX, false);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "unbounded vehicle path count rejects");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "unbounded vehicle path count preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "unbounded path count preserves the published vehicle state");
    ClearOutVehicleList();
}

void nullCursor() {
    publishSentinelVehicle();
    expect(!LoadVehicleInformationFromSavedGame(nullptr, 100),
           "null vehicle cursor rejects");
    expect(sentinelVehicleIsPublished(),
           "null vehicle cursor preserves the published vehicle state");

    CHAR8* cursor = nullptr;
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "null vehicle buffer rejects");
    expect(cursor == nullptr,
           "null vehicle buffer preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "null vehicle buffer preserves the published vehicle state");
    ClearOutVehicleList();
}

void invalidValidFlag() {
    publishSentinelVehicle();
    auto bytes = vehicleRecord(0, false);
    bytes[sizeof(UINT8)] = 2;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "non-Boolean vehicle-valid flag rejects");
    expect(!gfBufferLoadFailed,
           "invalid vehicle-valid flag is a semantic rejection");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "invalid vehicle-valid flag preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "invalid vehicle-valid flag preserves the published vehicle state");
    ClearOutVehicleList();
}

void excessiveVehicleCount() {
    publishSentinelVehicle();
    Bytes bytes{static_cast<UINT8>(MAX_VEHICLES + 1)};
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "vehicle count beyond the runtime table rejects");
    expect(!gfBufferLoadFailed,
           "excessive vehicle count is a semantic rejection");
    expect(cursor == reinterpret_cast<CHAR8*>(bytes.data()),
           "excessive vehicle count preserves the caller cursor");
    expect(sentinelVehicleIsPublished(),
           "excessive vehicle count preserves the published vehicle state");
    ClearOutVehicleList();
}

void obsoleteVehicleDefinitionPayload() {
    Bytes complete(sizeof(NEW_CAR) * NUM_PROFILES, 0);
    BufferBounds completeBounds(complete);
    auto* completeCursor = reinterpret_cast<CHAR8*>(complete.data());
    expect(LoadNewVehiclesFromSaveGame(&completeCursor),
           "complete obsolete vehicle-definition payload is skipped");
    expect(completeCursor == reinterpret_cast<CHAR8*>(complete.data() + complete.size()),
           "complete obsolete vehicle-definition payload consumes its exact extent");

    complete.resize(complete.size() - 1);
    BufferBounds truncatedBounds(complete);
    auto* truncatedCursor = reinterpret_cast<CHAR8*>(complete.data());
    expect(!LoadNewVehiclesFromSaveGame(&truncatedCursor),
           "truncated obsolete vehicle-definition payload rejects");
    expect(gfBufferLoadFailed,
           "truncated obsolete vehicle-definition payload records a bounded read failure");
    expect(truncatedCursor == reinterpret_cast<CHAR8*>(complete.data()),
           "truncated obsolete vehicle-definition payload preserves the caller cursor");

    expect(!LoadNewVehiclesFromSaveGame(nullptr),
           "null obsolete vehicle-definition cursor rejects");
    CHAR8* nullCursor = nullptr;
    expect(!LoadNewVehiclesFromSaveGame(&nullCursor),
           "null obsolete vehicle-definition buffer rejects");
    expect(nullCursor == nullptr,
           "null obsolete vehicle-definition buffer preserves the caller cursor");
}
}

int main() {
    try {
        ClearOutVehicleList();
        emptyList();
        validPath();
        longValidPath();
        truncatedPath();
        truncatedPathCount();
        excessivePathCount();
        nullCursor();
        invalidValidFlag();
        excessiveVehicleCount();
        obsoleteVehicleDefinitionPayload();
        truncatedNewMovementTable();
        truncatedLegacyMovementTable();
        validCurrentMovementTable();
        validLegacyMovementTable();
        legacyMovementTableWithInsufficientGroupCapacity();
        nullMovementCursor();
        ClearOutVehicleList();
        RemoveAllGroups();
        std::cout << "vehicle save: " << checks << " checks, " << failures
                  << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) {
        ClearOutVehicleList();
        std::cerr << "vehicle save failed at check " << checks << ": "
                  << error.what() << '\n';
        return 2;
    }
}
