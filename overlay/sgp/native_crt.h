/*
 * P710 native snapshot update: sgp/native_crt.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Small native equivalents of the CRT facilities used by the original engine.
// These supply no Win32 window, handle, file, or other operating-system API.
#ifndef SGP_NATIVE_CRT_H
#define SGP_NATIVE_CRT_H
#if !defined(_WIN32)
#include <cstddef>
#include <cwchar>
#include <cwctype>
#include <strings.h>
template <class Element, std::size_t Size>
constexpr std::size_t _countof(const Element (&)[Size]) noexcept { return Size; }
inline wchar_t* _wcsupr(wchar_t* text)
{
    for (wchar_t* current = text; *current; ++current) *current = std::towupper(*current);
    return text;
}
inline int _stricmp(const char* first, const char* second) { return ::strcasecmp(first, second); }
inline int _strnicmp(const char* first, const char* second, std::size_t count) { return ::strncasecmp(first, second, count); }
inline int _wcsicmp(const wchar_t* first, const wchar_t* second) { return ::wcscasecmp(first, second); }
inline int _wcsnicmp(const wchar_t* first, const wchar_t* second, std::size_t count) { return ::wcsncasecmp(first, second, count); }
inline wchar_t* wcstok(wchar_t* text, const wchar_t* delimiters)
{
    thread_local wchar_t* state = nullptr;
    return std::wcstok(text, delimiters, &state);
}
// Preserve the original MSVC macros' usual arithmetic conversions and
// evaluation behavior; std::min would reject many mixed-width legacy calls.
#ifndef __min
#define __min(first, second) (((first) < (second)) ? (first) : (second))
#endif
#ifndef __max
#define __max(first, second) (((first) > (second)) ? (first) : (second))
#endif
#endif
#endif
