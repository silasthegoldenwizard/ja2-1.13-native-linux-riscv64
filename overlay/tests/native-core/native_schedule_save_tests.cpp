/*
 * P710 native snapshot update: tests/native-core/native_schedule_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted schedule, mini-event, and facility checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "Scheduling.h"

#include <cstdint>
#include <iostream>
#include <vector>

extern BOOLEAN gfSchedulesHosed;

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

SCHEDULENODE schedule(UINT8 id, UINT16 time)
{
    SCHEDULENODE value{};
    value.next = reinterpret_cast<SCHEDULENODE*>(
        static_cast<std::uintptr_t>(0x1234));
    value.usTime[0] = time;
    value.ubAction[0] = SCHEDULE_ACTION_GRIDNO;
    value.ubScheduleID = id;
    value.ubSoldierID = SoldierID(7);
    return value;
}

Bytes scheduleList(const std::vector<SCHEDULENODE>& records)
{
    Bytes bytes;
    const auto count = static_cast<UINT8>(records.size());
    append(bytes, count);
    for (const auto& record : records) append(bytes, record);
    return bytes;
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadSchedulesFromSave(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

std::size_t listLength()
{
    std::size_t count = 0;
    for (auto* node = gpScheduleList; node; node = node->next) ++count;
    return count;
}
}

int main()
{
    DestroyAllSchedulesWithoutDestroyingEvents();
    gfSchedulesHosed = FALSE;

    auto first = scheduleList({schedule(4, 420)});
    std::size_t consumed = 0;
    check(load(first, consumed), "one schedule loads");
    check(consumed == first.size(), "one schedule consumes its exact extent");
    check(listLength() == 1, "one schedule publishes one node");
    check(gpScheduleList && gpScheduleList->next == nullptr,
          "serialized next pointer is not imported");
    check(gpScheduleList && gpScheduleList->ubScheduleID == 4 &&
              gpScheduleList->usTime[0] == 420,
          "one schedule preserves its serialized fields");
    check(gubScheduleID == 2, "one schedule preserves the legacy next-ID state");

    auto replacement = scheduleList({schedule(8, 600), schedule(9, 720)});
    consumed = 0;
    check(load(replacement, consumed), "a complete list replaces existing schedules");
    check(consumed == replacement.size(), "replacement consumes its exact extent");
    check(listLength() == 2, "replacement publishes exactly two nodes");
    check(gpScheduleList && gpScheduleList->ubScheduleID == 8 &&
              gpScheduleList->next && gpScheduleList->next->ubScheduleID == 9 &&
              gpScheduleList->next->next == nullptr,
          "replacement rebuilds a terminated list in serialized order");
    check(gubScheduleID == 3, "replacement preserves the legacy next-ID state");

    auto* acceptedHead = gpScheduleList;
    auto truncated = scheduleList({schedule(10, 800), schedule(11, 900)});
    truncated.resize(truncated.size() - sizeof(SCHEDULENODE) / 2);
    consumed = 0;
    check(!load(truncated, consumed), "a truncated schedule list is rejected");
    check(gfBufferLoadFailed, "truncation marks the bounded-load failure");
    check(consumed <= truncated.size(), "truncation keeps the cursor in bounds");
    check(gpScheduleList == acceptedHead && listLength() == 2,
          "truncation preserves the previously accepted list");
    check(gubScheduleID == 3,
          "truncation preserves the previously accepted next-ID state");

    Bytes impossible{0};
    gfSchedulesHosed = TRUE;
    consumed = 0;
    check(!load(impossible, consumed), "an unbacked wrapped schedule count is rejected");
    check(gfBufferLoadFailed, "wrapped-count rejection marks load failure");
    check(gpScheduleList == acceptedHead && listLength() == 2,
          "wrapped-count rejection preserves live schedules");
    gfSchedulesHosed = FALSE;

    gfBufferLoadFailed = FALSE;
    check(!LoadSchedulesFromSave(nullptr), "a null cursor owner is rejected");
    check(gfBufferLoadFailed, "a null cursor owner marks load failure");
    check(gpScheduleList == acceptedHead && listLength() == 2,
          "a null cursor owner preserves live schedules");

    auto empty = scheduleList({});
    consumed = 0;
    check(load(empty, consumed), "an empty schedule list loads");
    check(consumed == empty.size(), "an empty list consumes only its count");
    check(gpScheduleList == nullptr, "an empty list clears prior schedules");
    check(gubScheduleID == 1, "an empty list preserves the legacy next-ID state");

    DestroyAllSchedulesWithoutDestroyingEvents();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    gfSchedulesHosed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " schedule save lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " schedule save lifecycle assertions\n";
    return 0;
}
