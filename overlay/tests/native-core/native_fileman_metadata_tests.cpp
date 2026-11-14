/*
 * P710 native snapshot update: tests/native-core/native_fileman_metadata_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native FileMan public-service tests. All writable data is generated below a
// unique owned /tmp directory; no installation, game data, or dependencies.
#include "FileMan.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool condition, const char* why) {
    ++checks; if (!condition) throw std::runtime_error(why);
}
struct Temporary {
    fs::path previous = fs::current_path();
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-fileman-metadata-XXXXXX";
        char* made = ::mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
    }
    ~Temporary() {
        std::error_code error;
        fs::current_path(previous, error);
        fs::remove_all(root, error);
    }
};
void write(const fs::path& path, const std::string& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    output << bytes;
    if (!output) throw std::runtime_error("create owned fixture");
}
std::string content(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
void mount(const fs::path& root, const char* name, bool writable) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(name, vfs::Path(root.u8string()), writable);
    vfs::IBaseLocation* location = nullptr;
    if (writable) {
        auto directory = std::make_unique<vfs::CDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "writable profile initialization");
        profile->addLocation(directory.get()); location = directory.release();
    } else {
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "read-only profile initialization");
        profile->addLocation(directory.get()); location = directory.release();
    }
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "profile mounted");
}
HWFILE open(const char* filename, UINT32 options, const char* profile = nullptr) {
    return FileOpen(const_cast<char*>(filename), options, FALSE, const_cast<char*>(profile));
}
void metadataAndIO(const Temporary& temporary) {
    check(InitializeFileManager(nullptr), "metadata service initializes without index database");
    check(FileExists("shared.txt") && FileExistsNoDB("readonly.txt"), "both existence APIs use VFS");
    check(!FileExists(nullptr) && !FileExists("") && !FileExistsNoDB("missing"), "invalid and absent existence");
    check(!FileExists((temporary.root / "outside.txt").c_str()), "physical unmounted file is not a VFS file");
    check(FileSize("shared.txt") == 7 && FileSize("readonly.txt") == 4 && FileSize("missing") == 0 &&
          FileSize(nullptr) == 0, "metadata sizes follow selected VFS profile");
    char bytes[16]{}; UINT32 count = 99;
    check(FileLoad("shared.txt", bytes, 7, &count) && count == 7 && std::string(bytes, 7) == "overlay",
          "FileLoad reads SF_TOP profile through opaque registry");
    count = 99;
    check(!FileLoad("readonly.txt", bytes, 8, &count) && count == 99 && std::string(bytes, 4) == "base",
          "short FileLoad leaves published count unchanged as original");
    check(FileLoad("readonly.txt", nullptr, 0, nullptr) && !FileLoad(nullptr, bytes, 1, &count) &&
          !FileLoad("missing", bytes, 1, &count) && !FileLoad("readonly.txt", nullptr, 1, &count),
          "FileLoad zero/null/missing contracts");
    HWFILE named = open("shared.txt", FILE_ACCESS_READ, "base");
    check(named != 0, "named read profile");
    count = 0;
    check(FileRead(named, bytes, 4, &count) && count == 4 && std::string(bytes, 4) == "base", "named profile remains independent");
    FileClose(named);

    const HWFILE printed = open("printed.txt", FILE_ACCESS_WRITE);
    check(printed != 0 && FilePrintf(printed, "%s:%d", "hello", 17), "formatted write uses real writable VFS profile");
    FileClose(printed);
    check(content(temporary.root / "writable/printed.txt") == "hello:17", "formatted bytes read from actual file");
    const HWFILE truncated = open("truncated.txt", FILE_ACCESS_WRITE);
    const std::string longText(300, 'x');
    check(truncated && FilePrintf(truncated, "%s", longText.c_str()), "bounded long formatting"); FileClose(truncated);
    check(content(temporary.root / "writable/truncated.txt") == std::string(159, 'x'), "original159-byte truncation preserved");
    check(!FilePrintf(0, "%s", "bad") && !FilePrintf(0, nullptr), "invalid formatted-write arguments");
    check(FileExists("printed.txt") && !fs::exists(temporary.root / "base/printed.txt"), "writes stay in writable profile");
    check(FileDelete("shared.txt") && FileExists("shared.txt") && FileSize("shared.txt") == 4,
          "deleting overlay reveals original read-only profile");
    check(!fs::exists(temporary.root / "writable/shared.txt") && content(temporary.root / "base/shared.txt") == "base",
          "delete preserves lower profile physical bytes");
    check(!FileDelete("readonly.txt") && !FileDelete(nullptr) && !FileDelete("missing"), "read-only and invalid deletes fail");
    check(FileSize("oversized.bin") == 0, "unrepresentable32-bit file size fails without truncation");
}
void enumerationAndSound() {
    GETFILESTRUCT entry{};
    std::map<std::string, GETFILESTRUCT> found;
    check(GetFileFirst("*.txt", &entry), "VFS wildcard enumeration begins");
    do { found.emplace(entry.zFileName, entry); } while (GetFileNext(&entry));
    check(found.count("readonly.txt") && found.count("printed.txt") && found.count("%s%n.txt"),
          "enumeration emits literal names including format tokens");
    check(found.at("readonly.txt").uiFileAttribs == FILE_IS_READONLY &&
          found.at("printed.txt").uiFileAttribs == FILE_IS_NORMAL &&
          found.at("printed.txt").uiFileSize == 8 && found.at("printed.txt").iFindHandle == 0,
          "original basename,size,write-capability metadata");
    GetFileClose(&entry); GetFileClose(&entry);
    check(!GetFileNext(&entry) && !GetFileNext(nullptr), "closed/null enumeration safe");
    check(GetFileFirst("readonly.txt", &entry) && GetFileFirst("printed.txt", &entry) &&
          std::string(entry.zFileName) == "printed.txt" && !GetFileNext(&entry), "first replaces singleton cursor safely");
    GetFileClose(nullptr);
    std::strcpy(entry.zFileName, "unchanged"); entry.uiFileSize = 77;
    check(!GetFileFirst("oversized.bin", &entry) && std::string(entry.zFileName) == "unchanged" && entry.uiFileSize == 77,
          "oversized enumeration fails with output unchanged");
    check(!GetFileFirst(nullptr, &entry) && !GetFileFirst("*", nullptr), "invalid enumeration inputs");

    char sound[32] = "sentinel";
    check(SoundFileExists("sound", sound) && std::string(sound) == "sound.mp3", "mp3 preferred over ogg/wav");
    check(SoundFileExists("other", sound) && std::string(sound) == "other.ogg", "ogg fallback preferred over wav");
    check(SoundFileExists("last", sound) && std::string(sound) == "last.wav", "wav fallback");
    check(!SoundFileExists("absent", sound) && std::string(sound) == "absent.wav", "missing sound retains original wav candidate");
    char small[5] = "safe";
    check(!SoundFileExists("sound", small) && std::string(small) == "safe", "inferred capacity rejects overflow unchanged");
    check(!SoundFileExists("sound", sound, 3) && !SoundFileExists(nullptr, sound) &&
          !SoundFileExists("sound", nullptr, 32), "explicit capacity and null sound outputs");
    std::strcpy(sound, "sound");
    check(SoundFileExists(sound, sound) && std::string(sound) == "sound.mp3", "sound input and output may alias");
}
void directoriesAndLifecycle(const Temporary& temporary) {
    STRING512 current{}; STRING512 executable{};
    check(GetFileManCurrentDirectory(current) && fs::path(current) == fs::current_path(), "OS current directory readback");
    check(GetExecutableDirectory(executable) && fs::path(executable) == fs::read_symlink("/proc/self/exe").parent_path(),
          "real Linux executable directory independent of VFS");
    check(SetFileManCurrentDirectory(temporary.root.c_str()) && GetFileManCurrentDirectory(current) &&
          fs::path(current) == temporary.root && FileExists("readonly.txt"), "OS CWD change leaves mounted VFS routing intact");
    check(!SetFileManCurrentDirectory(nullptr) && !SetFileManCurrentDirectory("no-such-directory") &&
          !GetFileManCurrentDirectory(nullptr) && !GetExecutableDirectory(nullptr), "invalid OS path/output arguments");
    fs::path deep = temporary.root;
    for (int i = 0; i < 6; ++i) deep /= std::string(90, static_cast<char>('a' + i));
    fs::create_directories(deep);
    check(SetFileManCurrentDirectory(deep.c_str()), "owned long current directory");
    std::strcpy(current, "unchanged");
    check(!GetFileManCurrentDirectory(current) && std::string(current) == "unchanged", "fixed512-byte capacity guarded");
    check(SetFileManCurrentDirectory(temporary.previous.c_str()), "restore OS current directory");

    check(RemoveFileManDirectory("wipe", TRUE) && !FileExists("wipe/remove.txt") && FileExists("wipe/keep.txt"),
          "directory removal delegates VFS and preserves read-only profile files");
    check(fs::is_directory(temporary.root / "writable/wipe") && FileExists("wipe/sub/nested.txt"),
          "actual VFS matching retains directory and unmatched subdirectory even for recursive true");
    check(EraseDirectory("erase") && !FileExists("erase/remove.txt") && fs::is_directory(temporary.root / "writable/erase"),
          "erase uses same VFS operation and retains physical directory");
    check(!RemoveFileManDirectory(nullptr, FALSE) && !EraseDirectory(""), "invalid directory operations");
    const HWFILE stale = open("readonly.txt", FILE_ACCESS_READ);
    GETFILESTRUCT entry{};
    check(stale && GetFileFirst("*.txt", &entry), "active I/O and enumeration before shutdown");
    ShutdownFileManager(); ShutdownFileManager();
    char byte = 0;
    check(!FileRead(stale, &byte, 1, nullptr) && !GetFileNext(&entry), "shutdown invalidates opaque handles and enumeration");
    check(InitializeFileManager(nullptr) && FileExists("readonly.txt"), "service reinit preserves caller-owned VFS profiles");
}
void cleanup() {
    ShutdownFileManager(); vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc, char**) {
    try {
        if (argc != 1) throw std::runtime_error("no arguments: generates its own temporary VFS fixtures");
        Temporary temporary;
        write(temporary.root / "base/shared.txt", "base");
        write(temporary.root / "base/readonly.txt", "base");
        write(temporary.root / "base/%s%n.txt", "literal");
        write(temporary.root / "base/oversized.bin", "");
        fs::resize_file(temporary.root / "base/oversized.bin", std::uintmax_t{1} << 32);
        write(temporary.root / "base/sound.mp3", "mp3");
        write(temporary.root / "base/sound.ogg", "ogg");
        write(temporary.root / "base/sound.wav", "wav");
        write(temporary.root / "base/other.ogg", "ogg");
        write(temporary.root / "base/other.wav", "wav");
        write(temporary.root / "base/last.wav", "wav");
        write(temporary.root / "base/wipe/keep.txt", "base");
        write(temporary.root / "writable/shared.txt", "overlay");
        write(temporary.root / "writable/wipe/remove.txt", "remove");
        write(temporary.root / "writable/wipe/sub/nested.txt", "retained");
        write(temporary.root / "writable/erase/remove.txt", "remove");
        write(temporary.root / "outside.txt", "not-mounted");
        mount(temporary.root / "base", "base", false);
        mount(temporary.root / "writable", "writable", true);
        metadataAndIO(temporary); enumerationAndSound(); directoriesAndLifecycle(temporary); cleanup();
        std::cout << "PASS: " << checks << " native FileMan metadata/service assertions; owned temporary fixtures only\n";
        return 0;
    } catch (const std::exception& error) {
        cleanup(); std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n'; return 1;
    }
}
