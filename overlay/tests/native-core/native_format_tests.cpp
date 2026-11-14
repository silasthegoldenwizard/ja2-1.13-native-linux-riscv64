/*
 * P710 native snapshot update: tests/native-core/native_format_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Source-only tests of the original MSVC format contract; no game assets.
#include "native_format.h"
#include "core_types.h"
#include <cerrno>
#include <clocale>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
unsigned checks = 0;
void check(bool condition, const char* reason)
{
    ++checks;
    if (!condition) throw std::runtime_error(reason);
}
void repeatedWide(const wchar_t* format, ...)
{
    wchar_t first[128], second[128];
    va_list arguments; va_start(arguments, format);
    const int a = vswprintf(first, format, arguments);
    const int b = ja2::native::VFormatWide(second, 128, format, arguments);
    va_end(arguments);
    check(a == b && a > 0 && std::wcscmp(first, second) == 0,
          "VFormatWide copies rather than consumes the caller va_list");
    check(std::wstring(first) == L"text 1234567890123 2.5 23",
          "mixed pointer, integer and floating argument ABI");
}
void repeatedNarrow(const char* format, ...)
{
    char first[128], second[128];
    va_list arguments; va_start(arguments, format);
    const int a = ja2::native::VFormatNarrow(first, 128, format, arguments);
    const int b = ja2::native::VFormatNarrow(second, 128, format, arguments);
    va_end(arguments);
    check(a == b && a > 0 && std::strcmp(first, second) == 0,
          "VFormatNarrow copies rather than consumes the caller va_list");
    check(std::string(first) == "wide 7 1.25", "narrow mixed argument ABI");
}
void strings()
{
    wchar_t output[128];
    check(swprintf(output, L"%s: %s %S %s", L"Version", L"JA2", "1.13", L"native") == 24 &&
          std::wstring(output) == L"Version: JA2 1.13 native", "original main-menu Windows string convention");
    check(swprintf(output, L"%s/%S/%ls/%hs", L"\u03a9\u4e2d", "ASCII", L"wide", "narrow") == 20 &&
          std::wstring(output) == L"\u03a9\u4e2d/ASCII/wide/narrow", "BMP wide text and explicit string modifiers");
    check(swprintf(output, L"%lS:%hS:%lc:%hc:%c:%C", L"wide", "narrow", L'\u03a9', 'A', L'\u4e2d', 'B') == 19 &&
          std::wstring(output) == L"wide:narrow:\u03a9:A:\u4e2d:B", "character case and length select argument width");
    // The three-argument ISO signature must keep ISO narrow %s semantics.
    check(swprintf(output, 128, L"%s/%ls", "narrow", L"\u03a9") == 8 &&
          std::wstring(output) == L"narrow/\u03a9", "ISO explicit-capacity overload is preserved");
    char narrow[128];
    check(sprintf_s(narrow, "%s/%S/%hs/%ls", "a", L"wide", "b", L"W") == 10 &&
          std::string(narrow) == "a/wide/b/W", "narrow secure-array string convention");
    check(sprintf_s(narrow, sizeof narrow, "%s%s%d%s", "Data/", "Inventory", 3, ".xml") == 19 &&
          std::string(narrow) == "Data/Inventory3.xml", "original initialization explicit-capacity signature");
    check(sprintf_s(narrow, "%s\\%s", "ShadeTables", "tiles") == 17 &&
          std::string(narrow) == "ShadeTables\\tiles", "original shade-table array signature");
    check(sprintf_s(narrow, "%c/%C/%hc/%lc", 'A', L'B', 'C', L'D') == 7 &&
          std::string(narrow) == "A/B/C/D", "narrow character argument widths");
}
void fieldsAndArguments()
{
    wchar_t output[128];
    const int result = swprintf(output, L"[%*.*s][%-6.3S] %04d %#x %.2f %%", 6, 2, L"\u03a9\u4e2dQ", "abcdef", 7, 42u, 1.25);
    const std::wstring expected = L"[    \u03a9\u4e2d][abc   ] 0007 0x2a 1.25 %";
    check(result == static_cast<int>(expected.size()) && output == expected,
          "star width and precision preserve argument order and percent escapes");
    check(swprintf(output, L"%*.*s", -5, -1, L"abc") == 5 && std::wstring(output) == L"abc  ",
          "negative star width and precision follow printf rules");
    const signed char small = -7;
    const short medium = -300;
    const long large = -1234567L;
    const long long largest = -1234567890123LL;
    const std::size_t size = 123;
    const std::ptrdiff_t difference = -12;
    const std::intmax_t maximum = -987654321;
    check(swprintf(output, L"%hhd %hd %ld %lld %zu %td %jd", small, medium, large, largest, size, difference, maximum) > 0 &&
          std::wstring(output) == L"-7 -300 -1234567 -1234567890123 123 -12 -987654321",
          "host integer lengths retain their actual promoted argument types");
    repeatedWide(L"%s %lld %.1f %u", L"text", 1234567890123LL, 2.5, 23u);
    repeatedNarrow("%S %u %.2f", L"wide", 7u, 1.25);
    check(swprintf(output, L"%.1Lf", 1.25L) == 3 && std::wstring(output) == L"1.2",
          "long-double argument retains host ABI");
    char narrow[128];
    check(sprintf_s(narrow, "[%*.*S] %% %u", 5, 2, L"wide", 9u) == 11 &&
          std::string(narrow) == "[   wi] % 9", "narrow star precision and percent escapes");
    wchar_t iso[128];
    void* address = output;
    const int isoCount = std::swprintf(iso, 128, L"%p", address);
    check(swprintf(output, L"%p", address) == isoCount && std::wcscmp(output, iso) == 0,
          "pointer argument representation is delegated to host CRT");
    const INT32 signedValue = std::numeric_limits<INT32>::min();
    const UINT32 unsignedValue = std::numeric_limits<UINT32>::max();
    check(swprintf(output, L"%d %u", signedValue, unsignedValue) == 22 &&
          std::wstring(output) == L"-2147483648 4294967295",
          "original fixed-width engine arguments use matching signed and unsigned conversions");
    struct ScanGuard { INT32 before = 17; INT32 value = 0; INT32 after = 23; } scanned;
    check(std::swscanf(L"-2147483648", L"%d", &scanned.value) == 1 && scanned.value == signedValue &&
          scanned.before == 17 && scanned.after == 23,
          "quest-debug fixed-width scanf writes only its INT32 destination");
}
void boundsAndErrors()
{
    struct WideGuard { wchar_t before = L'X'; wchar_t text[5] = L"old"; wchar_t after = L'Y'; } wide;
    check(swprintf(wide.text, L"%s", L"1234") == 4 && std::wstring(wide.text) == L"1234",
          "exact-fit wide string includes terminator");
    errno = 0;
    check(swprintf(wide.text, L"%s", L"12345") == -1 && errno == ERANGE && wide.text[0] == 0,
          "wide overflow fails with empty terminated result");
    check(wide.before == L'X' && wide.after == L'Y', "wide destination guards remain intact");
    struct NarrowGuard { char before = 'X'; char text[5] = "old"; char after = 'Y'; } narrow;
    check(sprintf_s(narrow.text, "%s", "1234") == 4 && std::string(narrow.text) == "1234",
          "exact-fit narrow string includes terminator");
    errno = 0;
    check(sprintf_s(narrow.text, "%s", "12345") == -1 && errno == ERANGE && narrow.text[0] == 0,
          "narrow overflow fails with empty terminated result");
    check(narrow.before == 'X' && narrow.after == 'Y', "narrow destination guards remain intact");
    wchar_t one[1] = {L'X'};
    check(swprintf(one, L"") == 0 && one[0] == 0, "single-unit empty destination");
    check(swprintf(one, L"x") == -1 && one[0] == 0, "single-unit nonempty output fails safely");
    check(ja2::native::FormatWide(nullptr, 0, L"x") == -1 && errno == EINVAL, "null destination is invalid");
    wide.text[0] = L'X';
    check(ja2::native::FormatWide(wide.text, 0, L"x") == -1 && wide.text[0] == L'X', "zero capacity writes nothing");
    check(swprintf(wide.text, static_cast<const wchar_t*>(nullptr)) == -1 && errno == EINVAL && wide.text[0] == 0,
          "null format fails safely");
    for (const wchar_t* format : {L"%", L"%n", L"%lls", L"%Ld", L"%hs %q", L"%1$s", L"%*2$s", L"%I64d"})
    {
        // No arguments supplied: validation must reject the entire format
        // before even the valid-looking %hs prefix can read a va_arg.
        check(swprintf(wide.text, format) == -1 && errno == EINVAL && wide.text[0] == 0,
              "unsupported format rejected before consuming arguments");
    }
    int written = 99;
    check(sprintf_s(narrow.text, "%n", &written) == -1 && written == 99 && errno == EINVAL,
          "write-through percent-n is rejected");
    check(sprintf_s(narrow.text, static_cast<const char*>(nullptr)) == -1 && errno == EINVAL && narrow.text[0] == 0,
          "narrow null format fails safely");
    // The C locale cannot encode BMP in a narrow result or decode UTF-8.
    check(sprintf_s(narrow.text, "%S", L"\u03a9") == -1 && errno == EILSEQ && narrow.text[0] == 0,
          "narrow BMP encoding failure is explicit and terminated");
    check(swprintf(wide.text, L"%S", "\xce\xa9") == -1 && errno == EILSEQ && wide.text[0] == 0,
          "invalid narrow multibyte input fails explicitly in C locale");
}
}
int main()
{
    try
    {
        check(std::setlocale(LC_ALL, "C") != nullptr, "portable C locale");
        strings(); fieldsAndArguments(); boundsAndErrors();
        std::cout << "native formatting: " << checks << " checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "native formatting failure after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
