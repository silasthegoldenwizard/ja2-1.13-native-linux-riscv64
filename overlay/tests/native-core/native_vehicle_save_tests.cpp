/*
 * P710 native snapshot update: tests/native-core/native_vehicle_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production vehicle save-load lifecycle tests with synthetic records only.
#include "BufferMan.h"
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

void emptyList() {
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
    auto bytes = vehicleRecord(1, false);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "truncated vehicle path rejects");
    expect(gfBufferLoadFailed,
           "truncated vehicle path records the bounded read failure");
    expect(pVehicleList == nullptr && ubNumberOfVehicles == 0,
           "failed vehicle load releases its partial global state");
    ClearOutVehicleList();
}

void truncatedPathCount() {
    auto bytes = vehicleRecord(0, false);
    bytes.resize(bytes.size() - sizeof(UINT32));
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "truncated vehicle path count rejects");
    expect(pVehicleList == nullptr && ubNumberOfVehicles == 0,
           "failed path-count load discards serialized pointer ownership");
    ClearOutVehicleList();
}

void excessivePathCount() {
    auto bytes = vehicleRecord(UINT32_MAX, false);
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<CHAR8*>(bytes.data());
    expect(!LoadVehicleInformationFromSavedGame(&cursor, 100),
           "unbounded vehicle path count rejects");
    expect(pVehicleList == nullptr && ubNumberOfVehicles == 0,
           "unbounded path count cannot retain or allocate vehicle state");
    ClearOutVehicleList();
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
        ClearOutVehicleList();
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
