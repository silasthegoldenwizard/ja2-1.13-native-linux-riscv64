/*
 * P710 native snapshot update: tests/native-core/native_email_save_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for the production email save decoder.
#include "BufferMan.h"
#include "GameVersion.h"
#include "SaveLoadGame.h"
#include "email.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

extern BOOLEAN LoadEmailFromSavedGame(CHAR8** ppBuffer);

namespace {
using Bytes = std::vector<UINT8>;

[[noreturn]] void invariantFailure()
{
    std::abort();
}

template<class T>
void append(Bytes& bytes, const T& value)
{
    const auto* begin = reinterpret_cast<const UINT8*>(&value);
    bytes.insert(bytes.end(), begin, begin + sizeof(T));
}

UINT8 byteAt(const std::uint8_t* data, std::size_t size, std::size_t index)
{
    return size ? data[index % size] : 0;
}

void appendSubject(Bytes& bytes, const std::uint8_t* data, std::size_t size,
                   std::size_t seed)
{
    const UINT32 characters = byteAt(data, size, seed) % 9;
    const UINT32 extent = (characters + 1) * 2;
    append(bytes, extent);
    for (UINT32 index = 0; index < characters; ++index) {
        bytes.push_back('A' + byteAt(data, size, seed + index + 1) % 26);
        bytes.push_back(0);
    }
    bytes.push_back(0);
    bytes.push_back(0);
}

Bytes validPayload(const std::uint8_t* data, std::size_t size)
{
    Bytes bytes;
    const UINT32 count = 1 + byteAt(data, size, 1) % 3;
    append(bytes, count);
    for (UINT32 index = 0; index < count; ++index) {
        appendSubject(bytes, data, size, index * 5 + 2);
        SavedEmailStruct record{};
        record.usOffset = (byteAt(data, size, index + 10) % 4 == 0) ?
            BOBBYR_ORDER_CONFIRMED : 0xffff;
        record.usLength = 1;
        record.ubSender = 1;
        record.iId = static_cast<INT32>(index);
        record.iCurrentShipmentDestinationID = -1;
        append(bytes, record);
        if (record.usOffset == BOBBYR_ORDER_CONFIRMED) {
            const UINT32 bodyCharacters = byteAt(data, size, index + 20) % 8;
            const UINT32 bodyUnits = bodyCharacters + 1;
            append(bytes, bodyUnits);
            for (UINT32 body = 0; body < bodyCharacters; ++body) {
                const CHAR16 value = static_cast<CHAR16>(L'a' + body % 26);
                append(bytes, value);
            }
            const CHAR16 terminator = 0;
            append(bytes, terminator);
        }
    }
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data, std::size_t size)
{
    if (!size) return {};
    switch (data[0] % 8) {
        case 0:
            return Bytes(data + 1, data + size);
        case 1:
            return validPayload(data, size);
        case 2: {
            auto bytes = validPayload(data, size);
            bytes.resize(byteAt(data, size, 2) * bytes.size() / 255);
            return bytes;
        }
        case 3: {
            auto bytes = validPayload(data, size);
            for (std::size_t index = 1; index + 2 < size && !bytes.empty(); index += 3) {
                const std::size_t position =
                    ((static_cast<std::size_t>(data[index]) << 8) | data[index + 1]) %
                    bytes.size();
                bytes[position] ^= data[index + 2];
            }
            return bytes;
        }
        case 4: {
            Bytes bytes;
            const UINT32 count = std::numeric_limits<UINT32>::max();
            append(bytes, count);
            return bytes;
        }
        case 5: {
            Bytes bytes;
            const UINT32 count = 1;
            const UINT32 invalidSubject = (byteAt(data, size, 2) & 1) ? 1 : 0xffffffffu;
            append(bytes, count);
            append(bytes, invalidSubject);
            return bytes;
        }
        case 6: {
            auto bytes = validPayload(data, size);
            if (bytes.size() >= sizeof(UINT32)) {
                const UINT32 excessive = std::numeric_limits<UINT32>::max();
                std::memcpy(bytes.data() + bytes.size() - sizeof(UINT32),
                            &excessive, sizeof(excessive));
            }
            return bytes;
        }
        default: {
            auto bytes = validPayload(data, size);
            if (bytes.size() > 1) bytes.resize(bytes.size() - 1);
            return bytes;
        }
    }
}

void verifyList(UINT32 expected)
{
    UINT32 count = 0;
    EmailPtr previous = nullptr;
    for (EmailPtr email = pEmailList; email; email = email->Next) {
        if (++count > expected || email->Prev != previous || !email->pSubject)
            invariantFailure();
        bool terminated = false;
        for (UINT32 index = 0; index < EMAIL_SUBJECT_LENGTH; ++index) {
            if (!email->pSubject[index]) {
                terminated = true;
                break;
            }
        }
        if (!terminated) invariantFailure();
        previous = email;
    }
    if (count != expected) invariantFailure();
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,
                                       std::size_t size)
{
    if (!size || size > 65536) return 0;
    ShutDownEmailList();
    guiCurrentSaveGameVersion = SAVE_GAME_VERSION;
    auto bytes = inputFromRecipe(data, size);
    if (bytes.empty()) return 0;

    gpLoadBufferStart = reinterpret_cast<CHAR8*>(bytes.data());
    giLoadBufferSize = static_cast<INT32>(bytes.size());
    gfBufferLoadFailed = FALSE;
    CHAR8* begin = gpLoadBufferStart;
    CHAR8* cursor = begin;
    const BOOLEAN loaded = LoadEmailFromSavedGame(&cursor);
    const auto startAddress = reinterpret_cast<std::uintptr_t>(begin);
    const auto cursorAddress = reinterpret_cast<std::uintptr_t>(cursor);
    if (cursorAddress < startAddress || cursorAddress - startAddress > bytes.size())
        invariantFailure();

    UINT32 expected = 0;
    if (bytes.size() >= sizeof(expected)) std::memcpy(&expected, bytes.data(), sizeof(expected));
    if (!loaded || gfBufferLoadFailed) {
        if (loaded || pEmailList) invariantFailure();
    } else {
        verifyList(expected);
    }

    ShutDownEmailList();
    if (pEmailList) invariantFailure();
    gpLoadBufferStart = nullptr;
    giLoadBufferSize = 0;
    gfBufferLoadFailed = FALSE;
    return 0;
}
