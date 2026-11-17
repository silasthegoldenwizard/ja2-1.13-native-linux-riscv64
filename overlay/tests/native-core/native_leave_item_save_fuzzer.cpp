/*
 * P710 native snapshot update: tests/native-core/native_leave_item_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production mercenary leave-item save decoding.
#include "BufferMan.h"
#include "GameVersion.h"
#include "Map Screen Interface.h"
#include "SaveLoadGame.h"
#include "soldier profile type.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

extern MERC_LEAVE_ITEM* gpLeaveListHead[CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS];
extern UINT32 guiLeaveListOwnerProfileId[CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS];

namespace {
using Bytes = std::vector<UINT8>;

[[noreturn]] void invariantFailure()
{
    std::abort();
}

template<class T>
void append(Bytes& bytes, const T& value, std::size_t extent = sizeof(T))
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + extent);
}

UINT8 byteAt(const std::uint8_t* data, std::size_t size, std::size_t index)
{
    return size ? data[index % size] : 0;
}

void appendCurrentObject(Bytes& bytes)
{
    OBJECTTYPE object;
    object.initialize();
    append(bytes, object, SIZEOF_OBJECTTYPE_POD);
    const int stackSize = 0;
    append(bytes, stackSize);
}

void appendLegacyObject(Bytes& bytes, std::uintptr_t poison)
{
    OLD_MERC_LEAVE_ITEM_101 item{};
    item.pNext = reinterpret_cast<MERC_LEAVE_ITEM*>(poison | 1);
    append(bytes, item);
}

Bytes validTable(const std::uint8_t* data, std::size_t size, bool legacy)
{
    Bytes bytes;
    const UINT32 populatedSlots = 1 + byteAt(data, size, 1) % 3;
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const BOOLEAN exists = slot < populatedSlots ? TRUE : FALSE;
        append(bytes, exists);
        if (!exists) continue;

        const UINT32 count = 1 + byteAt(data, size, slot + 2) % 4;
        append(bytes, count);
        for (UINT32 item = 0; item < count; ++item) {
            if (legacy) {
                appendLegacyObject(bytes, UINT32_C(0x1000) + slot * 0x100 +
                                              item * 0x10);
            } else {
                appendCurrentObject(bytes);
            }
        }
    }

    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const UINT32 owner = slot < populatedSlots ?
            byteAt(data, size, slot + 7) % NUM_PROFILES : NO_PROFILE;
        append(bytes, owner);
    }
    return bytes;
}

Bytes invalidMetadata(const std::uint8_t* data, std::size_t size)
{
    Bytes bytes;
    const auto selector = byteAt(data, size, 1) % 4;
    if (selector == 0) {
        const BOOLEAN invalidFlag = 2;
        append(bytes, invalidFlag);
    } else {
        const BOOLEAN exists = TRUE;
        append(bytes, exists);
        UINT32 count = 0;
        if (selector == 1) count = 1001;
        if (selector == 2) count = std::numeric_limits<UINT32>::max();
        append(bytes, count);
    }
    return bytes;
}

Bytes invalidOwner(const std::uint8_t* data, std::size_t size, bool legacy)
{
    auto bytes = validTable(data, size, legacy);
    if (bytes.size() < sizeof(UINT32)) return bytes;
    const UINT32 owner = (byteAt(data, size, 2) & 1) ?
        NUM_PROFILES : std::numeric_limits<UINT32>::max();
    std::memcpy(bytes.data() + bytes.size() -
                    CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS * sizeof(UINT32),
                &owner, sizeof(owner));
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size, bool legacy)
{
    if (!size) return {};
    switch (data[0] % 7) {
        case 0:
            return Bytes(data + 1, data + size);
        case 1:
            return validTable(data, size, legacy);
        case 2:
            return invalidMetadata(data, size);
        case 3: {
            auto bytes = validTable(data, size, legacy);
            const auto extent = static_cast<std::size_t>(byteAt(data, size, 1)) *
                bytes.size() / 255;
            bytes.resize(std::min(extent, bytes.size()));
            return bytes;
        }
        case 4:
            return invalidOwner(data, size, legacy);
        case 5: {
            auto bytes = validTable(data, size, legacy);
            for (std::size_t index = 1; index + 2 < size; index += 3) {
                const auto position =
                    ((static_cast<std::size_t>(data[index]) << 8) |
                     data[index + 1]) % bytes.size();
                bytes[position] ^= data[index + 2];
            }
            return bytes;
        }
        default: {
            auto bytes = validTable(data, size, legacy);
            const auto ownerBytes = CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS *
                sizeof(UINT32);
            if (bytes.size() > ownerBytes) {
                bytes.resize(bytes.size() - 1 -
                             byteAt(data, size, 2) % std::min<std::size_t>(
                                 ownerBytes, bytes.size() - ownerBytes));
            }
            return bytes;
        }
    }
}

bool liveStateIsEmpty()
{
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        if (gpLeaveListHead[slot] != nullptr ||
            guiLeaveListOwnerProfileId[slot] != NO_PROFILE) {
            return false;
        }
    }
    return true;
}

void verifyLiveState()
{
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        if (guiLeaveListOwnerProfileId[slot] >= NUM_PROFILES) invariantFailure();
        UINT32 count = 0;
        for (auto* item = gpLeaveListHead[slot]; item; item = item->pNext) {
            if (++count > 1000) invariantFailure();
        }
    }
}

void clearState()
{
    ShutDownLeaveList();
    InitLeaveList();
    if (!liveStateIsEmpty()) invariantFailure();
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (size > 65536) return 0;
    clearState();

    const bool legacy = size && (data[0] & 0x80);
    guiCurrentSaveGameVersion = legacy ?
        NIV_SAVEGAME_DATATYPE_CHANGE - 1 : SAVE_GAME_VERSION;
    auto bytes = inputFromRecipe(data, size, legacy);
    if (bytes.empty()) return 0;

    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadLeaveItemList(&cursor);
    const auto consumed = cursor - begin;
    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }

    if (!loaded) {
        if (!liveStateIsEmpty()) invariantFailure();
    } else {
        verifyLiveState();
    }

    clearState();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
