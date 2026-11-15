/*
 * P710 native snapshot update: tests/native-core/native_soldier_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Genuine SOLDIERTYPE Save/Load and FileMan/bfVFS, with authored state only.
// This is a current native-ABI round trip, not a historical wire-format test.
#include "Soldier Control.h"
#include "SaveLoadGame.h"
#include "GameVersion.h"
#include "FileMan.h"
#include "BufferMan.h"
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

namespace fs = std::filesystem;
namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0, failures = 0;
void require(bool ok, const char* why) {
    ++checks;
    if (!ok) throw std::runtime_error(why);
}
void expect(bool ok, const char* why) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << why << '\n'; }
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-soldier-save-XXXXXX";
        const auto* made = mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
    }
    ~Temporary() { std::error_code error; fs::remove_all(root, error); }
};
void mount(const fs::path& root) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(
        "soldier-save-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(
        vfs::Path(), vfs::Path(root.u8string()));
    require(directory->init(), "fixture VFS directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    require(getVFS()->addLocation(location, mounted), "fixture VFS mounted");
}
struct BufferBounds {
    explicit BufferBounds(Bytes& bytes, std::size_t length) {
        gpLoadBufferStart = reinterpret_cast<char*>(bytes.data());
        giLoadBufferSize = static_cast<INT32>(length);
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() {
        gpLoadBufferStart = nullptr; giLoadBufferSize = 0; gfBufferLoadFailed = FALSE;
    }
};
template<class T> std::size_t offset(const SOLDIERTYPE& soldier, const T& field) {
    return reinterpret_cast<const char*>(&field) - reinterpret_cast<const char*>(&soldier);
}
Bytes save(SOLDIERTYPE& soldier) {
    char filename[] = "soldier";
    auto file = FileOpen(filename, FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE);
    require(file != 0, "FileMan opens owned save fixture");
    const auto saved = soldier.Save(file);
    const auto length = FileGetPos(file);
    FileClose(file);
    require(saved, "genuine SOLDIERTYPE::Save succeeds");
    require(length > SIZEOF_SOLDIERTYPE_POD, "save contains inventory and trailing structures");
    file = FileOpen(filename, FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    require(file != 0, "FileMan reopens owned fixture");
    Bytes bytes(length);
    UINT32 read = 0;
    const auto readOK = FileRead(file, bytes.data(), length, &read);
    FileClose(file);
    require(readOK && read == length, "FileMan reads the complete saved record");
    require(std::memcmp(bytes.data(), &soldier, SIZEOF_SOLDIERTYPE_POD) == 0,
            "save POD bytes match actual compiler layout");
    return bytes;
}
void roundtrip(bool occupied) {
    SOLDIERTYPE source, loaded;
    // These live objects own the two pointer targets. No loaded pointer is
    // dereferenced or freed. SOLDIERTYPE does not own these fixture targets.
    OBJECTTYPE temporaryObject;
    KEY_ON_RING key{};
    UINT16 cachedSurface = 42;
    INT16 cacheHits = 7;
    source.ubID = 17;
    source.name[0] = static_cast<CHAR16>('S');
    source.name[1] = static_cast<CHAR16>(0x00e9);
    source.ubBodyType = 3;
    source.bActionPoints = 83;
    source.bInitialActionPoints = 91;
    source.bOldLife = 62;
    source.bVisible = 1;
    source.bActive = 1;
    source.bTeam = 2;
    source.pTempObject = &temporaryObject;
    source.pKeyRing = &key;
    source.bInSector = 1;
    source.bFlashPortraitFrame = 5;
    source.sFractLife = 1234;
    source.bBleeding = 7;
    source.bBreath = 73;
    source.bBreathMax = 96;
    source.sBreathRed = 321;
    source.iHealableInjury = 0x123456;
    source.lUnregainableBreath = 0x13579;
    source.dXPos = 123.5f;
    source.dYPos = 456.25f;
    source.sGridNo = 12345;
    source.usAnimState = 123;
    source.sReloadDelay = -1234;
    source.ubAttackerID = 19;
    source.ubPreviousAttackerID = 23;
    source.sInsertionGridNo = 32100;
    source.AnimCache.usCachedSurfaces = &cachedSurface;
    source.AnimCache.sCacheHits = &cacheHits;
    source.AnimCache.ubCacheSize = 1;
    source.bSide = 2;
    source.bViewRange = 13;
    source.bNewOppCnt = 3;
    source.bService = 5;
    source.usAniCode = 17;
    source.usAniFrame = 31;
    source.sAniDelay = 543;
    source.iEndofContractTime = 0x12345678;
    source.usMedicalDeposit = 4321;
    source.usSoldierFlagMask2 = 0x13572468;
    source.sDragGridNo = 54321;
    source.stats.bLife = 77;
    source.stats.bLifeMax = 94;
    source.stats.bAgility = 81;
    source.stats.bDexterity = 79;
    source.stats.bStrength = 71;
    source.stats.bMarksmanship = 67;
    source.stats.bMedical = 23;
    source.stats.bMechanical = 29;
    source.stats.bExplosive = 31;
    source.stats.bExpLevel = 4;
    source.aiData.usActionData = 7654;
    source.pathing.sDestination = 23456;
    if (occupied) {
        // Assign an authored object directly. No item definitions or assets
        // are loaded, and there are no attachments or active LBE objects.
        source.inv[0].usItem = 7;
        source.inv[0].ubNumberOfObjects = 1;
        source.inv[0][0]->data.objectStatus = 73;
        source.inv.bNewItemCount[0] = 3;
        source.inv.bNewItemCycleCount[0] = 5;
    }
    auto bytes = save(source);
    const auto length = bytes.size();
    bytes.insert(bytes.end(), {0xa5, 0x5a, 0xc3, 0x3c});
    const auto before = bytes;
    BufferBounds bounds(bytes, length); // The canary is outside the valid record.
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    const bool result = loaded.Load(&cursor);
    const auto consumed = cursor - reinterpret_cast<char*>(bytes.data());
    std::cout << "case=" << (occupied ? "occupied" : "empty")
              << " saved=" << length << " consumed=" << consumed
              << " load=" << result << " buffer_failed=" << int(gfBufferLoadFailed)
              << " bInSector=" << int(loaded.bInSector)
              << " fractLife=" << loaded.sFractLife << '\n';
    expect(result, "genuine buffered SOLDIERTYPE::Load accepts its own Save output");
    expect(!gfBufferLoadFailed, "load stays inside the saved record");
    expect(consumed == static_cast<std::ptrdiff_t>(length), "load consumes exactly one record");
    expect(bytes == before, "load preserves input bytes and all trailing canaries");
    expect(loaded.ubID == source.ubID && !std::memcmp(loaded.name, source.name, sizeof(source.name)),
           "ID and name before pointer padding survive");
    expect(loaded.bActionPoints == 83 && loaded.bInitialActionPoints == 91 &&
           loaded.bOldLife == 62 && loaded.bTeam == 2, "scalars before first pointer survive");
    expect(!std::memcmp(&loaded.pTempObject, &source.pTempObject, sizeof(source.pTempObject)) &&
           !std::memcmp(&loaded.pKeyRing, &source.pKeyRing, sizeof(source.pKeyRing)),
           "first two pointer representations retain compiler padding alignment");
    // Clear representations before any further action. Targets remain owned by
    // the two local fixture objects, including when Load rejects the record.
    loaded.pTempObject = nullptr; loaded.pKeyRing = nullptr;
    source.pTempObject = nullptr; source.pKeyRing = nullptr;
    expect(loaded.bInSector == 1 && loaded.bFlashPortraitFrame == 5 && loaded.sFractLife == 1234,
           "fields immediately after first pointers survive");
    expect(loaded.bBleeding == 7 && loaded.bBreath == 73 && loaded.bBreathMax == 96 &&
           loaded.sBreathRed == 321, "life and breath fields after pointers survive");
    expect(loaded.iHealableInjury == 0x123456 && loaded.lUnregainableBreath == 0x13579,
           "injury and native long fields survive");
    expect(loaded.dXPos == 123.5f && loaded.dYPos == 456.25f && loaded.sGridNo == 12345,
           "position fields survive");
    expect(loaded.usAnimState == 123 && loaded.sReloadDelay == -1234 &&
           loaded.ubAttackerID == 19 && loaded.ubPreviousAttackerID == 23 &&
           loaded.sInsertionGridNo == 32100, "fields before animation cache survive");
    expect(loaded.AnimCache.usCachedSurfaces == &cachedSurface &&
           loaded.AnimCache.sCacheHits == &cacheHits && loaded.AnimCache.ubCacheSize == 1,
           "nested animation cache retains compiler alignment");
    loaded.AnimCache = {}; source.AnimCache = {};
    expect(loaded.bSide == 2 && loaded.bViewRange == 13 && loaded.bNewOppCnt == 3 &&
           loaded.bService == 5, "fields immediately after animation cache survive");
    expect(loaded.usAniCode == 17 && loaded.usAniFrame == 31 && loaded.sAniDelay == 543,
           "animation script index, frame, and delay survive");
    expect(loaded.iEndofContractTime == 0x12345678 && loaded.usMedicalDeposit == 4321,
           "fields around later pointer boundaries survive");
    expect(loaded.usSoldierFlagMask2 == 0x13572468 && loaded.sDragGridNo == 54321,
           "fields at end of POD survive");
    expect(loaded.inv.size() == source.inv.size(), "inventory slot count survives");
    expect(loaded.inv[0].usItem == source.inv[0].usItem &&
           loaded.inv[0].ubNumberOfObjects == source.inv[0].ubNumberOfObjects &&
           loaded.inv[0][0]->data.objectStatus == source.inv[0][0]->data.objectStatus &&
           loaded.inv.bNewItemCount[0] == source.inv.bNewItemCount[0] &&
           loaded.inv.bNewItemCycleCount[0] == source.inv.bNewItemCycleCount[0],
           "inventory object, stack data, and item counters survive");
    expect(loaded.stats.bLife == 77 && loaded.stats.bLifeMax == 94 && loaded.stats.bAgility == 81,
           "statistics after inventory survive");
    expect(loaded.aiData.usActionData == 7654 && loaded.pathing.sDestination == 23456,
           "AI and pathing structures after inventory survive");
    expect(loaded.uiMercChecksum == source.uiMercChecksum && loaded.GetChecksum() == source.uiMercChecksum,
           "saved and recomputed soldier checksums match");
    expect(loaded.ai_masterplan_ == nullptr, "nonserialized AI ownership stays empty");
}
}
int main() {
    try {
        Temporary temporary;
        mount(temporary.root);
        require(InitializeFileManager(nullptr), "original FileMan initialized");
        guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
        SOLDIERTYPE layout;
        std::cout << "pointer_size=" << sizeof(void*) << " char16_size=" << sizeof(CHAR16)
                  << " long_size=" << sizeof(long) << " pod=" << SIZEOF_SOLDIERTYPE_POD
                  << " team_end=" << offset(layout, layout.bTeam) + sizeof(layout.bTeam)
                  << " pTempObject=" << offset(layout, layout.pTempObject)
                  << " pKeyRing=" << offset(layout, layout.pKeyRing)
                  << " bInSector=" << offset(layout, layout.bInSector) << '\n';
        roundtrip(false);
        roundtrip(true);
        ShutdownFileManager();
        std::cout << "soldier save: " << checks << " checks, " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) {
        ShutdownFileManager();
        std::cerr << "soldier save harness failed at check " << checks << ": " << error.what() << '\n';
        return 2;
    }
}
