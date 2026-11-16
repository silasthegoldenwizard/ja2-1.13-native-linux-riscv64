/*
 * P710 native snapshot update: Tactical/native_map_soldiercreate.h
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_MAP_SOLDIERCREATE_H
#define P710_NATIVE_MAP_SOLDIERCREATE_H

#include "Soldier Create.h"
#include "native_profile101.h"
#include <array>
#include <cstddef>
#include <cstring>

namespace p710::mapSoldierCreate {
// Static placements in the Windows map formats. Native saves use a separate
// HWFILE overload and retain their existing native layout.
inline constexpr std::size_t v6Size = 239;
inline constexpr std::size_t v7Size = 263;
static_assert(sizeof(void*) == 8 && sizeof(CHAR16) == 4);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, sInsertionGridNo) == 12);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, HeadPal) == 32);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, sPatrolGrid) == 182);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, name) == 204);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, ubSoldierClass) == 244);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, pExistingSoldier) == 248);
static_assert(offsetof(_OLD_SOLDIERCREATE_STRUCT, fUseExistingSoldier) == 256);
static_assert(_OLD_SIZEOF_SOLDIERCREATE_STRUCT_POD == 263);
static_assert(offsetof(SOLDIERCREATE_STRUCT, sInsertionGridNo) == 12);
static_assert(offsetof(SOLDIERCREATE_STRUCT, HeadPal) == 34);
static_assert(offsetof(SOLDIERCREATE_STRUCT, sPatrolGrid) == 184);
static_assert(offsetof(SOLDIERCREATE_STRUCT, bPatrolCnt) == 224);
static_assert(offsetof(SOLDIERCREATE_STRUCT, fVisible) == 225);
static_assert(offsetof(SOLDIERCREATE_STRUCT, name) == 228);
static_assert(offsetof(SOLDIERCREATE_STRUCT, ubSoldierClass) == 268);
static_assert(offsetof(SOLDIERCREATE_STRUCT, pExistingSoldier) == 272);
static_assert(offsetof(SOLDIERCREATE_STRUCT, fUseExistingSoldier) == 280);
static_assert(SIZEOF_SOLDIERCREATE_STRUCT_POD == 287);

inline bool decode(const std::array<UINT8, v6Size>& wire,
                   _OLD_SOLDIERCREATE_STRUCT& output) {
    std::array<CHAR16, 10> name{};
    if (!profile101::name(wire.data() + 204, name)) return false;
    std::memcpy(&output, wire.data(), 204);
    std::memcpy(output.name, name.data(), sizeof(output.name));
    std::memcpy(&output.ubSoldierClass, wire.data() + 224, 3);
    std::memcpy(&output.fUseExistingSoldier, wire.data() + 232, 7);
    output.pExistingSoldier = nullptr;
    output.fUseExistingSoldier = FALSE;
    return true;
}

inline bool decode(const std::array<UINT8, v7Size>& wire,
                   SOLDIERCREATE_STRUCT& output) {
    std::array<CHAR16, 10> name{};
    if (!profile101::name(wire.data() + 226, name)) return false;
    std::memcpy(&output, wire.data(), 226);
    std::memcpy(output.name, name.data(), sizeof(output.name));
    std::memcpy(&output.ubSoldierClass, wire.data() + 246, 3);
    std::memcpy(&output.fUseExistingSoldier, wire.data() + 256, 7);
    // Static map placement creates a fresh soldier. Never restore a process
    // pointer or an active-use flag from the authoring Windows process.
    output.pExistingSoldier = nullptr;
    output.fUseExistingSoldier = FALSE;
    return true;
}
} // namespace p710::mapSoldierCreate
#endif
