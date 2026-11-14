/*
 * P710 native snapshot update: tests/native-core/native_format_caller_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Source-only caller-buffer contracts; no engine stubs or game assets.
#include "types.h"
#include "native_text_output.h"
#include "strategicmap.h"
#include "Town Militia.h"
#include "Campaign.h"
#include "personnel.h"
#include "BobbyRGuns.h"
#include "SaveLoadGame.h"
#include "SaveLoadScreen.h"
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>

using SectorWriter = void (*)(INT16, INT16, INT8, STR16, BOOLEAN, std::size_t);
using SaveWriter = BOOLEAN (*)(int, STR16, std::size_t);
static_assert(std::is_same_v<decltype(static_cast<SectorWriter>(&GetSectorIDString)), SectorWriter>);
static_assert(std::is_same_v<decltype(static_cast<SaveWriter>(&SaveGame)), SaveWriter>);
static_assert(std::is_same_v<decltype(static_cast<SaveWriter>(&DoAutoSave)), SaveWriter>);
static_assert(std::is_same_v<decltype(GetShortSectorString(1, 1, std::declval<CHAR16(&)[4]>())), void>);
static_assert(std::is_same_v<decltype(GetHelpTextForItemInLaptop(std::declval<CHAR16(&)[5000]>(), 1)), void>);
static_assert(std::is_same_v<decltype(BuildMilitiaPromotionsString(std::declval<CHAR16(&)[512]>())), void>);
static_assert(std::is_same_v<decltype(BuildStatChangeString(std::declval<CHAR16(&)[256]>(), std::declval<STR16>(), TRUE, 1, 1)), void>);
static_assert(std::is_same_v<decltype(AssignPersonnelSkillTraitHelpText(1, FALSE, FALSE, std::declval<CHAR16(&)[5000]>())), void>);

namespace {
unsigned checks = 0;
void check(bool value, const char* reason)
{
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
}
int main()
{
    try {
        struct Guarded { wchar_t before = L'B'; wchar_t text[8] = {}; wchar_t after = L'A'; } guarded;
        ja2::native::WideTextOutput output(guarded.text, 8);
        check(output.Format(L"%s", L"50%") == 3, "wide MSVC percent-containing literal value");
        check(output.AppendFormat(L" %S", "ok") == 3, "append uses Windows narrow uppercase S");
        check(std::wstring(guarded.text) == L"50% ok", "preserved component ordering");
        check(!output.Append(L"!!") && errno == ERANGE, "one-past capacity rejected");
        check(guarded.text[0] == 0, "compound failure empties whole output");
        check(!output.Append(L"x") && output.Format(L"x") == -1, "failure cannot become a misleading suffix");
        check(guarded.before == L'B' && guarded.after == L'A', "caller allocation guard elements preserved");

        wchar_t exact[5] = {};
        ja2::native::WideTextOutput exactOutput(exact, 5);
        check(exactOutput.Copy(L"ab") && exactOutput.Append(L"cd"), "exact-fit copy and append");
        check(std::wstring(exact) == L"abcd", "exact-fit terminator retained");
        ja2::native::WideTextOutput overlap(exact, 5);
        check(overlap.Copy(exact + 1) && std::wstring(exact) == L"bcd", "overlapping copy bounded with memmove");

        wchar_t appendFit[5] = L"A";
        ja2::native::WideTextOutput appendOutput(appendFit, 5);
        check(appendOutput.AppendFormat(L"%d", 123) == 3 && std::wstring(appendFit) == L"A123", "append formatter receives true remaining capacity");
        check(appendOutput.AppendFormat(L"%d", 4) == -1 && appendFit[0] == 0, "append overflow empties base allocation");

        // Intermediate arrays are part of the same compound transaction.
        struct PartGuard { wchar_t before = L'P'; wchar_t text[4] = {}; wchar_t after = L'Q'; } part;
        struct FinalGuard { wchar_t before = L'F'; wchar_t text[32] = {}; wchar_t after = L'G'; } compound;
        ja2::native::WideTextOutput partOutput(compound.text, 32);
        check(partOutput.Copy(L"prefix"), "compound prefix before intermediate failure");
        check(partOutput.FormatPart(part.text, L"%s", L"four") == -1 && errno == ERANGE,
              "intermediate array true capacity rejects overflow");
        check(part.text[0] == 0 && compound.text[0] == 0, "intermediate overflow empties part and final output");
        part.text[0] = L'X';
        check(partOutput.FormatPart(part.text, L"%s", L"ok") == -1 && part.text[0] == 0,
              "later valid intermediate cannot escape sticky failure");
        check(!partOutput.Append(part.text) && !partOutput.Append(L"suffix") && compound.text[0] == 0,
              "later intermediate and direct suffix stay suppressed");
        check(part.before == L'P' && part.after == L'Q' && compound.before == L'F' && compound.after == L'G',
              "intermediate overflow preserves both allocation guards");

        ja2::native::WideTextOutput invalidPartOutput(compound.text, 32);
        check(invalidPartOutput.Copy(L"prefix"), "prefix before invalid intermediate format");
        int untouched = 17;
        check(invalidPartOutput.FormatPart(part.text, L"%n", &untouched) == -1 && untouched == 17,
              "invalid intermediate format rejected before pointer use");
        check(part.text[0] == 0 && compound.text[0] == 0 && !invalidPartOutput.Append(L"suffix"),
              "invalid intermediate format invalidates entire compound");
        ja2::native::WideTextOutput appendedPartOutput(compound.text, 32);
        check(appendedPartOutput.FormatPart(part.text, L"%s", L"abc") == 3,
              "exact-fit intermediate formatting succeeds");
        check(!appendedPartOutput.AppendPart(part.text, L"d") && errno == ERANGE,
              "intermediate subcomposition uses actual remaining capacity");
        check(part.text[0] == 0 && compound.text[0] == 0 && !appendedPartOutput.Append(L"suffix"),
              "intermediate append failure remains sticky at final output");

        // Same helper sequence as the ammunition help final-format/suffix path.
        struct TinyGuard { wchar_t before = L'L'; wchar_t text[1] = {}; wchar_t after = L'R'; } tiny;
        ja2::native::WideTextOutput tinyOutput(tiny.text, 1);
        check(tinyOutput.Format(L"%s", L"name") == -1 && !tinyOutput.Append(L"suffix"),
              "failed initial tiny destination cannot receive ammunition suffix");
        check(tiny.text[0] == 0 && tiny.before == L'L' && tiny.after == L'R',
              "tiny final destination guards survive later suffix");
        wchar_t full[5] = L"full";
        ja2::native::WideTextOutput fullOutput(full, 5);
        check(fullOutput.FormatPart(part.text, L"%s", L"!") == 1 && !fullOutput.Append(part.text) && full[0] == 0,
              "full final destination rejects formatted suffix");
        wchar_t literal[8] = {};
        check(P710_WIDE_LITERAL_FORMAT(literal, 8, L"100%%") == 5 && std::wstring(literal) == L"100%%",
              "native literal path preserves supplied escaped percent text literally");

        wchar_t one[1] = { L'X' };
        check(ja2::native::FormatWide(one, 1, L"%s", L"") == 0 && one[0] == 0, "empty text fits one element");
        check(ja2::native::FormatWide(one, 1, L"%s", L"%n") == -1 && one[0] == 0, "literal percent text cannot be executed as format");
        ja2::native::WideTextOutput zero(one, 0);
        one[0] = L'X';
        check(!zero.Copy(L"") && one[0] == L'X', "zero capacity writes no element");
        wchar_t unterminated[2] = { L'a', L'b' };
        ja2::native::WideTextOutput invalid(unterminated, 2);
        check(!invalid.Append(L"c") && unterminated[0] == 0, "bounded terminator search rejects invalid destination");
        std::cout << checks << " caller capacity checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
