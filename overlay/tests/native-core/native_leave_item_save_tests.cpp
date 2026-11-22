/*
 * P710 native snapshot update: tests/native-core/native_leave_item_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_leave_item_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "GameVersion.h"
#include "Map Screen Interface.h"
#include "SaveLoadGame.h"
#include "soldier profile type.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern MERC_LEAVE_ITEM* gpLeaveListHead[CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS];
extern UINT32 guiLeaveListOwnerProfileId[CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS];

namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (condition) return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

template<class T>
void append(Bytes& bytes, const T& value, std::size_t extent = sizeof(T))
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + extent);
}

void appendEmptyObject(Bytes& bytes)
{
    OBJECTTYPE object;
    object.initialize();
    append(bytes, object, SIZEOF_OBJECTTYPE_POD);
    const int stackSize = 0;
    append(bytes, stackSize);
}

Bytes table(UINT32 firstCount = 0, bool firstExists = false,
            UINT32 firstOwner = NO_PROFILE)
{
    Bytes bytes;
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const BOOLEAN exists = slot == 0 && firstExists ? TRUE : FALSE;
        append(bytes, exists);
        if (exists) {
            append(bytes, firstCount);
            for (UINT32 item = 0; item < firstCount && item < 4; ++item) {
                appendEmptyObject(bytes);
            }
        }
    }
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const UINT32 owner = slot == 0 ? firstOwner : NO_PROFILE;
        append(bytes, owner);
    }
    return bytes;
}

Bytes legacyTable(UINT16 itemId, UINT8 objectCount, UINT32 owner)
{
    Bytes bytes;
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const BOOLEAN exists = slot == 0 ? TRUE : FALSE;
        append(bytes, exists);
        if (exists) {
            const UINT32 count = 1;
            append(bytes, count);
            OLD_MERC_LEAVE_ITEM_101 item{};
            item.oldObject.usItem = itemId;
            item.oldObject.ubNumberOfObjects = objectCount;
            append(bytes, item);
        }
    }
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        const UINT32 profile = slot == 0 ? owner : NO_PROFILE;
        append(bytes, profile);
    }
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadLeaveItemList(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

bool stateIsEmpty()
{
    for (UINT32 slot = 0; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        if (gpLeaveListHead[slot] != nullptr ||
            guiLeaveListOwnerProfileId[slot] != NO_PROFILE) {
            return false;
        }
    }
    return true;
}

void resetState()
{
    ShutDownLeaveList();
    InitLeaveList();
}

void publishSentinelState()
{
    resetState();
    auto* item = new MERC_LEAVE_ITEM;
    item->object.initialize();
    item->pNext = nullptr;
    gpLeaveListHead[0] = item;
    guiLeaveListOwnerProfileId[0] = 17;
}

bool sentinelStateIsPublished()
{
    if (gpLeaveListHead[0] == nullptr || gpLeaveListHead[0]->pNext != nullptr ||
        guiLeaveListOwnerProfileId[0] != 17) {
        return false;
    }
    for (UINT32 slot = 1; slot < CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS; ++slot) {
        if (gpLeaveListHead[slot] != nullptr ||
            guiLeaveListOwnerProfileId[slot] != NO_PROFILE) {
            return false;
        }
    }
    return true;
}

bool twoItemStateIsPublished()
{
    return gpLeaveListHead[0] != nullptr &&
           gpLeaveListHead[0]->pNext != nullptr &&
           gpLeaveListHead[0]->pNext->pNext == nullptr &&
           guiLeaveListOwnerProfileId[0] == 11;
}

int finish()
{
    resetState();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " leave-item save assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks << " leave-item save assertions\n";
    return 0;
}
}

int main(int argc, char** argv)
{
    guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
    InitLeaveList();

    if (argc > 1 && std::string(argv[1]) == "valid-cleanup") {
        auto valid = table(1, true, 7);
        std::size_t consumed = 0;
        check(load(valid, consumed), "valid leave-item list loads");
        check(consumed == valid.size(), "valid leave-item list consumes its extent");
        check(gpLeaveListHead[0] && !gpLeaveListHead[0]->pNext,
              "valid leave-item list publishes one terminated node");
        check(guiLeaveListOwnerProfileId[0] == 7,
              "valid leave-item list publishes its owner");
        return finish();
    }

    publishSentinelState();
    check(!LoadLeaveItemList(nullptr), "null leave-item cursor is rejected");
    check(sentinelStateIsPublished(), "null cursor preserves the live list");
    CHAR8* nullCursor = nullptr;
    publishSentinelState();
    check(!LoadLeaveItemList(&nullCursor), "null leave-item buffer is rejected");
    check(sentinelStateIsPublished(), "null buffer preserves the live list");

    auto invalidFlag = table();
    invalidFlag[0] = 2;
    std::size_t consumed = 0;
    publishSentinelState();
    check(!load(invalidFlag, consumed), "non-Boolean node flag is rejected");
    check(sentinelStateIsPublished(), "invalid node flag preserves the live list");

    auto zeroCount = table(0, true, 7);
    consumed = 0;
    publishSentinelState();
    check(!load(zeroCount, consumed),
          "node-exists flag with a zero count is rejected");
    check(sentinelStateIsPublished(), "zero-count failure preserves the live list");

    auto excessive = table(1001, true, 7);
    consumed = 0;
    publishSentinelState();
    check(!load(excessive, consumed), "excessive leave-item count is rejected");
    check(sentinelStateIsPublished(), "excessive count preserves the live list");

    auto truncatedItem = table(1, true, 7);
    truncatedItem.resize(sizeof(BOOLEAN) + sizeof(UINT32) +
                         SIZEOF_OBJECTTYPE_POD / 2);
    consumed = 0;
    publishSentinelState();
    check(!load(truncatedItem, consumed), "truncated leave item is rejected");
    check(gfBufferLoadFailed, "truncated leave item marks bounded-read failure");
    check(consumed <= truncatedItem.size(), "truncated item cursor remains bounded");
    check(sentinelStateIsPublished(), "truncated item preserves all live ownership");

    auto truncatedOwners = table(1, true, 7);
    truncatedOwners.resize(truncatedOwners.size() - sizeof(UINT32));
    consumed = 0;
    publishSentinelState();
    check(!load(truncatedOwners, consumed), "truncated owner table is rejected");
    check(sentinelStateIsPublished(), "truncated owner table preserves live nodes");

    auto invalidOwner = table(1, true, NUM_PROFILES);
    consumed = 0;
    publishSentinelState();
    check(!load(invalidOwner, consumed), "out-of-range owner profile is rejected");
    check(sentinelStateIsPublished(), "invalid owner preserves live nodes");

    auto twoItems = table(2, true, 11);
    consumed = 0;
    check(load(twoItems, consumed), "two-item leave list loads");
    check(consumed == twoItems.size(), "two-item leave list consumes its extent");
    check(gpLeaveListHead[0] && gpLeaveListHead[0]->pNext &&
              !gpLeaveListHead[0]->pNext->pNext,
          "two-item leave list publishes exactly two terminated nodes");
    check(guiLeaveListOwnerProfileId[0] == 11,
          "two-item leave list publishes its owner");

    auto replacementFailure = table(1, true, NUM_PROFILES);
    consumed = 0;
    check(!load(replacementFailure, consumed),
          "failed replacement rejects an invalid owner");
    check(twoItemStateIsPublished(), "failed replacement preserves the previous live list");

    guiCurrentSaveGameVersion = NIV_SAVEGAME_DATATYPE_CHANGE - 1;
    auto invalidLegacy = legacyTable(MAXITEMS, 1, 7);
    consumed = 0;
    publishSentinelState();
    check(!load(invalidLegacy, consumed),
          "legacy leave item with an out-of-range item ID is rejected");
    check(sentinelStateIsPublished(), "invalid legacy item preserves the live list");

    auto validLegacy = legacyTable(NONE, 0, 7);
    consumed = 0;
    check(load(validLegacy, consumed), "valid legacy leave-item list loads");
    check(consumed == validLegacy.size(),
          "valid legacy leave-item list consumes its extent");
    check(gpLeaveListHead[0] && !gpLeaveListHead[0]->pNext,
          "valid legacy leave-item list publishes one terminated node");
    check(guiLeaveListOwnerProfileId[0] == 7,
          "valid legacy leave-item list publishes its owner");
    resetState();
    guiCurrentSaveGameVersion = SAVE_GAME_VERSION;

    auto empty = table();
    consumed = 0;
    check(load(empty, consumed), "empty leave-item table loads");
    check(consumed == empty.size(), "empty leave-item table consumes its extent");
    check(stateIsEmpty(), "empty leave-item table stays empty");

    return finish();
}
