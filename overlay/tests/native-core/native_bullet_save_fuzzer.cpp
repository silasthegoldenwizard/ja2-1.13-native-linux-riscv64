/*
 * P710 native snapshot update: tests/native-core/native_bullet_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for production bullet-table save decoding.
#include "BufferMan.h"
#include "Bullets.h"
#include "Overhead.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace {
using Bytes = std::vector<UINT8>;
constexpr UINT32 kBulletSlots = 1000;

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

BULLET recordFromSeed(const std::uint8_t* data, std::size_t size,
                      std::size_t index)
{
    const auto byte = [data, size](std::size_t position) {
        return size ? data[position % size] : std::uint8_t{0};
    };
    BULLET bullet{};
    bullet.iBullet = static_cast<INT32>(byte(index + 1));
    bullet.sGridNo = static_cast<INT32>(byte(index + 2)) * 17;
    bullet.iImpact = static_cast<INT32>(byte(index + 3));
    bullet.fAllocated = byte(index + 4) & 1;
    bullet.fReal = FALSE;
    bullet.usFlags = static_cast<UINT16>(byte(index + 5));
    bullet.uiLastUpdate = UINT32_MAX;

    switch (byte(index + 6) % 4) {
        case 0: bullet.ubFirerID = NOBODY; break;
        case 1: bullet.ubFirerID.i = 0; break;
        case 2: bullet.ubFirerID.i = TOTAL_SOLDIERS + 1; break;
        default: bullet.ubFirerID.i = UINT16_MAX; break;
    }
    switch (byte(index + 7) % 4) {
        case 0: bullet.ubTargetID = NOBODY; break;
        case 1: bullet.ubTargetID.i = 0; break;
        case 2: bullet.ubTargetID.i = TOTAL_SOLDIERS + 1; break;
        default: bullet.ubTargetID.i = UINT16_MAX; break;
    }

    bullet.pFirer = reinterpret_cast<SOLDIERTYPE*>(
        static_cast<std::uintptr_t>(0x1000 + index));
    bullet.pAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x2000 + index));
    bullet.pShadowAniTile = reinterpret_cast<ANITILE*>(
        static_cast<std::uintptr_t>(0x3000 + index));
    for (std::size_t node = 0; node < BULLET_TRACER_MAX_LENGTH; ++node) {
        bullet.pNodes[node] = reinterpret_cast<LEVELNODE*>(
            static_cast<std::uintptr_t>(0x4000 + index + node));
    }
    return bullet;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    const auto mode = data[0] % 5;
    Bytes bytes;

    if (mode == 0) {
        const UINT32 count = 0;
        append(bytes, count);
        return bytes;
    }
    if (mode == 1) {
        const UINT32 count = kBulletSlots + 1 + (size > 1 ? data[1] : 0);
        append(bytes, count);
        return bytes;
    }

    UINT32 count = size > 1 ? data[1] % 16 : 1;
    if (mode == 4 && size > 2 && data[2] == 0xff) count = kBulletSlots;
    append(bytes, count);
    for (UINT32 index = 0; index < count; ++index) {
        const auto bullet = recordFromSeed(data, size, index * 8);
        append(bytes, bullet);
    }

    if (mode == 3 && bytes.size() > sizeof(UINT32)) {
        const auto extent = sizeof(UINT32) +
            (static_cast<std::size_t>(size > 2 ? data[2] : 0) *
             (bytes.size() - sizeof(UINT32))) / 255;
        bytes.resize(std::min(extent, bytes.size()));
    }
    return bytes;
}

void installBounds(Bytes& bytes)
{
    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
}

}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (size > 65536) return 0;
    MercPtrs[0] = &Menptr[0];
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    installBounds(bytes);
    auto* begin = reinterpret_cast<CHAR8*>(bytes.data());
    auto* cursor = begin;
    const BOOLEAN loaded = LoadBulletStructureFromSavedGame(&cursor);
    const auto consumed = cursor - begin;
    if (consumed < 0 || static_cast<std::size_t>(consumed) > bytes.size()) {
        invariantFailure();
    }

    if (!loaded) {
        if (guiNumBullets != 0) invariantFailure();
    } else {
        if (guiNumBullets > kBulletSlots) invariantFailure();
        for (UINT32 index = 0; index < guiNumBullets; ++index) {
            const auto* bullet = GetBulletPtr(static_cast<INT32>(index));
            if (!bullet || bullet->iBullet != static_cast<INT32>(index) ||
                bullet->uiLastUpdate != 0 || bullet->pAniTile != nullptr ||
                bullet->pShadowAniTile != nullptr ||
                bullet->ubFirerID.i > TOTAL_SOLDIERS ||
                bullet->ubTargetID.i > TOTAL_SOLDIERS) {
                invariantFailure();
            }
            if (bullet->ubFirerID == NOBODY) {
                if (bullet->pFirer != nullptr) invariantFailure();
            } else if (bullet->pFirer != MercPtrs[bullet->ubFirerID.i]) {
                invariantFailure();
            }
            for (auto* node : bullet->pNodes) {
                if (node != nullptr) invariantFailure();
            }
        }
    }

    // The next production load clears the complete table before it decodes.
    // No accepted recipe creates animation ownership, so no harness cleanup is
    // required between inputs.
    guiNumBullets = 0;
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
