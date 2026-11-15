/*
 * P710 native snapshot update: sgp/native_format.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_FORMAT_H
#define SGP_NATIVE_FORMAT_H

#ifndef _WIN32
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cwchar>

namespace ja2::native
{
// Original MSVC printf string semantics: wide %s/%c are wide, %S/%C
// are narrow; an explicit h/l modifier selects narrow/wide respectively.
// Capacities include the terminator. Success returns the character count;
// failure returns -1, sets errno and empties a valid destination. No truncation
// is accepted. Format validation occurs before the va_list is inspected.
int VFormatWide(wchar_t* destination, std::size_t capacity,
                const wchar_t* format, va_list arguments) noexcept;
int FormatWide(wchar_t* destination, std::size_t capacity,
               const wchar_t* format, ...) noexcept;
int VFormatNarrow(char* destination, std::size_t capacity,
                  const char* format, va_list arguments) noexcept;
int FormatNarrow(char* destination, std::size_t capacity,
                 const char* format, ...) noexcept;
}

// Only the original MSVC array form is adapted. The ISO explicit-capacity
// swprintf remains untouched, including its ISO %s/%ls contract. A raw pointer
// must use FormatWide with its actual allocation capacity at the call site.
template<std::size_t N, typename... Arguments>
int swprintf(wchar_t (&destination)[N], const wchar_t* format, Arguments... arguments) noexcept
{
    return ja2::native::FormatWide(destination, N, format, arguments...);
}

template<std::size_t N>
int vswprintf(wchar_t (&destination)[N], const wchar_t* format, va_list arguments) noexcept
{
    return ja2::native::VFormatWide(destination, N, format, arguments);
}

inline int sprintf_s(char* destination, std::size_t capacity, const char* format, ...) noexcept
{
    va_list arguments;
    va_start(arguments, format);
    const int result = ja2::native::VFormatNarrow(destination, capacity, format, arguments);
    va_end(arguments);
    return result;
}

template<std::size_t N, typename... Arguments>
int sprintf_s(char (&destination)[N], const char* format, Arguments... arguments) noexcept
{
    return ja2::native::FormatNarrow(destination, N, format, arguments...);
}
#endif
#endif
