/*
 * P710 native snapshot update: tests/native-core/native_strategic_path_construction_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "core_types.h"

#include <cstdlib>
#include <iostream>

enum { NORTH, SOUTH, EAST, WEST };
constexpr INT32 MAP_WORLD_X = 18;
constexpr INT32 NORTH_MOVE = -18;
constexpr INT32 SOUTH_MOVE = 18;
constexpr INT32 EAST_MOVE = 1;
constexpr INT32 WEST_MOVE = -1;
constexpr UINT8 NORMAL_MVT = 1;

struct path
{
    UINT8 fSpeed;
    UINT16 uiSectorId;
    path* pNext;
    path* pPrev;
    UINT32 uiEta;
};
using PathSt = path;
using PathStPtr = path*;

UINT16 gusMapPathingData[256]{};

namespace {
INT32 requestedLength = 0;
INT32 allocationCalls = 0;
INT32 failAllocationAt = 0;
INT32 liveAllocations = 0;
INT32 releasedAllocations = 0;
INT32 checks = 0;
INT32 failures = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

void* MemAlloc(std::size_t size)
{
    ++allocationCalls;
    if (failAllocationAt != 0 && allocationCalls == failAllocationAt)
        return nullptr;
    void* allocation = std::malloc(size);
    if (allocation != nullptr)
        ++liveAllocations;
    return allocation;
}

void MemFree(void* allocation)
{
    if (allocation != nullptr)
    {
        --liveAllocations;
        ++releasedAllocations;
        std::free(allocation);
    }
}

UINT32 GetWorldTotalMin()
{
    return 0;
}

INT32 FindStratPath(INT16, INT16, INT16, BOOLEAN)
{
    for (INT32 index = 0; index < requestedLength; ++index)
        gusMapPathingData[index] = EAST;
    return requestedLength;
}

PathStPtr ClearStrategicPathList(PathStPtr head, INT16)
{
    while (head != nullptr)
    {
        PathStPtr next = head->pNext;
        MemFree(head);
        head = next;
    }
    return nullptr;
}

BOOLEAN AddSectorToPathList(PathStPtr, UINT16);
#include "strategic_path_construction.inc"

namespace {
void reset(INT32 length, INT32 failAt)
{
    requestedLength = length;
    allocationCalls = 0;
    failAllocationAt = failAt;
    liveAllocations = 0;
    releasedAllocations = 0;
}

void checkLateAppendFailure()
{
    reset(4, 4);
    PathStPtr result = BuildAStrategicPath(nullptr, 18, 22, -1, FALSE);
    check(result == nullptr, "a late append allocation failure rejects the path");
    check(releasedAllocations == 3,
          "a late append failure releases the head and complete allocated prefix");
    check(liveAllocations == 0, "a late append failure leaks no prefix node");
}

void checkSuccessfulOwnership()
{
    reset(4, 0);
    PathStPtr result = BuildAStrategicPath(nullptr, 18, 22, -1, FALSE);
    INT32 count = 0;
    PathStPtr previous = nullptr;
    for (PathStPtr node = result; node != nullptr; node = node->pNext)
    {
        check(node->pPrev == previous, "each successful path node retains its predecessor");
        previous = node;
        ++count;
    }
    check(count == 5, "a four-step path returns its original head and all five nodes");
    ClearStrategicPathList(result, -1);
    check(liveAllocations == 0, "the successful path remains wholly owned by its returned head");
}
}

int main()
{
    checkLateAppendFailure();
    checkSuccessfulOwnership();
    if (failures != 0)
        return 1;
    std::cout << "PASS: " << checks << " strategic path ownership assertions\n";
    return 0;
}
