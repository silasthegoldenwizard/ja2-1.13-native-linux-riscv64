/*
 * P710 native snapshot update: tests/native-core/native_contract_renewal_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual renewal-menu scheduling and dismissal callbacks with authored actors.
#include "Merc Contract.h"
#include "Assignments.h"
#include "Map Screen Interface.h"
#include "Overhead.h"
#include "Dialogue Control.h"
#include "Game Clock.h"
#include "PopUpBox.h"
#include "jascreens.h"
#include "gameloop.h"
#include <language.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>

extern std::vector<CONTRACT_NEWAL_LIST_NODE> ContractRenewalList;
extern UINT8 ubNumContractRenewals,ubCurrentContractRenewal,ubCurrentContractRenewalInProgress;
extern BOOLEAN fOneFrame;
extern void AssignmentScreenMaskBtnCallback(MOUSE_REGION*,INT32);
extern SOLDIERTYPE* pProcessingSoldier;
extern BOOLEAN fProcessingAMerc;

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<why<<'\n'; std::abort(); }
}
void drainDialogue() {
    unsigned steps=0;
    while (!DialogueQueueIsEmpty() && steps++<8) HandleDialogue();
    check(DialogueQueueIsEmpty(),"real menu events drain without speech assets");
}
void dismissMenu() {
    fFirstClickInAssignmentScreenMask=FALSE;
    AssignmentScreenMaskBtnCallback(nullptr,MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(!fShowContractMenu,"actual outside-click callback hides contract menu");
    check(gfContractRenewalSquenceOn && gfInContractMenuFromRenewSequence &&
          ubCurrentContractRenewalInProgress==1 && ubCurrentContractRenewal==0,
          "dismissal leaves the observed outstanding renewal phase");
}
void noRecovery(const char* why) {
    HandleContractRenewalSequence();
    check(DialogueQueueIsEmpty() && !fShowContractMenu,why);
}
void recovery(SOLDIERTYPE& soldier) {
    HandleContractRenewalSequence();
    check(!DialogueQueueIsEmpty(),"hidden outstanding choice schedules its real menu event");
    // A second frame must defer while the queued menu event is pending.
    HandleContractRenewalSequence();
    drainDialogue();
    check(fShowContractMenu && gfInContractMenuFromRenewSequence,"real dialogue event restores renewal menu");
    check(ghContractBox>=0 && GetNumberOfLinesOfTextInBox(ghContractBox)==MAX_CONTRACT_MENU_STRING_COUNT,
          "restored menu has all actual contract choices");
    check(fProcessingAMerc && pProcessingSoldier==&soldier,"restored menu still targets the current mercenary");
    check(ubCurrentContractRenewal==0 && ubCurrentContractRenewalInProgress==1,
          "restoring the choice neither renews nor dismisses a mercenary");
    HandleContractRenewalSequence();
    check(DialogueQueueIsEmpty(),"visible contract menu does not queue repeated events");
}
}

int main() {
    BindLanguageStrings(i18n::Lang::en);
    MSYS_Init();
    check(InitalizeDialogueControl(),"production dialogue queue initializes");
    for (UINT16 i=0;i<TOTAL_SOLDIERS;++i) MercPtrs[i]=&Menptr[i];
    auto& soldier=Menptr[0];
    soldier.ubID=SoldierID(0); soldier.ubProfile=10; soldier.bActive=TRUE;
    soldier.bTeam=OUR_TEAM; soldier.stats.bLife=95; soldier.stats.bLifeMax=95;
    soldier.ubWhatKindOfMercAmI=MERC_TYPE__AIM_MERC;
    soldier.iFaceIndex=-1; soldier.flags.fContractPriceHasIncreased=FALSE;
    gTacticalStatus.Team[OUR_TEAM].bLastID=SoldierID(0);
    gTacticalStatus.ubCurrentTeam=OUR_TEAM;
    gTacticalStatus.uiFlags=0;
    guiCurrentScreen=MAP_SCREEN; guiPendingScreen=NO_PENDING_SCREEN;
    fShowAssignmentMenu=FALSE; fShowUpdateBox=FALSE; fOneFrame=FALSE;
    UnLockPauseState();
    // Enter through production contract expiry so the sequence owns pause lock 7.
    // Match the authored contract's arrival hour to the current clock hour.
    soldier.iEndofContractTime=GetWorldDay()*1440+GetWorldHour()*60;
    soldier.uiTimeSoldierWillArrive=GetWorldHour()*60;
    ubNumContractRenewals=1; ubCurrentContractRenewal=0;
    gfContractRenewalSquenceOn=FALSE;
    ContractRenewalList[0].ubProfileID=soldier.ubProfile;
    check(ContractIsExpiring(&soldier),"authored contract expires at the actual clock hour");
    BeginContractRenewalSequence();
    check(gfContractRenewalSquenceOn && !ubCurrentContractRenewalInProgress &&
          ubCurrentContractRenewal==0 && PauseStateLocked() && guiLockPauseStateLastReasonId==7,
          "actual sequence entry acquires its renewal-owned pause lock");
    drainDialogue();
    // Enter the captured menu phase without loading the preceding spoken quote.
    ubCurrentContractRenewalInProgress=1;
    HandleMercIsWillingToRenew(soldier.ubID);
    drainDialogue();
    check(fShowContractMenu && gfInContractMenuFromRenewSequence,"real willing-to-renew handler opens contract menu");
    dismissMenu();
    recovery(soldier);
    check(PauseStateLocked() && guiLockPauseStateLastReasonId==7,
          "menu recovery preserves the sequence-owned pause lock");

    dismissMenu();
    fOneFrame=TRUE;
    noRecovery("one-frame menu transition is not mistaken for dismissal");
    fOneFrame=FALSE;
    fShowAssignmentMenu=TRUE;
    noRecovery("active assignment menu defers renewal-menu recovery");
    fShowAssignmentMenu=FALSE;
    fShowUpdateBox=TRUE;
    noRecovery("active update box defers renewal-menu recovery");
    fShowUpdateBox=FALSE;
    LockPauseState(8);
    noRecovery("locked departure or other modal flow is not interrupted");
    check(PauseStateLocked() && guiLockPauseStateLastReasonId==8,
          "departure pause lock remains held while recovery defers");
    UnLockPauseState();
    guiPendingScreen=MSG_BOX_SCREEN;
    noRecovery("pending modal message box is not interrupted");
    guiPendingScreen=NO_PENDING_SCREEN;
    guiCurrentScreen=MSG_BOX_SCREEN;
    noRecovery("active modal screen is not interrupted");
    guiCurrentScreen=MAP_SCREEN;
    SpecialCharacterDialogueEvent(DIALOGUE_SPECIAL_EVENT_SKIP_A_FRAME,0,0,0,0,0);
    HandleContractRenewalSequence();
    HandleDialogue();
    check(DialogueQueueIsEmpty() && !fShowContractMenu,"pending dialogue finishes without duplicate recovery event");
    recovery(soldier);

    // Completion through the existing sequence API keeps the menu closed.
    dismissMenu();
    EndCurrentContractRenewal();
    ubNumContractRenewals=2;
    ContractRenewalList[1].ubProfileID=soldier.ubProfile;
    ubCurrentContractRenewalInProgress=1;
    noRecovery("completed menu phase is not reopened after sequence advancement");
    check(ubCurrentContractRenewal==1 && !gfInContractMenuFromRenewSequence,
          "existing completion advances to the next mercenary");
    gfContractRenewalSquenceOn=FALSE;
    gfInContractMenuFromRenewSequence=TRUE;
    noRecovery("ordinary menus outside a renewal sequence are not recovered");

    EmptyDialogueQueue();
    RemoveBox(ghContractBox); ghContractBox=-1;
    gfInContractMenuFromRenewSequence=FALSE;
    pProcessingSoldier=nullptr; fProcessingAMerc=FALSE;
    soldier.bActive=FALSE;
    MSYS_Shutdown();
    std::cout<<"PASS: "<<checks<<" actual contract renewal and dismissal assertions\n";
}
