/*
 * P710 native snapshot update: tests/native-core/native_strategic_movement_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production strategic-movement save decoding.
#include "BufferMan.h"
#include "GameVersion.h"
#include "Overhead.h"
#include "SaveLoadGame.h"
#include "Strategic Movement.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern UINT32 uniqueIDMask[8];

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

void appendMask(Bytes& bytes)
{
    const UINT32 mask[8] = {};
    const auto* begin = reinterpret_cast<const UINT8*>(mask);
    bytes.insert(bytes.end(), begin, begin + sizeof(mask));
}

GROUP group(UINT8 team, UINT8 id, UINT16 size)
{
    GROUP value{};
    value.usGroupTeam = team;
    value.ubGroupID = id;
    value.ubGroupSize = size;
    value.ubSectorX = 9;
    value.ubSectorY = 10;
    value.pWaypoints = reinterpret_cast<WAYPOINT*>(
        static_cast<std::uintptr_t>(0x1111));
    value.pEnemyGroup = reinterpret_cast<ENEMYGROUP*>(
        static_cast<std::uintptr_t>(0x2222));
    value.next = reinterpret_cast<GROUP*>(static_cast<std::uintptr_t>(0x3333));
    return value;
}

void appendWaypointList(Bytes& bytes, UINT32 count)
{
    append(bytes, count);
    for (UINT32 index = 0; index < count; ++index) {
        WAYPOINT waypoint{};
        waypoint.x = static_cast<UINT8>(10 + index);
        waypoint.y = 10;
        waypoint.next = reinterpret_cast<WAYPOINT*>(
            static_cast<std::uintptr_t>(0x4000 + index));
        append(bytes, waypoint);
    }
}

Bytes validList(bool duplicateIDs)
{
    Bytes bytes;
    const UINT32 count = 2;
    append(bytes, count);

    const auto player = group(OUR_TEAM, 4, 1);
    const UINT32 playerCount = 1;
    const UINT32 profile = 10;
    append(bytes, player);
    append(bytes, playerCount);
    append(bytes, profile);
    appendWaypointList(bytes, 1);

    const auto enemy = group(ENEMY_TEAM, duplicateIDs ? 4 : 255, 1);
    ENEMYGROUP enemyPayload{};
    enemyPayload.ubNumTroops = 1;
    append(bytes, enemy);
    append(bytes, enemyPayload);
    appendWaypointList(bytes, 2);
    appendMask(bytes);
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    const auto mode = data[0] % 4;
    if (mode == 0) return Bytes(data + 1, data + size);
    if (mode == 3) {
        Bytes empty;
        const UINT32 count = 0;
        append(empty, count);
        appendMask(empty);
        return empty;
    }

    auto bytes = validList(mode == 2);
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

void initializeSoldier()
{
    static bool initialized = false;
    if (initialized) return;
    MercPtrs[0] = &Menptr[0];
    Menptr[0].ubID = SoldierID(0);
    Menptr[0].ubProfile = 10;
    Menptr[0].bActive = TRUE;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
    initialized = true;
}

void installBounds(Bytes& bytes)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
}

void clearState()
{
    Bytes empty;
    const UINT32 count = 0;
    append(empty, count);
    appendMask(empty);
    installBounds(empty);
    auto* cursor = reinterpret_cast<CHAR8*>(empty.data());
    if (!LoadStrategicMovementGroupsFromSavedGameFile(&cursor)) invariantFailure();
    if (gpGroupList != nullptr) invariantFailure();
    for (const auto word : uniqueIDMask) {
        if (word != 0) invariantFailure();
    }
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (size > 65536) return 0;
    initializeSoldier();
    clearState();
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    guiCurrentSaveGameVersion =
        (data[0] & 0x80) ? STRATEGIC_TEAM_GROUPS - 1 : SAVE_GAME_VERSION;
    installBounds(bytes);
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadStrategicMovementGroupsFromSavedGameFile(&cursor);
    const auto consumed = cursor - begin;
    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }

    if (!loaded) {
        if (gpGroupList != nullptr) invariantFailure();
        for (const auto word : uniqueIDMask) {
            if (word != 0) invariantFailure();
        }
    } else {
        UINT32 expectedMask[8] = {};
        std::size_t groupCount = 0;
        for (auto* current = gpGroupList; current; current = current->next) {
            if (++groupCount > 255 || current->usGroupTeam >= MAXTEAMS) {
                invariantFailure();
            }
            expectedMask[current->ubGroupID / 32] |=
                UINT32_C(1) << (current->ubGroupID % 32);

            std::size_t waypointCount = 0;
            for (auto* waypoint = current->pWaypoints;
                 waypoint; waypoint = waypoint->next) {
                if (++waypointCount > 255) invariantFailure();
            }
            if (current->usGroupTeam == OUR_TEAM) {
                std::size_t memberCount = 0;
                for (auto* member = current->pPlayerList;
                     member; member = member->next) {
                    if (++memberCount > 255 || !member->pSoldier) {
                        invariantFailure();
                    }
                }
            } else if (!current->pEnemyGroup) {
                invariantFailure();
            }
        }
        for (std::size_t index = 0; index < 8; ++index) {
            if (uniqueIDMask[index] != expectedMask[index]) invariantFailure();
        }
    }

    clearState();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
    return 0;
}
