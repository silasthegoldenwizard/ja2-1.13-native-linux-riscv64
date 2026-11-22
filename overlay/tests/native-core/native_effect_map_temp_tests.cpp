/*
 * P710 native snapshot update: tests/native-core/native_effect_map_temp_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original map-temp light/smoke loaders and FileMan/bfVFS, with synthetic
// effect records only. No game assets or existing saves are used.
#include "BufferMan.h"
#include "Overhead Types.h"
#include "LightEffects.h"
#include "SmokeEffects.h"
#include "SaveLoadGame.h"
#include "Tactical Save.h"
#include "Campaign Types.h"
#include "GameVersion.h"
#include "FileMan.h"

#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

extern LIGHTEFFECT gLightEffectData[500];
extern UINT32 guiNumLightEffects;

namespace fs = std::filesystem;
namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;
constexpr UINT32 lightCapacity = 500;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-effect-map-temp-XXXXXX";
        auto* made = mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
        fs::create_directories(root / "Temp");
    }
    ~Temporary() {
        std::error_code error;
        fs::remove_all(root, error);
    }
};

void mount(const fs::path& root) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(
        "effect-map-temp-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(
        vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "fixture VFS directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "fixture VFS mounted");
}

Bytes countHeader(UINT32 count) {
    Bytes bytes(4, 0);
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (count >> (8 * i)) & 255;
    return bytes;
}

template <typename Record>
Bytes oneRecord(const Record& record) {
    auto bytes = countHeader(1);
    const auto oldSize = bytes.size();
    bytes.resize(oldSize + sizeof(record));
    std::memcpy(bytes.data() + oldSize, &record, sizeof(record));
    return bytes;
}

void writeFixture(UINT32 type, const Bytes& bytes) {
    char filename[128] = {};
    GetMapTempFileName(type, filename, 9, 1, 0);
    auto file = FileOpen(filename, FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, FALSE);
    check(file != 0, "original FileMan opens map-temp fixture");
    UINT32 written = 0;
    check(FileWrite(file, const_cast<UINT8*>(bytes.data()),
                    static_cast<UINT32>(bytes.size()), &written) &&
              written == bytes.size(),
          "original FileMan writes exact map-temp bytes");
    FileClose(file);
}

void seedLights() {
    std::memset(gLightEffectData, 0, sizeof(LIGHTEFFECT) * lightCapacity);
    gLightEffectData[0].fAllocated = TRUE;
    gLightEffectData[0].sGridNo = 4321;
    guiNumLightEffects = 7;
}

void checkLightsUnchanged(const char* why) {
    check(guiNumLightEffects == 7 && gLightEffectData[0].fAllocated &&
              gLightEffectData[0].sGridNo == 4321,
          why);
}

void rejectInvalidLights() {
    for (UINT32 count : {1u, lightCapacity + 1u, 0xffffffffu}) {
        writeFixture(SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, countHeader(count));
        seedLights();
        guiCurrentSaveGameVersion = DYNAMIC_FLASHLIGHTS;
        check(!LoadLightEffectsFromMapTempFile(9, 1, 0),
              "truncated or oversized map-temp light table rejected");
        checkLightsUnchanged(
            "invalid map-temp light table preserves published effects and count");
    }
}

void acceptLights() {
    writeFixture(SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, countHeader(0));
    seedLights();
    guiCurrentSaveGameVersion = DYNAMIC_FLASHLIGHTS;
    check(LoadLightEffectsFromMapTempFile(9, 1, 0), "empty map-temp light table accepted");
    check(guiNumLightEffects == 0 && !gLightEffectData[0].fAllocated,
          "empty map-temp light table atomically clears published effects");

    LIGHTEFFECT record = {};
    record.sGridNo = 1234;
    record.ubDuration = 19;
    writeFixture(SF_LIGHTING_EFFECTS_TEMP_FILE_EXISTS, oneRecord(record));
    check(LoadLightEffectsFromMapTempFile(9, 1, 0), "one map-temp light accepted");
    check(guiNumLightEffects == 1 && gLightEffectData[0].sGridNo == 1234 &&
              gLightEffectData[0].ubDuration == 19,
          "one map-temp light publishes exact decoded record");
}

void seedSmoke() {
    std::memset(gSmokeEffectData, 0, sizeof(gSmokeEffectData));
    gSmokeEffectData[0].fAllocated = TRUE;
    gSmokeEffectData[0].sGridNo = 8765;
    guiNumSmokeEffects = 7;
}

void checkSmokeUnchanged(const char* why) {
    check(guiNumSmokeEffects == 7 && gSmokeEffectData[0].fAllocated &&
              gSmokeEffectData[0].sGridNo == 8765,
          why);
}

void rejectInvalidSmoke() {
    for (UINT32 count : {1u, NUM_SMOKE_EFFECT_SLOTS + 1u, 0xffffffffu}) {
        writeFixture(SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS, countHeader(count));
        seedSmoke();
        check(!LoadSmokeEffectsFromMapTempFile(9, 1, 0),
              "truncated or oversized map-temp smoke table rejected");
        checkSmokeUnchanged(
            "invalid map-temp smoke table preserves published effects and count");
    }
}

void acceptSmoke() {
    writeFixture(SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS, countHeader(0));
    seedSmoke();
    check(LoadSmokeEffectsFromMapTempFile(9, 1, 0), "empty map-temp smoke table accepted");
    check(guiNumSmokeEffects == 0 && !gSmokeEffectData[0].fAllocated,
          "empty map-temp smoke table atomically clears published effects");

    SMOKEEFFECT record = {};
    record.sGridNo = 5678;
    record.ubDuration = 23;
    writeFixture(SF_SMOKE_EFFECTS_TEMP_FILE_EXISTS, oneRecord(record));
    check(LoadSmokeEffectsFromMapTempFile(9, 1, 0), "one map-temp smoke accepted");
    check(guiNumSmokeEffects == 1 && gSmokeEffectData[0].sGridNo == 5678 &&
              gSmokeEffectData[0].ubDuration == 23,
          "one map-temp smoke publishes exact decoded record");
}
}

int main() {
    try {
        Temporary temporary;
        mount(temporary.root);
        check(InitializeFileManager(nullptr), "original FileMan initialized");
        rejectInvalidLights();
        acceptLights();
        rejectInvalidSmoke();
        acceptSmoke();
        ShutdownFileManager();
        std::cout << "effect map-temp load: " << checks
                  << " checks passed; original loaders and FileMan, synthetic records only\n";
        return 0;
    } catch (const std::exception& error) {
        ShutdownFileManager();
        std::cerr << "effect map-temp load failed at check " << checks << ": "
                  << error.what() << '\n';
        return 1;
    }
}
