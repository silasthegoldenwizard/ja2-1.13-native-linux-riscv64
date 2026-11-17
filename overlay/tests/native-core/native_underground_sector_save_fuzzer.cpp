/*
 * P710 native snapshot update: tests/native-core/native_underground_sector_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production underground-sector save decoding.
#include "BufferMan.h"
#include "Campaign Init.h"
#include "Queen Command.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

extern UNDERGROUND_SECTORINFO* gpUndergroundSectorInfoTail;

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

Bytes validList()
{
    Bytes bytes;
    const UINT32 count = 3;
    append(bytes, count);
    for (UINT32 index = 0; index < count; ++index) {
        UNDERGROUND_SECTORINFO sector{};
        sector.ubSectorX = static_cast<UINT8>(9 + index);
        sector.ubSectorY = 10;
        sector.ubSectorZ = static_cast<UINT8>(1 + index % 2);
        sector.ubNumTroops = static_cast<UINT16>(5 + index);
        sector.next = reinterpret_cast<UNDERGROUND_SECTORINFO*>(
            static_cast<std::uintptr_t>(0x1000 + index));
        append(bytes, sector);
    }
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    const auto mode = data[0] % 3;
    if (mode == 0) return Bytes(data + 1, data + size);
    if (mode == 2) return Bytes{0, 0, 0, 0};
    auto bytes = validList();
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
                                       std::size_t size)
{
    if (size > 65536) return 0;
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    TrashUndergroundSectorInfo();
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadUnderGroundSectorInfoFromSavedGame(&cursor);
    const auto consumed = cursor - begin;

    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }
    if (!loaded) {
        if (gpUndergroundSectorInfoHead != nullptr ||
            gpUndergroundSectorInfoTail != nullptr) {
            invariantFailure();
        }
    } else {
        const auto maximumNodes =
            bytes.size() / sizeof(UNDERGROUND_SECTORINFO) + 1;
        std::size_t nodes = 0;
        UNDERGROUND_SECTORINFO* last = nullptr;
        for (auto* sector = gpUndergroundSectorInfoHead;
             sector != nullptr; sector = sector->next) {
            if (++nodes > maximumNodes) invariantFailure();
            last = sector;
        }
        if (last != gpUndergroundSectorInfoTail) invariantFailure();
        if (last && last->next != nullptr) invariantFailure();
        if (!last && gpUndergroundSectorInfoTail != nullptr) invariantFailure();
    }

    TrashUndergroundSectorInfo();
    if (gpUndergroundSectorInfoHead != nullptr ||
        gpUndergroundSectorInfoTail != nullptr) {
        invariantFailure();
    }
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
