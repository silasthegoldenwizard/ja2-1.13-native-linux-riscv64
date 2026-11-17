/*
 * P710 native snapshot update: tests/native-core/native_squad_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production squad-table save decoding.
#include "BufferMan.h"
#include "Overhead.h"
#include "Squads.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

extern INT8 SquadMovementGroups[NUMBER_OF_SQUADS];

namespace {
using Bytes = std::vector<UINT8>;

struct SquadRecord {
    INT16 id;
    INT16 padding[5];
};

static_assert(sizeof(SquadRecord) == 12, "squad save record wire size changed");

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

INT16 validID(const std::uint8_t* data, std::size_t size, std::size_t index)
{
    if (!size || (data[index % size] & 1) == 0) return -1;
    const auto high = static_cast<UINT16>(data[(index + 1) % size]);
    const auto low = static_cast<UINT16>(data[(index + 2) % size]);
    return static_cast<INT16>(((high << 8) | low) % TOTAL_SOLDIERS);
}

INT16 invalidID(const std::uint8_t* data, std::size_t size)
{
    if (!size) return -2;
    switch (data[0] % 4) {
        case 0: return -2;
        case 1: return static_cast<INT16>(TOTAL_SOLDIERS);
        case 2: return std::numeric_limits<INT16>::max();
        default: return std::numeric_limits<INT16>::min();
    }
}

Bytes structuredInput(const std::uint8_t* data, std::size_t size,
                      bool invalid)
{
    Bytes bytes;
    bytes.reserve(NUMBER_OF_SQUADS * NUMBER_OF_SOLDIERS_PER_SQUAD *
                      sizeof(SquadRecord) + NUMBER_OF_SQUADS);
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            const auto index = static_cast<std::size_t>(
                squad * NUMBER_OF_SOLDIERS_PER_SQUAD + slot);
            SquadRecord record{};
            record.id = invalid && index == 0 ? invalidID(data, size) :
                                                validID(data, size, index);
            append(bytes, record);
        }
    }
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        const INT8 movement = size ? static_cast<INT8>(data[squad % size]) : 0;
        append(bytes, movement);
    }
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    const auto mode = data[0] % 5;
    if (mode == 0) return structuredInput(data, size, false);
    if (mode == 1) return structuredInput(data, size, true);
    if (mode == 2) {
        auto bytes = structuredInput(data, size, false);
        const auto extent = static_cast<std::size_t>(size > 1 ? data[1] : 0) *
            bytes.size() / 255;
        bytes.resize(std::min(extent, bytes.size()));
        return bytes;
    }
    if (mode == 3) {
        Bytes bytes(data, data + size);
        return bytes;
    }

    auto bytes = structuredInput(data, size, false);
    if (bytes.size() > NUMBER_OF_SQUADS) {
        bytes.resize(bytes.size() - (1 + (size > 1 ? data[1] % NUMBER_OF_SQUADS : 0)));
    }
    return bytes;
}

bool memberPointerIsValid(const SOLDIERTYPE* member)
{
    if (!member) return true;
    const auto address = reinterpret_cast<std::uintptr_t>(member);
    const auto begin = reinterpret_cast<std::uintptr_t>(&Menptr[0]);
    const auto end = begin + sizeof(Menptr);
    return address >= begin && address < end &&
        (address - begin) % sizeof(SOLDIERTYPE) == 0;
}

void clearLiveState()
{
    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            Squad[squad][slot] = nullptr;
        }
    }
    std::memset(SquadMovementGroups, 0, sizeof(INT8) * NUMBER_OF_SQUADS);
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (size > 65536) return 0;
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadSquadInfoFromSavedGameFile(&cursor);
    const auto consumed = cursor - begin;
    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }

    for (INT32 squad = 0; squad < NUMBER_OF_SQUADS; ++squad) {
        for (INT32 slot = 0; slot < NUMBER_OF_SOLDIERS_PER_SQUAD; ++slot) {
            const auto* member = Squad[squad][slot];
            if (!memberPointerIsValid(member)) invariantFailure();
            if (!loaded && member != nullptr) invariantFailure();
        }
        if (!loaded && SquadMovementGroups[squad] != 0) invariantFailure();
    }

    clearLiveState();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
