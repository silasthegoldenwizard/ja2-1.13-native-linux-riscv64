/*
 * P710 native snapshot update: tests/native-core/native_save_header_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_save_header_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted review correction checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production-linked save-header boundary tests. All files are synthetic and
// live below one owned temporary VFS directory.
#include "FileMan.h"
#include "SaveLoadGame.h"
#include "SaveLoadScreen.h"
#include "GameVersion.h"
#include "MessageBoxScreen.h"
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

extern BOOLEAN LoadSavedGameHeader(INT32 bEntry, SAVED_GAME_HEADER* pSaveGameHeader);
extern UINT8 CompareSaveGameVersion(INT32 bSaveGameID);
extern BOOLEAN InitSaveGameArray();
extern void SaveHeaderLoadFailedCallBack(UINT8 bExitValue);
extern CHAR8 gSaveDir[];
extern UINT16 VAL_SLOT_START;
extern UINT16 NUM_SLOT;
extern BOOLEAN gbSaveGameArray[];
extern INT8 gbSaveGameSelectedLocation[];
extern INT32 gbSelectedSaveLocation;
extern BOOLEAN gfRedrawSaveLoadScreen;
extern BOOLEAN gfExitAfterMessageBox;
extern BOOLEAN gfSaveLoadScreenExit;
extern UINT32 guiSaveLoadExitScreen;
extern UINT32 SLG_UNSELECTED_SLOT_GRAPHICS_NUMBER;

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
constexpr INT32 fixtureSlot = 8;

void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}

struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-save-header-XXXXXX";
        char* made = ::mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
        fs::create_directories(root / "saves");
    }
    ~Temporary() {
        std::error_code error;
        fs::remove_all(root, error);
    }
};

void mount(const fs::path& root) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(
        "save-header-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(
        vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "fixture VFS directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "fixture VFS mounted");
}

std::string filename(INT32 slot = fixtureSlot) {
    CHAR8 path[512]{};
    CreateSavedGameFileNameFromNumber(static_cast<UINT8>(slot), path);
    return path;
}

void writeFixture(INT32 slot, const void* bytes, UINT32 length) {
    const std::string path = filename(slot);
    FileDelete(const_cast<char*>(path.c_str()));
    const HWFILE file = FileOpen(const_cast<char*>(path.c_str()),
                                 FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS, FALSE);
    check(file != 0, "synthetic save header opens for writing");
    UINT32 written = 0;
    check(FileWrite(file, const_cast<void*>(bytes), length, &written) && written == length,
          "synthetic save header writes completely");
    FileClose(file);
    gbSaveGameArray[slot] = TRUE;
}

void writeFixture(const void* bytes, UINT32 length) {
    writeFixture(fixtureSlot, bytes, length);
}

bool allZero(const SAVED_GAME_HEADER& header) {
    const auto* begin = reinterpret_cast<const unsigned char*>(&header);
    return std::all_of(begin, begin + sizeof(header), [](unsigned char value) {
        return value == 0;
    });
}

SAVED_GAME_HEADER validHeader(const wchar_t* description = L"synthetic header") {
    SAVED_GAME_HEADER header{};
    header.uiSavedGameVersion = SAVE_GAME_VERSION;
    std::strncpy(header.zGameVersionNumber, czVersionString,
                 sizeof(header.zGameVersionNumber) - 1);
    std::wcsncpy(header.sSavedGameDesc, description,
                 sizeof(header.sSavedGameDesc) / sizeof(header.sSavedGameDesc[0]) - 1);
    return header;
}

void rejectShortHeader() {
    const UINT8 byte = 0xa5;
    writeFixture(&byte, sizeof(byte));
    SAVED_GAME_HEADER header;
    std::memset(&header, 0xa5, sizeof(header));
    check(!LoadSavedGameHeader(fixtureSlot, &header), "short header is rejected");
    check(allZero(header), "short read clears the caller-visible header transactionally");
    check(!gbSaveGameArray[fixtureSlot], "short read invalidates the selected slot");
    check(CompareSaveGameVersion(fixtureSlot) != 0,
          "version comparison propagates header-load failure");
}

void rejectMissingAndInvalidOutput() {
    const std::string path = filename();
    FileDelete(const_cast<char*>(path.c_str()));
    SAVED_GAME_HEADER header;
    std::memset(&header, 0xa5, sizeof(header));
    check(!LoadSavedGameHeader(fixtureSlot, &header), "missing header is rejected");
    check(allZero(header), "missing header leaves a deterministic zero result");
    check(!LoadSavedGameHeader(fixtureSlot, nullptr), "null output is rejected");
}

void rejectUnterminatedStrings() {
    auto header = validHeader();
    std::fill(std::begin(header.sSavedGameDesc), std::end(header.sSavedGameDesc), L'A');
    writeFixture(&header, sizeof(header));
    SAVED_GAME_HEADER loaded;
    std::memset(&loaded, 0xa5, sizeof(loaded));
    check(!LoadSavedGameHeader(fixtureSlot, &loaded),
          "description without an in-field terminator is rejected");
    check(allZero(loaded), "invalid description clears the complete result");

    header = validHeader();
    std::fill(std::begin(header.zGameVersionNumber),
              std::end(header.zGameVersionNumber), 'A');
    writeFixture(&header, sizeof(header));
    std::memset(&loaded, 0xa5, sizeof(loaded));
    check(!LoadSavedGameHeader(fixtureSlot, &loaded),
          "version without an in-field terminator is rejected");
    check(allZero(loaded), "invalid version clears the complete result");
}

void acceptBoundaryTerminators() {
    auto header = validHeader();
    std::fill(std::begin(header.zGameVersionNumber),
              std::end(header.zGameVersionNumber), 'V');
    header.zGameVersionNumber[GAME_VERSION_LENGTH - 1] = '\0';
    std::fill(std::begin(header.sSavedGameDesc), std::end(header.sSavedGameDesc), L'D');
    header.sSavedGameDesc[SIZE_OF_SAVE_GAME_DESC - 1] = L'\0';
    writeFixture(&header, sizeof(header));
    SAVED_GAME_HEADER loaded{};
    check(LoadSavedGameHeader(fixtureSlot, &loaded),
          "terminators in the last valid array elements are accepted");
    check(loaded.zGameVersionNumber[GAME_VERSION_LENGTH - 1] == '\0' &&
          loaded.sSavedGameDesc[SIZE_OF_SAVE_GAME_DESC - 1] == L'\0',
          "accepted boundary terminators remain intact");
}

void acceptCurrentHeader() {
    const auto header = validHeader();
    writeFixture(&header, sizeof(header));
    SAVED_GAME_HEADER loaded{};
    check(LoadSavedGameHeader(fixtureSlot, &loaded), "valid current header loads");
    check(loaded.uiSavedGameVersion == SAVE_GAME_VERSION &&
          std::strcmp(loaded.zGameVersionNumber, czVersionString) == 0 &&
          std::wcscmp(loaded.sSavedGameDesc, L"synthetic header") == 0,
          "valid header fields are preserved");
    check(CompareSaveGameVersion(fixtureSlot) == 0,
          "valid current header passes version comparison");
}

void acceptPagedSaveListings() {
    NUM_SLOT = NUM_SAVE_GAMES_NEW;

    VAL_SLOT_START = NUM_SLOT;
    const INT32 firstPageTwoSlot = VAL_SLOT_START;
    auto header = validHeader(L"first page-two save");
    writeFixture(firstPageTwoSlot, &header, sizeof(header));
    FileDelete(const_cast<char*>(filename(firstPageTwoSlot + VAL_SLOT_START).c_str()));
    check(InitSaveGameArray(), "page-two save listing initializes");
    check(gbSaveGameArray[firstPageTwoSlot],
          "first save on a nonzero page remains visible");
    SAVED_GAME_HEADER loaded{};
    check(LoadSavedGameHeader(0, &loaded),
          "page-relative first entry loads the intended save");
    check(std::wcscmp(loaded.sSavedGameDesc, L"first page-two save") == 0,
          "page-relative load preserves the nonzero-page header identity");

    VAL_SLOT_START = NUM_SAVE_GAMES - NUM_SLOT;
    const INT32 lastSaveSlot = NUM_SAVE_GAMES - 1;
    header = validHeader(L"last representable save");
    writeFixture(lastSaveSlot, &header, sizeof(header));
    check(InitSaveGameArray(), "last-page save listing initializes");
    check(gbSaveGameArray[lastSaveSlot],
          "last representable save remains visible on the final page");
    std::memset(&loaded, 0, sizeof(loaded));
    check(LoadSavedGameHeader(NUM_SLOT - 1, &loaded),
          "last page-relative entry loads within the save array bound");
    check(std::wcscmp(loaded.sSavedGameDesc, L"last representable save") == 0,
          "last representable header identity is preserved");

    VAL_SLOT_START = 0;
}

void rejectHeaderWithoutRestartingCampaign() {
    NUM_SLOT = NUM_SAVE_GAMES_NEW;
    VAL_SLOT_START = NUM_SLOT;
    gbSelectedSaveLocation = 2;
    const UINT32 selectedAbsoluteSlot = VAL_SLOT_START + gbSelectedSaveLocation;
    const UINT32 untouchedAbsoluteSlot = selectedAbsoluteSlot + 1;
    gbSaveGameArray[selectedAbsoluteSlot] = TRUE;
    gbSaveGameArray[untouchedAbsoluteSlot] = TRUE;
    SLG_UNSELECTED_SLOT_GRAPHICS_NUMBER = 7;
    gbSaveGameSelectedLocation[gbSelectedSaveLocation] = 9;
    gfRedrawSaveLoadScreen = FALSE;
    gfExitAfterMessageBox = TRUE;
    gfSaveLoadScreenExit = FALSE;
    guiSaveLoadExitScreen = 0x5a5a5a5a;

    SaveHeaderLoadFailedCallBack(MSG_BOX_RETURN_OK);

    check(!gbSaveGameArray[selectedAbsoluteSlot],
          "corrupt selected header is invalidated");
    check(gbSaveGameArray[untouchedAbsoluteSlot],
          "unrelated save slot remains valid");
    check(gbSaveGameSelectedLocation[2] == 7,
          "selected save graphic is reset");
    check(gbSelectedSaveLocation == -1,
          "bad header selection is cleared");
    check(gfRedrawSaveLoadScreen,
          "save list redraw is requested");
    check(!gfExitAfterMessageBox,
          "message-box exit state is cleared");
    check(!gfSaveLoadScreenExit && guiSaveLoadExitScreen == 0x5a5a5a5a,
          "preflight failure neither exits nor restarts the current screen");

    VAL_SLOT_START = 0;
}

void cleanup() {
    ShutdownFileManager();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}
}

int main() {
    try {
        Temporary temporary;
        mount(temporary.root);
        check(InitializeFileManager(nullptr), "original FileMan initialized");
        VAL_SLOT_START = 0;
        std::strcpy(gSaveDir, "saves");
        rejectShortHeader();
        rejectMissingAndInvalidOutput();
        rejectUnterminatedStrings();
        acceptBoundaryTerminators();
        acceptCurrentHeader();
        acceptPagedSaveListings();
        rejectHeaderWithoutRestartingCampaign();
        cleanup();
        std::cout << "PASS: " << checks
                  << " production save-header boundary assertions; synthetic files only\n";
        return 0;
    } catch (const std::exception& error) {
        cleanup();
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
