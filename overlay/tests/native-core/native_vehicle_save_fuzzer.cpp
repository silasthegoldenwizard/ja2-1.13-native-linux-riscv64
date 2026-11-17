/*
 * P710 native snapshot update: tests/native-core/native_vehicle_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production vehicle save rehydration.
#include "BufferMan.h"
#include "Overhead.h"
#include "Soldier Profile.h"
#include "Strategic Pathing.h"
#include "Vehicles.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;

[[noreturn]] void invariantFailure() {
    std::abort();
}

template<class T>
void append(Bytes& bytes, const T& value) {
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

Bytes validRecord(UINT32 pathCount = 1) {
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
    append(bytes, pathCount);
    for (UINT32 index = 0; index < pathCount; ++index) {
        PathSt path{};
        path.uiSectorId = 17 + index % 2;
        append(bytes, path);
    }
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size) {
    if (!size) return {};
    const auto mode = data[0] % 4;
    if (mode == 0) return Bytes(data + 1, data + size);
    if (mode == 2) return Bytes{0};
    if (mode == 3) return validRecord(MAX_PATH_LIST_SIZE + 1);
    auto bytes = validRecord();
    for (std::size_t index = 2; index + 2 < size; index += 3) {
        const auto position = (static_cast<std::size_t>(data[index]) << 8 |
                               data[index + 1]) % bytes.size();
        bytes[position] ^= data[index + 2];
    }
    if (size > 1 && (data[1] & 1)) {
        const auto extent = (static_cast<std::size_t>(data[1] >> 1) *
                             bytes.size()) / 127;
        bytes.resize(std::min(extent, bytes.size()));
    }
    return bytes;
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    if (size > 65536) return 0;
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    // InitOverhead establishes this invariant before production save loading.
    for (UINT16 soldier = 0; soldier < TOTAL_SOLDIERS; ++soldier) {
        MercPtrs[soldier] = &Menptr[soldier];
        Menptr[soldier].bActive = FALSE;
    }
    ClearOutVehicleList();
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadVehicleInformationFromSavedGame(&cursor, 100);
    const auto consumed = cursor - begin;

    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }
    if (!loaded) {
        if (pVehicleList != nullptr || ubNumberOfVehicles != 0) {
            invariantFailure();
        }
    } else {
        if ((ubNumberOfVehicles == 0) != (pVehicleList == nullptr)) {
            invariantFailure();
        }
        for (UINT8 vehicle = 0; vehicle < ubNumberOfVehicles; ++vehicle) {
            if (!pVehicleList[vehicle].fValid) continue;
            PathSt* previous = nullptr;
            std::size_t nodes = 0;
            const auto nodeLimit = bytes.size() / sizeof(PathSt);
            for (auto* path = pVehicleList[vehicle].pMercPath;
                 path != nullptr; path = path->pNext) {
                if (path->pPrev != previous || ++nodes > nodeLimit) {
                    invariantFailure();
                }
                previous = path;
            }
        }
    }

    ClearOutVehicleList();
    if (pVehicleList != nullptr || ubNumberOfVehicles != 0) {
        invariantFailure();
    }
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
