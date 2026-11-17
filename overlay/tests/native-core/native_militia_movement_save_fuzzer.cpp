/*
 * P710 native snapshot update: tests/native-core/native_militia_movement_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production militia-movement save decoding.
#include "BufferMan.h"
#include "GameVersion.h"
#include "MilitiaSquads.h"
#include "Strategic Pathing.h"
#include "mapscreen.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;

[[noreturn]] void invariantFailure()
{
    std::abort();
}

template<class T>
void append(Bytes& bytes, const T& value)
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

PathSt pathNode(UINT32 sector, UINT8 speed, std::uintptr_t poison)
{
    PathSt node{};
    node.uiSectorId = sector;
    node.uiEta = 1000 + sector;
    node.fSpeed = speed;
    node.pNext = reinterpret_cast<PathSt*>(poison | 1);
    node.pPrev = reinterpret_cast<PathSt*>(poison | 3);
    return node;
}

void appendSlot(Bytes& bytes, const std::vector<PathSt>& nodes, INT16 groupID)
{
    const auto count = static_cast<UINT32>(nodes.size());
    append(bytes, count);
    for (const auto& node : nodes) append(bytes, node);
    append(bytes, groupID);
}

Bytes validTable(const std::uint8_t* data, std::size_t size)
{
    Bytes bytes;
    const UINT8 pathCount = MILITIA_PATROLS_MAX;
    append(bytes, pathCount);
    for (UINT32 slot = 0; slot < MILITIA_PATROLS_MAX; ++slot) {
        const auto selector = size ? data[(slot + 1) % size] : 0;
        const auto nodeCount = static_cast<UINT32>(
            selector & 0x20 ? MAX_PATH_LIST_SIZE + 1 : selector % 4);
        std::vector<PathSt> nodes;
        nodes.reserve(nodeCount);
        for (UINT32 node = 0; node < nodeCount; ++node) {
            const auto byte = size ? data[(slot + node + 2) % size] : 0;
            const UINT32 sector = byte % (MAP_WORLD_X * MAP_WORLD_Y);
            const UINT8 speed = byte & 1;
            const auto poison = static_cast<std::uintptr_t>(
                UINT32_C(0x1000) + slot * 0x100 + node * 0x10);
            nodes.push_back(pathNode(sector, speed, poison));
        }
        const INT16 group = (selector & 4) ?
            static_cast<INT16>(selector) : static_cast<INT16>(-1);
        appendSlot(bytes, nodes, group);
    }
    return bytes;
}

Bytes invalidMetadata(const std::uint8_t* data, std::size_t size)
{
    Bytes bytes;
    const auto selector = size > 1 ? data[1] : 0;
    if ((selector & 3) == 0) {
        const UINT8 wrongPathCount = MILITIA_PATROLS_MAX - 1;
        append(bytes, wrongPathCount);
    } else {
        const UINT8 pathCount = MILITIA_PATROLS_MAX;
        append(bytes, pathCount);
        if ((selector & 3) == 1) {
            const UINT32 count = MAX_PATH_LIST_SIZE + 1;
            append(bytes, count);
        } else if ((selector & 3) == 2) {
            const UINT32 count = std::numeric_limits<UINT32>::max();
            append(bytes, count);
        } else {
            appendSlot(bytes,
                       {pathNode(MAP_WORLD_X * MAP_WORLD_Y, NORMAL_MVT,
                                 UINT32_C(0x1110))},
                       1);
        }
    }
    return bytes;
}

Bytes invalidPayload(const std::uint8_t* data, std::size_t size)
{
    auto bytes = validTable(data, size);
    if (bytes.empty()) return bytes;
    const auto selector = size > 1 ? data[1] : 0;
    if ((selector & 1) == 0) {
        const auto extent = static_cast<std::size_t>(selector) * bytes.size() / 255;
        bytes.resize(std::min(extent, bytes.size()));
    } else {
        const auto firstSelector = size ? data[1 % size] : 0;
        const auto firstNodeCount = static_cast<std::size_t>(
            firstSelector & 0x20 ? MAX_PATH_LIST_SIZE + 1 : firstSelector % 4);
        const auto groupOffset = sizeof(UINT8) + sizeof(UINT32) +
            firstNodeCount * sizeof(PathSt);
        if (groupOffset + sizeof(INT16) <= bytes.size()) {
            const INT16 invalidGroup = (selector & 2) ? 256 : -2;
            const auto* begin = reinterpret_cast<const UINT8*>(&invalidGroup);
            std::copy(begin, begin + sizeof(invalidGroup),
                      bytes.begin() + groupOffset);
        }
    }
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    switch (data[0] % 5) {
        case 0: return Bytes(data + 1, data + size);
        case 1: return validTable(data, size);
        case 2: return invalidMetadata(data, size);
        case 3: return invalidPayload(data, size);
        default: {
            auto bytes = validTable(data, size);
            for (std::size_t index = 1; index + 2 < size; index += 3) {
                const auto position =
                    ((static_cast<std::size_t>(data[index]) << 8) |
                     data[index + 1]) % bytes.size();
                bytes[position] ^= data[index + 2];
            }
            return bytes;
        }
    }
}

bool liveStateIsEmpty()
{
    for (const auto& entry : gMilitiaPath) {
        if (entry.path != nullptr || entry.sGroupid != -1) return false;
    }
    return true;
}

void verifyLiveState()
{
    for (const auto& entry : gMilitiaPath) {
        if (entry.sGroupid < -1 || entry.sGroupid > 255) invariantFailure();
        const PathSt* previous = nullptr;
        std::size_t count = 0;
        const auto nodeLimit = static_cast<std::size_t>(giLoadBufferSize) /
            sizeof(PathSt);
        for (auto* node = entry.path; node; node = node->pNext) {
            if (++count > nodeLimit || node->pPrev != previous ||
                node->uiSectorId >= MAP_WORLD_X * MAP_WORLD_Y ||
                node->fSpeed > NORMAL_MVT) {
                invariantFailure();
            }
            previous = node;
        }
    }
}

void clearState()
{
    DeleteAllMilitiaPaths();
    if (!liveStateIsEmpty()) invariantFailure();
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (size > 65536) return 0;
    clearState();
    auto bytes = inputFromRecipe(data, size);
    const UINT32 version = size && (data[0] & 0x80) ?
        MILITIA_PATH_PLOTTING - 1 : MILITIA_PATH_PLOTTING;
    if (bytes.empty() && version >= MILITIA_PATH_PLOTTING) return 0;

    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded =
        LoadMilitiaMovementInformationFromSavedGame(&cursor, version);
    const auto consumed = cursor - begin;
    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }

    if (!loaded) {
        if (!liveStateIsEmpty()) invariantFailure();
    } else {
        verifyLiveState();
        if (version < MILITIA_PATH_PLOTTING &&
            (consumed != 0 || !liveStateIsEmpty())) {
            invariantFailure();
        }
    }

    clearState();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
