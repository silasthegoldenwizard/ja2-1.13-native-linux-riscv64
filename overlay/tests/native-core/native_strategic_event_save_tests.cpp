/*
 * P710 native snapshot update: tests/native-core/native_strategic_event_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_strategic_event_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "Game Events.h"
#include "GameSettings.h"
#include "MemMan.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;

void check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}

template<class T>
void append(Bytes& bytes, const T& value)
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(value));
}

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadStrategicEventsFromSavedGame(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

STRATEGICEVENT event(UINT32 stamp, UINT32 parameter, UINT8 callback)
{
    STRATEGICEVENT value{};
    value.next = reinterpret_cast<STRATEGICEVENT*>(static_cast<std::uintptr_t>(0x1234));
    value.uiTimeStamp = stamp;
    value.uiParam = parameter;
    value.ubEventType = ONETIME_EVENT;
    value.ubCallbackID = callback;
    return value;
}

STRATEGICEVENT* seedPublishedEvent()
{
    DeleteAllStrategicEvents();
    gpEventList = static_cast<STRATEGICEVENT*>(MemAlloc(sizeof(STRATEGICEVENT)));
    check(gpEventList != nullptr, "published strategic-event seed allocated");
    *gpEventList = event(777, 888, 9);
    gpEventList->next = nullptr;
    return gpEventList;
}

void checkPublishedEventUnchanged(STRATEGICEVENT* published, const char* message)
{
    check(gpEventList == published && gpEventList && !gpEventList->next &&
              gpEventList->uiTimeStamp == 777 && gpEventList->uiParam == 888 &&
              gpEventList->ubCallbackID == 9,
          message);
}
}

int main()
{
    const auto savedMiniEvents = gGameExternalOptions.fMiniEventsEnabled;
    gGameExternalOptions.fMiniEventsEnabled = FALSE;
    try {
        DeleteAllStrategicEvents();

        Bytes valid;
        const UINT32 count = 2;
        const auto first = event(100, 11, 1);
        const auto second = event(200, 22, 2);
        append(valid, count);
        append(valid, first);
        append(valid, second);
        std::size_t consumed = 0;
        check(load(valid, consumed), "two strategic events load");
        check(consumed == valid.size(), "valid load consumes its complete record");
        check(gpEventList && gpEventList->next && !gpEventList->next->next,
              "valid load rebuilds a terminated two-node queue");
        check(gpEventList->uiTimeStamp == 100 && gpEventList->uiParam == 11,
              "first event data is preserved");
        check(gpEventList->next->uiTimeStamp == 200 && gpEventList->next->uiParam == 22,
              "second event data is preserved");
        DeleteAllStrategicEvents();

        Bytes truncated;
        append(truncated, count);
        append(truncated, first);
        const auto truncatedSize = truncated.size() - sizeof(STRATEGICEVENT) / 2;
        truncated.resize(truncatedSize);
        auto* published = seedPublishedEvent();
        consumed = 0;
        check(!load(truncated, consumed), "truncated event queue is rejected");
        check(gfBufferLoadFailed, "truncated event queue marks the bounded load failure");
        checkPublishedEventUnchanged(
            published, "failed event load preserves the published queue");
        check(consumed <= truncated.size(), "failed event load keeps the cursor in bounds");
        DeleteAllStrategicEvents();

        Bytes missingRecord;
        const UINT32 one = 1;
        append(missingRecord, one);
        published = seedPublishedEvent();
        consumed = 0;
        check(!load(missingRecord, consumed), "missing event record is rejected");
        checkPublishedEventUnchanged(published, "missing record preserves live event queue");
        DeleteAllStrategicEvents();

        Bytes impossibleCount;
        const UINT32 maximum = UINT32_MAX;
        append(impossibleCount, maximum);
        published = seedPublishedEvent();
        consumed = 0;
        check(!load(impossibleCount, consumed), "unbacked maximum event count is rejected");
        checkPublishedEventUnchanged(
            published, "maximum count failure preserves live event queue");
        DeleteAllStrategicEvents();

        gfBufferLoadFailed = FALSE;
        gGameExternalOptions.fMiniEventsEnabled = savedMiniEvents;
        std::cout << "PASS: " << checks
                  << " strategic event save lifecycle assertions\n";
        return 0;
    } catch (const std::exception& error) {
        DeleteAllStrategicEvents();
        gpLoadBufferStart = nullptr;
        giLoadBufferSize = 0;
        gfBufferLoadFailed = FALSE;
        gGameExternalOptions.fMiniEventsEnabled = savedMiniEvents;
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
