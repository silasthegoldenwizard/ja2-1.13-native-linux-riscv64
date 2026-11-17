/*
 * P710 native snapshot update: tests/native-core/native_map_placement_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_map_placement_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored legacy map placement records through the production native decoder.
#include "native_soldiercreate101.h"
#include "native_map_soldiercreate.h"
#include "Exit Grids.h"
#include "Soldier Init List.h"
#include "Map Information.h"
#include "SaveLoadGame.h"
#include "Scheduling.h"
#include "Items.h"
#include "Action Items.h"
#include <climits>
#include <array>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

extern int gLastLBEUniqueID;

namespace {
using Wire=std::array<UINT8,p710::soldiercreate101::wireSize>;
unsigned checks=0;

void require(bool value,const char* reason) {
    ++checks;
    if(!value)throw std::runtime_error(reason);
}

void put16(Wire& wire,std::size_t offset,UINT16 value) {
    wire.at(offset)=static_cast<UINT8>(value);
    wire.at(offset+1)=static_cast<UINT8>(value>>8);
}

Wire mapRecord() {
    Wire wire{};
    wire[1]=101;
    put16(wire,888,'F');
    put16(wire,890,'a');
    put16(wire,892,'t');
    put16(wire,894,'i');
    put16(wire,896,'m');
    put16(wire,898,'a');
    // Original A10 placements retain MSVC debug-fill in the obsolete pointer
    // and use flag. Static map creation must clear both values.
    for(std::size_t offset=912;offset<=916;++offset)wire[offset]=0xcd;
    return wire;
}

using Bytes=std::vector<UINT8>;
void put(Bytes& bytes,std::size_t offset,UINT32 value,unsigned width=4) {
    for(unsigned i=0;i<width;++i)bytes.at(offset+i)=static_cast<UINT8>(value>>(i*8));
}

Bytes modernRecord(bool version7,unsigned slots) {
    const std::size_t pod=version7?263:239;
    // Empty OBJECTTYPE POD (5), stack count (4), and two inventory counters (8).
    Bytes bytes(pod+4+slots*17,0);
    bytes[0]=1; bytes[1]=57;
    put(bytes,6,10,2); put(bytes,8,1,2);
    put(bytes,12,version7?90000:12345,version7?4:2);
    put(bytes,version7?184:182,version7?120000:23456,version7?4:2);
    const auto name=version7?226:204;
    put(bytes,name,'T',2); put(bytes,name+2,0x00e9,2);
    put(bytes,name+4,0xd83d,2); put(bytes,name+6,0xde00,2);
    const auto tail=version7?256:232;
    for(unsigned i=0;i<4;++i)bytes[tail-4+i]=0xcd;
    bytes[tail]=0xcd; bytes[tail+1]=3; bytes[tail+2]=1;
    bytes[tail+3]=17; bytes[tail+4]=1; bytes[tail+5]=2; bytes[tail+6]=1;
    put(bytes,pod,slots);
    return bytes;
}

void rejectPlacement(const Bytes& bytes,float version=7.0f) {
    auto input=bytes;
    auto* begin=reinterpret_cast<INT8*>(input.data());
    auto* cursor=begin;
    SOLDIERCREATE_STRUCT placement;
    placement.ubProfile=42;
    const auto slots=placement.Inv.size();
    const auto lbes=LBEArray.size();
    const int lastLBE=gLastLBEUniqueID;
    require(!placement.Load(&cursor,version,31,input.size()),
            "malformed or truncated placement is rejected");
    require(cursor==begin && placement.ubProfile==42 && placement.Inv.size()==slots &&
                LBEArray.size()==lbes && gLastLBEUniqueID==lastLBE && input==bytes,
            "rejected placement preserves cursor, destination, LBE list, and source");
}

void placementPrefixes(const Bytes& bytes,float version) {
    for(std::size_t size=0;size<bytes.size();++size)
        rejectPlacement(Bytes(bytes.begin(),bytes.begin()+size),version);
}

void modernCases(bool version7) {
    for(unsigned slots:{0u,55u}) {
        auto bytes=modernRecord(version7,slots);
        placementPrefixes(bytes,version7?7.0f:6.0f);
        const auto length=bytes.size();
        // Following exit-grid count and first record are independent sentinels.
        bytes.resize(length+14);
        put(bytes,length,1,2);
        put(bytes,length+2,12562); put(bytes,length+6,8048);
        bytes[length+10]=10; bytes[length+11]=1;
        const auto before=bytes;
        auto* cursor=reinterpret_cast<INT8*>(bytes.data());
        SOLDIERCREATE_STRUCT placement;
        require(placement.Load(&cursor,version7?7.0f:6.0f,31,bytes.size()),
                "modern Windows placement with inventory loads");
        require(cursor==reinterpret_cast<INT8*>(bytes.data()+length),
                "modern placement leaves cursor at the next map section");
        require(placement.Inv.size()==slots,"modern inventory retains slot count");
        require(placement.sInsertionGridNo==(version7?90000:12345) &&
                    placement.sPatrolGrid[0]==(version7?120000:23456),
                "version-specific grid widths are preserved");
        require(placement.name[0]=='T' && placement.name[1]==0x00e9 &&
                    placement.name[2]==0x1f600 && placement.name[3]==0,
                "modern UTF16 BMP and surrogate-pair names decode");
        require(!placement.pExistingSoldier && !placement.fUseExistingSoldier &&
                    placement.ubCivilianGroup==3 && placement.fKillSlotIfOwnerDies==1 &&
                    placement.ubScheduleID==17 && placement.fUseGivenVehicle==1 &&
                    placement.bUseGivenVehicleID==2 && placement.fHasKeys==1,
                "modern trailing flags remain separate from the obsolete pointer");
        UINT16 count=0;
        std::memcpy(&count,cursor,2); cursor+=2;
        EXITGRID exit;
        require(count==1 && exit.Load(&cursor,7.0f) && exit.iMapIndex==12562 &&
                    exit.usGridNo==8048 && exit.ubGotoSectorX==10 &&
                    exit.ubGotoSectorY==1 && exit.ubGotoSectorZ==0,
                "following exit grid decodes through its production reader");
        require(cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()) && bytes==before,
                "all fixture bytes consumed exactly and input remains unchanged");
    }
    for(int invalid:{-1,static_cast<int>(NUM_INV_SLOTS)+1}) {
        auto bytes=modernRecord(version7,0);
        put(bytes,version7?263:239,static_cast<UINT32>(invalid));
        auto* begin=reinterpret_cast<INT8*>(bytes.data());
        auto* cursor=begin;
        SOLDIERCREATE_STRUCT placement; placement.ubProfile=42;
        require(!placement.Load(&cursor,version7?7.0f:6.0f,31,bytes.size()),
                "invalid modern inventory count propagates failure");
        require(cursor==begin && placement.ubProfile==42,
                "inventory failure preserves destination and caller cursor");
    }
    auto bytes=modernRecord(version7,0);
    put(bytes,version7?226:204,0xdc00,2);
    auto* begin=reinterpret_cast<INT8*>(bytes.data());
    auto* cursor=begin;
    SOLDIERCREATE_STRUCT placement; placement.ubProfile=42;
    require(!placement.Load(&cursor,version7?7.0f:6.0f,31,bytes.size()) &&
                cursor==begin && placement.ubProfile==42,
            "malformed modern UTF16 is rejected without committing state");
}

Bytes populatedRecord() {
    auto bytes=modernRecord(true,0);
    put(bytes,263,1); // One inventory slot, followed by an object and attachment.
    const auto object=bytes.size();
    bytes.resize(object+9+48+4+9+48+4+8,0);
    put(bytes,object,7,2); bytes[object+2]=1; put(bytes,object+5,1);
    put(bytes,object+9,75,2); put(bytes,object+9+30,100,2);
    put(bytes,object+9+48,1); // One attached object.
    const auto attachment=object+9+48+4;
    put(bytes,attachment,8,2); bytes[attachment+2]=1; put(bytes,attachment+5,1);
    put(bytes,attachment+9,85,2); put(bytes,attachment+9+30,100,2);
    put(bytes,bytes.size()-8,3); put(bytes,bytes.size()-4,5);
    return bytes;
}

void populatedModernInventory() {
    auto bytes=populatedRecord();
    guiCurrentSaveGameVersion=187;
    placementPrefixes(bytes,7.0f);
    auto* cursor=reinterpret_cast<INT8*>(bytes.data());
    SOLDIERCREATE_STRUCT placement;
    guiCurrentSaveGameVersion=187;
    require(placement.Load(&cursor,7.0f,31,bytes.size()),"populated Windows inventory and attachment load");
    require(cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()),
            "populated inventory retains the next-section boundary");
    require(placement.Inv.size()==1 && placement.Inv[0].usItem==7 &&
                placement.Inv[0].ubNumberOfObjects==1 &&
                placement.Inv[0][0]->data.objectStatus==75,
            "populated inventory retains object identity and condition");
    const auto& attachments=placement.Inv[0][0]->attachments;
    require(attachments.size()==1 && attachments.front().usItem==8 &&
                attachments.front().ubNumberOfObjects==1 &&
                attachments.front()[0]->data.objectStatus==85,
            "recursive attachment identity and condition remain intact");
    require(placement.Inv.bNewItemCount[0]==3 && placement.Inv.bNewItemCycleCount[0]==5,
            "populated inventory counters remain aligned");
}

void malformedNestedRecords() {
    // Four-byte signed counts must fail before allocation. Nested item IDs
    // must fail before Item[] access or default-attachment processing.
    constexpr std::size_t object=267, stack=object+9, child=stack+48+4;
    for(const auto offset:{object+5,stack+48,child+5,child+9+48}) {
        for(const auto count:{UINT32(-1),UINT32(INT_MAX)}) {
            auto bytes=populatedRecord(); put(bytes,offset,count); rejectPlacement(bytes);
        }
    }
    for(const auto offset:{object,child}) {
        auto bytes=populatedRecord(); put(bytes,offset,MAXITEMS,2); rejectPlacement(bytes);
    }
    auto mismatched=populatedRecord(); put(mismatched,object+5,0); rejectPlacement(mismatched);
    auto excessive=populatedRecord(); put(excessive,object+5,MAX_OBJECTS_PER_SLOT+1); rejectPlacement(excessive);
    auto action=populatedRecord();
    put(action,object,ACTION_ITEM,2); put(action,stack+4,MAXITEMS,2);
    action[stack+8]=ACTION_ITEM_BLOW_UP;
    rejectPlacement(action);
    const auto legacy=mapRecord();
    Bytes oldAction(legacy.begin(),legacy.end());
    put(oldAction,32,ACTION_ITEM,2); oldAction[34]=1;
    put(oldAction,38,MAXITEMS,2); oldAction[42]=ACTION_ITEM_BLOW_UP;
    rejectPlacement(oldAction,5.0f);

    auto recursive=modernRecord(true,0); put(recursive,263,1);
    for(unsigned level=0;level<70;++level) {
        const auto offset=recursive.size(); recursive.resize(offset+61,0);
        put(recursive,offset,7,2); recursive[offset+2]=1; put(recursive,offset+5,1);
        put(recursive,offset+57,level==69?0:1);
    }
    recursive.resize(recursive.size()+8,0);
    rejectPlacement(recursive);
}

void mapObjectCommitCases() {
    // A non-empty item must own at least one serialized stack entry. Without
    // this guard, ACTION_ITEM assignment and default-attachment processing
    // dereference objectStack.begin().
    for (const UINT16 item : {static_cast<UINT16>(ACTION_ITEM), static_cast<UINT16>(7)}) {
        Bytes empty(9,0);
        put(empty,0,item,2);
        auto* begin=reinterpret_cast<INT8*>(empty.data());
        auto* cursor=begin;
        OBJECTTYPE destination;
        destination.usItem=8;
        const auto lbeCount=LBEArray.size();
        const auto lbeID=gLastLBEUniqueID;
        const auto savedDefault=Item[7].defaultattachments[0];
        if (item==7) Item[7].defaultattachments[0]=8;
        require(!destination.Load(&cursor,7.0f,31,empty.size()) && cursor==begin &&
                    destination.usItem==8 && LBEArray.size()==lbeCount &&
                    gLastLBEUniqueID==lbeID,
                "nonzero item with an empty serialized stack is rejected transactionally");
        Item[7].defaultattachments[0]=savedDefault;
    }

    // Modern action-item fields are native map data. Committing a validated
    // record must not run the legacy assignment repair that rewrites them.
    Bytes bytes(9+sizeof(ObjectData_PRE_ITS)+4,0);
    put(bytes,0,ACTION_ITEM,2); bytes[2]=1; put(bytes,5,1);
    ObjectData_PRE_ITS wire{};
    wire.misc.bBombStatus=77;
    wire.misc.bDetonatorType=2;
    wire.misc.usBombItem=7;
    wire.misc.bDelay=3;
    wire.misc.ubBombOwner=4;
    wire.misc.bActionValue=ACTION_ITEM_BLOW_UP;
    wire.misc.ubTolerance=5;
    wire.bTrap=6;
    wire.fUsed=7;
    wire.ubImprintID=8;
    wire.bTemperature=9.0f;
    wire.ubDirection=SOUTHWEST;
    wire.ubWireNetworkFlag=0x12345678u;
    wire.bDefuseFrequency=11;
    wire.sRepairThreshold=12;
    std::memcpy(bytes.data()+9,&wire,sizeof(wire));
    auto* cursor=reinterpret_cast<INT8*>(bytes.data());
    OBJECTTYPE object;
    require(object.Load(&cursor,7.0f,31,bytes.size()) &&
                cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()) &&
                object.usItem==ACTION_ITEM && object.objectStack.size()==1,
            "modern action item loads through the bounded production reader");
    const auto& data=object[0]->data;
    require(data.misc.bBombStatus==77 && data.misc.bDetonatorType==2 &&
                data.misc.usBombItem==7 && data.misc.bDelay==3 &&
                data.misc.ubBombOwner==4 && data.misc.bActionValue==ACTION_ITEM_BLOW_UP &&
                data.misc.ubTolerance==5 && data.bTrap==6 && data.fUsed==7 &&
                data.ubImprintID==8 && data.bTemperature==9.0f &&
                data.ubDirection==SOUTHWEST && data.ubWireNetworkFlag==0x12345678u &&
                data.bDefuseFrequency==11 && data.sRepairThreshold==12,
            "modern action-item trap fields survive transactional commit unchanged");
}

void nestedLBERecords() {
    constexpr std::size_t object=267, node=object+9+48+4;
    auto bytes=modernRecord(true,0); put(bytes,263,1);
    bytes.resize(node+SIZEOF_LBENODE_POD+4+9+48+4+8,0);
    put(bytes,object,7,2); bytes[object+2]=1; put(bytes,object+5,1);
    bytes[object+9+2]=255; put(bytes,object+9+4,100);
    put(bytes,node+offsetof(LBENODE,uniqueID),100);
    put(bytes,node+SIZEOF_LBENODE_POD,1);
    const auto child=node+SIZEOF_LBENODE_POD+4;
    put(bytes,child,8,2); bytes[child+2]=1; put(bytes,child+5,1);
    const auto savedClass=Item[7].usItemClass;
    Item[7].usItemClass=IC_LBEGEAR;
    const auto savedCount=LBEArray.size(); const auto savedID=gLastLBEUniqueID;
    placementPrefixes(bytes,7.0f);
    for(const auto count:{UINT32(-1),UINT32(MAX_ITEMS_IN_LBE+1),UINT32(INT_MAX)}) {
        auto malformed=bytes; put(malformed,node+SIZEOF_LBENODE_POD,count);
        rejectPlacement(malformed);
    }
    auto invalidID=bytes; put(invalidID,node+offsetof(LBENODE,uniqueID),INT_MAX);
    rejectPlacement(invalidID);
    auto invalidChild=bytes; put(invalidChild,child,MAXITEMS,2); rejectPlacement(invalidChild);
    auto* cursor=reinterpret_cast<INT8*>(bytes.data());
    SOLDIERCREATE_STRUCT placement;
    require(placement.Load(&cursor,7.0f,31,bytes.size()) &&
                cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()),
            "nested LBE placement consumes its complete extent");
    require(LBEArray.size()==savedCount+1 && LBEArray.back().uniqueID==100 &&
                LBEArray.back().inv.size()==1 && LBEArray.back().inv[0].usItem==8 &&
                gLastLBEUniqueID>=101,"nested LBE identity and contained object survive loading");
    while(LBEArray.size()>savedCount)LBEArray.pop_back();
    gLastLBEUniqueID=savedID; Item[7].usItemClass=savedClass;
}

void basicAndSectionCases() {
    for(const float version:{5.0f,6.0f,7.0f}) {
        const auto size=version<7.0f?sizeof(_OLD_BASIC_SOLDIERCREATE_STRUCT):sizeof(BASIC_SOLDIERCREATE_STRUCT);
        Bytes basic(size,0);
        for(std::size_t prefix=0;prefix<size;++prefix) {
            Bytes input(basic.begin(),basic.begin()+prefix);
            auto* begin=reinterpret_cast<INT8*>(input.data()); auto* cursor=begin;
            BASIC_SOLDIERCREATE_STRUCT placement{}; placement.usStartingGridNo=42;
            require(!placement.Load(&cursor,version,input.size()) && cursor==begin &&
                        placement.usStartingGridNo==42,"every basic-placement prefix is rejected transactionally");
        }
        auto* cursor=reinterpret_cast<INT8*>(basic.data());
        BASIC_SOLDIERCREATE_STRUCT decoded{};
        require(decoded.Load(&cursor,version,basic.size()) &&
                    cursor==reinterpret_cast<INT8*>(basic.data()+basic.size()),
                "complete basic placement consumes its exact record");
    }

    BASIC_SOLDIERCREATE_STRUCT basic{}; basic.fDetailedPlacement=1;
    Bytes section(sizeof(basic)); std::memcpy(section.data(),&basic,sizeof(basic));
    const auto detail=modernRecord(true,0);
    section.insert(section.end(),detail.begin(),detail.end());
    // Seed a list that malformed section preflight must preserve.
    auto* cursor=reinterpret_cast<INT8*>(section.data());
    gMapInformation.ubNumIndividuals=1;
    require(LoadSoldiersFromMap(&cursor,7.0f,31,section.size()),"complete placement section loads");
    auto* existing=gSoldierInitHead;
    require(existing && existing->pDetailedPlacement && cursor==reinterpret_cast<INT8*>(section.data()+section.size()),
            "complete placement section creates one detailed list node");
    for(std::size_t size=0;size<section.size();++size) {
        Bytes input(section.begin(),section.begin()+size);
        auto* begin=reinterpret_cast<INT8*>(input.data()); cursor=begin;
        require(!LoadSoldiersFromMap(&cursor,7.0f,31,input.size()) && cursor==begin &&
                    gSoldierInitHead==existing && gMapInformation.ubNumIndividuals==1,
                "section prefix failure preserves prior list and caller cursor");
    }
    KillSoldierInitList();
    gMapInformation.ubNumIndividuals=0;
}

Bytes modernScheduleSection() {
    constexpr std::size_t recordBytes=52;
    Bytes bytes(1+recordBytes,0);
    bytes[0]=1;
    const std::size_t record=1;
    for(unsigned i=0;i<4;++i)bytes[record+i]=0xcd;
    put(bytes,record+4,60,2);
    put(bytes,record+6,120,2);
    put(bytes,record+8,0xffff,2);
    put(bytes,record+10,720,2);
    put(bytes,record+12,70000);
    put(bytes,record+16,120000);
    put(bytes,record+20,0xffffffff);
    put(bytes,record+24,135790);
    put(bytes,record+28,246810);
    put(bytes,record+32,345678);
    put(bytes,record+36,456789);
    put(bytes,record+40,567890);
    bytes[record+44]=SCHEDULE_ACTION_GRIDNO;
    bytes[record+45]=SCHEDULE_ACTION_SLEEP;
    bytes[record+46]=SCHEDULE_ACTION_WAKE;
    bytes[record+47]=SCHEDULE_ACTION_LEAVESECTOR;
    bytes[record+48]=7;
    bytes[record+49]=254;
    put(bytes,record+50,SCHEDULE_FLAGS_ACTIVE1|SCHEDULE_FLAGS_VARIANCE2,2);
    return bytes;
}

void schedulePrefixes() {
    for(const float version:{6.0f,7.0f}) {
        auto bytes=modernScheduleSection();
        if(version<7.0f) {
            bytes.assign(37,0); bytes[0]=1;
            put(bytes,5,60,2); put(bytes,13,12345,2); put(bytes,21,23456,2);
            bytes[29]=SCHEDULE_ACTION_GRIDNO;
        }
        for(std::size_t size=0;size<bytes.size();++size) {
            Bytes input(bytes.begin(),bytes.begin()+size);
            auto* begin=reinterpret_cast<INT8*>(input.data()); auto* cursor=begin;
            require(!LoadSchedules(&cursor,version,input.size()) && cursor==begin && !gpScheduleList,
                    "every schedule-section prefix is rejected before list changes");
        }
        auto* cursor=reinterpret_cast<INT8*>(bytes.data());
        require(LoadSchedules(&cursor,version,bytes.size()) && gpScheduleList &&
                    cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()),
                "complete version-specific schedule section consumes its exact extent");
        if(version<7.0f)
            require(gpScheduleList->usTime[0]==60 && gpScheduleList->usData1[0]==12345 &&
                        gpScheduleList->usData2[0]==23456 && gpScheduleList->ubAction[0]==SCHEDULE_ACTION_GRIDNO,
                    "version-6 schedule preserves legacy grid widths and action offsets");
        DestroyAllSchedulesWithoutDestroyingEvents();
    }
}

void modernScheduleCases() {
    auto bytes=modernScheduleSection();
    const auto sectionEnd=bytes.size();
    bytes.insert(bytes.end(),{0x12,0x34,0x56,0x78});
    auto* cursor=reinterpret_cast<INT8*>(bytes.data());
    require(LoadSchedules(&cursor,7.0f,bytes.size()),
            "version-7 Windows schedule section loads");
    require(cursor==reinterpret_cast<INT8*>(bytes.data()+sectionEnd) &&
                bytes[sectionEnd]==0x12 && bytes[sectionEnd+3]==0x78,
            "version-7 schedule consumes its 52-byte Windows record exactly");
    require(gpScheduleList && !gpScheduleList->next && gubScheduleID==2,
            "version-7 schedule count creates one list node");
    require(gpScheduleList->usTime[0]==60 && gpScheduleList->usTime[1]==120 &&
                gpScheduleList->usTime[2]==0xffff && gpScheduleList->usTime[3]==720 &&
                gpScheduleList->usData1[0]==70000 && gpScheduleList->usData1[1]==120000 &&
                gpScheduleList->usData1[2]==0xffffffff && gpScheduleList->usData1[3]==135790 &&
                gpScheduleList->usData2[0]==246810 && gpScheduleList->usData2[1]==345678 &&
                gpScheduleList->usData2[2]==456789 && gpScheduleList->usData2[3]==567890,
            "version-7 schedule preserves time and grid fields");
    require(gpScheduleList->ubAction[0]==SCHEDULE_ACTION_GRIDNO &&
                gpScheduleList->ubAction[1]==SCHEDULE_ACTION_SLEEP &&
                gpScheduleList->ubAction[2]==SCHEDULE_ACTION_WAKE &&
                gpScheduleList->ubAction[3]==SCHEDULE_ACTION_LEAVESECTOR &&
                gpScheduleList->ubScheduleID==1 && gpScheduleList->ubSoldierID==NOBODY &&
                gpScheduleList->usFlags==(SCHEDULE_FLAGS_ACTIVE1|SCHEDULE_FLAGS_VARIANCE2),
            "version-7 schedule preserves actions and flags while clearing its runtime soldier");

    auto truncated=modernScheduleSection();
    auto* begin=reinterpret_cast<INT8*>(truncated.data());
    cursor=begin;
    auto* existing=gpScheduleList;
    require(!LoadSchedules(&cursor,7.0f,truncated.size()-1),
            "truncated version-7 schedule section is rejected");
    require(cursor==begin && gpScheduleList==existing && gubScheduleID==2,
            "schedule preflight failure preserves cursor and list state");
    DestroyAllSchedulesWithoutDestroyingEvents();

    Bytes zero{0,0xaa};
    cursor=reinterpret_cast<INT8*>(zero.data());
    require(LoadSchedules(&cursor,7.0f,zero.size()) &&
                cursor==reinterpret_cast<INT8*>(zero.data()+1) && !gpScheduleList,
            "zero-count schedule section consumes only its count");
    DestroyAllSchedulesWithoutDestroyingEvents();

    UINT8 byte=0;
    begin=reinterpret_cast<INT8*>(&byte); cursor=begin;
    require(!LoadSchedules(&cursor,7.0f,0) && cursor==begin,
            "missing schedule count is rejected without advancing");
    INT8* nullCursor=nullptr;
    require(!LoadSchedules(nullptr,7.0f,1) && !LoadSchedules(&nullCursor,7.0f,1),
            "null schedule cursors are rejected");

    auto oversized=modernScheduleSection();
    oversized[0]=255;
    begin=reinterpret_cast<INT8*>(oversized.data()); cursor=begin;
    require(!LoadSchedules(&cursor,7.0f,oversized.size()) && cursor==begin,
            "unsigned oversized schedule count is rejected by preflight");
}

void soldierInitListLifecycle() {
    KillSoldierInitList();
    gSoldierInitHead=nullptr;
    gSoldierInitTail=nullptr;
    gMapInformation.ubNumIndividuals=0;

    BASIC_SOLDIERCREATE_STRUCT basic{};
    auto* node=AddBasicPlacementToSoldierInitList(&basic);
    require(node && gSoldierInitHead==node && gSoldierInitTail==node &&
                !node->prev && !node->next && gMapInformation.ubNumIndividuals==1,
            "single soldier-init placement publishes matching head and tail ownership");

    KillSoldierInitList();
    require(!gSoldierInitHead && !gSoldierInitTail &&
                gMapInformation.ubNumIndividuals==0,
            "single soldier-init placement teardown clears both list owners");

    KillSoldierInitList();
    require(!gSoldierInitHead && !gSoldierInitTail &&
                gMapInformation.ubNumIndividuals==0,
            "empty soldier-init placement teardown is idempotent");
}

// Optional local diagnostic for the separately hashed installed A10 basement.
// No installed data is embedded in this test or required by CTest.
void installedBasement(const char* path) {
    std::ifstream input(path,std::ios::binary|std::ios::ate);
    require(input.good(),"open caller-supplied local basement map");
    const auto length=input.tellg();
    require(length>=357130 && length<4*1024*1024,"basement map extent");
    Bytes bytes(static_cast<std::size_t>(length));
    input.seekg(0); input.read(reinterpret_cast<char*>(bytes.data()),bytes.size());
    require(input.good(),"read complete local map");
    float version=0; std::memcpy(&version,bytes.data(),4);
    require(version==7.0f && bytes[4]==31,"local map format is 7.31");
    auto* cursor=reinterpret_cast<INT8*>(bytes.data()+272740);
    require(gMapInformation.Load(&cursor,version) && gMapInformation.ubNumIndividuals==20,
            "local map information declares twenty placements");
    guiCurrentSaveGameVersion=187;
    require(LoadSoldiersFromMap(&cursor,version,31,bytes.size()-static_cast<std::size_t>(cursor-reinterpret_cast<INT8*>(bytes.data()))),
            "production loader reads local basement placements");
    require(cursor==reinterpret_cast<INT8*>(bytes.data()+357104),
            "local placements finish exactly at the exit section");
    UINT16 count=0; std::memcpy(&count,cursor,2); cursor+=2;
    require(count==2,"local basement has two exit tiles");
    for(int grid:{12562,12563}) {
        EXITGRID exit;
        require(exit.Load(&cursor,version) && exit.iMapIndex==grid && exit.usGridNo==8048 &&
                    exit.ubGotoSectorX==10 && exit.ubGotoSectorY==1 && exit.ubGotoSectorZ==0,
                "local basement exit targets the A10 surface");
    }
    require(cursor==reinterpret_cast<INT8*>(bytes.data()+357130),
            "local exit section ends at the expected offset");
    cursor=reinterpret_cast<INT8*>(bytes.data()+357163);
    require(LoadSchedules(&cursor,version,bytes.size()-357163),
            "production loader reads four local basement schedules");
    require(cursor==reinterpret_cast<INT8*>(bytes.data()+bytes.size()) &&
                gubScheduleID==5,
            "local schedule section consumes four Windows records to end of file");
    require(gpScheduleList && gpScheduleList->ubAction[0]==SCHEDULE_ACTION_STAYINSECTOR &&
                gpScheduleList->ubScheduleID==1 && gpScheduleList->usTime[0]==0xffff,
            "local basement schedule fields decode at their Windows offsets");
    DestroyAllSchedulesWithoutDestroyingEvents();
    KillSoldierInitList();
}
}

int main(int argc,char** argv) {
    try {
        auto wire=mapRecord();
        OLD_SOLDIERCREATE_STRUCT_101 decoded;
        require(p710::soldiercreate101::decode(
                    wire,decoded,
                    p710::soldiercreate101::Context::staticMapPlacement),
                "static map accepts obsolete debug-fill");
        require(decoded.pExistingSoldier==nullptr && !decoded.fUseExistingSoldier,
                "static map clears foreign pointer and use flag");
        require(decoded.ubProfile==101 && decoded.name[0]=='F' && decoded.name[5]=='a',
                "static map preserves profile and UTF16 name");

        OLD_SOLDIERCREATE_STRUCT_101 saved;
        require(!p710::soldiercreate101::decode(
                    wire,saved,p710::soldiercreate101::Context::savedState),
                "saved state rejects an active foreign-pointer marker");
        wire[916]=0;
        require(p710::soldiercreate101::decode(
                    wire,saved,p710::soldiercreate101::Context::savedState),
                "saved state accepts an inactive foreign pointer");
        require(saved.pExistingSoldier==nullptr && !saved.fUseExistingSoldier,
                "saved state clears inactive foreign pointer bits");

        auto invalidCount=wire;
        invalidCount[32+2]=9;
        require(!p710::soldiercreate101::decode(
                    invalidCount,decoded,
                    p710::soldiercreate101::Context::staticMapPlacement),
                "map still rejects an oversized old object stack");
        auto invalidItem=wire;
        put16(invalidItem,32,MAXITEMS);
        require(!p710::soldiercreate101::decode(
                    invalidItem,decoded,
                    p710::soldiercreate101::Context::staticMapPlacement),
                "map still rejects an invalid item ID");
        auto invalidAttachment=wire;
        invalidAttachment[32+2]=1;
        put16(invalidAttachment,32+16,MAXITEMS);
        require(!p710::soldiercreate101::decode(
                    invalidAttachment,decoded,
                    p710::soldiercreate101::Context::staticMapPlacement),
                "map still rejects an invalid attachment ID");
        auto invalidName=wire;
        put16(invalidName,888,0xd800);
        put16(invalidName,890,'X');
        require(!p710::soldiercreate101::decode(
                    invalidName,decoded,
                    p710::soldiercreate101::Context::staticMapPlacement),
                "map still rejects malformed UTF16");

        auto productionWire=mapRecord();
        auto* cursor=reinterpret_cast<INT8*>(productionWire.data());
        SOLDIERCREATE_STRUCT placement;
        placementPrefixes(Bytes(productionWire.begin(),productionWire.end()),5.0f);
        require(placement.Load(&cursor,5.0f,25,productionWire.size()),
                "production map Load accepts the legacy static placement");
        require(cursor==reinterpret_cast<INT8*>(productionWire.data()+productionWire.size()),
                "production map Load consumes exactly one Windows record");
        require(placement.ubProfile==101 && !placement.fUseExistingSoldier &&
                    placement.pExistingSoldier==nullptr,
                "production map Load preserves profile and clears obsolete link state");

        // Authored version-7 Windows record: 263 POD bytes, then a four-byte
        // empty inventory. Padding permits the old native-size read to fail
        // the cursor assertion without reading outside this fixture.
        std::array<UINT8,512> modern{};
        modern[0]=1;
        modern[1]=57;
        modern[226]='I';
        modern[228]='r';
        modern[230]='a';
        for(std::size_t i=252;i<=256;++i)modern[i]=0xcd;
        auto* modernCursor=reinterpret_cast<INT8*>(modern.data());
        SOLDIERCREATE_STRUCT modernPlacement;
        require(modernPlacement.Load(&modernCursor,7.0f,31,modern.size()),
                "production map Load accepts a version-7 Windows placement");
        require(modernCursor==reinterpret_cast<INT8*>(modern.data()+267),
                "version-7 placement consumes Windows POD and inventory only");
        require(modernPlacement.name[0]=='I' && modernPlacement.name[1]=='r' &&
                    modernPlacement.name[2]=='a' && modernPlacement.ubProfile==57,
                "version-7 placement preserves its UTF16 name and profile");
        require(!modernPlacement.pExistingSoldier && !modernPlacement.fUseExistingSoldier,
                "version-7 static placement clears obsolete link state");
        modernCases(false);
        modernCases(true);
        populatedModernInventory();
        malformedNestedRecords();
        mapObjectCommitCases();
        nestedLBERecords();
        basicAndSectionCases();
        modernScheduleCases();
        schedulePrefixes();
        soldierInitListLifecycle();
        require(!modernPlacement.Load(static_cast<INT8**>(nullptr),7.0f,31),
                "null modern map cursor is rejected");
        if(argc==2)installedBasement(argv[1]);
        else require(argc==1,"optional diagnostic takes exactly one local map path");

        std::cout<<"map placement: "<<checks<<" checks passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"map placement failed at check "<<checks<<": "<<error.what()<<'\n';
        return 1;
    }
}
