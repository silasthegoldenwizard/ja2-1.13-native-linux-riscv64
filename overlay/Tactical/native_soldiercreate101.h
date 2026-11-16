/*
 * P710 native snapshot update: Tactical/native_soldiercreate101.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: Tactical/native_soldiercreate101.h
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_SOLDIERCREATE101_H
#define P710_NATIVE_SOLDIERCREATE101_H
#include "Soldier Create.h"
#include "native_profile101.h" // Reuse the accepted bounded UTF16LE conversion.
#include <array>
#include <cstddef>
#include <cstring>

namespace p710::soldiercreate101 {
inline constexpr std::size_t wireSize=1040;
enum class Context { staticMapPlacement, savedState };
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__);
static_assert(sizeof(OLD_OBJECTTYPE_101)==36 && sizeof(Version101::OLD_OBJECTTYPE_101_UNION)==12);
static_assert(offsetof(OLD_OBJECTTYPE_101,ugYucky)==4 && offsetof(OLD_OBJECTTYPE_101,usAttachItem)==16);
static_assert(offsetof(OLD_OBJECTTYPE_101,bAttachStatus)==24 && offsetof(OLD_OBJECTTYPE_101,fUsed)==33);
static_assert(OldInventory::NUM_INV_SLOTS==19 && OLD_MAX_OBJECTS_PER_SLOT_101==8);
// Before name, the Windows layout is unchanged:32scalar/padding bytes,
//19old36byte objects,150palette bytes,20patrol bytes and2flags.
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,HeadPal)==716);
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,sPatrolGrid)==866);
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,name)==888);
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,ubSoldierClass)==928);
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,pExistingSoldier)==936 && sizeof(void*)==8);
static_assert(offsetof(OLD_SOLDIERCREATE_STRUCT_101,fUseExistingSoldier)==944);
static_assert(SIZEOF_OLD_SOLDIERCREATE_STRUCT_101_POD==1068);
static_assert(SIZEOF_OLD_SOLDIERCREATE_STRUCT_101_POD-944==wireSize-916);

// Source buffer has exactly1040already-read bytes; preserve destination until
// UTF16, old inventory cardinalities and foreign-pointer policy validate.
inline bool decode(const std::array<UINT8,wireSize>& bytes,
                   OLD_SOLDIERCREATE_STRUCT_101& output,Context context) {
    std::array<CHAR16,10> name{};
    if(!profile101::name(bytes.data()+888,name))return false;
    // A serialized Windows process pointer cannot designate an existing native
    // soldier. Old maps can contain MSVC debug-fill in this unused field; map
    // placement creation initializes a fresh soldier and never consumes it.
    // Saved state keeps the stricter policy because active use would require a
    // real reconstructed pointer, not a discarded foreign address.
    if(context==Context::savedState && bytes[916])return false;
    for(unsigned slot=0;slot<19;++slot){
        const auto* object=bytes.data()+32+slot*36;
        if(object[2]>8 || profile101::unit(object)>=MAXITEMS)return false;
        if(object[2]==1)
            for(unsigned attachment=0;attachment<4;++attachment)
                if(profile101::unit(object+16+attachment*2)>=MAXITEMS)return false;
    }
    auto* target=reinterpret_cast<UINT8*>(&output);
    std::memcpy(target,bytes.data(),888);
    std::memcpy(output.name,name.data(),sizeof(output.name));
    std::memcpy(&output.ubSoldierClass,bytes.data()+908,3);
    std::memcpy(target+944,bytes.data()+916,124);
    output.pExistingSoldier=nullptr;
    output.fUseExistingSoldier=FALSE;
    return true;
}
} // namespace p710::soldiercreate101
#endif
