/*
 * P710 native snapshot update: sgp/NativeText.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
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
