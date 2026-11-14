/*
 * P710 native snapshot update: Utils/native_text_output.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef JA2_NATIVE_TEXT_OUTPUT_H
#define JA2_NATIVE_TEXT_OUTPUT_H

// Windows keeps its original calls. Native calls always supply a capacity in
// wchar_t elements, including the terminator, to the existing MSVC formatter.
#ifdef _WIN32
#define P710_WIDE_FORMAT(destination, capacity, ...) swprintf(destination, __VA_ARGS__)
#define P710_WIDE_LITERAL_FORMAT(destination, capacity, source) swprintf(destination, source)
#define P710_WIDE_LITERAL_COPY(destination, capacity, source) wcscpy(destination, source)
#define P710_TEXT_OUTPUT(destination, capacity)
#define P710_TEXT_FORMAT(destination, ...) swprintf(destination, __VA_ARGS__)
#define P710_TEXT_FORMAT_PART(destination, ...) swprintf(destination, __VA_ARGS__)
#define P710_TEXT_APPEND_PART(destination, source) wcscat(destination, source)
#define P710_TEXT_COPY(destination, source) wcscpy(destination, source)
#define P710_TEXT_APPEND(destination, source) wcscat(destination, source)
#define P710_TEXT_APPEND_FORMAT(destination, ...) swprintf((destination) + wcslen(destination), __VA_ARGS__)
#else
#include "native_format.h"
#include <cerrno>
#include <cwchar>

namespace ja2::native
{
// A compound UI string is all-or-empty: once any component fails, later
// components cannot turn it into a misleading suffix. No capacity is inferred.
class WideTextOutput
{
public:
    WideTextOutput(wchar_t* destination, std::size_t capacity) noexcept
        : destination_(destination), capacity_(capacity) {}

    template<typename... Arguments>
    int Format(const wchar_t* format, Arguments... arguments) noexcept
    {
        if (failed_) return -1;
        const int result = FormatWide(destination_, capacity_, format, arguments...);
        if (result < 0) failed_ = true;
        return result;
    }

    // Intermediate formatting belongs to this compound output transaction.
    // Retain the real intermediate array extent; an error also invalidates the
    // final destination, and subsequent parts are emptied without formatting.
    template<std::size_t N, typename... Arguments>
    int FormatPart(wchar_t (&part)[N], const wchar_t* format, Arguments... arguments) noexcept
    {
        if (failed_) { part[0] = L'\0'; return -1; }
        const int result = FormatWide(part, N, format, arguments...);
        if (result < 0) Fail(errno);
        return result;
    }

    template<std::size_t N>
    bool AppendPart(wchar_t (&part)[N], const wchar_t* source) noexcept
    {
        if (failed_) { part[0] = L'\0'; return false; }
        WideTextOutput component(part, N);
        if (!component.Append(source)) return Fail(errno);
        return true;
    }

    template<typename... Arguments>
    int AppendFormat(const wchar_t* format, Arguments... arguments) noexcept
    {
        if (failed_) return -1;
        if (!destination_ || !capacity_) { Fail(EINVAL); return -1; }
        std::size_t used = 0;
        while (used < capacity_ && destination_[used]) ++used;
        if (used == capacity_) { Fail(EINVAL); return -1; }
        const int result = FormatWide(destination_ + used, capacity_ - used, format, arguments...);
        if (result < 0) { Fail(errno); return -1; }
        return result;
    }

    bool Copy(const wchar_t* source) noexcept
    {
        if (failed_) return false;
        if (!destination_ || !capacity_ || !source) return Fail(EINVAL);
        const std::size_t length = std::wcslen(source);
        if (length >= capacity_) return Fail(ERANGE);
        std::wmemmove(destination_, source, length + 1);
        return true;
    }

    bool Append(const wchar_t* source) noexcept
    {
        if (failed_) return false;
        if (!destination_ || !capacity_ || !source) return Fail(EINVAL);
        std::size_t used = 0;
        while (used < capacity_ && destination_[used]) ++used;
        if (used == capacity_) return Fail(EINVAL);
        const std::size_t length = std::wcslen(source);
        if (length >= capacity_ - used) return Fail(ERANGE);
        std::wmemmove(destination_ + used, source, length + 1);
        return true;
    }

private:
    bool Fail(int error) noexcept
    {
        failed_ = true;
        if (destination_ && capacity_) destination_[0] = L'\0';
        errno = error;
        return false;
    }
    wchar_t* destination_;
    std::size_t capacity_;
    bool failed_ = false;
};
}

#define P710_WIDE_FORMAT(destination, capacity, ...) ja2::native::FormatWide(destination, capacity, __VA_ARGS__)
#define P710_WIDE_LITERAL_FORMAT(destination, capacity, source) ja2::native::FormatWide(destination, capacity, L"%s", source)
#define P710_WIDE_LITERAL_COPY(destination, capacity, source) ja2::native::FormatWide(destination, capacity, L"%s", source)
#define P710_TEXT_OUTPUT(destination, capacity) ja2::native::WideTextOutput p710TextOutput(destination, capacity)
#define P710_TEXT_FORMAT(destination, ...) p710TextOutput.Format(__VA_ARGS__)
#define P710_TEXT_FORMAT_PART(destination, ...) p710TextOutput.FormatPart(destination, __VA_ARGS__)
#define P710_TEXT_APPEND_PART(destination, source) p710TextOutput.AppendPart(destination, source)
#define P710_TEXT_COPY(destination, source) p710TextOutput.Copy(source)
#define P710_TEXT_APPEND(destination, source) p710TextOutput.Append(source)
#define P710_TEXT_APPEND_FORMAT(destination, ...) p710TextOutput.AppendFormat(__VA_ARGS__)
#endif
#endif
