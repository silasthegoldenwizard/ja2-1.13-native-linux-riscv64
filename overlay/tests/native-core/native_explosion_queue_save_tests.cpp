/*
 * P710 native snapshot update: tests/native-core/native_explosion_queue_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_explosion_queue_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 21 transactional save-loader checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Genuine original explosion save/load functions and FileMan/bfVFS, with
// synthetic queue records only. No game assets or existing saves are used.
#include "FileMan.h"
#include "Explosion Control.h"
#include "BufferMan.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern ExplosionQueueElement gExplosionQueue[40];
extern UINT32 guiNumExplosions;

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
using Bytes = std::vector<UINT8>;
constexpr unsigned queueCapacity = 40;
void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-explosion-save-XXXXXX";
        auto* made = mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
    }
    ~Temporary() {
        std::error_code error;
        fs::remove_all(root, error);
    }
};
void mount(const fs::path& root) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(
        "explosion-save-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(
        vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "fixture VFS directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "fixture VFS mounted");
}
struct BufferBounds {
    explicit BufferBounds(Bytes& bytes, INT32 length = -1) {
        gpLoadBufferStart = reinterpret_cast<char*>(bytes.data());
        giLoadBufferSize = length < 0 ? static_cast<INT32>(bytes.size()) : length;
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() {
        gpLoadBufferStart = nullptr;
        giLoadBufferSize = 0;
        gfBufferLoadFailed = FALSE;
    }
};
void put32(Bytes& bytes, UINT32 value) {
    // Independent literal little-endian header oracle for current native target.
    for (unsigned i = 0; i < 4; ++i) bytes.at(i) = (value >> (8 * i)) & 255;
}
Bytes save(const Temporary& temporary, unsigned count) {
    std::memset(gExplosionQueue, 0, sizeof(gExplosionQueue));
    for (unsigned i = 0; i < queueCapacity; ++i) {
        gExplosionQueue[i].uiWorldBombIndex = 0x10203000u + i;
        gExplosionQueue[i].uiTimeStamp = 0x50607000u + i;
        gExplosionQueue[i].fExists = i < count;
    }
    for (auto& explosion : gExplosionData) explosion.fAllocated = FALSE;
    gubElementsOnExplosionQueue = static_cast<UINT8>(count);
    gfExplosionQueueActive = TRUE;
    gfExplosionQueueMayHaveChangedSight = TRUE;
    const auto filename = "queue" + std::to_string(count);
    const auto file = FileOpen(const_cast<char*>(filename.c_str()),
                              FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE);
    check(file != 0, "original FileMan opens owned save fixture");
    check(SaveExplosionTableToSaveGameFile(file), "original explosion save succeeds");
    check(FileGetPos(file) == 4 + sizeof(gExplosionQueue) + 4,
          "save preserves four-byte count, all queue slots, explosion count order");
    FileClose(file);
    std::ifstream input(temporary.root / filename, std::ios::binary);
    Bytes bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    check(bytes.size() == 4 + sizeof(gExplosionQueue) + 4, "exact saved table length");
    check(bytes[0] == count && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0,
          "count is widened to four canonical bytes without adjacent-global leakage");
    check(!std::memcmp(bytes.data() + 4, gExplosionQueue, sizeof(gExplosionQueue)),
          "all original queue records preserved immediately after four-byte count");
    const auto end = bytes.size();
    check(!bytes[end-4] && !bytes[end-3] && !bytes[end-2] && !bytes[end-1],
          "zero active-explosion count follows all queue records");
    check(gubElementsOnExplosionQueue == count && gfExplosionQueueActive &&
          gfExplosionQueueMayHaveChangedSight, "save leaves count and queue flags intact");
    return bytes;
}
void roundtrip(const Temporary& temporary, unsigned count) {
    auto bytes = save(temporary, count);
    const auto tableLength = bytes.size();
    bytes.push_back(0xa5);
    std::memset(gExplosionQueue, 0, sizeof(gExplosionQueue));
    gubElementsOnExplosionQueue = 7;
    guiNumExplosions = 99;
    gfExplosionQueueActive = TRUE;
    gfExplosionQueueMayHaveChangedSight = TRUE;
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(LoadExplosionTableFromSavedGame(&cursor), "original explosion load succeeds");
    check(gubElementsOnExplosionQueue == count && guiNumExplosions == 0,
          "load restores scalar counts");
    check(cursor == reinterpret_cast<char*>(bytes.data() + tableLength) &&
          static_cast<UINT8>(*cursor) == 0xa5 && !gfBufferLoadFailed,
          "load consumes unchanged table boundary and preserves following canary");
    check(gfExplosionQueueActive && gfExplosionQueueMayHaveChangedSight,
          "four-byte count load leaves adjacent queue flags intact");
    for (unsigned i = 0; i < queueCapacity; ++i) {
        check(gExplosionQueue[i].uiWorldBombIndex == 0x10203000u + i &&
              gExplosionQueue[i].uiTimeStamp == 0x50607000u + i &&
              gExplosionQueue[i].fExists == (i < count),
              "original queue entry order and fields restored");
    }
}
void rejectHeaders() {
    // Include high-bit values whose low byte would look like a valid count.
    for (UINT32 count : {41u, 255u, 256u, 0x01000001u, 0xffffffffu}) {
        Bytes bytes(4 + sizeof(gExplosionQueue) + 4, 0);
        put32(bytes, count);
        gubElementsOnExplosionQueue = 7;
        guiNumExplosions = 23;
        gfExplosionQueueActive = TRUE;
        gfExplosionQueueMayHaveChangedSight = TRUE;
        BufferBounds bounds(bytes);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!LoadExplosionTableFromSavedGame(&cursor), "out-of-capacity wire count rejected");
        check(cursor == reinterpret_cast<char*>(bytes.data() + 4),
              "invalid count rejected before queue records or explosion count read");
        check(gubElementsOnExplosionQueue == 7 && guiNumExplosions == 23 &&
              gfExplosionQueueActive && gfExplosionQueueMayHaveChangedSight,
              "invalid count never narrows or overwrites neighboring scalar state");
    }
    for (INT32 length = 0; length < 4; ++length) {
        Bytes bytes(4, 0);
        gubElementsOnExplosionQueue = 7;
        gfExplosionQueueActive = TRUE;
        gfExplosionQueueMayHaveChangedSight = TRUE;
        BufferBounds bounds(bytes, length);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!LoadExplosionTableFromSavedGame(&cursor) && gfBufferLoadFailed,
              "short four-byte header rejected by original buffer bounds");
        check(cursor == reinterpret_cast<char*>(bytes.data()) &&
              gubElementsOnExplosionQueue == 7 && gfExplosionQueueActive &&
              gfExplosionQueueMayHaveChangedSight,
              "short header leaves cursor, byte count and neighboring flags intact");
    }
}
void rejectExplosionHeaders() {
    for (UINT32 count : {NUM_EXPLOSION_SLOTS + 1u, 0xffffffffu}) {
        Bytes bytes(4 + sizeof(gExplosionQueue) + 4, 0);
        put32(bytes, 0);
        const auto explosionCountOffset = 4 + sizeof(gExplosionQueue);
        for (unsigned i = 0; i < 4; ++i) {
            bytes.at(explosionCountOffset + i) = (count >> (8 * i)) & 255;
        }

        std::memset(gExplosionQueue, 0x5a, sizeof(gExplosionQueue));
        std::memset(gExplosionData, 0, sizeof(gExplosionData));
        gubElementsOnExplosionQueue = 7;
        guiNumExplosions = 23;
        BufferBounds bounds(bytes);
        auto* cursor = reinterpret_cast<char*>(bytes.data());
        check(!LoadExplosionTableFromSavedGame(&cursor),
              "out-of-capacity active-explosion count rejected");
        check(cursor == reinterpret_cast<char*>(bytes.data() + bytes.size()),
              "invalid active-explosion count rejected after its complete header");
        check(gubElementsOnExplosionQueue == 7 && guiNumExplosions == 23,
              "invalid active-explosion count leaves published counts unchanged");
        check(static_cast<unsigned char*>(static_cast<void*>(gExplosionQueue))[0] == 0x5a,
              "invalid active-explosion count leaves published queue unchanged");
    }
}
}
int main() {
    try {
        Temporary temporary;
        mount(temporary.root);
        check(InitializeFileManager(nullptr), "original FileMan initialized");
        for (unsigned count : {0u, 1u, queueCapacity}) roundtrip(temporary, count);
        rejectHeaders();
        rejectExplosionHeaders();
        ShutdownFileManager();
        std::cout << "explosion queue save: " << checks
                  << " checks passed; original save/load + FileMan/VFS, synthetic data only\n";
        return 0;
    } catch (const std::exception& error) {
        ShutdownFileManager();
        std::cerr << "explosion queue save failed at check " << checks << ": "
                  << error.what() << '\n';
        return 1;
    }
}
