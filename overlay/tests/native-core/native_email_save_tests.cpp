/*
 * P710 native snapshot update: tests/native-core/native_email_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Genuine email save/load functions and FileMan/bfVFS with authored email data.
// No game assets, user profiles, or existing saves are used.
#include "FileMan.h"
#include "MemMan.h"
#include "SaveLoadGame.h"
#include "GameVersion.h"
#include "BufferMan.h"
#include "email.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern BOOLEAN SaveEmailToSavedGame(HWFILE);
extern BOOLEAN LoadEmailFromSavedGame(CHAR8**);

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
using Bytes = std::vector<UINT8>;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-email-save-XXXXXX";
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
        "email-save-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(
        vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "owned fixture directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "owned fixture VFS mounted");
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
void put32(Bytes& bytes, std::size_t at, UINT32 value) {
    for (unsigned i = 0; i < 4; ++i) bytes.at(at + i) = (value >> (8 * i)) & 255;
}
UINT32 get32(const Bytes& bytes, std::size_t at) {
    UINT32 value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= UINT32(bytes.at(at + i)) << (8 * i);
    return value;
}
Bytes wire(const std::u16string& text) {
    // Independent UTF16LE oracle from literal code units, including NUL.
    Bytes bytes;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const UINT16 unit = i == text.size() ? 0 : text[i];
        bytes.push_back(unit & 255);
        bytes.push_back(unit >> 8);
    }
    return bytes;
}
void setEmails(const std::vector<std::wstring>& subjects) {
    ShutDownEmailList();
    EmailPtr previous = nullptr;
    unsigned id = 0;
    for (const auto& subject : subjects) {
        auto* email = static_cast<EmailPtr>(MemAlloc(sizeof(Email)));
        check(email != nullptr, "authored email allocated with original allocator");
        std::memset(email, 0, sizeof(Email));
        email->pSubject = static_cast<STR16>(MemAlloc((subject.size() + 1) * sizeof(CHAR16)));
        check(email->pSubject != nullptr, "authored subject allocated");
        std::memcpy(email->pSubject, subject.c_str(), (subject.size() + 1) * sizeof(CHAR16));
        email->usOffset = 0xffff; // Avoid the separate Bobby Ray body branch.
        email->usLength = 31 + id;
        email->ubSender = 17 + id;
        email->iDate = 0x12345678u + id;
        email->iId = id;
        email->iFirstData = -101 - id;
        email->uiSecondData = 0xfedcba00u + id;
        email->fRead = id % 2;
        email->fNew = !email->fRead;
        email->iCurrentShipmentDestinationID = -7;
        email->Prev = previous;
        if (previous) previous->Next = email;
        else pEmailList = email;
        previous = email;
        ++id;
    }
}
Bytes save(unsigned index, bool expected = true) {
    const std::string filename = "email" + std::to_string(index);
    auto file = FileOpen(const_cast<char*>(filename.c_str()),
                         FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, FALSE);
    check(file != 0, "original FileMan opens owned save fixture");
    const bool saved = SaveEmailToSavedGame(file);
    FileClose(file);
    check(saved == expected, "original email save result");
    file = FileOpen(const_cast<char*>(filename.c_str()),
                    FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    check(file != 0, "original FileMan reopens owned save fixture");
    Bytes bytes(FileGetSize(file));
    UINT32 count = 0;
    check(FileRead(file, bytes.data(), bytes.size(), &count) && count == bytes.size(),
          "original FileMan reads exact synthetic bytes");
    FileClose(file);
    return bytes;
}
void roundtrip() {
    const std::vector<std::wstring> subjects{
        L"ABC", L"ABCD", L"", L"\u00e4\u03a9\u4e2d", L"x\U0001f642y",
        std::wstring(EMAIL_SUBJECT_LENGTH - 1, L'Z')};
    const std::vector<std::u16string> expected{
        u"ABC", u"ABCD", u"", u"\u00e4\u03a9\u4e2d", u"x\U0001f642y",
        std::u16string(EMAIL_SUBJECT_LENGTH - 1, u'Z')};
    setEmails(subjects);
    auto bytes = save(0);
    check(get32(bytes, 0) == subjects.size(), "saved email count");
    std::size_t at = 4;
    for (const auto& subject : expected) {
        const auto literal = wire(subject);
        check(get32(bytes, at) == literal.size(), "subject length counts UTF16LE bytes");
        at += 4;
        check(at + literal.size() <= bytes.size() &&
              std::equal(literal.begin(), literal.end(), bytes.begin() + at),
              "literal subject bytes preserve UTF16 and surrogate pairs");
        at += literal.size() + sizeof(SavedEmailStruct);
    }
    check(at == bytes.size(), "all following email record boundaries are exact");
    bytes.push_back(0xa5);
    ShutDownEmailList();
    BufferBounds bounds(bytes);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(LoadEmailFromSavedGame(&cursor), "original email load succeeds");
    check(!gfBufferLoadFailed && cursor == reinterpret_cast<char*>(bytes.data() + at) &&
          static_cast<UINT8>(*cursor) == 0xa5, "load consumes exact section before following canary");
    auto* email = pEmailList;
    EmailPtr previous = nullptr;
    for (unsigned id = 0; id < subjects.size(); ++id) {
        check(email && std::wstring(email->pSubject) == subjects[id], "native subject equality after roundtrip");
        check(email->Prev == previous && email->iId == static_cast<INT32>(id) &&
              email->usLength == 31 + id && email->ubSender == 17 + id &&
              email->iDate == 0x12345678u + id &&
              email->iFirstData == -101 - static_cast<INT32>(id) &&
              email->uiSecondData == 0xfedcba00u + id &&
              email->iCurrentShipmentDestinationID == -7 &&
              email->fRead == id % 2 && email->fNew == !(id % 2),
              "following email fields and list links survive subject conversion");
        previous = email;
        email = email->Next;
    }
    check(email == nullptr, "no additional email record");
    ShutDownEmailList();
}
void reject(Bytes bytes, INT32 length = -1, std::size_t expectedOffset = 8) {
    setEmails({L"old state"});
    BufferBounds bounds(bytes, length);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(!LoadEmailFromSavedGame(&cursor) && gfBufferLoadFailed,
          "malformed subject rejects through original buffered-load error flag");
    check(cursor == reinterpret_cast<char*>(bytes.data() + expectedOffset),
          "subject rejection retains exact bounded cursor");
    check(pEmailList == nullptr, "subject rejection releases original temporary email list");
}
Bytes malformed(UINT32 length, const Bytes& subject = {}) {
    Bytes bytes(8, 0);
    put32(bytes, 0, 1);
    put32(bytes, 4, length);
    bytes.insert(bytes.end(), subject.begin(), subject.end());
    bytes.push_back(0xa5);
    return bytes;
}
void rejections() {
    for (UINT32 length : {0u, 1u, 3u, 258u, 0xffffffffu}) reject(malformed(length));
    for (const Bytes& subject : {
            Bytes{0x41, 0},                         // Missing NUL.
            Bytes{0, 0xd8, 0, 0},                  // Lone high surrogate.
            Bytes{0, 0xdc, 0, 0},                  // Lone low surrogate.
            Bytes{0, 0xd8, 0x41, 0, 0, 0},        // Invalid surrogate pair.
            Bytes{0, 0, 0x41, 0, 0, 0}})          // Embedded NUL.
        reject(malformed(subject.size(), subject), -1, 8 + subject.size());
    const auto subject = wire(u"AB");
    auto bytes = malformed(subject.size(), subject);
    for (INT32 length = 0; length < static_cast<INT32>(subject.size()); ++length)
        reject(bytes, 8 + length);
    for (INT32 length = 0; length < 4; ++length) reject(bytes, 4 + length, 4);
    setEmails({L"first", L"second"});
    auto partial = save(3);
    const std::size_t second = 8 + get32(partial, 4) + sizeof(SavedEmailStruct);
    put32(partial, second, 0);
    reject(partial, -1, second + 4);
    setEmails({std::wstring(EMAIL_SUBJECT_LENGTH, L'A')});
    check(save(1, false).size() == 4, "oversize subject rejects before its wire header");
    setEmails({std::wstring(1, static_cast<wchar_t>(0xd800))});
    check(save(2, false).size() == 4, "invalid native scalar rejects before its wire header");
    ShutDownEmailList();
}
}
int main() {
    try {
        static_assert(sizeof(wchar_t) == 4, "exercise native wide-character boundary");
        Temporary temporary;
        mount(temporary.root);
        check(InitializeFileManager(nullptr), "original FileMan initialized");
        guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
        roundtrip();
        rejections();
        ShutdownFileManager();
        std::cout << "email save: " << checks
                  << " checks passed; original email save/load and FileMan/VFS, synthetic data only\n";
        return 0;
    } catch (const std::exception& error) {
        ShutDownEmailList();
        ShutdownFileManager();
        std::cerr << "email save failed at check " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
