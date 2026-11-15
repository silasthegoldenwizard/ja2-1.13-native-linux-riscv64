/*
 * P710 native snapshot update: Utils/NativeEncryptedText.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native boundary for original fixed-width, encrypted UTF16LE EDT records.
// Preserve on-disk byte offsets and the original decrement-above-33 cipher;
// use the already bundled UTF8-CPP validator for native wchar_t conversion.
#include "Encrypted File.h"
#include "FileMan.h"
#include "random.h"
#include "../ext/VFS/ext/utfcpp/source/utf8.h"
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

namespace {
bool decode(STR16 destination, UINT32 bytes) {
    if (!destination) return false;
    try {
        if (bytes < 2 || bytes % 2) {
            destination[0] = 0;
            return false;
        }
        const auto* wire = reinterpret_cast<const unsigned char*>(destination);
        std::vector<UINT16> units;
        bool terminated = false;
        for (UINT32 offset = 0; offset < bytes; offset += 2) {
            UINT16 value = static_cast<UINT16>(wire[offset] | (UINT16(wire[offset+1]) << 8));
            if (!value) { terminated = true; break; }
            if (value > 33) --value;
            units.push_back(value);
        }
        // An exact-width caller has bytes/2 native slots. Require its original
        // record terminator instead of inventing a write one slot past it.
        if (!terminated) { destination[0] = 0; return false; }
        std::string unicode;
        utf8::utf16to8(units.begin(), units.end(), std::back_inserter(unicode));
        std::wstring text;
        utf8::utf8to32(unicode.begin(), unicode.end(), std::back_inserter(text));
        std::copy(text.begin(), text.end(), destination);
        destination[text.size()] = 0;
        return true;
    } catch (...) {
        destination[0] = 0;
        return false;
    }
}
bool valid(STR16 destination, UINT32 bytes) {
    if (!destination) return false;
    destination[0] = 0;
    return bytes >= 2 && bytes % 2 == 0;
}
struct File {
    HWFILE handle;
    ~File() { if (handle) FileClose(handle); }
};
BOOLEAN readRecord(HWFILE file, STR16 destination, UINT32 offset, UINT32 bytes) {
    UINT32 read = 0;
    if (!FileSeek(file, offset, FILE_SEEK_FROM_START) ||
        !FileRead(file, destination, bytes, &read) || read != bytes) {
        destination[0] = 0;
        return FALSE;
    }
    return decode(destination, bytes) ? TRUE : FALSE;
}
}

void DecodeString(STR16 destination, UINT32 bytes) { decode(destination, bytes); }

BOOLEAN LoadEncryptedDataFromFile(STR name, STR16 destination, UINT32 offset, UINT32 bytes) {
    if (!valid(destination, bytes) || !name) return FALSE;
    File file{FileOpen(name, FILE_ACCESS_READ, FALSE)};
    return file.handle ? readRecord(file.handle, destination, offset, bytes) : FALSE;
}

BOOLEAN LoadEncryptedDataFromFileRandomLine(STR name, STR16 destination, UINT32 bytes) {
    if (!valid(destination, bytes) || !name) return FALSE;
    File file{FileOpen(name, FILE_ACCESS_READ, FALSE)};
    if (!file.handle) return FALSE;
    const UINT32 records = FileGetSize(file.handle) / bytes;
    if (!records) return FALSE;
    // Preserve the original exclusion of the final record and avoid consuming
    // randomness for a single record, whose only valid index is zero.
    const UINT32 index = records > 1 ? Random(records - 1) : 0;
    return readRecord(file.handle, destination, index * bytes, bytes);
}
