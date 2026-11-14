/*
 * P710 native snapshot update: Tactical/native_profile101.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_PROFILE101_H
#define P710_NATIVE_PROFILE101_H

// Windows Prof.dat / pre-NIV profile records have two UTF16LE fixed arrays,
// then a fixed-width integer/byte tail with Windows member padding. The host
// OLD type uses wchar4: reading its796byte POD directly consumes the next record.
#include "soldier profile type.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <type_traits>

namespace p710::profile101 {
inline constexpr std::size_t wireSize=716;
inline constexpr std::size_t wireTail=80;
inline constexpr std::size_t hostTail=offsetof(OLD_MERCPROFILESTRUCT_101,uiAttnSound);
static_assert(__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__,"profile101 tail requires little-endian native integers");
static_assert(sizeof(CHAR16)==4&&NAME_LENGTH==30&&NICKNAME_LENGTH==10);
static_assert(sizeof(INT8)==1&&sizeof(UINT8)==1&&sizeof(BOOLEAN)==1&&sizeof(INT16)==2&&sizeof(UINT16)==2&&sizeof(INT32)==4&&sizeof(UINT32)==4);
static_assert(sizeof(PaletteRepID)==30&&OldInventory::NUM_INV_SLOTS==19&&NUMBER_OF_OPINIONS_OLD==75);
static_assert(std::is_standard_layout_v<OLD_MERCPROFILESTRUCT_101> && alignof(OLD_MERCPROFILESTRUCT_101)==4);
static_assert(hostTail==160&&SIZEOF_OLD_MERCPROFILESTRUCT_101_POD==796);
static_assert(SIZEOF_OLD_MERCPROFILESTRUCT_101_POD-hostTail==wireSize-wireTail);
// Every tail member in the retained OLD definition is a1/2/4byte fixed-width
// integer, BOOLEAN or byte array. Anchor the alignment-sensitive sections;
// this adapter must fail to compile if that host-layout assumption drifts.
#define P710_PROFILE101_ANCHOR(field,wire) static_assert(offsetof(OLD_MERCPROFILESTRUCT_101,field)-hostTail==(wire)-wireTail)
P710_PROFILE101_ANCHOR(ubFaceIndex,108);
P710_PROFILE101_ANCHOR(PANTS,109);
P710_PROFILE101_ANCHOR(bSex,229);
P710_PROFILE101_ANCHOR(sExpLevelGain,242);
P710_PROFILE101_ANCHOR(bMedical,261);
P710_PROFILE101_ANCHOR(uiEyeDelay,272);
P710_PROFILE101_ANCHOR(bStrength,296);
P710_PROFILE101_ANCHOR(usKills,310);
P710_PROFILE101_ANCHOR(uiBodyTypeSubFlags,328);
P710_PROFILE101_ANCHOR(bExpLevel,352);
P710_PROFILE101_ANCHOR(usApproachFactor,396);
P710_PROFILE101_ANCHOR(sGridNo,408);
P710_PROFILE101_ANCHOR(DO_NOT_USE_inv,416);
P710_PROFILE101_ANCHOR(usStatChangeChances,474);
P710_PROFILE101_ANCHOR(uiWeeklySalary,540);
P710_PROFILE101_ANCHOR(sMedicalDepositAmount,552);
P710_PROFILE101_ANCHOR(usOptionalGearCost,574);
P710_PROFILE101_ANCHOR(bMercOpinion,576);
P710_PROFILE101_ANCHOR(usStrategicInsertionData,670);
P710_PROFILE101_ANCHOR(iBalance,676);
P710_PROFILE101_ANCHOR(uiMoney,684);
P710_PROFILE101_ANCHOR(uiProfileChecksum,696);
P710_PROFILE101_ANCHOR(uiTotalCostToDate,708);
P710_PROFILE101_ANCHOR(ubBuffer,712);
#undef P710_PROFILE101_ANCHOR

inline unsigned unit(const UINT8* p) { return unsigned(p[0])|(unsigned(p[1])<<8); }
template<std::size_t N> inline bool name(const UINT8* bytes,std::array<CHAR16,N>& output) {
    std::size_t dest=0;
    for(std::size_t i=0;i<N;++i) {
        unsigned value=unit(bytes+2*i);
        if(!value)break; // Bytes after the original string terminator are unused.
        if(value>=0xd800&&value<=0xdbff) {
            if(i+1==N)return false;
            unsigned low=unit(bytes+2*(++i));
            if(low<0xdc00||low>0xdfff)return false;
            value=0x10000+((value-0xd800)<<10)+(low-0xdc00);
        } else if(value>=0xdc00&&value<=0xdfff)return false;
        output[dest++]=static_cast<CHAR16>(value);
    }
    return true;
}

// The caller owns exactly716already-decrypted bytes. Validate both fixed names
// before mutating the destination. Full nonterminated arrays remain bounded and
// accepted, matching the old fixed-capacity record contract.
inline bool decode(const std::array<UINT8,wireSize>& bytes,OLD_MERCPROFILESTRUCT_101& output) {
    std::array<CHAR16,NAME_LENGTH> full{};
    std::array<CHAR16,NICKNAME_LENGTH> nick{};
    if(!name(bytes.data(),full)||!name(bytes.data()+60,nick))return false;
    std::memcpy(output.zName,full.data(),sizeof(output.zName));
    std::memcpy(output.zNickname,nick.data(),sizeof(output.zNickname));
    std::memcpy(reinterpret_cast<UINT8*>(&output)+hostTail,bytes.data()+wireTail,wireSize-wireTail);
    return true;
}
} // namespace p710::profile101
#endif
