/*
 * P710 native snapshot update: tests/native-core/native_text_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_text.h"
#include "native_crt.h"
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
unsigned checks = 0;
void check(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
}
int main()
{
    try {
        const std::string input = "A\xc3\xa9\xe4\xb8\xad\xf0\x9f\x98\x80";
        const std::wstring expected = L"A\u00e9\u4e2d\U0001f600";
        const int need = static_cast<int>(expected.size() + 1);
        check(sgp::Utf8ToNativeWide(input.c_str(), -1, nullptr, 0) == need, "query native wide units");
        std::array<wchar_t, 12> wide; wide.fill(L'#');
        check(sgp::Utf8ToNativeWide(input.c_str(), -1, wide.data() + 1, need) == need, "ASCII BMP supplementary conversion");
        check(std::wstring(wide.data() + 1) == expected && wide[0] == L'#' && wide[need + 1] == L'#', "literal text and guards");
        const auto before = wide;
        check(!sgp::Utf8ToNativeWide(input.c_str(), -1, wide.data() + 1, need - 1) && wide == before, "insufficient capacity leaves output unchanged");
        check(!sgp::Utf8ToNativeWide("\xc0\xaf", -1, wide.data(), wide.size()) && wide == before, "overlong UTF8 rejected unchanged");
        check(!sgp::Utf8ToNativeWide("\xe2\x82", -1, wide.data(), wide.size()) && wide == before, "truncated UTF8 rejected unchanged");
        std::array<wchar_t, 24> label; label.fill(L'#');
        const char fullField[20] = {'1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0'};
        check(sgp::Utf8FieldToNativeWide(fullField, sizeof(fullField), label.data()+1, 22) &&
              std::wstring(label.data()+1)==L"12345678901234567890" && label[0]==L'#' && label[23]==L'#',
              "full-width unterminated XML label is bounded and terminated");
        const char cutField[] = {'o','k',char(0xe2),char(0x82)};
        check(!sgp::Utf8FieldToNativeWide(cutField, sizeof(cutField), label.data()+1, 22) && label[1]==0 && label[0]==L'#' && label[23]==L'#',
              "truncated UTF8 resource label has safe empty fallback");
        check(!sgp::Utf8FieldToNativeWide(fullField, sizeof(fullField), label.data()+1, 4) && label[1]==0,
              "resource label capacity failure is empty and terminated");
        const char emptyField[] = {'\0','x'};
        check(sgp::Utf8FieldToNativeWide(emptyField, sizeof(emptyField), label.data()+1, 22) && label[1]==0,
              "empty field remains valid empty text");
        check(!sgp::Utf8ToNativeWide(nullptr, -1, wide.data(), wide.size()) && !sgp::Utf8ToNativeWide("A", -2, wide.data(), wide.size()), "invalid input contract");
        check(!sgp::Utf8ToNativeWide("A", 0, wide.data(), wide.size()) && !sgp::Utf8ToNativeWide("A", -1, nullptr, 1), "invalid length and query");
        const char embedded[] = {'A', 0, 'B'};
        check(sgp::Utf8ToNativeWide(embedded, 3, wide.data(), 3) == 3 && wide[0] == L'A' && wide[1] == 0 && wide[2] == L'B', "explicit byte length preserves embedded NUL");
        const auto explicitBefore = wide;
        check(!sgp::Utf8ToNativeWide("\xed\xa0\x80", -1, wide.data(), wide.size()) && wide == explicitBefore, "UTF8 encoded surrogate rejected");
        check(sgp::NativeWideToUtf8(expected.c_str(), -1, nullptr, 0) == static_cast<int>(input.size() + 1), "UTF8 byte-count query");
        std::array<char, 32> utf8; utf8.fill('#');
        check(sgp::NativeWideToUtf8(expected.c_str(), -1, utf8.data() + 1, input.size() + 1) == static_cast<int>(input.size() + 1), "native wide to UTF8");
        check(std::string(utf8.data() + 1) == input && utf8[0] == '#' && utf8[input.size() + 2] == '#', "roundtrip bytes and guards");
        const auto utf8Before = utf8;
        check(!sgp::NativeWideToUtf8(expected.c_str(), -1, utf8.data(), 1) && utf8 == utf8Before, "narrow capacity failure unchanged");
        const wchar_t bad[] = {0xd800, 0};
        check(!sgp::NativeWideToUtf8(bad, -1, utf8.data(), utf8.size()) && utf8 == utf8Before, "native wide surrogate rejected");
        check(_stricmp("Data\\Name", "data\\name") == 0 && _stricmp("a", "b") < 0 && _stricmp("b", "a") > 0, "CRT comparison ordering");
        check(_strnicmp("ABCtail", "abcDIFF", 3) == 0 && _strnicmp("a", "B", 1) < 0, "bounded CRT comparison");
        check(_wcsicmp(L"Actor", L"ACTOR") == 0 && _wcsnicmp(L"Alpha", L"ALTERNATE", 2) == 0, "wide comparison");
        int evaluated=1;
        check(__min(++evaluated,3)==3 && evaluated==3, "legacy min retains selected-operand reevaluation");
        evaluated=0;
        check(__max(++evaluated,3)==3 && evaluated==1, "legacy max retains conditional evaluation");
        check(__min(short(-1),1u)==1u && __max(short(-1),1u)==~0u,
              "mixed-width min/max retain original arithmetic conversions");
        wchar_t tokens[] = L"one,two";
        check(std::wstring(wcstok(tokens, L",")) == L"one" &&
              std::wstring(wcstok(nullptr, L",")) == L"two" &&
              wcstok(nullptr, L",") == nullptr, "legacy wide tokenization state");
        std::cout << "PASS: " << checks << " native UTF8/resource-text and CRT assertions\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL after " << checks << ": " << error.what() << '\n'; return 1; }
}
