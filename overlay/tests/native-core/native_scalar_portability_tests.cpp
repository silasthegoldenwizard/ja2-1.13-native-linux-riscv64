/*
 * P710 native snapshot update: tests/native-core/native_scalar_portability_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_crt.h"
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <utility>

template<class Value, class = void>
struct HasArrayCount : std::false_type {};
template<class Value>
struct HasArrayCount<Value, std::void_t<decltype(_countof(std::declval<Value>()))>> : std::true_type {};

static_assert(_countof("text") == 5);
static_assert(HasArrayCount<const int (&)[3]>::value);
static_assert(!HasArrayCount<int*>::value);
static_assert(!HasArrayCount<const int*>::value);
static_assert(std::is_same_v<decltype(__min(-1, std::uint32_t{1})), std::uint32_t>);
static_assert(__min(-1, std::uint32_t{1}) == 1);
static_assert(__max(-1, std::uint32_t{1}) == UINT32_MAX);

int main()
{
    const int matrix[2][3] = {};
    assert(_countof(matrix) == 2);
    assert(_countof(matrix[0]) == 3);
    wchar_t text[] = L"aBz 09";
    assert(_wcsupr(text) == text);
    assert(std::wcscmp(text, L"ABZ 09") == 0);
    wchar_t empty[] = L"";
    assert(_wcsupr(empty) == empty && empty[0] == L'\0');
    // The selected operand is evaluated twice, as in the original MSVC macro.
    int first = 1, second = 5;
    assert(__min(first++, second++) == 2);
    assert(first == 3 && second == 6);
}
