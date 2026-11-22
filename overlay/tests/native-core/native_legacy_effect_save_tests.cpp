/*
 * P710 native snapshot update: tests/native-core/native_legacy_effect_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Genuine original legacy light/smoke save loaders with synthetic bytes only.
// No game assets or existing saves are used.
#include "BufferMan.h"
#include "Overhead Types.h"
#include "LightEffects.h"
#include "SmokeEffects.h"
#include "SaveLoadGame.h"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

extern LIGHTEFFECT gLightEffectData[500];
extern UINT32 guiNumLightEffects;

namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;
constexpr UINT32 lightCapacity = 500;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

struct BufferBounds {
    explicit BufferBounds(Bytes& bytes) {
        gpLoadBufferStart = reinterpret_cast<char*>(bytes.data());
        giLoadBufferSize = static_cast<INT32>(bytes.size());
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() {
        gpLoadBufferStart = nullptr;
        giLoadBufferSize = 0;
        gfBufferLoadFailed = FALSE;
    }
};

Bytes countHeader(UINT32 count) {
    Bytes bytes(4, 0);
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (count >> (8 * i)) & 255;
    return bytes;
}

void seedLights() {
    std::memset(gLightEffectData, 0, sizeof(LIGHTEFFECT) * lightCapacity);
    gLightEffectData[0].fAllocated = TRUE;
    gLightEffectData[0].sGridNo = 4321;
    guiNumLightEffects = 7;
}

void checkLightsUnchanged(const char* why) {
    check(guiNumLightEffects == 7 && gLightEffectData[0].fAllocated &&
          gLightEffectData[0].sGridNo == 4321, why);
}

void rejectInvalidLights() {
    for (UINT32 count : {lightCapacity + 1u, 0xffffffffu, 1u}) {
        auto bytes = countHeader(count);
        seedLights();
        guiCurrentSaveGameVersion = 75;
        BufferBounds bounds(bytes);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!LoadLightEffectsFromSavedGame(&cursor),
              "oversized or truncated legacy light table rejected");
        check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()),
              "invalid legacy light table stops after its complete count header");
        checkLightsUnchanged(
            "invalid legacy light table preserves published effects and count");
    }
}

void acceptEmptyLights() {
    auto bytes = countHeader(0);
    seedLights();
    guiCurrentSaveGameVersion = 75;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(LoadLightEffectsFromSavedGame(&cursor), "empty legacy light table accepted");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) &&
          guiNumLightEffects == 0 && !gLightEffectData[0].fAllocated,
          "empty legacy light table atomically clears published effects");
}

void seedSmoke() {
    std::memset(gSmokeEffectData, 0, sizeof(gSmokeEffectData));
    gSmokeEffectData[0].fAllocated = TRUE;
    gSmokeEffectData[0].sGridNo = 8765;
    guiNumSmokeEffects = 7;
}

void checkSmokeUnchanged(const char* why) {
    check(guiNumSmokeEffects == 7 && gSmokeEffectData[0].fAllocated &&
          gSmokeEffectData[0].sGridNo == 8765, why);
}

void rejectInvalidSmoke() {
    for (UINT32 count : {NUM_SMOKE_EFFECT_SLOTS + 1u, 0xffffffffu, 1u}) {
        auto bytes = countHeader(count);
        seedSmoke();
        guiCurrentSaveGameVersion = 74;
        BufferBounds bounds(bytes);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!LoadSmokeEffectsFromLoadGameFile(&cursor),
              "oversized or truncated legacy smoke table rejected");
        check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()),
              "invalid legacy smoke table stops after its complete count header");
        checkSmokeUnchanged(
            "invalid legacy smoke table preserves published effects and count");
    }
}

void acceptEmptySmoke() {
    auto bytes = countHeader(0);
    seedSmoke();
    guiCurrentSaveGameVersion = 74;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(LoadSmokeEffectsFromLoadGameFile(&cursor), "empty legacy smoke table accepted");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) &&
          guiNumSmokeEffects == 0 && !gSmokeEffectData[0].fAllocated,
          "empty legacy smoke table atomically clears published effects");
}

void acceptPre37AlignmentRecord() {
    SMOKEEFFECT record = {};
    record.sGridNo = 1234;
    auto bytes = countHeader(0);
    const auto oldSize = bytes.size();
    bytes.resize(oldSize + sizeof(record));
    std::memcpy(bytes.data() + oldSize, &record, sizeof(record));
    seedSmoke();
    guiCurrentSaveGameVersion = 36;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(LoadSmokeEffectsFromLoadGameFile(&cursor),
          "pre-37 zero-count alignment record accepted");
    check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) &&
          guiNumSmokeEffects == 0 && gSmokeEffectData[0].sGridNo == 1234,
          "pre-37 alignment record is consumed without publishing an active count");
}
}

int main() {
    try {
        rejectInvalidLights();
        acceptEmptyLights();
        rejectInvalidSmoke();
        acceptEmptySmoke();
        acceptPre37AlignmentRecord();
        std::cout << "legacy effect save load: " << checks
                  << " checks passed; original loaders, synthetic bytes only\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "legacy effect save load failed at check " << checks << ": "
                  << error.what() << '\n';
        return 1;
    }
}
