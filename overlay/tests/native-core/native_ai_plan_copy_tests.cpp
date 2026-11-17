/*
 * P710 native snapshot update: tests/native-core/native_ai_plan_copy_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_ai_plan_copy_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Real soldier records and autoresolve snapshots, with the production UI swap.
#include "Soldier Control.h"
#include "Soldier Profile.h"
#include "Overhead.h"
#include "Isometric Utils.h"
#include "Interface.h"
#include "Interface Panels.h"
#include "GameSettings.h"
#include "Soldier Create.h"
#include "PreBattle Interface.h"
#include "jascreens.h"
#include "screenids.h"
#include "faces.h"
#include "../../ModularizedTacticalAI/include/Plan.h"
#include <cstdlib>
#include <iostream>
#include <memory>

extern SOLDIERTYPE* ReserveTacticalSoldierForAutoresolve(UINT8);
extern SOLDIERTYPE* ReserveTacticalMilitiaSoldierForAutoresolve(UINT8);
extern void TrashAllSoldiers();
extern void SwapMercPortraits(SOLDIERTYPE*,INT8);
extern FACETYPE* gpCurrentTalkingFace;

#include "merc_portrait_plan_swap.inc"

namespace {
unsigned checks=0;
void check(bool value,const char* reason) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<reason<<'\n'; std::abort(); }
}
struct Counts { unsigned destroyed=0,detached=0; };
class CountingPlan final : public AI::tactical::Plan {
    Counts& counts;
public:
    CountingPlan(SOLDIERTYPE& soldier,Counts& observed) : Plan(&soldier),counts(observed) {}
    ~CountingPlan() override {
        ++counts.destroyed;
        if (!get_npc()->ai_masterplan_) ++counts.detached;
    }
    void execute(AI::tactical::PlanInputData&) override {}
    bool done() const override { return false; }
};
void swaps() {
    Counts sourceCounts,targetCounts;
    SOLDIERTYPE source,target;
    source.ubProfile=20; source.stats.bLife=75; source.bCamo=13;
    target.ubProfile=21; target.stats.bLife=63; target.bCamo=29;
    source.ai_masterplan_=new CountingPlan(source,sourceCounts);
    target.ai_masterplan_=new CountingPlan(target,targetCounts);
    SwapMercPortraitRecords(source,target);
    check(sourceCounts.destroyed==1 && targetCounts.destroyed==1 &&
          sourceCounts.detached==1 && targetCounts.detached==1,
          "production swap retires both plans before record addresses change");
    check(!source.ai_masterplan_ && !target.ai_masterplan_,"neither swapped record aliases a retired plan");
    check(source.ubProfile==21 && source.stats.bLife==63 && source.bCamo==29 &&
          target.ubProfile==20 && target.stats.bLife==75 && target.bCamo==13,
          "record swap preserves the original profile, health, and camouflage transfer");
    SwapMercPortraitRecords(source,target);
    check(source.ubProfile==20 && target.ubProfile==21 &&
          sourceCounts.destroyed==1 && targetCounts.destroyed==1,
          "existing plan-free swaps remain reversible without repeated destruction");
    source.initialize(); target.initialize();
    check(sourceCounts.destroyed==1 && targetCounts.destroyed==1,
          "later reset does not delete either pre-swap plan twice");

    Counts sharedCounts;
    source.ai_masterplan_=new CountingPlan(source,sharedCounts);
    target.ai_masterplan_=source.ai_masterplan_;
    SwapMercPortraitRecords(source,target);
    check(sharedCounts.destroyed==1 && sharedCounts.detached==1 &&
          !source.ai_masterplan_ && !target.ai_masterplan_,
          "a live legacy alias is detached on both records and retired once");
}

void portraitBoundaries() {
    const UINT8 savedSquadSize=gGameOptions.ubSquadSize;
    const INT16 savedPanel=gsCurInterfacePanel;
    const SoldierID savedSelected=gusSelectedSoldier;
    FACETYPE* const savedTalkingFace=gpCurrentTalkingFace;
    TEAM_PANEL_SLOTS_TYPE savedPanelSlots[NUM_TEAM_SLOTS];
    for (UINT8 slot=0;slot<NUM_TEAM_SLOTS;++slot) {
        savedPanelSlots[slot]=gTeamPanel[slot];
        gTeamPanel[slot].fOccupied=FALSE;
        gTeamPanel[slot].ubID=NOBODY;
    }

    gsCurInterfacePanel=TEAM_PANEL;
    gpCurrentTalkingFace=nullptr;
    gGameOptions.ubSquadSize=2;
    auto& first=Menptr[0];
    auto& last=Menptr[1];
    first.initialize(); last.initialize();
    first.ubID=SoldierID(0); last.ubID=SoldierID(1);
    first.stats.bLife=80; last.stats.bLife=70;
    first.bActive=TRUE; last.bActive=TRUE;
    first.ubProfile=10; last.ubProfile=25;
    MercPtrs[0]=&first; MercPtrs[1]=&last;
    gTeamPanel[0].fOccupied=TRUE; gTeamPanel[0].ubID=SoldierID(0);
    gTeamPanel[1].fOccupied=TRUE; gTeamPanel[1].ubID=SoldierID(1);

    gusSelectedSoldier=SoldierID(0);
    SwapMercPortraits(&first,-1);
    check(first.ubProfile==10 && last.ubProfile==25 && gusSelectedSoldier==SoldierID(0),
          "left from the first portrait is a bounded no-op");

    first.flags.uiStatusFlags|=SOLDIER_DRIVER;
    SwapMercPortraits(&first,-1);
    check(first.ubProfile==10 && last.ubProfile==25,
          "vehicle-seat swap rejects a negative target before vehicle lookup");
    first.flags.uiStatusFlags&=~SOLDIER_DRIVER;

    gusSelectedSoldier=SoldierID(1);
    SwapMercPortraits(&last,1);
    check(first.ubProfile==10 && last.ubProfile==25 && gusSelectedSoldier==SoldierID(1),
          "right from the last configured portrait is a bounded no-op");

    gTeamPanel[0].fOccupied=FALSE;
    gTeamPanel[0].ubID=NOBODY;
    gusSelectedSoldier=SoldierID(0);
    SwapMercPortraits(&first,1);
    check(first.ubProfile==10 && last.ubProfile==25,
          "a selected soldier outside the team panel cannot fabricate a swap position");

    for (UINT8 slot=0;slot<NUM_TEAM_SLOTS;++slot) gTeamPanel[slot]=savedPanelSlots[slot];
    gGameOptions.ubSquadSize=savedSquadSize;
    gsCurInterfacePanel=savedPanel;
    gusSelectedSoldier=savedSelected;
    gpCurrentTalkingFace=savedTalkingFace;
    first.initialize(); last.initialize();
}

void snapshot(bool militia) {
    Counts counts;
    const UINT16 index=militia ? 1 : 0;
    auto& source=Menptr[index];
    source.initialize();
    source.ubID=SoldierID(index);
    MercPtrs[index]=&source;
    source.bActive=TRUE; source.bInSector=TRUE;
    source.stats.bLife=71; source.stats.bLifeMax=90;
    source.bCamo=17; source.ubProfile=42;
    source.sGridNo=1;
    const auto team=militia ? MILITIA_TEAM : ENEMY_TEAM;
    const auto soldierClass=militia ? SOLDIER_CLASS_GREEN_MILITIA : SOLDIER_CLASS_ADMINISTRATOR;
    source.bTeam=team;
    source.ubSoldierClass=soldierClass;
    gTacticalStatus.Team[team].bFirstID=SoldierID(index);
    gTacticalStatus.Team[team].bLastID=SoldierID(index);
    source.ai_masterplan_=new CountingPlan(source,counts);
    auto* originalPlan=source.ai_masterplan_;
    std::unique_ptr<SOLDIERTYPE> copy(militia ?
        ReserveTacticalMilitiaSoldierForAutoresolve(soldierClass) :
        ReserveTacticalSoldierForAutoresolve(soldierClass));
    check(copy!=nullptr,"actual autoresolve reservation returns the authored live soldier snapshot");
    check(!copy->ai_masterplan_ && source.ai_masterplan_==originalPlan && !counts.destroyed,
          "snapshot does not own or retire the source's live AI plan");
    check(copy->ubID==NUM_PROFILES && copy->ubProfile==42 && copy->stats.bLife==71 &&
          copy->bCamo==17 && copy->bTeam==team && source.sGridNo==NOWHERE,
          "reservation preserves copied state, source marker, and snapshot ID");
    if (militia)
        check((source.usSoldierFlagMask&SOLDIER_EQUIPMENT_DROPPED)!=0 &&
              !(copy->usSoldierFlagMask&SOLDIER_EQUIPMENT_DROPPED),
              "militia snapshot preserves the existing equipment-drop ownership marker");
    copy->initialize();
    check(!counts.destroyed && source.ai_masterplan_==originalPlan && !originalPlan->done(),
          "resetting the real snapshot leaves the original plan alive");
    copy.reset();
    check(!counts.destroyed && source.ai_masterplan_==originalPlan,
          "destroying the snapshot leaves source plan ownership unchanged");
    source.initialize();
    check(counts.destroyed==1 && counts.detached==1,"original source alone retires its plan exactly once");

    source.bActive=TRUE; source.bInSector=TRUE; source.stats.bLife=71;
    source.ubSoldierClass=soldierClass; source.sGridNo=1;
    copy.reset(militia ? ReserveTacticalMilitiaSoldierForAutoresolve(soldierClass) :
                        ReserveTacticalSoldierForAutoresolve(soldierClass));
    check(copy && !copy->ai_masterplan_ && !source.ai_masterplan_,
          "existing plan-free autoresolve reservations remain plan-free");
    copy.reset(); source.initialize();
}

void autoresolveCleanup() {
    const UINT32 savedScreen=guiCurrentScreen;
    const BOOLEAN savedPersistent=gfPersistantPBI;
    auto* savedFirst=MercPtrs[0];
    auto* savedSecond=MercPtrs[1];

    auto& source=Menptr[0];
    source.initialize();
    source.ubID=SoldierID(0);
    source.bActive=TRUE;
    source.bInSector=TRUE;
    source.stats.bLife=71;
    source.bTeam=ENEMY_TEAM;
    source.ubSoldierClass=SOLDIER_CLASS_ADMINISTRATOR;
    source.sGridNo=1;
    MercPtrs[0]=&source;

    auto* single=ReserveTacticalSoldierForAutoresolve(SOLDIER_CLASS_ADMINISTRATOR);
    check(single && single!=&source,"autoresolve cleanup fixture uses a separately owned snapshot");
    guiCurrentScreen=AUTORESOLVE_SCREEN;
    gfPersistantPBI=FALSE;
    check(TacticalRemoveSoldierPointer(single,FALSE),
          "production autoresolve cleanup releases one C++ snapshot");

    source.sGridNo=1;
    auto* first=ReserveTacticalSoldierForAutoresolve(SOLDIER_CLASS_ADMINISTRATOR);
    source.sGridNo=1;
    auto* second=ReserveTacticalSoldierForAutoresolve(SOLDIER_CLASS_ADMINISTRATOR);
    check(first && second && first!=second,
          "bulk autoresolve cleanup fixture owns two independent snapshots");
    MercPtrs[0]=first;
    MercPtrs[1]=second;
    for (INT32 index=2;index<MAX_NUM_SOLDIERS;++index) {
        MercPtrs[index]=&Menptr[index];
        Menptr[index].bActive=FALSE;
    }
    TrashAllSoldiers();
    check(true,"bulk cleanup advances through soldier IDs after releasing snapshots");

    MercPtrs[0]=savedFirst;
    MercPtrs[1]=savedSecond;
    guiCurrentScreen=savedScreen;
    gfPersistantPBI=savedPersistent;
    source.initialize();
}
}
int main() {
    swaps();
    portraitBoundaries();
    snapshot(false);
    snapshot(true);
    autoresolveCleanup();
    std::cout<<"PASS: "<<checks<<" production soldier AI-plan copy assertions\n";
}
