/*
 * P710 native snapshot update: tests/native-core/native_soldier_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_soldier_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
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
#include "Action Items.h"
#include "Items.h"
#include <climits>
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
extern int gLastLBEUniqueID;
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

template<class T> void append(Bytes& bytes, const T& value, std::size_t size = sizeof(T)) {
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + size);
}
void appendBytes(Bytes& bytes, const Bytes& child) { bytes.insert(bytes.end(), child.begin(), child.end()); }
Bytes objectRecord(UINT16 item, int count, UINT8 number = 1) {
    OBJECTTYPE object;
    object.usItem = item; object.ubNumberOfObjects = number;
    Bytes bytes;
    append(bytes, object, SIZEOF_OBJECTTYPE_POD); append(bytes, count);
    return bytes;
}
Bytes stackRecord(int count, ObjectData data = ObjectData()) {
    Bytes bytes;
    append(bytes, data); append(bytes, count);
    return bytes;
}
Bytes lbeRecord(int count, int id = 1) {
    LBENODE node{};
    node.uniqueID = id;
    Bytes bytes;
    append(bytes, node, SIZEOF_LBENODE_POD); append(bytes, count);
    return bytes;
}
void mark(OBJECTTYPE& value) { value.usItem = 7; value.ubNumberOfObjects = 1; value[0]->data.objectStatus = 73; }
bool marked(OBJECTTYPE& value) {
    return value.usItem == 7 && value.ubNumberOfObjects == 1 && value.objectStack.size() == 1 &&
           value[0]->data.objectStatus == 73;
}
void mark(StackedObjectData& value) {
    value.data.objectStatus = 73; value.attachments.resize(1); mark(value.attachments.front());
}
bool marked(StackedObjectData& value) {
    return value.data.objectStatus == 73 && value.attachments.size() == 1 && marked(value.attachments.front());
}
void mark(LBENODE& value) { value.uniqueID = 17; value.inv.resize(1); mark(value.inv[0]); }
bool marked(LBENODE& value) { return value.uniqueID == 17 && value.inv.size() == 1 && marked(value.inv[0]); }
void mark(Inventory& value) {
    value = Inventory(1); value.bNewItemCount.assign(1, 3); value.bNewItemCycleCount.assign(1, 5); mark(value[0]);
}
bool marked(Inventory& value) {
    return value.size() == 1 && value.bNewItemCount == std::vector<int>{3} &&
           value.bNewItemCycleCount == std::vector<int>{5} && marked(value[0]);
}
template<class T> void rejects(Bytes bytes, const char* label) {
    std::cout << "reject=" << label << '\n';
    const auto original = bytes;
    const auto lbeCount = LBEArray.size();
    const auto lastID = gLastLBEUniqueID;
    T destination;
    mark(destination);
    {
        BufferBounds bounds(bytes, bytes.size());
        auto* start = reinterpret_cast<char*>(bytes.data());
        auto* cursor = start;
        expect(!destination.Load(&cursor), "buffer rejects malformed nested save record");
        expect(cursor == start, "failed buffer record preserves its cursor");
        expect(marked(destination), "failed buffer record preserves its destination");
        expect(bytes == original && LBEArray.size() == lbeCount && gLastLBEUniqueID == lastID,
               "failed buffer record preserves source and LBE state");
    }
    auto filename = std::string("malformed-") + std::to_string(checks);
    auto file = FileOpen(filename.data(), FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE);
    require(file != 0, "open malformed fixture");
    UINT32 written = 0;
    require(FileWrite(file, bytes.data(), bytes.size(), &written) && written == bytes.size(), "write malformed fixture");
    FileClose(file);
    file = FileOpen(filename.data(), FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    require(file != 0, "read malformed fixture");
    expect(!destination.Load(file), "file rejects malformed nested save record");
    expect(FileGetPos(file) == 0, "failed file record preserves its cursor");
    FileClose(file);
    expect(marked(destination) && LBEArray.size() == lbeCount && gLastLBEUniqueID == lastID,
           "failed file record preserves destination and LBE state");
}
template<class T, class Check> void accepts(Bytes bytes, Check valid) {
    const auto lastID = gLastLBEUniqueID;
    const auto original = bytes;
    T destination;
    {
        BufferBounds bounds(bytes, bytes.size());
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        expect(destination.Load(&cursor) && valid(destination), "valid nested buffer record preserves its state");
        expect(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()) && !gfBufferLoadFailed,
               "valid nested buffer record consumes its exact extent");
    }
    auto filename = std::string("valid-") + std::to_string(checks);
    auto file = FileOpen(filename.data(), FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE);
    require(file != 0, "open valid nested fixture");
    UINT32 written = 0;
    require(FileWrite(file, bytes.data(), bytes.size(), &written) && written == bytes.size(), "write valid nested fixture");
    FileClose(file);
    file = FileOpen(filename.data(), FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    require(file != 0, "read valid nested fixture");
    expect(destination.Load(file) && valid(destination), "valid nested file record preserves its state");
    expect(FileGetPos(file) == static_cast<INT32>(bytes.size()), "valid nested file record consumes its exact extent");
    FileClose(file);
    expect(bytes == original, "valid nested input remains exact");
    gLastLBEUniqueID = lastID;
}
Bytes objectChain(unsigned levels) {
    Bytes bytes;
    ObjectData data;
    data.objectStatus = 73;
    for (unsigned level = 0; level < levels; ++level) {
        // Empty attachment slots retain their nested state without item definitions.
        appendBytes(bytes, objectRecord(NONE, 1, 0));
        appendBytes(bytes, stackRecord(level + 1 < levels ? 1 : 0, data));
    }
    return bytes;
}
bool hasObjectChain(OBJECTTYPE& value, unsigned levels) {
    auto* object = &value;
    for (unsigned level = 0; level < levels; ++level) {
        if (object->usItem != NONE || object->ubNumberOfObjects != 0 ||
            object->objectStack.size() != 1 || (*object)[0]->data.objectStatus != 73) return false;
        const auto children = level + 1 < levels ? 1u : 0u;
        if ((*object)[0]->attachments.size() != children) return false;
        if (children) object = &(*object)[0]->attachments.front();
    }
    return true;
}
void depthRecords() {
    const auto atLimit = objectChain(64);
    const auto valid = [](OBJECTTYPE& value) { return hasObjectChain(value, 64); };
    accepts<OBJECTTYPE>(atLimit, valid);
    rejects<OBJECTTYPE>(objectChain(65), "attachment depth exceeds 64");

    // A completed LBE child must roll back when a later attachment exceeds the limit.
    const auto itemClass = Item[7].usItemClass;
    Item[7].usItemClass = IC_LBEGEAR;
    ObjectData lbe;
    lbe.lbe.bLBE = -1; lbe.lbe.uniqueID = 100;
    auto laterDepthFailure = objectRecord(7, 2, 2);
    appendBytes(laterDepthFailure, stackRecord(0, lbe));
    appendBytes(laterDepthFailure, lbeRecord(0, 100));
    appendBytes(laterDepthFailure, stackRecord(1));
    appendBytes(laterDepthFailure, atLimit);
    rejects<OBJECTTYPE>(laterDepthFailure, "depth failure rolls back prior LBE child");
    Item[7].usItemClass = itemClass;
    accepts<OBJECTTYPE>(atLimit, valid);
}
void ammoRecords() {
    const auto itemClass = Item[7].usItemClass;
    const auto version = guiCurrentSaveGameVersion;
    for (auto gunClass : {IC_GUN, IC_LAUNCHER}) {
        Item[7].usItemClass = gunClass;
        for (UINT16 ammo : {UINT16(NONE), UINT16(MAXITEMS - 1)}) {
            guiCurrentSaveGameVersion = version;
            ObjectData data;
            data.gun.bGunStatus = 73; data.gun.usGunAmmoItem = ammo;
            auto current = objectRecord(7, 1);
            appendBytes(current, stackRecord(0, data));
            const auto valid = [ammo](OBJECTTYPE& value) {
                return marked(value) && value[0]->data.gun.usGunAmmoItem == ammo;
            };
            accepts<OBJECTTYPE>(current, valid);

            OLD_OBJECTTYPE_101 legacy{};
            legacy.usItem = 7; legacy.ubNumberOfObjects = 1;
            legacy.ugYucky.bGunStatus = 73; legacy.ugYucky.usGunAmmoItem = ammo;
            Bytes old; append(old, legacy);
            guiCurrentSaveGameVersion = NIV_SAVEGAME_DATATYPE_CHANGE - 1;
            accepts<OBJECTTYPE>(old, valid);
        }
        for (UINT16 ammo : {UINT16(MAXITEMS), UINT16(UINT16_MAX)}) {
            guiCurrentSaveGameVersion = version;
            ObjectData data;
            data.gun.bGunStatus = 73;
            auto current = objectRecord(7, 2, 2);
            appendBytes(current, stackRecord(0, data));
            data.gun.usGunAmmoItem = ammo;
            appendBytes(current, stackRecord(0, data));
            rejects<OBJECTTYPE>(current, "later current gun stack has invalid ammo ID");
            auto child = objectRecord(7, 1);
            appendBytes(child, stackRecord(0, data));
            auto parent = objectRecord(NONE, 1, 0);
            appendBytes(parent, stackRecord(1)); appendBytes(parent, child);
            rejects<OBJECTTYPE>(parent, "attachment has invalid embedded gun ammo ID");

            OLD_OBJECTTYPE_101 legacy{};
            legacy.usItem = 7; legacy.ubNumberOfObjects = 1;
            legacy.ugYucky.bGunStatus = 73; legacy.ugYucky.usGunAmmoItem = ammo;
            Bytes old; append(old, legacy);
            guiCurrentSaveGameVersion = NIV_SAVEGAME_DATATYPE_CHANGE - 1;
            rejects<OBJECTTYPE>(old, "legacy gun has invalid ammo ID");
        }
    }

    // Money shares the gun union bytes. Its amount is not an ammo item ID.
    Item[7].usItemClass = IC_MONEY;
    ObjectData money;
    money.money.bMoneyStatus = 73; money.money.uiMoneyAmount = UINT32_MAX;
    require(money.gun.usGunAmmoItem >= MAXITEMS, "current money overlaps invalid ammo bits");
    auto currentMoney = objectRecord(7, 1);
    appendBytes(currentMoney, stackRecord(0, money));
    const auto validMoney = [](OBJECTTYPE& value) {
        return marked(value) && value[0]->data.money.uiMoneyAmount == UINT32_MAX;
    };
    guiCurrentSaveGameVersion = version;
    accepts<OBJECTTYPE>(currentMoney, validMoney);
    OLD_OBJECTTYPE_101 legacyMoney{};
    legacyMoney.usItem = 7; legacyMoney.ubNumberOfObjects = 1;
    legacyMoney.ugYucky.bMoneyStatus = 73; legacyMoney.ugYucky.uiMoneyAmount = UINT32_MAX;
    require(legacyMoney.ugYucky.usGunAmmoItem >= MAXITEMS, "legacy money overlaps invalid ammo bits");
    Bytes oldMoney; append(oldMoney, legacyMoney);
    guiCurrentSaveGameVersion = NIV_SAVEGAME_DATATYPE_CHANGE - 1;
    accepts<OBJECTTYPE>(oldMoney, validMoney);

    // Legacy stacks use status arrays in the union, including for gun item classes.
    Item[7].usItemClass = IC_GUN;
    OLD_OBJECTTYPE_101 legacyStack{};
    legacyStack.usItem = 7; legacyStack.ubNumberOfObjects = 2;
    std::memset(&legacyStack.ugYucky, 0xff, sizeof(legacyStack.ugYucky));
    legacyStack.ugYucky.bStatus[0] = 73; legacyStack.ugYucky.bStatus[1] = 68;
    require(legacyStack.ugYucky.usGunAmmoItem >= MAXITEMS, "legacy status array overlaps invalid ammo bits");
    Bytes oldStack; append(oldStack, legacyStack);
    accepts<OBJECTTYPE>(oldStack, [](OBJECTTYPE& value) {
        return value.usItem == 7 && value.ubNumberOfObjects == 2 && value.objectStack.size() == 2 &&
               value[0]->data.objectStatus == 73 && value[1]->data.objectStatus == 68;
    });
    Item[7].usItemClass = itemClass;
    guiCurrentSaveGameVersion = version;
}
void malformedRecords() {
    for (int count : {-1, MAX_ITEMS_IN_LBE + 1, INT_MAX}) rejects<LBENODE>(lbeRecord(count), "LBE count");
    rejects<LBENODE>(lbeRecord(1), "truncated LBE inventory");
    rejects<LBENODE>(lbeRecord(0, INT_MAX), "LBE ID overflow");
    for (int count : {-1, INT_MAX, 1}) rejects<StackedObjectData>(stackRecord(count), "attachment count or truncation");
    auto invalidChild = stackRecord(1);
    appendBytes(invalidChild, objectRecord(MAXITEMS, 0, 0));
    rejects<StackedObjectData>(invalidChild, "invalid attachment child");
    rejects<OBJECTTYPE>(objectRecord(MAXITEMS, 0, 0), "modern item ID");
    rejects<OBJECTTYPE>(objectRecord(7, 0), "nonzero item with empty stack");
    for (int count : {-1, 512, INT_MAX, 1}) rejects<OBJECTTYPE>(objectRecord(7, count), "object stack count or truncation");
    ObjectData ordinary;
    ordinary.objectStatus = 73;
    auto validObject = objectRecord(7, 1);
    appendBytes(validObject, stackRecord(0, ordinary));
    accepts<OBJECTTYPE>(validObject, [](OBJECTTYPE& value) { return marked(value); });
    Bytes validInventory;
    append(validInventory, 1); appendBytes(validInventory, validObject);
    append(validInventory, 3); append(validInventory, 5);
    accepts<Inventory>(validInventory, [](Inventory& value) { return marked(value); });
    auto badStack = objectRecord(7, 1);
    appendBytes(badStack, stackRecord(-1));
    rejects<OBJECTTYPE>(badStack, "stack child failure propagates");
    auto badAttachment = objectRecord(7, 1);
    appendBytes(badAttachment, invalidChild);
    rejects<OBJECTTYPE>(badAttachment, "attachment failure propagates");
    auto shortStack = objectRecord(7, 1, 2);
    appendBytes(shortStack, stackRecord(0));
    rejects<OBJECTTYPE>(shortStack, "stack smaller than object count");
    auto manyAttachments = stackRecord(600);
    for (int index = 0; index < 600; ++index) appendBytes(manyAttachments, objectRecord(NONE, 0, 0));
    accepts<StackedObjectData>(manyAttachments, [](StackedObjectData& value) { return value.attachments.size() == 600; });
    auto fullLBE = lbeRecord(MAX_ITEMS_IN_LBE);
    for (int index = 0; index < MAX_ITEMS_IN_LBE; ++index) appendBytes(fullLBE, objectRecord(NONE, 0, 0));
    accepts<LBENODE>(fullLBE, [](LBENODE& value) { return value.inv.size() == MAX_ITEMS_IN_LBE; });
    // The first LBE child succeeds before a later child fails. Both globals must roll back.
    const auto itemClass = Item[7].usItemClass;
    Item[7].usItemClass = IC_LBEGEAR;
    ObjectData lbeData;
    lbeData.lbe.bLBE = -1; lbeData.lbe.uniqueID = 100;
    auto nestedLBE = objectRecord(7, 2, 2);
    appendBytes(nestedLBE, stackRecord(0, lbeData));
    appendBytes(nestedLBE, lbeRecord(0, 100));
    lbeData.lbe.uniqueID = 101;
    appendBytes(nestedLBE, stackRecord(0, lbeData));
    appendBytes(nestedLBE, lbeRecord(1, 101));
    appendBytes(nestedLBE, objectRecord(MAXITEMS, 0, 0));
    rejects<OBJECTTYPE>(nestedLBE, "later nested LBE failure rolls back prior child");
    for (std::size_t length = 1; length < nestedLBE.size(); ++length)
        rejects<OBJECTTYPE>(Bytes(nestedLBE.begin(), nestedLBE.begin() + length), "nested LBE byte prefix");
    Item[7].usItemClass = itemClass;
    ObjectData action;
    action.misc.bActionValue = ACTION_ITEM_BLOW_UP; action.misc.usBombItem = MAXITEMS;
    auto badAction = objectRecord(ACTION_ITEM, 1);
    appendBytes(badAction, stackRecord(0, action));
    rejects<OBJECTTYPE>(badAction, "action payload item ID");
    auto missingCounters = Bytes{};
    append(missingCounters, 1);
    appendBytes(missingCounters, objectRecord(NONE, 0, 0));
    rejects<Inventory>(missingCounters, "first inventory counter truncated");
    append(missingCounters, 3);
    rejects<Inventory>(missingCounters, "second inventory counter truncated");
    OLD_OBJECTTYPE_101 legacy{};
    const auto version = guiCurrentSaveGameVersion;
    guiCurrentSaveGameVersion = NIV_SAVEGAME_DATATYPE_CHANGE - 1;
    for (int which = 0; which != 3; ++which) {
        legacy = {};
        legacy.usItem = which == 0 ? MAXITEMS : (which == 2 ? ACTION_ITEM : 7);
        legacy.ubNumberOfObjects = 1;
        if (which == 1) legacy.usAttachItem[0] = MAXITEMS;
        if (which == 2) { legacy.ugYucky.bActionValue = ACTION_ITEM_BLOW_UP; legacy.ugYucky.usBombItem = MAXITEMS; }
        Bytes bytes; append(bytes, legacy);
        rejects<OBJECTTYPE>(bytes, "legacy item or payload ID");
    }
    guiCurrentSaveGameVersion = version;
}
void bufferRanges() {
    Bytes bytes(8, 0x5a);
    BufferBounds bounds(bytes, bytes.size());
    auto* start = reinterpret_cast<char*>(bytes.data());
    char destination = 'x';
    auto* cursor = start;
    expect(!LoadDataFromBuffer(&destination, cursor, -1, "negative"), "negative copy size rejected");
    expect(cursor == start && destination == 'x', "negative copy preserves cursor and destination");
    cursor = start + 1;
    expect(!LoadDataFromBuffer(&destination, cursor, INT_MAX, "overflow"), "overflow-sized copy rejected");
    expect(cursor == start + 1 && destination == 'x', "oversized copy preserves cursor and destination");
    cursor = start + bytes.size();
    expect(LoadDataFromBuffer(nullptr, cursor, 0, "end"), "zero read at exact end accepted");
    expect(!LoadDataFromBuffer(&destination, cursor, 1, "past end"), "read beyond exact end rejected");
    cursor = reinterpret_cast<char*>(reinterpret_cast<std::uintptr_t>(start) - 1);
    expect(!LoadDataFromBuffer(&destination, cursor, 1, "before start"), "cursor before buffer rejected");
    cursor = start;
    expect(!LoadDataFromBuffer(&destination, cursor, 1, start, -1, "negative bound"), "negative buffer extent rejected");
    auto* wrapped = reinterpret_cast<char*>(UINTPTR_MAX - 2);
    cursor = wrapped;
    expect(!LoadDataFromBuffer(&destination, cursor, 1, wrapped, 8, "wrapped address"), "wrapped address extent rejected");
    cursor = nullptr;
    expect(!LoadDataFromBuffer(&destination, cursor, 1, "null cursor"), "null cursor rejected");
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
        malformedRecords();
        depthRecords();
        ammoRecords();
        bufferRanges();
        ShutdownFileManager();
        std::cout << "soldier save: " << checks << " checks, " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) {
        ShutdownFileManager();
        std::cerr << "soldier save harness failed at check " << checks << ": " << error.what() << '\n';
        return 2;
    }
}
