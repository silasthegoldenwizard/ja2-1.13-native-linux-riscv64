/*
 * P710 native notice preparation: tests/native-core/native_vfs_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add native bfVFS archive/filesystem/Unicode integration tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/native_vfs_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "FileManIO.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_init.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/vfs_os_functions.h>
#include <vfs/Core/File/vfs_file.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Ext/slf/vfs_slf_library.h>
#include <vfs/Ext/7z/vfs_7z_library.h>
#include <vfs/Tools/vfs_log.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace
{
unsigned checks = 0;
fs::path root;

void check(bool condition, const std::string& label)
{
    ++checks;
    if (!condition) throw std::runtime_error(label);
}

template<class Function> bool throws(Function function)
{
    try { function(); } catch (const std::exception&) { return true; }
    return false;
}

vfs::Path path(const fs::path& value) { return vfs::Path(value.u8string()); }

std::string diskBytes(const fs::path& value)
{
    std::ifstream in(value, std::ios::binary);
    if (!in) throw std::runtime_error("cannot inspect generated fixture: " + value.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

HWFILE open(std::string name, UINT32 access = FILE_ACCESS_READ, std::string profile = {})
{
    return FileOpen(name.data(), access, FALSE, profile.empty() ? nullptr : profile.data());
}

std::string readAll(const std::string& name, const std::string& profile = {})
{
    const auto file = open(name, FILE_ACCESS_READ, profile);
    check(file != 0, "open actual VFS file: " + name);
    try
    {
        const UINT32 size = FileGetSize(file);
        std::string bytes(size, '\0');
        UINT32 count = 99;
        check(FileRead(file, bytes.data(), size, &count) && count == size, "read exact VFS bytes: " + name);
        check(FileCheckEndOfFile(file), "actual VFS EOF: " + name);
        FileClose(file);
        check(FileGetPos(file) == -1, "closed actual VFS handle: " + name);
        return bytes;
    }
    catch (...) { FileClose(file); throw; }
}

void resetVFS()
{
    fileman::CloseAll();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
}

void mount(std::unique_ptr<vfs::IBaseLocation> location,
           const std::string& name, const fs::path& realRoot, bool writable)
{
    auto profile = std::make_unique<vfs::CVirtualProfile>(name, path(realRoot), writable);
    profile->addLocation(location.get());
    location.release(); // The profile owns its unique locations.
    auto* entry = profile.get();
    getVFS()->getProfileStack()->pushProfile(entry);
    profile.release(); // The VFS profile stack owns it.
    for (auto it = entry->begin(); !it.end(); it.next())
        check(getVFS()->addLocation(it.value(), entry), "mount actual VFS location: " + name);
}

void mountDirectory(const std::string& name, bool writable)
{
    std::unique_ptr<vfs::IBaseLocation> location;
    if (writable)
    {
        auto directory = std::make_unique<vfs::CDirectoryTree>(vfs::Path(), path(root / name));
        check(directory->init(), "initialize writable directory");
        location = std::move(directory);
    }
    else
    {
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), path(root / name));
        check(directory->init(), "initialize readonly directory: " + name);
        location = std::move(directory);
    }
    mount(std::move(location), name, root / name, writable);
}

std::unique_ptr<vfs::CUncompressedLibraryBase> library(const fs::path& file)
{
    auto source = std::make_unique<vfs::CReadOnlyFile>(path(file));
    std::unique_ptr<vfs::CUncompressedLibraryBase> result;
    if (file.extension() == ".slf")
        result = std::make_unique<vfs::CSLFLibrary>(source.get(), vfs::Path(), true);
    else
        result = std::make_unique<vfs::CUncompressed7zLibrary>(source.get(), vfs::Path(), true);
    source.release();
    return result;
}

void unicode()
{
    const std::string utf8 = u8"Grüße 🌍";
    const std::wstring wide = L"Grüße \U0001f30d";
    check(vfs::String::as_utf16(utf8) == wide, "UTF-8 to native wide scalars");
    check(vfs::String::as_utf8(wide) == utf8, "native wide string to UTF-8");
    check(vfs::String::as_utf8(wide.c_str()) == utf8, "native wide pointer to UTF-8");
    check(vfs::String::as_utf8(wide.data(), wide.size()) == utf8, "explicit wide length");
    check(vfs::String(utf8).utf8() == utf8, "VFS String Unicode round trip");
    std::wstring output = L"stale";
    vfs::String::as_utf16("", output);
    check(output.empty(), "empty conversion replaces previous output");
    for (const std::string invalid : {std::string("\xc0\xaf", 2), std::string("\xf0\x9f", 2),
                                     std::string("\xed\xa0\x80", 3), std::string("\xf4\x90\x80\x80", 4)})
        check(throws([&] { vfs::String::as_utf16(invalid); }), "reject invalid UTF-8");
    check(throws([&] { vfs::String::as_utf8(std::wstring(1, wchar_t{0xd800})); }), "reject isolated wide surrogate");
    check(throws([&] { vfs::String::as_utf8(std::wstring(1, wchar_t{0x110000})); }), "reject invalid wide scalar");
}

int liveIterators = 0;
bool rejectClone = false;
class CountedIterator : public vfs::TIterator<int>::IImplementation
{
    int* data;
    unsigned index;
public:
    explicit CountedIterator(int* values, unsigned at = 0) : data(values), index(at) { ++liveIterators; }
    ~CountedIterator() override { --liveIterators; }
    int* value() override { return index < 3 ? data + index : nullptr; }
    void next() override { ++index; }
protected:
    IImplementation* clone() override {
        if (rejectClone) throw std::runtime_error("authored clone failure");
        return new CountedIterator(data, index);
    }
};
void iteratorOwnership()
{
    int values[] = {11, 22, 33};
    {
        vfs::TIterator<int> first(new CountedIterator(values));
        vfs::TIterator<int> copied(first);
        check(liveIterators == 2 && *first.value() == 11 && *copied.value() == 11, "iterator copy owns an independent implementation");
        copied.next();
        check(*first.value() == 11 && *copied.value() == 22, "copied iterator positions advance independently");
        vfs::TIterator<int> assigned(new CountedIterator(values, 2));
        assigned = first;
        check(liveIterators == 3 && *assigned.value() == 11, "copy assignment releases prior implementation");
        auto* const self = &assigned;
        assigned = *self;
        assigned.next();
        check(liveIterators == 3 && *assigned.value() == 22, "self assignment preserves usable iterator ownership");
        rejectClone = true;
        check(throws([&] { assigned = first; }), "clone failure propagates");
        rejectClone = false;
        check(liveIterators == 3 && *assigned.value() == 22, "failed assignment preserves prior position and ownership");
        vfs::TIterator<int> empty;
        assigned = empty;
        check(assigned.end() && liveIterators == 2, "assignment from empty releases implementation");
        copied.next(); copied.next();
        check(copied.end() && liveIterators == 1 && *first.value() == 11, "end releases only copied implementation");
    }
    check(liveIterators == 0, "all iterator implementations retire exactly once");
}

void looseProfiles()
{
    const auto aliasPath = root / "writable/NativeAlias.txt";
    {
        std::ofstream alias(aliasPath, std::ios::binary);
        alias << "abcdef";
        check(bool(alias), "create owned alias-upgrade fixture");
    }
    mountDirectory("base", false);
    mountDirectory("mod", false);
    mountDirectory("writable", true);
    check(readAll("shared.TXT") == "mod shared\n", "real profile overlay priority");
    check(readAll("Shared.txt", "base") == "base shared\n", "real named base profile");
    check(readAll("Shared.txt", "mod") == "mod shared\n", "real named mod profile");
    check(readAll("BaseOnly.txt") == "base only\n", "real base fallback");
    check(readAll("loose\\mixed.TXT") == "loose mixed payload\n", "case and backslash loose lookup");
    check(readAll(u8"Loose/Grüße.txt") == "loose bmp payload\n", "loose BMP filename");
    check(readAll(u8"Loose/🌍.txt") == "loose astral payload\n", "loose astral filename");
    check(open("Shared.txt", FILE_ACCESS_READ, "absent") == 0, "unknown named profile fails");

    auto file = open("WriteTarget.txt", FILE_ACCESS_WRITE);
    check(file != 0, "open real writable profile");
    UINT32 count = 0;
    check(FileWrite(file, "NEW", 3, &count) && count == 3, "write actual VFS bytes");
    check(FileGetSize(file) == 17 && FileGetPos(file) == 3, "size query preserves write cursor");
    FileClose(file);
    check(readAll("WriteTarget.txt") == "NEWtial writable\n", "update preserves remaining bytes");

    file = open("Generated/Created.txt", FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS);
    check(file != 0, "create through actual writable profile");
    check(FileWrite(file, "created\n", 8, &count) && count == 8, "write new profile file");
    FileClose(file);
    check(readAll("Generated/Created.txt") == "created\n", "read back newly created VFS file");
    check(fs::exists(root / "writable/Generated/Created.txt"), "creation stays in writable root");
    check(diskBytes(root / "base/Shared.txt") == "base shared\n", "base file preserved");
    check(diskBytes(root / "mod/Shared.txt") == "mod shared\n", "mod file preserved");

    const auto first = open("BaseOnly.txt"), second = open("BaseOnly.txt");
    check(first && second && first != second, "distinct IDs for actual VFS shared object");
    FileClose(first);
    char byte = 0;
    check(FileRead(second, &byte, 1, &count) && byte == 'b', "real alias remains readable");
    FileClose(first);
    FileClose(second);

    const auto reader = open("NativeAlias.txt");
    check(reader && FileSeek(reader, 2, FILE_SEEK_FROM_START), "reader before write upgrade");
    const auto writer = open("NativeAlias.txt", FILE_ACCESS_WRITE);
    check(writer && writer != reader, "write upgrade retains distinct opaque IDs");
    check(FileGetPos(reader) == 2, "write upgrade preserves shared read position");
    char original = 0;
    check(FileRead(reader, &original, 1, &count) && original == 'c', "read alias survives write upgrade");
    check(FileWrite(writer, "Y", 1, &count), "update stream changes read to write");
    check(FileRead(reader, &original, 1, &count) && original == 'e', "update stream changes write to read");
    check(FileSeek(writer, 2, FILE_SEEK_FROM_START) && FileWrite(writer, "Z", 1, &count), "upgraded writer remains usable");
    check(FileSeek(reader, 0, FILE_SEEK_FROM_START), "read alias seeks after write");
    char updated[6]{};
    check(FileRead(reader, updated, 6, &count) && std::string(updated, 6) == "abZYef", "read alias sees committed write bytes");
    FileClose(reader);
    check(FileGetPos(writer) == 6, "closing upgraded read alias retains writer");
    FileClose(writer);

    const auto retainedReader = open("NativeAlias.txt");
    check(retainedReader && FileSeek(retainedReader, 2, FILE_SEEK_FROM_START), "reader before failed upgrade");
    const auto backupPath = root / "writable/NativeAlias.owned-backup";
    fs::rename(aliasPath, backupPath);
    fs::create_directory(aliasPath); // Force EISDIR even when the test user is root.
    const auto rejectedWriter = open("NativeAlias.txt", FILE_ACCESS_WRITE);
    fs::remove(aliasPath);
    fs::rename(backupPath, aliasPath);
    check(rejectedWriter == 0, "write upgrade fails without replacing live reader");
    check(FileGetPos(retainedReader) == 2, "failed upgrade preserves read cursor");
    check(FileRead(retainedReader, &original, 1, &count) && original == 'Z', "reader survives failed write upgrade");
    FileClose(retainedReader);

    std::set<std::string> names;
    vfs::OS::CIterateDirectory iterator(path(root / "base"), "*.txt");
    vfs::String name;
    vfs::OS::CIterateDirectory::EFileAttribute attribute;
    while (iterator.nextFile(name, attribute)) names.insert(name.utf8());
    check(names == std::set<std::string>{"BaseOnly.txt", "Shared.txt"}, "POSIX wildcard filters directory entries");
    check(vfs::OS::checkRealDirectory(path(root / "base")), "real directory detection");
    check(!vfs::OS::checkRealDirectory(path(root / "base/Shared.txt")), "regular file is not a directory");
    resetVFS();

    mountDirectory("base", false);
    check(open("Shared.txt", FILE_ACCESS_WRITE) == 0, "no writable profile cannot overwrite base");
    check(diskBytes(root / "base/Shared.txt") == "base shared\n", "failed write preserves base");
    resetVFS();
}

void mountedArchive(const std::string& archive,
                    const std::vector<std::pair<std::string, std::string>>& expected)
{
    auto source = library(root / "archives" / archive);
    check(source->init(), "initialize actual archive: " + archive);
    mount(std::move(source), "archive", root / "archives", false);
    for (const auto& item : expected)
    {
        check(readAll(item.first) == item.second, "archive bytes: " + archive + ":" + item.first);
        const auto file = open(item.first);
        check(file != 0, "reopen archive file");
        if (!item.second.empty())
        {
            check(FileSeek(file, 1, FILE_SEEK_FROM_END), "archive seek from EOF");
            char last = 0; UINT32 count = 0;
            check(FileRead(file, &last, 1, &count) && last == item.second.back(), "archive last-byte oracle");
            check(FileSeek(file, 0, FILE_SEEK_FROM_START), "archive rewind");
            std::string bytes(item.second.size() + 5, '\0');
            check(!FileRead(file, bytes.data(), static_cast<UINT32>(bytes.size()), &count) && count == item.second.size(), "archive short read count");
            check(bytes.substr(0, count) == item.second, "archive short read bytes");
        }
        FileClose(file);
    }
    if (archive == "valid.slf")
    {
        check(readAll("ARCHIVE\\MIXED.txt") == "slf mixed payload\n", "SLF case/separator contract");
        for (const auto* hidden : {"Archive/Deleted.txt", "Archive/Old.txt", "Archive/Absent.txt"})
            check(open(hidden) == 0, "inactive SLF record absent");
        check(open("Archive/Mixed.TXT", FILE_ACCESS_WRITE) == 0, "readonly archive rejects write");
    }
    resetVFS();
}

void configuredArchive()
{
    // Exercise the original config-driven mount path used by full startup,
    // not just the direct library mounting helper above.
    vfs_init::VfsConfig config;
    auto* profile = new vfs_init::Profile;
    config.addProfile(profile, true);
    profile->m_name = "configured-archive";
    profile->m_root = path(root / "archives");
    auto* location = new vfs_init::Location;
    profile->addLocation(location, true);
    location->m_type = "LIBRARY";
    location->m_path = "valid.slf";
    check(vfs_init::initVirtualFileSystem(config), "original configured SLF initialization");
    check(readAll("Archive/Mixed.TXT") == "slf mixed payload\n", "configured SLF retains exact bytes");
    check(open("Archive/Mixed.TXT", FILE_ACCESS_WRITE) == 0, "configured SLF stays readonly");
    resetVFS();
}

void archives()
{
    mountedArchive("valid.slf", {{"Archive/Mixed.TXT", "slf mixed payload\n"},
                                 {u8"Archive/Grüße.txt", "slf unicode payload\n"}});
    mountedArchive("prefixed.slf", {{"Prefix/Inner.TXT", "prefixed payload\n"}});
    mountedArchive("empty.slf", {});
    const std::vector<std::pair<std::string, std::string>> seven{
        {"Seven/Mixed.TXT", "7z mixed payload\n"}, {u8"Seven/Grüße.txt", "7z bmp payload\n"},
        {u8"Seven/🌍.txt", "7z astral payload\n"}, {"Seven/Empty.txt", ""}};
    for (const auto* name : {"copy.7z", "copy-nonsolid.7z", "solid-copy.7z"})
        mountedArchive(name, seven);
    for (const auto* name : {"short-header.slf", "negative-count.slf", "overflow-count.slf", "impossible-count.slf",
            "truncated-directory.slf", "range-before-payload.slf", "range-past-eof.slf", "range-in-directory.slf",
            "range-cross-directory.slf", "no-nul-entry.slf", "no-nul-library-name.slf", "no-nul-library-path.slf",
            "case-collision.slf", "truncated.7z", "bad-signature.7z", "truncated-body.7z", "compressed.7z"})
    {
        bool accepted = false;
        try { auto source = library(root / "archives" / name); accepted = source->init(); }
        catch (const std::exception&) {}
        check(!accepted, std::string("reject invalid/unsupported archive: ") + name);
    }
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("expected generated fixture directory");
        root = fs::canonical(argv[1]);
        check(fs::exists(root / "manifest.json"), "independent fixture manifest exists");
        check(sizeof(void*) == 8 && sizeof(wchar_t) == 4, "native Linux LP64 representation");
        unicode();
        iteratorOwnership();
        looseProfiles();
        archives();
        configuredArchive();
        resetVFS();
        vfs::ObjectAllocator::clear();
        std::ofstream result(root / "native-vfs-result.json");
        result << "{\"passed\":true,\"checks\":" << checks
               << ",\"provider\":\"production bfVFS + FileManVFS\",\"synthetic_only\":true}\n";
        std::cout << "PASS: " << checks << " actual VFS/FileMan/SLF/7z checks\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        resetVFS();
        vfs::ObjectAllocator::clear();
        return 1;
    }
}
