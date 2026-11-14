/*
 * P710 native snapshot update: tests/native-core/native_item_file_services_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Real native template-path/host-file boundaries, using only owned /tmp data.
#include "native_fileman.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-item-file-XXXXXX";
        char* made = ::mkdtemp(pattern);
        if (!made) throw std::runtime_error("create owned temporary directory");
        root = made;
    }
    void remove() {
        std::error_code error;
        fs::remove_all(root, error);
        check(!error && !fs::exists(root), "owned temporary fixture cleanup");
    }
    ~Temporary() { std::error_code error; fs::remove_all(root, error); }
};
struct VfsLifetime {
    ~VfsLifetime() {
        ShutdownFileManager();
        vfs::Log::flushDeleteAll();
        vfs::CVirtualFileSystem::shutdownVFS();
        vfs::ObjectAllocator::clear();
    }
};
void write(const fs::path& path, const std::string& bytes = "fixture") {
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << bytes;
    if (!file) throw std::runtime_error("write owned temporary fixture");
}
void mount(const fs::path& root, const char* name, bool writable) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(name, vfs::Path(root.u8string()), writable);
    vfs::IBaseLocation* location;
    if (writable) {
        auto directory = std::make_unique<vfs::CDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "initialize actual writable VFS directory");
        profile->addLocation(directory.get());
        location = directory.release();
    } else {
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "initialize actual read-only VFS directory");
        profile->addLocation(directory.get());
        location = directory.release();
    }
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "mount actual VFS profile");
}
void paths() {
    std::string path = "unchanged";
    check(MakeNativeWideFilePath("GearTemplate\\", L"Gr\u00fc\u00dfe \u96ea \U0001f600 %n", ".txt", path) &&
          path == u8"GearTemplate\\Grüße 雪 😀 %n.txt", "Unicode path uses actual UTF8 converter; percent is literal");
    check(MakeNativeWideFilePath("FortificationPlan\\", L"A9", "_3.txt", path) &&
          path == "FortificationPlan\\A9_3.txt", "original underground sector suffix preserved");
    check(MakeNativeWideFilePath("GearTemplate\\", L"", ".txt", path) &&
          path == "GearTemplate\\.txt", "empty name retains original concatenation behavior");
    const std::wstring longName(600, L'x');
    check(MakeNativeWideFilePath("GearTemplate\\", longName.c_str(), ".txt", path) && path.size() > 600,
          "path conversion has no fabricated MAX_PATH limit");
    path = "unchanged";
    const wchar_t surrogate[] = {static_cast<wchar_t>(0xd800), 0};
    const wchar_t beyondUnicode[] = {static_cast<wchar_t>(0x110000), 0};
    check(!MakeNativeWideFilePath("GearTemplate\\", nullptr, ".txt", path) && path == "unchanged",
          "null wide input fails without partial path");
    check(!MakeNativeWideFilePath("GearTemplate\\", surrogate, ".txt", path) && path == "unchanged",
          "invalid surrogate fails without partial path");
    check(!MakeNativeWideFilePath("GearTemplate\\", beyondUnicode, ".txt", path) && path == "unchanged",
          "out-of-range Unicode fails without partial path");
    check(!MakeNativeWideFilePath(std::string("bad\0prefix", 10), L"name", ".txt", path) && path == "unchanged" &&
          !MakeNativeWideFilePath("prefix", L"name", std::string("bad\0suffix", 10), path) && path == "unchanged",
          "embedded NUL path components fail before OS/VFS truncation");
}
void enumeration(const fs::path& root) {
    const fs::path folder = root / u8"host 雪";
    fs::create_directories(folder / "subdirectory");
    write(folder / "subdirectory/nested.txt");
    write(folder / "plain");
    write(folder / "a.txt");
    write(folder / ".hidden");
    write(folder / u8"Grüße 雪 😀.txt");
    write(folder / "%s%n.txt");
    fs::create_symlink(folder / "a.txt", folder / "file-link");
    fs::create_directory_symlink(folder / "subdirectory", folder / "directory-link");
    std::vector<std::string> names = {"old"};
    check(EnumerateHostDirectoryFiles(folder.u8string(), names), "enumerate owned Unicode host folder");
    const std::set<std::string> expected = {"plain", "a.txt", ".hidden", u8"Grüße 雪 😀.txt", "%s%n.txt", "file-link"};
    check(std::set<std::string>(names.begin(), names.end()) == expected && names.size() == expected.size(),
          "all non-directory names; dotless/hidden/Unicode/percent and file symlink retained; no recursion");
    names = {"unchanged"};
    check(!EnumerateHostDirectoryFiles((folder / "missing").u8string(), names) && names == std::vector<std::string>{"unchanged"},
          "missing host folder reports error and preserves output");
    check(!EnumerateHostDirectoryFiles((folder / "a.txt").u8string(), names) && names == std::vector<std::string>{"unchanged"},
          "regular file is not a folder");
    check(!EnumerateHostDirectoryFiles("", names) && !EnumerateHostDirectoryFiles(std::string("bad\0path", 8), names) &&
          names == std::vector<std::string>{"unchanged"}, "empty/NUL host paths rejected without stale-success output");
    fs::create_directories(root / "empty");
    check(EnumerateHostDirectoryFiles((root / "empty").u8string(), names) && names.empty(), "empty folder succeeds with empty output");
    fs::create_symlink(folder / "absent-target", folder / "broken-link");
    names = {"unchanged"};
    check(!EnumerateHostDirectoryFiles(folder.u8string(), names) && names == std::vector<std::string>{"unchanged"},
          "entry status error does not publish partial enumeration");
    fs::remove(folder / "broken-link");
    check(EnumerateHostDirectoryFiles(folder.u8string(), names) && names.size() == expected.size(),
          "error releases iterator and next enumeration recovers");
    fs::path deep = root;
    for (int i = 0; i < 4; ++i) deep /= std::string(80, static_cast<char>('a' + i));
    write(deep / "long.txt");
    check(deep.u8string().size() > 260 && EnumerateHostDirectoryFiles(deep.u8string(), names) &&
          names == std::vector<std::string>{"long.txt"}, "real host directory beyond260bytes is admitted");
}
void selectedProfile(const fs::path& root) {
    write(root / "base/GearTemplate/base-only.txt");
    write(root / "writable/GearTemplate/writable-only.txt");
    write(root / u8"writable/GearTemplate/雪.txt");
    mount(root / "base", "base", false);
    mount(root / "writable", "writable", true);
    std::string selected;
    check(MakeNativeWideFilePath("GearTemplate\\", L"Update Gear", ".txt", selected), "build actual selected-profile anchor path");
    vfs::Path real;
    {
        vfs::COpenWriteFile file(vfs::Path(selected), true);
        file->_getRealPath(real);
    }
    vfs::Path directory, filename;
    real.splitLast(directory, filename);
    check(fs::equivalent(fs::u8path(directory.to_string()), root / "writable/GearTemplate"),
          "original COpenWriteFile resolves writable profile real path");
    std::vector<std::string> names;
    check(EnumerateHostDirectoryFiles(directory.to_string(), names), "production adapter enumerates VFS-resolved host path");
    const std::set<std::string> expected = {"writable-only.txt", u8"雪.txt", "Update Gear.txt"};
    check(std::set<std::string>(names.begin(), names.end()) == expected && names.size() == expected.size(),
          "selected host folder excludes read-only VFS overlay names and retains created anchor");
    check(fs::exists(root / "base/GearTemplate/base-only.txt"), "read-only profile fixture preserved");
}
}
int main(int argc, char**) {
    try {
        check(argc == 1, "fixture accepts no installed-data arguments");
        Temporary temporary;
        paths(); enumeration(temporary.root);
        { VfsLifetime lifetime; selectedProfile(temporary.root); }
        temporary.remove();
        std::cout << "PASS: " << checks << " native item/file boundary assertions; owned temporary files only\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
