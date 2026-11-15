/*
 * P710 native snapshot update: sgp/NativeFormat.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_format.h"

#ifndef _WIN32
#include <cerrno>
#include <climits>
#include <new>
#include <string>

namespace
{
template<typename C> bool digit(C c) { return c >= C('0') && c <= C('9'); }
template<typename C> bool contains(const char* set, C c)
{
    for (; *set; ++set) if (c == C(*set)) return true;
    return false;
}

// The host CRT performs conversion, width/precision, argument traversal and
// encoding. This pass changes only the original string/character convention
// and rejects unsupported conversions before they can consume wrong arguments.
// VFS offers Unicode string conversion, but no printf compatibility helper.
template<typename C>
bool hostFormat(const C* input, bool wide, std::basic_string<C>& output)
{
    for (const C* p = input; *p;)
    {
        if (*p != C('%')) { output += *p++; continue; }
        output += *p++;
        if (*p == C('%')) { output += *p++; continue; }
        while (*p && contains("-+ #0", *p)) output += *p++;
        if (*p == C('*')) output += *p++;
        else while (digit(*p)) output += *p++;
        if (*p == C('.'))
        {
            output += *p++;
            if (*p == C('*')) output += *p++;
            else while (digit(*p)) output += *p++;
        }
        std::basic_string<C> length;
        if (*p && contains("hljztL", *p))
        {
            const C first = *p++;
            length += first;
            if ((first == C('h') || first == C('l')) && *p == first) length += *p++;
        }
        const C conversion = *p;
        if (!conversion) return false;
        ++p;
        if (contains("sScC", conversion))
        {
            if (!length.empty() && length != std::basic_string<C>(1, C('h')) &&
                length != std::basic_string<C>(1, C('l'))) return false;
            const bool uppercase = conversion == C('S') || conversion == C('C');
            const bool wideArgument = !length.empty() ? length[0] == C('l') : wide != uppercase;
            if (wideArgument) output += C('l');
            output += contains("sS", conversion) ? C('s') : C('c');
        }
        else if (contains("diouxX", conversion))
        {
            if (length == std::basic_string<C>(1, C('L'))) return false;
            output += length; output += conversion;
        }
        else if (contains("aAeEfFgG", conversion))
        {
            if (!length.empty() && length != std::basic_string<C>(1, C('l')) &&
                length != std::basic_string<C>(1, C('L'))) return false;
            output += length; output += conversion;
        }
        else if (conversion == C('p') && length.empty()) output += conversion;
        else return false; // Includes %n, positional arguments, I64 and trailing %.
    }
    return true;
}

template<typename C> int fail(C* destination, std::size_t capacity, int error)
{
    if (destination && capacity) destination[0] = C(0);
    errno = error;
    return -1;
}

template<typename C, typename Formatter>
int render(C* destination, std::size_t capacity, const C* format,
           va_list arguments, bool wide, Formatter formatter) noexcept
{
    if (!destination || !capacity || !format || capacity > static_cast<std::size_t>(INT_MAX))
        return fail(destination, capacity, EINVAL);
    try
    {
        std::basic_string<C> translated;
        if (!hostFormat(format, wide, translated)) return fail(destination, capacity, EINVAL);
        va_list copy;
        va_copy(copy, arguments);
        errno = 0;
        const int count = formatter(destination, capacity, translated.c_str(), copy);
        const int error = errno;
        va_end(copy);
        if (count < 0) return fail(destination, capacity, error ? error : ERANGE);
        if (static_cast<std::size_t>(count) >= capacity) return fail(destination, capacity, ERANGE);
        return count;
    }
    catch (const std::bad_alloc&) { return fail(destination, capacity, ENOMEM); }
    catch (...) { return fail(destination, capacity, EINVAL); }
}
}

int ja2::native::VFormatWide(wchar_t* destination, std::size_t capacity,
                            const wchar_t* format, va_list arguments) noexcept
{
    return render(destination, capacity, format, arguments, true,
                  [](wchar_t* d, std::size_t n, const wchar_t* f, va_list a) { return std::vswprintf(d, n, f, a); });
}
int ja2::native::FormatWide(wchar_t* destination, std::size_t capacity, const wchar_t* format, ...) noexcept
{
    va_list arguments; va_start(arguments, format);
    const int result = VFormatWide(destination, capacity, format, arguments);
    va_end(arguments); return result;
}
int ja2::native::VFormatNarrow(char* destination, std::size_t capacity,
                              const char* format, va_list arguments) noexcept
{
    return render(destination, capacity, format, arguments, false,
                  [](char* d, std::size_t n, const char* f, va_list a) { return std::vsnprintf(d, n, f, a); });
}
int ja2::native::FormatNarrow(char* destination, std::size_t capacity, const char* format, ...) noexcept
{
    va_list arguments; va_start(arguments, format);
    const int result = VFormatNarrow(destination, capacity, format, arguments);
    va_end(arguments); return result;
}
#endif
