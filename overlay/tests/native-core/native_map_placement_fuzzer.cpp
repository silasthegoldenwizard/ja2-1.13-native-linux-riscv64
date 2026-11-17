/*
 * P710 native snapshot update: tests/native-core/native_map_placement_fuzzer.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Stateful sanitizer fuzzing for the production version-7 map-placement decoder.
#include "Action Items.h"
#include "Items.h"
#include "SaveLoadGame.h"
#include "Soldier Init List.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

extern int gLastLBEUniqueID;

namespace {
using Bytes=std::vector<UINT8>;

[[noreturn]] void invariantFailure() {
    std::abort();
}

void put(Bytes& bytes,std::size_t offset,UINT32 value,unsigned width=4) {
    for(unsigned i=0;i<width;++i) bytes.at(offset+i)=static_cast<UINT8>(value>>(i*8));
}

Bytes modernRecord() {
    constexpr std::size_t pod=263;
    Bytes bytes(pod+4,0);
    bytes[0]=1;
    bytes[1]=57;
    put(bytes,6,10,2);
    put(bytes,8,1,2);
    put(bytes,12,90000);
    put(bytes,184,120000);
    put(bytes,226,'T',2);
    put(bytes,228,0x00e9,2);
    put(bytes,230,0xd83d,2);
    put(bytes,232,0xde00,2);
    for(unsigned i=0;i<4;++i) bytes[252+i]=0xcd;
    bytes[256]=0xcd;
    bytes[257]=3;
    bytes[258]=1;
    bytes[259]=17;
    bytes[260]=1;
    bytes[261]=2;
    bytes[262]=1;
    put(bytes,pod,0);
    return bytes;
}

Bytes populatedRecord() {
    auto bytes=modernRecord();
    put(bytes,263,1);
    const auto object=bytes.size();
    bytes.resize(object+9+48+4+9+48+4+8,0);
    put(bytes,object,7,2);
    bytes[object+2]=1;
    put(bytes,object+5,1);
    put(bytes,object+9,75,2);
    put(bytes,object+9+30,100,2);
    put(bytes,object+9+48,1);
    const auto attachment=object+9+48+4;
    put(bytes,attachment,8,2);
    bytes[attachment+2]=1;
    put(bytes,attachment+5,1);
    put(bytes,attachment+9,85,2);
    put(bytes,attachment+9+30,100,2);
    put(bytes,bytes.size()-8,3);
    put(bytes,bytes.size()-4,5);
    return bytes;
}

Bytes nestedLBERecord() {
    constexpr std::size_t object=267;
    constexpr std::size_t node=object+9+48+4;
    auto bytes=modernRecord();
    put(bytes,263,1);
    bytes.resize(node+SIZEOF_LBENODE_POD+4+9+48+4+8,0);
    put(bytes,object,9,2);
    bytes[object+2]=1;
    put(bytes,object+5,1);
    bytes[object+9+2]=255;
    put(bytes,object+9+4,100);
    put(bytes,node+offsetof(LBENODE,uniqueID),100);
    put(bytes,node+SIZEOF_LBENODE_POD,1);
    const auto child=node+SIZEOF_LBENODE_POD+4;
    put(bytes,child,8,2);
    bytes[child+2]=1;
    put(bytes,child+5,1);
    return bytes;
}

Bytes inputFromRecipe(const std::uint8_t* data,std::size_t size) {
    if(!size) return {};
    const auto mode=data[0]&3;
    if(mode==3) return Bytes(data+1,data+size);
    Bytes bytes=mode==0?modernRecord():(mode==1?populatedRecord():nestedLBERecord());
    for(std::size_t i=2;i+2<size;i+=3) {
        const auto position=(static_cast<std::size_t>(data[i])<<8|data[i+1])%bytes.size();
        bytes[position]^=data[i+2];
    }
    if(size>1 && (data[1]&1)) {
        const auto extent=(static_cast<std::size_t>(data[1]>>1)*bytes.size())/127;
        bytes.resize(std::min(extent,bytes.size()));
    }
    return bytes;
}
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data,std::size_t size) {
    if(size>65536) return 0;
    auto bytes=inputFromRecipe(data,size);
    if(bytes.empty()) return 0;
    const auto original=bytes;
    const auto savedLBECount=LBEArray.size();
    const auto savedLBEID=gLastLBEUniqueID;
    const auto savedSaveVersion=guiCurrentSaveGameVersion;
    const auto savedItemClass=Item[9].usItemClass;
    Item[9].usItemClass=IC_LBEGEAR;
    guiCurrentSaveGameVersion=187;

    auto* begin=reinterpret_cast<INT8*>(bytes.data());
    auto* cursor=begin;
    {
        SOLDIERCREATE_STRUCT placement;
        placement.ubProfile=42;
        const auto loaded=placement.Load(&cursor,7.0f,31,bytes.size());
        if(!loaded) {
            if(cursor!=begin || placement.ubProfile!=42 || LBEArray.size()!=savedLBECount ||
                    gLastLBEUniqueID!=savedLBEID || bytes!=original)
                invariantFailure();
        } else {
            const auto consumed=cursor-begin;
            if(consumed<0 || static_cast<std::size_t>(consumed)>bytes.size() ||
                    placement.pExistingSoldier || placement.fUseExistingSoldier || bytes!=original ||
                    LBEArray.size()<savedLBECount)
                invariantFailure();
        }
    }

    while(LBEArray.size()>savedLBECount) LBEArray.pop_back();
    if(LBEArray.size()!=savedLBECount) invariantFailure();
    gLastLBEUniqueID=savedLBEID;
    guiCurrentSaveGameVersion=savedSaveVersion;
    Item[9].usItemClass=savedItemClass;
    return 0;
}
