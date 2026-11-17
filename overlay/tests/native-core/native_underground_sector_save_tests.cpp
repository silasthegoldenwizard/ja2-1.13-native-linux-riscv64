/*
 * P710 native snapshot update: tests/native-core/native_underground_sector_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "Campaign Init.h"
#include "Queen Command.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

extern UNDERGROUND_SECTORINFO* gpUndergroundSectorInfoTail;
extern UNDERGROUND_SECTORINFO* NewUndergroundNode(
    UINT8 ubSectorX, UINT8 ubSectorY, UINT8 ubSectorZ);

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

BOOLEAN load(Bytes& bytes, std::size_t& consumed)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN result = LoadUnderGroundSectorInfoFromSavedGame(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

UNDERGROUND_SECTORINFO sector(UINT8 x, UINT8 y, UINT8 z, UINT16 troops)
{
    UNDERGROUND_SECTORINFO value{};
    value.ubSectorX = x;
    value.ubSectorY = y;
    value.ubSectorZ = z;
    value.ubNumTroops = troops;
    value.next = reinterpret_cast<UNDERGROUND_SECTORINFO*>(
        static_cast<std::uintptr_t>(0x1234));
    return value;
}
}

int main()
{
    TrashUndergroundSectorInfo();

    const UINT32 one = 1;
    const auto firstRecord = sector(9, 10, 1, 7);
    Bytes oneRecord;
    append(oneRecord, one);
    append(oneRecord, firstRecord);
    std::size_t consumed = 0;
    check(load(oneRecord, consumed), "one underground sector loads");
    check(consumed == oneRecord.size(), "one-record load consumes the complete record");
    auto* originalHead = gpUndergroundSectorInfoHead;
    check(originalHead != nullptr, "one-record load publishes a head");
    check(gpUndergroundSectorInfoTail == originalHead,
          "one-record load publishes the same node as its tail");
    check(originalHead && originalHead->next == nullptr,
          "serialized next pointer is not imported");
    check(originalHead && originalHead->ubSectorX == 9 &&
              originalHead->ubSectorY == 10 && originalHead->ubSectorZ == 1 &&
              originalHead->ubNumTroops == 7,
          "one-record load preserves sector data");

    auto* appended = NewUndergroundNode(10, 10, 2);
    check(gpUndergroundSectorInfoHead == originalHead,
          "appending after load preserves the loaded head");
    check(originalHead && originalHead->next == appended,
          "appending after load links from the loaded node");
    check(gpUndergroundSectorInfoTail == appended && appended->next == nullptr,
          "appending after load advances a terminated tail");
    TrashUndergroundSectorInfo();

    const UINT32 two = 2;
    const auto secondRecord = sector(9, 10, 2, 5);
    Bytes validTwo;
    append(validTwo, two);
    append(validTwo, firstRecord);
    append(validTwo, secondRecord);
    consumed = 0;
    check(load(validTwo, consumed), "two underground sectors load");
    check(consumed == validTwo.size(), "two-record load consumes the complete record");
    check(gpUndergroundSectorInfoHead && gpUndergroundSectorInfoTail &&
              gpUndergroundSectorInfoHead != gpUndergroundSectorInfoTail &&
              gpUndergroundSectorInfoHead->next == gpUndergroundSectorInfoTail &&
              gpUndergroundSectorInfoTail->next == nullptr,
          "two-record load rebuilds a terminated head and tail");
    TrashUndergroundSectorInfo();

    Bytes truncated;
    append(truncated, two);
    append(truncated, firstRecord);
    append(truncated, secondRecord);
    truncated.resize(truncated.size() - sizeof(UNDERGROUND_SECTORINFO) / 2);
    consumed = 0;
    check(!load(truncated, consumed), "truncated underground-sector list is rejected");
    check(gfBufferLoadFailed, "truncated list marks the bounded load failure");
    check(consumed <= truncated.size(), "truncated load keeps the cursor in bounds");
    check(gpUndergroundSectorInfoHead == nullptr &&
              gpUndergroundSectorInfoTail == nullptr,
          "truncated load releases all partial global state");
    TrashUndergroundSectorInfo();

    Bytes missingRecord;
    append(missingRecord, one);
    consumed = 0;
    check(!load(missingRecord, consumed), "missing underground-sector record is rejected");
    check(gpUndergroundSectorInfoHead == nullptr &&
              gpUndergroundSectorInfoTail == nullptr,
          "missing record leaves no global list");

    Bytes impossibleCount;
    const UINT32 maximum = UINT32_MAX;
    append(impossibleCount, maximum);
    consumed = 0;
    check(!load(impossibleCount, consumed), "unbacked maximum record count is rejected");
    check(gpUndergroundSectorInfoHead == nullptr &&
              gpUndergroundSectorInfoTail == nullptr,
          "maximum count failure leaves no global list");

    TrashUndergroundSectorInfo();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " underground-sector lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " underground-sector lifecycle assertions\n";
    return 0;
}
