/*
 * P710 native snapshot update: tests/native-core/native_bullet_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_bullet_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "BufferMan.h"
#include "Bullets.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;
constexpr UINT32 kBulletSlots = 1000;
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
    const BOOLEAN result = LoadBulletStructureFromSavedGame(&cursor);
    consumed = static_cast<std::size_t>(cursor - begin);
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    return result;
}

void clearLoadedState()
{
    Bytes empty;
    const UINT32 count = 0;
    append(empty, count);
    std::size_t consumed = 0;
    check(load(empty, consumed), "empty bullet table clears live state");
    check(consumed == empty.size(), "empty bullet table consumes its exact extent");
    check(guiNumBullets == 0, "empty bullet table publishes a zero count");
}

void publishSentinelBullet()
{
    guiNumBullets = 1;
    auto* bullet = GetBulletPtr(0);
    std::memset(bullet, 0, sizeof(*bullet));
    bullet->iBullet = 0;
    bullet->fAllocated = TRUE;
    bullet->sGridNo = 123;
    bullet->ubFirerID = NOBODY;
}

bool sentinelBulletIsPublished()
{
    const auto* bullet = GetBulletPtr(0);
    return guiNumBullets == 1 && bullet != nullptr && bullet->fAllocated &&
           bullet->iBullet == 0 && bullet->sGridNo == 123 &&
           bullet->ubFirerID == NOBODY;
}

int finish()
{
    guiNumBullets = 0;
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    if (failures) {
        std::cerr << "FAIL: " << failures << " of " << checks
                  << " bullet-save lifecycle assertions\n";
        return 1;
    }
    std::cout << "PASS: " << checks
              << " bullet-save lifecycle assertions\n";
    return 0;
}

void overflowRegression()
{
    publishSentinelBullet();

    BULLET record{};
    record.ubFirerID = NOBODY;
    record.pFirer = reinterpret_cast<SOLDIERTYPE*>(
        static_cast<std::uintptr_t>(0x1111));
    record.pAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x2222));
    record.pShadowAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x3333));

    Bytes excessive;
    const UINT32 count = kBulletSlots + 1;
    excessive.reserve(sizeof(count) + count * sizeof(record));
    append(excessive, count);
    for (UINT32 index = 0; index < count; ++index) append(excessive, record);

    std::size_t consumed = 0;
    check(!load(excessive, consumed), "oversized bullet table is rejected");
    check(!gfBufferLoadFailed,
          "oversized bullet table is a semantic rejection, not a short read");
    check(consumed == sizeof(count), "oversized count is rejected before records");
    check(sentinelBulletIsPublished(),
          "oversized bullet table preserves the previously published bullets");
}

void invalidSoldierIDRegression()
{
    publishSentinelBullet();

    BULLET record{};
    record.ubFirerID.i = UINT16_MAX;
    Bytes invalid;
    const UINT32 count = 1;
    append(invalid, count);
    append(invalid, record);

    std::size_t consumed = 0;
    check(!load(invalid, consumed), "invalid bullet firer ID is rejected");
    check(!gfBufferLoadFailed,
          "invalid bullet firer ID is a semantic rejection, not a short read");
    check(consumed == invalid.size(), "invalid firer record has a bounded extent");
    check(sentinelBulletIsPublished(),
          "invalid firer preserves the previously published bullets");
}
}

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--overflow") == 0) {
        overflowRegression();
        return finish();
    }
    if (argc == 2 && std::strcmp(argv[1], "--invalid-soldier-id") == 0) {
        invalidSoldierIDRegression();
        return finish();
    }

    clearLoadedState();

    BULLET record{};
    record.iBullet = 77;
    record.ubFirerID = NOBODY;
    record.uiLastUpdate = 1234;
    record.pFirer = reinterpret_cast<SOLDIERTYPE*>(
        static_cast<std::uintptr_t>(0x1111));
    record.pAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x2222));
    record.pShadowAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x3333));
    record.pNodes[0] = reinterpret_cast<LEVELNODE*>(
        static_cast<std::uintptr_t>(0x4444));

    Bytes valid;
    const UINT32 one = 1;
    append(valid, one);
    append(valid, record);
    std::size_t consumed = 0;
    check(load(valid, consumed), "one bullet loads");
    check(consumed == valid.size(), "one-bullet load consumes its exact extent");
    check(guiNumBullets == 1, "one-bullet load publishes its count");
    auto* loaded = GetBulletPtr(0);
    check(loaded && loaded->iBullet == 0, "loaded bullet receives its live slot index");
    check(loaded && loaded->uiLastUpdate == 0, "loaded bullet resets its update clock");
    check(loaded && loaded->pFirer == nullptr,
          "loaded bullet rebuilds a missing firer pointer");
    check(loaded && loaded->pAniTile == nullptr && loaded->pShadowAniTile == nullptr,
          "loaded bullet discards serialized animation pointers");
    bool trailsCleared = loaded != nullptr;
    for (auto* node : loaded->pNodes) trailsCleared = trailsCleared && node == nullptr;
    check(trailsCleared, "loaded bullet discards serialized trail-node pointers");

    publishSentinelBullet();
    auto invalidTargetRecord = record;
    invalidTargetRecord.ubTargetID.i = UINT16_MAX;
    Bytes invalidTarget;
    append(invalidTarget, one);
    append(invalidTarget, invalidTargetRecord);
    consumed = 0;
    check(!load(invalidTarget, consumed), "invalid bullet target ID is rejected");
    check(!gfBufferLoadFailed,
          "invalid bullet target ID is a semantic rejection, not a short read");
    check(consumed == invalidTarget.size(), "invalid target record has a bounded extent");
    check(sentinelBulletIsPublished(),
          "invalid target preserves the previously published bullets");

    publishSentinelBullet();
    Bytes truncated;
    append(truncated, one);
    append(truncated, record);
    truncated.resize(sizeof(one) + sizeof(record) / 2);
    consumed = 0;
    check(!load(truncated, consumed), "truncated bullet record is rejected");
    check(gfBufferLoadFailed, "truncated bullet record marks the bounded load failure");
    check(consumed <= truncated.size(), "truncated bullet cursor remains in bounds");
    check(sentinelBulletIsPublished(),
          "truncated bullet table preserves the previously published bullets");

    publishSentinelBullet();
    Bytes excessiveCount;
    const UINT32 excessive = kBulletSlots + 1;
    append(excessiveCount, excessive);
    consumed = 0;
    check(!load(excessiveCount, consumed), "unbounded bullet count is rejected");
    check(consumed == excessiveCount.size(),
          "unbounded bullet count is rejected before reading a record");
    check(sentinelBulletIsPublished(),
          "unbounded bullet count preserves the previously published bullets");

    clearLoadedState();
    overflowRegression();
    invalidSoldierIDRegression();
    return finish();
}
