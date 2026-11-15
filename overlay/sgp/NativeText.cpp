/*
 * P710 native snapshot update: sgp/NativeText.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Reuse the accepted VFS UTF8-CPP conversion rather than a second Unicode codec.
#include "native_text.h"
#include <vfs/Core/vfs_string.h>
#include <algorithm>
#include <climits>
#include <cstring>
#include <cwchar>
#include <string>
#include <iterator>
#include <vector>
#include "../ext/VFS/ext/utfcpp/source/utf8.h"

namespace {
template<typename C, typename Convert>
int copyText(const C* source, int length, typename Convert::output* destination,
             std::size_t capacity, Convert convert) noexcept
{
    if (!source || length < -1 || length == 0 || (!destination && capacity)) return 0;
    try {
        const bool terminate = length == -1;
        const std::size_t count = terminate ? std::char_traits<C>::length(source) : static_cast<std::size_t>(length);
        const auto text = convert(std::basic_string<C>(source, count));
        if (text.size() > static_cast<std::size_t>(INT_MAX) - (terminate ? 1u : 0u)) return 0;
        const std::size_t required = text.size() + (terminate ? 1u : 0u);
        if (!destination) return capacity == 0 ? static_cast<int>(required) : 0;
        if (capacity < required) return 0;
        std::copy(text.begin(), text.end(), destination);
        if (terminate) destination[text.size()] = 0;
        return static_cast<int>(required);
    } catch (...) { return 0; }
}
struct ToWide {
    using output = wchar_t;
    std::wstring operator()(const std::string& text) const { return vfs::String::as_utf16(text); }
};
struct ToUtf8 {
    using output = char;
    std::string operator()(const std::wstring& text) const { return vfs::String::as_utf8(text); }
};
}
int sgp::Utf8ToNativeWide(const char* source, int length, wchar_t* destination, std::size_t capacity) noexcept
{ return copyText(source, length, destination, capacity, ToWide{}); }
int sgp::NativeWideToUtf8(const wchar_t* source, int length, char* destination, std::size_t capacity) noexcept
{ return copyText(source, length, destination, capacity, ToUtf8{}); }

bool sgp::Utf8FieldToNativeWide(const char* source, std::size_t fieldBytes,
                              wchar_t* destination, std::size_t capacity) noexcept
{
    if (!destination || !capacity) return false;
    destination[0] = 0;
    if (!source || fieldBytes > static_cast<std::size_t>(INT_MAX)) return false;
    const auto end = std::find(source, source + fieldBytes, '\0');
    const auto count = static_cast<int>(end - source);
    if (!count) return true;
    const int units = Utf8ToNativeWide(source, count, destination, capacity - 1);
    if (!units) return false;
    destination[units] = 0;
    return true;
}

bool sgp::NativeWideToUtf16LE(const wchar_t* source, std::size_t maxWireUnits,
                              std::vector<unsigned char>& output) noexcept
{
    if (!source || !maxWireUnits || maxWireUnits > static_cast<std::size_t>(INT_MAX) / 4) return false;
    try {
        const int size = NativeWideToUtf8(source, -1, nullptr, 0);
        if (size <= 0 || static_cast<std::size_t>(size) > maxWireUnits * 4) return false;
        std::vector<char> text(size);
        if (!NativeWideToUtf8(source, -1, text.data(), text.size())) return false;
        std::vector<unsigned short> units;
        utf8::utf8to16(text.begin(), text.end(), std::back_inserter(units));
        if (units.size() > maxWireUnits) return false;
        std::vector<unsigned char> bytes;
        bytes.reserve(units.size() * 2);
        for (auto unit : units) {
            bytes.push_back(static_cast<unsigned char>(unit));
            bytes.push_back(static_cast<unsigned char>(unit >> 8));
        }
        output.swap(bytes);
        return true;
    } catch (...) { return false; }
}

bool sgp::Utf16LEToNativeWide(const unsigned char* source, std::size_t bytes,
                              std::size_t maxWireUnits, std::wstring& output) noexcept
{
    if (!source || bytes < 2 || bytes % 2 || bytes / 2 > maxWireUnits || bytes > INT_MAX) return false;
    try {
        std::vector<unsigned short> units;
        units.reserve(bytes / 2);
        for (std::size_t i = 0; i < bytes; i += 2)
            units.push_back(static_cast<unsigned short>(source[i] | (unsigned(source[i + 1]) << 8)));
        if (units.back() || std::find(units.begin(), units.end() - 1, 0) != units.end() - 1) return false;
        std::vector<char> text;
        utf8::utf16to8(units.begin(), units.end(), std::back_inserter(text));
        if (text.size() > static_cast<std::size_t>(INT_MAX)) return false;
        const int size = Utf8ToNativeWide(text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (size <= 0) return false;
        std::wstring wide(size, 0);
        if (!Utf8ToNativeWide(text.data(), static_cast<int>(text.size()), wide.data(), wide.size())) return false;
        wide.resize(size - 1);
        output.swap(wide);
        return true;
    } catch (...) { return false; }
}
