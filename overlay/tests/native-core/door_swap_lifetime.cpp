/*
 * P710 native snapshot update: tests/native-core/door_swap_lifetime.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/door_swap_lifetime.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Synthetic services around the exact extracted original swap function.
// Field widths match the selected declarations; layout/full engine ABI is not modeled.
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using INT8 = std::int8_t;
using INT16 = std::int16_t;
using INT32 = std::int32_t;
using UINT8 = std::uint8_t;
using UINT16 = std::uint16_t;
using UINT32 = std::uint32_t;
using BOOLEAN = bool;
constexpr BOOLEAN TRUE = true, FALSE = false;
constexpr INT8 NO_PARTNER_STRUCTURE = 0;
constexpr INT16 PROFILE_Z_SIZE = 4;
constexpr UINT32 STRUCTURE_DOOR = 0x00080000, STRUCTURE_ANYDOOR = 0x00CC0000;
constexpr UINT32 STRUCTURE_SWITCH = 0x08000000;
constexpr INT32 NOBODY = 255;
#define CHECKF(value) do { if (!(value)) return nullptr; } while (false)
struct DB_STRUCTURE { INT8 bPartnerDelta = 0; };
struct DB_STRUCTURE_REF { DB_STRUCTURE* pDBStructure = nullptr; };
struct STRUCTURE {
    DB_STRUCTURE_REF* pDBStructureRef = nullptr;
    UINT32 fFlags = 0;
    INT32 sGridNo = 0;
    UINT8 ubHitPoints = 0;
    INT16 sCubeOffset = 0;
};
struct LEVELNODE { UINT16 usIndex = 0; STRUCTURE* pStructureData = nullptr; };
namespace {
int assertions = 0, cases = 0;
void require(bool ok, const char* why) {
    ++assertions;
    if (!ok) throw std::runtime_error(why);
}
struct Context {
    DB_STRUCTURE db[3];
    DB_STRUCTURE_REF refs[3];
    std::unique_ptr<STRUCTURE> old, replacement;
    LEVELNODE node, shadow;
    INT32 grid = 0;
    INT8 delta = 1, level = 0;
    bool door = true, refresh = true, withShadow = true, deleted = false, added = false;
    bool failAdd = false;
    int conversions = 0, switchCalls = 0;
    std::vector<std::string> events;
};
Context* current = nullptr;
Context& state() { return *current; }
STRUCTURE* FindBaseStructure(STRUCTURE* input) {
    auto& s = state(); s.events.emplace_back("base");
    require(input == s.old.get(), "base lookup receives original object"); return input;
}
LEVELNODE* FindLevelNodeBasedOnStructure(INT32 grid, STRUCTURE* input) {
    auto& s = state(); s.events.emplace_back("node");
    require(grid == s.grid && input == s.old.get(), "node lookup uses old base identity/grid");
    return &s.node;
}
LEVELNODE* FindShadow(INT32 grid, UINT16 index) {
    auto& s = state(); s.events.emplace_back("shadow");
    require(grid == s.grid && index == s.node.usIndex, "shadow lookup precedes graphics change");
    return s.withShadow ? &s.shadow : nullptr;
}
void ConvertGridNoToXY(INT32 grid, INT16* x, INT16* y) {
    auto& s = state(); s.events.emplace_back("xy"); ++s.conversions;
    require(grid == s.grid, "conversion uses original saved grid");
    // A deterministic synthetic grid mapping; no real map/data is used.
    *x = static_cast<INT16>(grid % 160); *y = static_cast<INT16>(grid / 160);
}
BOOLEAN LightSpriteStateChangePre(INT16 x, INT16 y) {
    auto& s = state(); s.events.emplace_back("pre");
    require(s.door && s.old && !s.deleted && !s.added, "erase observes old live door");
    require(x == s.grid % 160 && y == s.grid / 160, "erase receives old grid coordinates");
    return s.refresh;
}
BOOLEAN DeleteStructureFromWorld(STRUCTURE* object) {
    auto& s = state(); s.events.emplace_back("delete");
    require(object == s.old.get() && !s.deleted && !s.added, "delete original exactly once");
    s.old.reset(); s.deleted = true; // Actual heap retirement, not a logical marker alone.
    return TRUE;
}
STRUCTURE* InternalAddStructureToWorld(INT32 grid, INT8 level, DB_STRUCTURE_REF* partner, LEVELNODE* node) {
    auto& s = state(); s.events.emplace_back("add");
    require(s.deleted && !s.old && !s.added, "add follows retirement");
    require(grid == s.grid && level == s.level, "add receives saved grid and cube-derived level");
    require(partner == &s.refs[1+s.delta] && node == &s.node, "add receives selected partner and retained node");
    if (s.failAdd) return nullptr;
    s.added = true; s.replacement->sGridNo = grid; s.replacement->pDBStructureRef = partner;
    s.node.pStructureData = s.replacement.get();
    return s.replacement.get();
}
BOOLEAN LightSpriteStateChangePost(INT16 x, INT16 y) {
    auto& s = state(); s.events.emplace_back("post");
    require(s.door && s.refresh && s.deleted && s.added && !s.old, "redraw follows replacement");
    require(s.node.pStructureData == s.replacement.get(), "redraw observes new node binding");
    require(x == s.grid % 160 && y == s.grid / 160, "redraw receives pre-delete coordinates");
    return TRUE;
}
void ApplyMapChangesToMapTempFile(BOOLEAN enabled) {
    state().events.emplace_back(enabled ? "store-on" : "store-off");
}
void RemoveStructFromMapTempFile(INT32 grid, UINT16 index) {
    state().events.emplace_back("remove-map");
    require(grid == state().grid && index == 100, "map removal uses old graphic index");
}
void AddStructToMapTempFile(INT32 grid, UINT16 index) {
    state().events.emplace_back("add-map");
    require(grid == state().grid && index == static_cast<UINT16>(100+state().delta), "map addition uses new graphic index");
}
void ActivateSwitchInGridNo(INT32, INT32) { ++state().switchCalls; }

#include "door_swap_function.inc"

void scenario(bool door, bool refresh, bool shadow, bool store, INT8 delta, bool failAdd = false) {
    Context s; current = &s;
    s.grid = 323 + cases*161; s.door = door; s.refresh = refresh;
    s.withShadow = shadow; s.delta = delta; s.level = static_cast<INT8>(cases%3);
    s.failAdd = failAdd;
    for (int i = 0; i < 3; ++i) s.refs[i].pDBStructure = &s.db[i];
    s.db[1].bPartnerDelta = delta;
    s.old = std::make_unique<STRUCTURE>();
    s.replacement = std::make_unique<STRUCTURE>();
    const auto oldAddress = reinterpret_cast<std::uintptr_t>(s.old.get());
    require(s.old.get() != s.replacement.get(), "replacement allocation must be distinct before retirement");
    s.old->pDBStructureRef = &s.refs[1]; s.old->fFlags = door ? STRUCTURE_DOOR : STRUCTURE_SWITCH;
    s.old->sGridNo = s.grid; s.old->sCubeOffset = static_cast<INT16>(s.level*PROFILE_Z_SIZE);
    const UINT8 hitPoints = static_cast<UINT8>(72+cases);
    s.old->ubHitPoints = hitPoints; s.replacement->ubHitPoints = 222;
    s.replacement->fFlags = STRUCTURE_SWITCH; // Existing disabled switch path must stay disabled.
    s.node.usIndex = 100; s.node.pStructureData = s.old.get(); s.shadow.usIndex = 200;
    STRUCTURE* result = InternalSwapStructureForPartner(-77, s.old.get(), TRUE, store);
    if (failAdd) {
        require(result == nullptr && s.deleted && !s.old && !s.added,
                "failed replacement returns null after retiring old storage");
        require(s.node.pStructureData == nullptr, "failed replacement leaves no dangling binding");
        require(s.node.usIndex == 100 && s.shadow.usIndex == 200,
                "failed replacement preserves graphic indexes");
        require(s.switchCalls == 0, "failed replacement does not activate switches");
        std::vector<std::string> expected{"base", "node", "shadow"};
        if (door) { expected.emplace_back("xy"); expected.emplace_back("pre"); }
        expected.emplace_back("delete"); expected.emplace_back("add");
        require(s.events == expected, "failed replacement stops before redraw or map storage");
        ++cases; current = nullptr;
        return;
    }
    require(result == s.replacement.get(), "return replacement object");
    require(reinterpret_cast<std::uintptr_t>(result) != oldAddress, "no old-address reuse masks stale read");
    require(s.deleted && !s.old && s.added, "successful real deletion and replacement");
    require(s.node.pStructureData == result, "level node binds replacement");
    require(result->ubHitPoints == hitPoints, "old hit points retained");
    require(result->sGridNo == s.grid && result->pDBStructureRef == &s.refs[1+delta], "replacement grid and partner retained");
    require(s.conversions == (door ? (refresh ? 2 : 1) : 0), "conditional light conversion count");
    require(s.node.usIndex == static_cast<UINT16>(door ? 100 : 100+delta), "door/non-door graphics behavior retained");
    require(s.shadow.usIndex == static_cast<UINT16>(!door && shadow ? 200+delta : 200), "optional shadow behavior retained");
    require(s.switchCalls == 0, "existing disabled switch activation retained");
    std::vector<std::string> expected{"base", "node", "shadow"};
    if (door) { expected.emplace_back("xy"); expected.emplace_back("pre"); }
    expected.emplace_back("delete"); expected.emplace_back("add");
    if (door && refresh) { expected.emplace_back("xy"); expected.emplace_back("post"); }
    if (!door && store) {
        expected.emplace_back("store-on"); expected.emplace_back("remove-map");
        expected.emplace_back("add-map"); expected.emplace_back("store-off");
    }
    require(s.events == expected, "complete callback order retained");
    ++cases; current = nullptr; // unique_ptr frees the replacement; old was freed by the callback.
}
}
int main() {
    try {
        // Lit first: the unchanged routine must fail at its stale grid read, not in a stub.
        for (int kind = 0; kind < 3; ++kind)
            for (bool shadow : {false, true})
                for (bool store : {false, true})
                    for (INT8 delta : {INT8{1}, INT8{-1}})
                        scenario(kind != 2, kind == 0, shadow, store, delta);
        require(cases == 24, "all successful swap combinations ran");
        for (int kind = 0; kind < 3; ++kind)
            for (bool store : {false, true})
                for (INT8 delta : {INT8{1}, INT8{-1}})
                    scenario(kind != 2, kind == 0, true, store, delta, true);
        require(cases == 36, "all successful and failed swap combinations ran");
        std::cout << "PASS " << assertions << " door swap checks; " << cases << " scenarios\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}
