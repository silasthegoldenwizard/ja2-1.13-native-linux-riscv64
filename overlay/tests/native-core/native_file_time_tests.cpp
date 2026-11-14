/*
 * P710 native snapshot update: tests/native-core/native_file_time_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Real selected-profile metadata, using only authored temporary files.
#include "native_fileman.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool value, const char* why) {
    ++checks; if (!value) throw std::runtime_error(why);
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-file-time-XXXXXX";
        char* made = ::mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique owned temporary directory");
        root = made;
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
struct Timezone {
    bool present = std::getenv("TZ") != nullptr;
    std::string previous = present ? std::getenv("TZ") : "";
    Timezone() { check(::setenv("TZ", "UTC0", 1) == 0, "fixture timezone"); ::tzset(); }
    ~Timezone() {
        if (present) ::setenv("TZ", previous.c_str(), 1); else ::unsetenv("TZ");
        ::tzset();
    }
};
void write(const fs::path& path, std::time_t seconds, long nanoseconds = 0) {
    fs::create_directories(path.parent_path());
    { std::ofstream file(path, std::ios::binary); file << "abc"; check(bool(file), "write authored fixture"); }
    const timespec times[2] = {{seconds, nanoseconds}, {seconds, nanoseconds}};
    check(::utimensat(AT_FDCWD, path.c_str(), times, 0) == 0, "set owned fixture modification time");
}
void mount(const fs::path& root, const char* name, bool writable) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(name, vfs::Path(root.u8string()), writable);
    vfs::IBaseLocation* location;
    if (writable) {
        auto directory = std::make_unique<vfs::CDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "initialize writable profile");
        profile->addLocation(directory.get()); location = directory.release();
    } else {
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "initialize read-only profile");
        profile->addLocation(directory.get()); location = directory.release();
    }
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "mount real VFS profile");
}
bool same(const NativeFileDateTime& a, const NativeFileDateTime& b) {
    return a.year == b.year && a.month == b.month && a.day == b.day && a.hour == b.hour &&
        a.minute == b.minute && a.second == b.second && a.epochSeconds == b.epochSeconds && a.nanosecond == b.nanosecond;
}
}
int main(int argc, char**) {
    try {
        check(argc == 1, "test generates fixtures and accepts no paths");
        Timezone timezone;
        Temporary temporary;
        VfsLifetime lifetime;
        constexpr std::time_t leapDate = 951827696; // 2000-02-29 12:34:56 UTC
        write(temporary.root / "base/shared.sav", 1700000000);
        write(temporary.root / "base/base-only.sav", 946684800); // 2000-01-01 UTC
        write(temporary.root / "overlay/shared.sav", leapDate, 123456789);
        write(temporary.root / u8"overlay/Grüße 雪.sav", leapDate);
        write(temporary.root / "base/epoch.sav", 0);
        write(temporary.root / "outside.sav", leapDate);
        fs::create_directories(temporary.root / "overlay/directory.sav");
        mount(temporary.root / "base", "base", false);
        mount(temporary.root / "overlay", "overlay", true);
        NativeFileDateTime stamp{};
        check(GetNativeFileModificationTime("shared.sav", stamp), "real selected-profile metadata succeeds");
        check(stamp.epochSeconds == leapDate && stamp.nanosecond == 123456789,
              "SF_TOP overlay wins despite lower profile newer timestamp; nanoseconds retained");
        check(stamp.year == 2000 && stamp.month == 2 && stamp.day == 29 && stamp.hour == 12 &&
              stamp.minute == 34 && stamp.second == 56, "literal leap-day calendar fields in local timezone");
        NativeFileDateTime earlier{};
        check(GetNativeFileModificationTime("base-only.sav", earlier) && earlier.epochSeconds < stamp.epochSeconds,
              "read-only fallback file exposes actual earlier epoch for ordering");
        check(GetNativeFileModificationTime(u8"Grüße 雪.sav", earlier) && earlier.epochSeconds == leapDate,
              "Unicode virtual and selected physical paths");
        check(GetNativeFileModificationTime("epoch.sav", earlier) && earlier.epochSeconds == 0 &&
              earlier.year == 1970 && earlier.month == 1 && earlier.day == 1,
              "real Unix epoch is distinguishable from metadata failure");
        const NativeFileDateTime unchanged = stamp;
        for (const char* filename : {static_cast<const char*>(nullptr), "", "missing.sav", "directory.sav"}) {
            check(!GetNativeFileModificationTime(filename, stamp) && same(stamp, unchanged),
                  "null/empty/missing/directory metadata fails without publishing a false timestamp");
        }
        check(!GetNativeFileModificationTime((temporary.root / "outside.sav").c_str(), stamp) && same(stamp, unchanged),
              "unmounted host path is not a VFS save");
        HWFILE file = FileOpen(const_cast<char*>("shared.sav"), FILE_ACCESS_READ);
        check(file != 0, "open selected original VFS read handle");
        char byte = 0;
        check(FileRead(file, &byte, 1, nullptr) && byte == 'a', "read initial fixture byte");
        check(GetNativeFileModificationTime("shared.sav", stamp), "metadata while original handle open");
        check(FileRead(file, &byte, 1, nullptr) && byte == 'b', "metadata neither seeks nor closes original read handle");
        FileClose(file);
        check(fs::remove(temporary.root / "overlay/shared.sav"), "remove only authored overlay fixture");
        const NativeFileDateTime beforeFailure = stamp;
        check(!GetNativeFileModificationTime("shared.sav", stamp) && same(stamp, beforeFailure),
              "stale selected path failure cannot silently favor lower-profile save");
        std::cout << "PASS: " << checks << " native file-time assertions; authored temporary VFS profiles only\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n';
        return 1;
    }
}
