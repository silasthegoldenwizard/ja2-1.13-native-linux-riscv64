/*
 * P710 native snapshot update: tests/native-core/native_item_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production item and inventory save decoders.
#include "BufferMan.h"
#include "GameVersion.h"
#include "Item Types.h"
#include "Items.h"
#include "SaveLoadGame.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern int gLastLBEUniqueID;

namespace {
using Bytes = std::vector<UINT8>;

[[noreturn]] void invariantFailure() {
    std::abort();
}

template<class T>
void append(Bytes& bytes, const T& value, std::size_t extent = sizeof(T)) {
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + extent);
}

Bytes currentBase(UINT8 kind) {
    Bytes bytes;
    const int count = 0;
    if (kind == 0) {
        OBJECTTYPE object;
        object.usItem = NONE;
        object.ubNumberOfObjects = 0;
        append(bytes, object, SIZEOF_OBJECTTYPE_POD);
        append(bytes, count);
    } else if (kind == 1) {
        ObjectData object{};
        append(bytes, object);
        append(bytes, count);
    } else if (kind == 2) {
        LBENODE node{};
        node.uniqueID = 1;
        append(bytes, node, SIZEOF_LBENODE_POD);
        append(bytes, count);
    } else {
        append(bytes, count);
    }
    return bytes;
}

Bytes legacyBase() {
    OLD_OBJECTTYPE_101 object{};
    object.usItem = NONE;
    object.ubNumberOfObjects = 0;
    Bytes bytes;
    append(bytes, object);
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size,
                      UINT8 kind, bool legacy) {
    if (size < 2) return {};
    if (data[0] & 4) return Bytes(data + 2, data + size);
    auto bytes = legacy ? legacyBase() : currentBase(kind);
    for (std::size_t index = 2; index + 2 < size; index += 3) {
        const auto position = (static_cast<std::size_t>(data[index]) << 8 |
                               data[index + 1]) % bytes.size();
        bytes[position] ^= data[index + 2];
    }
    if (data[1] & 2) {
        const auto extent = (static_cast<std::size_t>(data[1] >> 2) *
                             bytes.size()) / 63;
        bytes.resize(std::min(extent, bytes.size()));
    }
    return bytes;
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size) {
    if (size < 2 || size > 65536) return 0;
    const UINT8 kind = data[0] & 3;
    const bool legacy = kind == 0 && (data[1] & 1);
    auto bytes = inputFromRecipe(data, size, kind, legacy);
    if (bytes.empty()) return 0;
    const auto original = bytes;

    const auto savedVersion = guiCurrentSaveGameVersion;
    const auto savedLBECount = LBEArray.size();
    const auto savedLBEID = gLastLBEUniqueID;
    const auto savedItemClass = Item[7].usItemClass;
    guiCurrentSaveGameVersion = legacy ? NIV_SAVEGAME_DATATYPE_CHANGE - 1
                                       : SAVE_GAME_VERSION;
    Item[7].usItemClass = IC_LBEGEAR;
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;

    BOOLEAN loaded = FALSE;
    if (kind == 0) {
        OBJECTTYPE destination;
        loaded = destination.Load(&cursor);
    } else if (kind == 1) {
        StackedObjectData destination;
        loaded = destination.Load(&cursor);
    } else if (kind == 2) {
        LBENODE destination{};
        loaded = destination.Load(&cursor);
    } else {
        Inventory destination;
        loaded = destination.Load(&cursor);
    }

    const auto consumed = cursor - begin;
    if (bytes != original || consumed < 0 ||
        static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }
    if (!loaded && (cursor != begin || LBEArray.size() != savedLBECount ||
                    gLastLBEUniqueID != savedLBEID)) {
        invariantFailure();
    }
    if (LBEArray.size() < savedLBECount) invariantFailure();
    while (LBEArray.size() > savedLBECount) LBEArray.pop_back();
    gLastLBEUniqueID = savedLBEID;
    guiCurrentSaveGameVersion = savedVersion;
    Item[7].usItemClass = savedItemClass;
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
