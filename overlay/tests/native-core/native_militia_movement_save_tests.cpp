/*
 * P710 native snapshot update: tests/native-core/native_militia_movement_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "GameVersion.h"
#include "MilitiaSquads.h"
#include "Strategic Pathing.h"
#include "mapscreen.h"

#include <cstdint>
#include <iostream>
#include <vector>

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

PathSt pathNode(UINT32 sector, UINT8 speed = NORMAL_MVT)
{
    PathSt node{};
    node.uiSectorId = sector;
    node.uiEta = 1234 + sector;
    node.fSpeed = speed;
    node.pNext = reinterpret_cast<PathSt*>(static_cast<std::uintptr_t>(0x1111));
    node.pPrev = reinterpret_cast<PathSt*>(static_cast<std::uintptr_t>(0x2222));
    return node;
}

void appendSlot(Bytes& bytes, const std::vector<PathSt>& nodes, INT16 groupID)
{
    const auto count = static_cast<UINT32>(nodes.size());
    append(bytes, count);
    for (const auto& node : nodes) append(bytes, node);
    append(bytes, groupID);
}

Bytes paths(const std::vector<PathSt>& firstNodes = {}, INT16 firstGroup = -1)
{
    Bytes bytes;
    const UINT8 count = MILITIA_PATROLS_MAX;
    append(bytes, count);
    appendSlot(bytes, firstNodes, firstGroup);
    for (UINT32 slot = 1; slot < MILITIA_PATROLS_MAX; ++slot) {
        appendSlot(bytes, {}, -1);
    }
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed,
             UINT32 version = MILITIA_PATH_PLOTTING)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result =
        LoadMilitiaMovementInformationFromSavedGame(&cursor, version);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

bool liveStateIsEmpty()
{
    for (const auto& entry : gMilitiaPath) {
        if (entry.path != nullptr || entry.sGroupid != -1) return false;
    }
    return true;
}

void clearState()
{
    DeleteAllMilitiaPaths();
    check(liveStateIsEmpty(), "militia path cleanup leaves an empty table");
}

void longValidPathRegression()
{
    const UINT32 sector = 9 + 9 * MAP_WORLD_X;
    std::vector<PathSt> nodes;
    for (UINT32 index = 0; index < MAX_PATH_LIST_SIZE + 1; ++index) {
        nodes.push_back(pathNode(sector + index % 2));
    }
    auto bytes = paths(nodes, 7);
    std::size_t consumed = 0;
    check(load(bytes, consumed),
          "appended militia route longer than the tactical path limit loads");
    check(consumed == bytes.size(),
          "long militia route consumes its exact extent");
    UINT32 loaded = 0;
    PathSt* previous = nullptr;
    for (PathSt* node = gMilitiaPath[0].path; node; node = node->pNext) {
        check(node->pPrev == previous,
              "long militia route rebuilds each backward link");
        previous = node;
        ++loaded;
    }
    check(loaded == nodes.size(),
          "long militia route retains every saved strategic node");
    clearState();
}

void invalidNodeCountRegression()
{
    Bytes excessive;
    const UINT8 pathCount = MILITIA_PATROLS_MAX;
    const UINT32 nodeCount = UINT32_MAX;
    append(excessive, pathCount);
    append(excessive, nodeCount);
    std::size_t consumed = 0;
    check(!load(excessive, consumed), "excessive militia path count is rejected");
    check(gfBufferLoadFailed,
          "path count that cannot fit the payload records a bounded read failure");
    check(consumed == sizeof(pathCount) + sizeof(nodeCount),
          "excessive count is rejected at its bounded prefix");
    check(liveStateIsEmpty(), "excessive count leaves no live militia paths");
}

int finish()
{
    DeleteAllMilitiaPaths();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " militia-movement save assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " militia-movement save assertions\n";
    return 0;
}
}

int main()
{
    clearState();

    auto empty = paths();
    std::size_t consumed = 0;
    check(load(empty, consumed), "empty militia path table loads");
    check(consumed == empty.size(), "empty militia path table consumes its extent");
    check(liveStateIsEmpty(), "empty militia path table publishes no ownership");

    const UINT32 sector = 9 + 9 * MAP_WORLD_X;
    auto valid = paths({pathNode(sector), pathNode(sector + 1)}, 7);
    consumed = 0;
    check(load(valid, consumed), "two-node militia path loads");
    check(consumed == valid.size(), "valid militia path consumes its exact extent");
    auto* head = gMilitiaPath[0].path;
    check(head && head->uiSectorId == sector && head->pPrev == nullptr,
          "loaded militia path publishes a clean head");
    check(head && head->pNext && head->pNext->uiSectorId == sector + 1 &&
              head->pNext->pPrev == head && head->pNext->pNext == nullptr,
          "loaded militia path rebuilds bidirectional ownership");
    check(gMilitiaPath[0].sGroupid == 7,
          "loaded militia path publishes its group ID");
    clearState();

    auto invalidSector = paths({pathNode(MAP_WORLD_X * MAP_WORLD_Y)}, 7);
    consumed = 0;
    check(!load(invalidSector, consumed), "out-of-map path sector is rejected");
    check(!gfBufferLoadFailed,
          "invalid militia sector is a semantic rejection, not a short read");
    check(liveStateIsEmpty(), "invalid sector leaves no live militia path");

    auto invalidSpeed = paths({pathNode(sector, NORMAL_MVT + 1)}, 7);
    consumed = 0;
    check(!load(invalidSpeed, consumed), "invalid path speed is rejected");
    check(!gfBufferLoadFailed,
          "invalid militia speed is a semantic rejection, not a short read");
    check(liveStateIsEmpty(), "invalid speed leaves no live militia path");

    auto invalidGroup = paths({pathNode(sector)}, 256);
    consumed = 0;
    check(!load(invalidGroup, consumed), "out-of-range militia group ID is rejected");
    check(!gfBufferLoadFailed,
          "invalid militia group is a semantic rejection, not a short read");
    check(liveStateIsEmpty(), "invalid group ID leaves no live militia path");

    auto truncated = valid;
    truncated.resize(1 + sizeof(UINT32) + sizeof(PathSt) / 2);
    consumed = 0;
    check(!load(truncated, consumed), "truncated militia path is rejected");
    check(gfBufferLoadFailed, "truncated path marks the bounded load failure");
    check(consumed <= truncated.size(), "truncated path cursor remains in bounds");
    check(liveStateIsEmpty(), "truncated path rolls back all militia ownership");

    longValidPathRegression();
    invalidNodeCountRegression();

    Bytes oldSave;
    consumed = 0;
    check(load(oldSave, consumed, MILITIA_PATH_PLOTTING - 1),
          "pre-militia-path save requires no payload");
    check(consumed == 0 && liveStateIsEmpty(),
          "old save leaves an empty path table without consuming data");

    return finish();
}
