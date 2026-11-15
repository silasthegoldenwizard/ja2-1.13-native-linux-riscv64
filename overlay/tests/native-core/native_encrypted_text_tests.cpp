/*
 * P710 native snapshot update: tests/native-core/native_encrypted_text_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Encrypted File.h"
#include "FileManIO.h"
#include "FileManBackend.h"
#include "GameSettings.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
UINT32 randomBound = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
std::vector<unsigned char> wire(std::initializer_list<UINT16> units) {
    std::vector<unsigned char> bytes;
    for (auto unit : units) {
        bytes.push_back(static_cast<unsigned char>(unit));
        bytes.push_back(static_cast<unsigned char>(unit >> 8));
    }
    return bytes;
}
void decoded(const std::vector<unsigned char>& bytes, const std::wstring& expected) {
    const std::size_t capacity = bytes.size()/2;
    std::vector<wchar_t> text(capacity + 2, L'#');
    std::memcpy(text.data()+1, bytes.data(), bytes.size());
    DecodeString(text.data()+1, static_cast<UINT32>(bytes.size()));
    check(std::wstring(text.data()+1) == expected, "packed UTF16LE becomes literal native text");
    check(text.front()==L'#' && text.back()==L'#', "in-place widening preserves exact-capacity guards");
}
void write(const fs::path& file, const std::vector<unsigned char>& bytes) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    check(bool(out), "write owned synthetic EDT fixture");
}
void mount(const fs::path& root) {
    auto location = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
    check(location->init(), "initialize actual readonly VFS fixture");
    auto profile = std::make_unique<vfs::CVirtualProfile>("encrypted-text", vfs::Path(root.u8string()), false);
    auto* mounted = profile.get();
    auto* directory = location.get();
    profile->addLocation(location.release());
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(directory, mounted), "mount actual FileMan/VFS fixture");
}
void cleanup() {
    fileman::CloseAll();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
}
}
GAME_EXTERNAL_OPTIONS gGameExternalOptions;
UINT32 NewRandom(UINT32 limit) { randomBound=limit; return 0; }
UINT32 GetRndNum(UINT32 limit) { randomBound=limit; return 0; }

int main(int argc, char** argv) {
    try {
        check(argc==2, "owned fixture directory argument");
        static_assert(sizeof(wchar_t)==4, "exercise Linux native character width");
        // Literal encrypted code units are independent of the production decoder.
        decoded(wire({0x42,0x4a,0x4e,0}), L"AIM");
        decoded(wire({0x20,0x21,0x26,0x000a,0}), L" !%\n");
        decoded(wire({0x00aa,0x4e2e,0xd83e,0xde01,0}), L"\u00a9\u4e2d\U0001f600");
        decoded(wire({0,0xffff,0xffff}), L"");
        decoded(wire({0xd801,0}), L"");
        decoded(wire({0xdc01,0}), L"");
        decoded(wire({0x42,0x42,0x42,0x42}), L"");
        std::array<wchar_t,4> invalid{{L'#',L'x',L'y',L'#'}};
        DecodeString(invalid.data()+1, 3);
        check(invalid[1]==0 && invalid[0]==L'#' && invalid[3]==L'#', "odd wire count is bounded empty text");
        DecodeString(nullptr, 8);
        DecodeString(invalid.data()+1, 0);
        check(invalid[1]==0, "zero count remains empty without division");
        const fs::path root = fs::absolute(argv[1]);
        fs::create_directories(root);
        const auto first=wire({0x42,0x4a,0x4e,0,0,0,0,0});
        auto two=first;
        const auto second=wire({0x55,0x66,0x74,0x75,0,0,0,0});
        two.insert(two.end(),second.begin(),second.end());
        write(root/"two.edt",two);
        write(root/"one.edt",first);
        write(root/"short.edt",{0x42,0,0,0});
        write(root/"empty.edt",{});
        write(root/"bad.edt",wire({0xd801,0,0,0,0,0,0,0}));
        mount(root);
        std::array<wchar_t,10> text;
        text.fill(L'#');
        check(LoadEncryptedDataFromFile("two.edt",text.data()+1,0,16) && std::wstring(text.data()+1)==L"AIM", "actual FileMan loads first fixed-width record");
        check(LoadEncryptedDataFromFile("two.edt",text.data()+1,16,16) && std::wstring(text.data()+1)==L"Test", "file offsets remain wire-byte offsets");
        check(!LoadEncryptedDataFromFile("missing.edt",text.data()+1,0,16) && !text[1], "missing record returns initialized empty text");
        check(!LoadEncryptedDataFromFile("short.edt",text.data()+1,0,16) && !text[1], "short read rejects partial/uninitialized characters");
        check(!LoadEncryptedDataFromFile("bad.edt",text.data()+1,0,16) && !text[1], "malformed surrogate is rejected by loader");
        check(!LoadEncryptedDataFromFile("two.edt",text.data()+1,0,15) && !text[1], "odd byte request rejected");
        check(!LoadEncryptedDataFromFile("two.edt",nullptr,0,16), "null destination rejected");
        check(!LoadEncryptedDataFromFileRandomLine("empty.edt",text.data()+1,16) && !text[1], "empty random file cannot underflow record count");
        check(!LoadEncryptedDataFromFileRandomLine("one.edt",text.data()+1,0), "zero random record size rejected");
        check(LoadEncryptedDataFromFileRandomLine("one.edt",text.data()+1,16) && std::wstring(text.data()+1)==L"AIM", "single random record works");
        check(LoadEncryptedDataFromFileRandomLine("two.edt",text.data()+1,16) && randomBound==1 && std::wstring(text.data()+1)==L"AIM", "existing last-record exclusion policy is preserved");
        check(text.front()==L'#' && text.back()==L'#', "loader exact output guards preserved");
        cleanup();
        std::cout << "PASS: " << checks << " encrypted UTF16 text/FileMan assertions\n";
        return 0;
    } catch (const std::exception& error) {
        cleanup();
        std::cerr << "FAIL after " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
