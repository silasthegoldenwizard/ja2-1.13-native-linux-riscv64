/*
 * P710 native snapshot update: tests/native-core/native_map_message_save_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original map-message save/load and FileMan/bfVFS, with authored text only.
#include "message.h"
#include "FileMan.h"
#include "BufferMan.h"
#include <language.hpp>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

extern void AddStringToMapScreenMessageList(STR16, UINT16, UINT32, BOOLEAN, UINT8);
extern UINT8 gubEndOfMapScreenMessageList, gubStartOfMapScreenMessageList;
namespace fs = std::filesystem;
namespace {
using Bytes = std::vector<UINT8>;
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
struct Temporary {
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-map-message-save-XXXXXX";
        auto* made = mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique temporary directory");
        root = made;
    }
    ~Temporary() { std::error_code error; fs::remove_all(root, error); }
};
void mount(const fs::path& root) {
    auto profile = std::make_unique<vfs::CVirtualProfile>(
        "map-message-save-fixtures", vfs::Path(root.u8string()), true);
    auto directory = std::make_unique<vfs::CDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "owned fixture directory initialized");
    auto* location = directory.get();
    profile->addLocation(directory.release());
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "owned fixture VFS mounted");
}
struct BufferBounds {
    explicit BufferBounds(Bytes& bytes, std::size_t length) {
        gpLoadBufferStart = reinterpret_cast<char*>(bytes.data());
        giLoadBufferSize = static_cast<INT32>(length);
        gfBufferLoadFailed = FALSE;
    }
    ~BufferBounds() { gpLoadBufferStart = nullptr; giLoadBufferSize = 0; gfBufferLoadFailed = FALSE; }
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
    Bytes bytes;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const UINT16 unit = i == text.size() ? 0 : text[i];
        bytes.push_back(unit & 255); bytes.push_back(unit >> 8);
    }
    return bytes;
}
Bytes save(unsigned id = 0) {
    const std::string filename = "messages" + std::to_string(id);
    auto file = FileOpen(const_cast<char*>(filename.c_str()), FILE_ACCESS_WRITE | FILE_CREATE_ALWAYS, FALSE);
    check(file != 0, "original FileMan opens owned save fixture");
    const bool saved = SaveMapScreenMessagesToSaveGameFile(file);
    FileClose(file);
    check(saved, "original map-message save succeeds");
    file = FileOpen(const_cast<char*>(filename.c_str()), FILE_ACCESS_READ | FILE_OPEN_EXISTING, FALSE);
    check(file != 0, "original FileMan reopens owned fixture");
    Bytes bytes(FileGetSize(file));
    UINT32 count = 0;
    check(FileRead(file, bytes.data(), bytes.size(), &count) && count == bytes.size(), "exact fixture read");
    FileClose(file);
    return bytes;
}
struct Record { Bytes text; UINT32 font, time, flags; UINT16 color; UINT8 begin; };
std::vector<Record> parse(const Bytes& bytes) {
    std::vector<Record> records;
    std::size_t at = 3;
    for (unsigned i = 0; i < 256; ++i) {
        const auto length = get32(bytes, at); at += 4;
        Record record{};
        if (length) {
            check(at + length + 28 <= bytes.size(), "message and metadata remain inside record");
            record.text.assign(bytes.begin() + at, bytes.begin() + at + length); at += length;
            // Literal original StringSaveStruct offsets; unused padding is excluded.
            record.font = get32(bytes, at); record.time = get32(bytes, at + 4);
            record.flags = get32(bytes, at + 8);
            record.color = bytes.at(at + 24) | (UINT16(bytes.at(at + 25)) << 8);
            record.begin = bytes.at(at + 26); at += 28;
        }
        records.push_back(record);
    }
    check(at == bytes.size(), "all 256 slots have exact record boundaries");
    return records;
}
void load(Bytes& bytes, std::size_t length, bool expected = true) {
    BufferBounds bounds(bytes, length);
    auto* cursor = reinterpret_cast<char*>(bytes.data());
    check(bool(LoadMapScreenMessagesFromSaveGameFile(&cursor)) == expected, "original map-message load result");
    check(bool(gfBufferLoadFailed) == !expected, "original buffer error flag agrees");
    if (expected) check(cursor == reinterpret_cast<char*>(bytes.data() + length) &&
                        static_cast<UINT8>(*cursor) == 0xa5, "exact cursor and following canary");
    else check(cursor >= reinterpret_cast<char*>(bytes.data()) &&
               cursor <= reinterpret_cast<char*>(bytes.data() + length), "rejection cursor remains bounded");
}
void compare(const std::vector<Record>& a, const std::vector<Record>& b) {
    for (unsigned i = 0; i < 256; ++i) {
        check(a[i].text == b[i].text, "native message text and null slots survive roundtrip");
        if (!a[i].text.empty()) check(a[i].font == b[i].font && a[i].time == b[i].time &&
            a[i].flags == b[i].flags && a[i].color == b[i].color && a[i].begin == b[i].begin,
            "all serialized message metadata survives");
    }
}
void roundtrip(bool wrap) {
    FreeGlobalMessageList();
    const std::vector<std::wstring> subjects{L"ABC",L"ABCD",L"",L"\u00e4\u03a9\u4e2d",L"x\U0001f642y",std::wstring(511,L'Z')};
    const std::vector<std::u16string> expected{u"ABC",u"ABCD",u"",u"\u00e4\u03a9\u4e2d",u"x\U0001f642y",std::u16string(511,u'Z')};
    const unsigned total = wrap ? 260 : subjects.size();
    for (unsigned i = 0; i < total; ++i)
        AddStringToMapScreenMessageList(const_cast<STR16>(subjects[i % subjects.size()].c_str()), 0x1234 + i, 17 + i, i % 2, i % 9);
    auto bytes = save();
    auto records = parse(bytes);
    for (unsigned i = 0; i < 256; ++i) {
        if (i < std::min(total, 256u)) {
            const unsigned inserted = wrap && i < 4 ? 256 + i : i;
            check(records[i].text == wire(expected[inserted % expected.size()]), "literal UTF16LE message bytes");
            check(records[i].font == 17 + inserted && records[i].color == 0x1234 + inserted &&
                  records[i].flags == inserted % 9 && records[i].begin == inserted % 2, "authored message fields");
        } else check(records[i].text.empty(), "null table entry has zero wire length");
    }
    const auto end = bytes[0], start = bytes[1];
    const auto length = bytes.size(); bytes.push_back(0xa5);
    // Load over an existing populated table to exercise replacement ownership.
    load(bytes, length);
    check(gubEndOfMapScreenMessageList == end && gubStartOfMapScreenMessageList == start,
          "ring start/end indexes survive");
    check(gubCurrentMapMessageString == static_cast<UINT8>((start + GetRangeOfMapScreenMessages() -
          std::min<unsigned>(GetRangeOfMapScreenMessages(), MAX_MESSAGES_ON_MAP_BOTTOM)) % 256), "current index retains original move-to-end policy");
    compare(records, parse(save(1)));
    FreeGlobalMessageList();
}
Bytes malformed(UINT32 size, const Bytes& subject) {
    Bytes bytes(7, 0); put32(bytes, 3, size);
    bytes.insert(bytes.end(), subject.begin(), subject.end());
    return bytes;
}
void rejections() {
    for (UINT32 size : {1u,3u,1026u,0xffffffffu}) {
        auto bytes = malformed(size, {}); load(bytes, bytes.size(), false);
    }
    for (const auto& subject : std::vector<Bytes>{{0x41,0},{0,0xd8,0,0},{0,0xdc,0,0},{0,0,0x41,0,0,0}}) {
        auto bytes = malformed(subject.size(), subject); load(bytes, bytes.size(), false);
    }
    auto bytes = malformed(6, wire(u"AB"));
    for (unsigned length = 0; length < bytes.size(); ++length) load(bytes, length, false);
    // A full subject followed by a truncated metadata block must release it.
    bytes.resize(bytes.size() + 27, 0);
    load(bytes, bytes.size(), false);
    FreeGlobalMessageList();
    const auto cleared = parse(save(2));
    check(std::all_of(cleared.begin(), cleared.end(), [](const Record& r){return r.text.empty();}),
          "failed native load releases every partial table entry");
}
}
int main() {
    try {
        static_assert(sizeof(wchar_t) == 4, "exercise native wide boundary");
        Temporary temporary; mount(temporary.root);
        check(InitializeFileManager(nullptr), "original FileMan initialized");
        roundtrip(false); roundtrip(true); rejections();
        FreeGlobalMessageList(); ShutdownFileManager();
        std::cout << "map-message save: " << checks << " checks passed; original functions and FileMan/VFS, authored text only\n";
        return 0;
    } catch (const std::exception& error) {
        FreeGlobalMessageList(); ShutdownFileManager();
        std::cerr << "map-message save failed at check " << checks << ": " << error.what() << '\n';
        return 1;
    }
}
