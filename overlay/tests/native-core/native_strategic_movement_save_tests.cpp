/*
 * P710 native snapshot update: tests/native-core/native_strategic_movement_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_strategic_movement_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production strategic-movement save lifecycle tests with synthetic records.
#include "BufferMan.h"
#include "GameVersion.h"
#include "MemMan.h"
#include "Overhead.h"
#include "SaveLoadGame.h"
#include "Strategic Movement.h"
#include "Strategic Pathing.h"
#include "strategicmap.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern UINT32 uniqueIDMask[8];

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

GROUP enemyGroup(UINT8 id, UINT16 size = 1)
{
    GROUP group{};
    group.usGroupTeam = ENEMY_TEAM;
    group.ubGroupID = id;
    group.ubGroupSize = size;
    group.ubSectorX = 9;
    group.ubSectorY = 10;
    group.pWaypoints = reinterpret_cast<WAYPOINT*>(
        static_cast<std::uintptr_t>(0x1111));
    group.pEnemyGroup = reinterpret_cast<ENEMYGROUP*>(
        static_cast<std::uintptr_t>(0x2222));
    group.next = reinterpret_cast<GROUP*>(static_cast<std::uintptr_t>(0x3333));
    return group;
}

void appendEnemy(Bytes& bytes, UINT8 id, UINT32 waypointCount)
{
    const auto group = enemyGroup(id);
    ENEMYGROUP enemy{};
    enemy.ubNumTroops = 1;
    append(bytes, group);
    append(bytes, enemy);
    append(bytes, waypointCount);
    for (UINT32 index = 0; index < waypointCount; ++index) {
        WAYPOINT waypoint{};
        waypoint.x = static_cast<UINT8>(10 + index);
        waypoint.y = 10;
        waypoint.next = reinterpret_cast<WAYPOINT*>(
            static_cast<std::uintptr_t>(0x4444 + index));
        append(bytes, waypoint);
    }
}

Bytes enemyList(const std::vector<UINT8>& ids, UINT32 waypointCount = 1)
{
    Bytes bytes;
    const auto count = static_cast<UINT32>(ids.size());
    append(bytes, count);
    for (const auto id : ids) appendEnemy(bytes, id, waypointCount);
    appendMask(bytes);
    return bytes;
}

Bytes playerList(UINT8 profile)
{
    Bytes bytes;
    const UINT32 count = 1;
    GROUP group{};
    group.usGroupTeam = OUR_TEAM;
    group.ubGroupID = 4;
    group.ubGroupSize = 1;
    group.ubSectorX = 9;
    group.ubSectorY = 10;
    group.pWaypoints = reinterpret_cast<WAYPOINT*>(
        static_cast<std::uintptr_t>(0x5555));
    group.pPlayerList = reinterpret_cast<PLAYERGROUP*>(
        static_cast<std::uintptr_t>(0x6666));
    group.next = reinterpret_cast<GROUP*>(static_cast<std::uintptr_t>(0x7777));
    const UINT32 memberCount = 1;
    const UINT32 profileID = profile;
    const UINT32 waypointCount = 0;
    append(bytes, count);
    append(bytes, group);
    append(bytes, memberCount);
    append(bytes, profileID);
    append(bytes, waypointCount);
    appendMask(bytes);
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadStrategicMovementGroupsFromSavedGameFile(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

void clearLoadedState()
{
    auto empty = enemyList({});
    std::size_t consumed = 0;
    check(load(empty, consumed), "empty movement list clears live state");
    check(consumed == empty.size(), "empty movement list consumes its exact extent");
}

void directRemovalLifecycle()
{
    auto* group = static_cast<GROUP*>(MemAlloc(sizeof(GROUP)));
    auto* enemy = static_cast<ENEMYGROUP*>(MemAlloc(sizeof(ENEMYGROUP)));
    auto* waypoint = static_cast<WAYPOINT*>(MemAlloc(sizeof(WAYPOINT)));
    std::memset(group, 0, sizeof(*group));
    std::memset(enemy, 0, sizeof(*enemy));
    std::memset(waypoint, 0, sizeof(*waypoint));
    group->usGroupTeam = ENEMY_TEAM;
    group->ubGroupID = 7;
    group->pEnemyGroup = enemy;
    group->pWaypoints = waypoint;
    gpGroupList = group;
    std::memset(uniqueIDMask, 0, sizeof(UINT32) * 8);
    uniqueIDMask[0] = UINT32_C(1) << 7;
    RemoveGroupFromList(group);
    check(gpGroupList == nullptr, "direct removal detaches the group");
    check((uniqueIDMask[0] & (UINT32_C(1) << 7)) == 0,
          "direct removal releases the group ID");
}

PathStPtr pathNode(UINT32 sector)
{
    auto* node = static_cast<PathStPtr>(MemAlloc(sizeof(PathSt)));
    std::memset(node, 0, sizeof(*node));
    node->uiSectorId = sector;
    return node;
}

PathStPtr pathList(UINT32 first, UINT32 second, UINT32 third)
{
    auto* head = pathNode(first);
    auto* middle = pathNode(second);
    auto* tail = pathNode(third);
    head->pNext = middle;
    middle->pPrev = head;
    middle->pNext = tail;
    tail->pPrev = middle;
    return head;
}

void releasePath(PathStPtr path)
{
    path = MoveToBeginningOfPathList(path);
    while (path) {
        auto* next = path->pNext;
        MemFree(path);
        path = next;
    }
}

void strategicPathRemovalLifecycle()
{
    check(!AddSectorToPathList(nullptr, CALCULATE_STRATEGIC_INDEX(1, 1)),
          "a null path cannot leak an unreachable first node");

    auto* single = pathNode(CALCULATE_STRATEGIC_INDEX(1, 1));
    auto* result = RemoveTailFromStrategicPath(single);
    check(result == nullptr, "removing the only path node returns an empty list");
    if (result) {
        // Production callers immediately normalize the returned head. A stale
        // pointer must therefore be sanitizer-visible in the regression.
        result = MoveToBeginningOfPathList(result);
    }

    auto* list = pathList(CALCULATE_STRATEGIC_INDEX(1, 1),
                          CALCULATE_STRATEGIC_INDEX(2, 1),
                          CALCULATE_STRATEGIC_INDEX(3, 1));
    result = RemoveTailFromStrategicPath(list);
    check(result == list && list->pNext && !list->pNext->pNext,
          "tail removal preserves a terminated two-node path");
    releasePath(result);

    single = pathNode(CALCULATE_STRATEGIC_INDEX(4, 4));
    result = RemoveSectorFromStrategicPathList(single, 4, 4);
    check(result == nullptr, "removing the only matching sector returns an empty list");

    list = pathList(CALCULATE_STRATEGIC_INDEX(1, 2),
                    CALCULATE_STRATEGIC_INDEX(2, 2),
                    CALCULATE_STRATEGIC_INDEX(3, 2));
    result = RemoveSectorFromStrategicPathList(list, 3, 2);
    check(result == list && list->pNext && !list->pNext->pNext,
          "sector removal unlinks a matching tail safely");
    releasePath(result);

    list = pathList(CALCULATE_STRATEGIC_INDEX(1, 3),
                    CALCULATE_STRATEGIC_INDEX(2, 3),
                    CALCULATE_STRATEGIC_INDEX(3, 3));
    result = RemoveSectorFromStrategicPathList(list, 1, 3);
    check(result && result->uiSectorId == CALCULATE_STRATEGIC_INDEX(2, 3) &&
              result->pPrev == nullptr,
          "sector removal publishes the next node when removing the head");
    releasePath(result);
}
}

int main()
{
    const UINT32 savedVersion = guiCurrentSaveGameVersion;
    guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
    MercPtrs[0] = &Menptr[0];
    Menptr[0].ubID = SoldierID(0);
    Menptr[0].ubProfile = 10;
    Menptr[0].bActive = TRUE;
    gTacticalStatus.Team[OUR_TEAM].bFirstID = SoldierID(0);
    gTacticalStatus.Team[OUR_TEAM].bLastID = SoldierID(0);
    clearLoadedState();
    directRemovalLifecycle();
    strategicPathRemovalLifecycle();

    auto valid = enemyList({5}, 1);
    std::size_t consumed = 0;
    check(load(valid, consumed), "one enemy movement group loads");
    check(consumed == valid.size(), "valid movement group consumes its exact extent");
    check(gpGroupList && gpGroupList->next == nullptr,
          "valid movement list publishes one terminated group");
    check(gpGroupList && gpGroupList->pEnemyGroup &&
              gpGroupList->pEnemyGroup->ubNumTroops == 1,
          "enemy-group ownership is rebuilt from serialized data");
    check(gpGroupList && gpGroupList->pWaypoints &&
              gpGroupList->pWaypoints->x == 10 &&
              gpGroupList->pWaypoints->next == nullptr,
          "waypoint ownership and termination are rebuilt");
    check(uniqueIDMask[0] == (UINT32_C(1) << 5),
          "valid movement load rebuilds its unique-ID bit");
    clearLoadedState();

    auto player = playerList(10);
    consumed = 0;
    check(load(player, consumed), "one player movement group loads");
    check(consumed == player.size(), "player movement group consumes its exact extent");
    check(gpGroupList && gpGroupList->pPlayerList &&
              gpGroupList->pPlayerList->ubProfileID == 10 &&
              gpGroupList->pPlayerList->ubID == SoldierID(0) &&
              gpGroupList->pPlayerList->pSoldier == &Menptr[0] &&
              gpGroupList->pPlayerList->next == nullptr,
          "player membership is rebuilt from the live mercenary");
    check(gpGroupList && gpGroupList->pWaypoints == nullptr,
          "zero-waypoint player group discards its serialized pointer");
    clearLoadedState();

    auto truncatedPlayer = playerList(10);
    truncatedPlayer.resize(sizeof(UINT32) + sizeof(GROUP) + sizeof(UINT32));
    consumed = 0;
    check(!load(truncatedPlayer, consumed), "truncated player membership is rejected");
    check(gpGroupList == nullptr, "truncated player membership leaves no movement list");

    auto missingMask = enemyList({6}, 0);
    missingMask.resize(missingMask.size() - sizeof(UINT32) * 8);
    consumed = 0;
    check(!load(missingMask, consumed), "missing unique-ID mask is rejected");
    check(gfBufferLoadFailed, "missing mask marks the bounded load failure");
    check(consumed <= missingMask.size(), "missing-mask cursor remains in bounds");
    check(gpGroupList == nullptr, "missing mask rolls back the partial movement list");
    check(uniqueIDMask[0] == 0, "missing mask rolls back the unique-ID state");

    auto truncatedEnemy = enemyList({8}, 0);
    truncatedEnemy.resize(sizeof(UINT32) + sizeof(GROUP) + sizeof(ENEMYGROUP) / 2);
    consumed = 0;
    check(!load(truncatedEnemy, consumed), "truncated enemy payload is rejected");
    check(gpGroupList == nullptr, "truncated enemy payload leaves no movement list");

    auto truncatedWaypoint = enemyList({9}, 1);
    truncatedWaypoint.resize(truncatedWaypoint.size() - sizeof(UINT32) * 8 -
                             sizeof(WAYPOINT) / 2);
    consumed = 0;
    check(!load(truncatedWaypoint, consumed), "truncated waypoint is rejected");
    check(gpGroupList == nullptr, "truncated waypoint leaves no movement list");

    Bytes excessive;
    const UINT32 maximum = UINT32_MAX;
    append(excessive, maximum);
    consumed = 0;
    check(!load(excessive, consumed), "unbounded movement-group count is rejected");
    check(gpGroupList == nullptr, "unbounded group count leaves no movement list");

    auto duplicate = enemyList({12, 12}, 0);
    consumed = 0;
    check(load(duplicate, consumed), "legacy duplicate group IDs remain loadable");
    check(uniqueIDMask[0] == (UINT32_C(1) << 12),
          "duplicate IDs cannot carry into an unrelated mask bit");
    clearLoadedState();

    auto highID = enemyList({255}, 0);
    consumed = 0;
    check(load(highID, consumed), "highest representable group ID loads");
    check(uniqueIDMask[7] == (UINT32_C(1) << 31),
          "highest group ID uses a defined unsigned mask bit");
    clearLoadedState();

    guiCurrentSaveGameVersion = savedVersion;
    Menptr[0].bActive = FALSE;
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " strategic-movement lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " strategic-movement lifecycle assertions\n";
    return 0;
}
