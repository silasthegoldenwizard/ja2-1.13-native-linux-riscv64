/*
 * P710 native snapshot update: tests/native-core/native_sm_panel_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_sm_panel_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_sm_panel_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise the original inventory-panel lifecycle with authored images only.
#include "Button System.h"
#include "Interface.h"
#include "Interface Panels.h"
#include "Interface Items.h"
#include "Items.h"
#include "Animation Control.h"
#include "Soldier Control.h"
#include "Soldier Profile.h"
#include "Squads.h"
#include "strategicmap.h"
#include "Text.h"
#include "MessageBoxScreen.h"
#include "vsurface.h"
#include "FileManBackend.h"
#include "input.h"
#include "english.h"
#include <language.hpp>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

extern INT32 iSMPanelButtons[NUM_SM_BUTTONS];
extern INT32 giSMStealthImages, giSMStealthButton;
extern INT32 giSMZipperImages, giSMZipperButton;
extern INT32 giSMDropPackImages, giSMDropPackButton;
extern UINT32 guiCurrentItemDescriptionScreen;
extern UINT32 guiTacticalInterfaceFlags;
extern BOOLEAN gfInKeyRingPopup;
extern BOOLEAN gfInItemStackPopup;
extern OBJECTTYPE* gpItemDescObject;
extern INT32 giItemDescAmmoButtonImages, giItemDescAmmoButton;
extern INT32 giMapInvDescButtonImage, giMapInvDescButton;
extern BOOLEAN gfUIStanceDifferent, gfAllDisabled;
extern SOLDIERTYPE* gpSMCurrentMerc;
extern SoldierID gusSMCurrentMerc;
extern SoldierID gubSelectSMPanelToMerc;
extern BOOLEAN gfSMDisableForItems;
extern MOUSE_REGION gSM_SELMERCBarsRegion;
extern BOOLEAN gfCheckForMouseOverItem;
extern INT8 gbCheckForMouseOverItemPos;
extern void SMInvMoveCallback(MOUSE_REGION*,INT32);
extern void SMInvMoveCammoCallback(MOUSE_REGION*,INT32);
extern void SMInvClickCallback(MOUSE_REGION*,INT32);
extern void SMInvClickCamoCallback(MOUSE_REGION*,INT32);
extern void MergeMessageBoxCallBack(UINT8);
extern void BtnStanceUpCallback(GUI_BUTTON*,INT32);
extern void BtnDropPackCallback(GUI_BUTTON*,INT32);
extern void BtnZipperCallback(GUI_BUTTON*,INT32);
extern void BtnUpdownCallback(GUI_BUTTON*,INT32);
extern void BtnClimbCallback(GUI_BUTTON*,INT32);
extern void BtnStanceDownCallback(GUI_BUTTON*,INT32);
extern void BtnStealthModeCallback(GUI_BUTTON*,INT32);
extern void BtnMuteCallback(GUI_BUTTON*,INT32);
extern void BtnPrevMercCallback(GUI_BUTTON*,INT32);
extern void BtnNextMercCallback(GUI_BUTTON*,INT32);
extern void BtnBurstModeCallback(GUI_BUTTON*,INT32);
extern void MercFacePanelMoveCallback(MOUSE_REGION*,INT32);
extern void EnemyIndicatorClickCallback(MOUSE_REGION*,INT32);
extern void MercFacePanelCallback(MOUSE_REGION*,INT32);
extern void TMFirstHandInvCallback(MOUSE_REGION*,INT32);
extern void TMClickFirstHandInvCallback(MOUSE_REGION*,INT32);
extern void TMClickSecondHandInvCallback(MOUSE_REGION*,INT32);
extern void HandleMouseOverTeamFaceForContMove(BOOLEAN);
extern BOOLEAN AddDeadCharacterToSquadDeadGuys(SOLDIERTYPE*,INT32);
extern BOOLEAN IsDeadGuyOnAnySquad(SOLDIERTYPE*);

namespace {
unsigned checks=0;
void check(bool ok,const char* why) {
    ++checks;
    if (!ok) throw std::runtime_error(why);
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown();
    ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager(); fileman::CloseAll();
    vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}
void dynamicButton(INT32& image,INT32& button) {
    image=LoadButtonImage("buttons.sti",4,0,1,2,3);
    button=QuickCreateButton(static_cast<UINT32>(image),2,3,0,31,nullptr,nullptr);
    check(image>=0 && button>=0,"dynamic inventory control");
}
}

int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::runtime_error("expected authored fixture directory");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(dir->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("sm-panel",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"mount authored panel images");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"resource managers");
        BindLanguageStrings(i18n::Lang::en);
        MSYS_Init(); check(InitButtonSystem(),"button manager");
        for (int cycle=0;cycle<MAX_BUTTON_PICS+1;++cycle) {
            gsCurInterfacePanel=SM_PANEL;
            guiCurrentItemDescriptionScreen=GAME_SCREEN;
            gfInKeyRingPopup=FALSE;
            check(CreateSMPanelButtons(),"all production inventory buttons create");
            for (INT32 id:iSMPanelButtons) check(GetButtonPtr(id)!=nullptr,"inventory button is live");
            dynamicButton(giSMStealthImages,giSMStealthButton);
            dynamicButton(giSMZipperImages,giSMZipperButton);
            dynamicButton(giSMDropPackImages,giSMDropPackButton);
            // These states caused RenderBackpackButtons to skip teardown.
            if (cycle%3==0) gsCurInterfacePanel=TEAM_PANEL;
            if (cycle%3==1) guiCurrentItemDescriptionScreen=MAP_SCREEN;
            if (cycle%3==2) gfInKeyRingPopup=TRUE;
            RemoveSMPanelButtons();
            check(std::none_of(ButtonList.begin(),ButtonList.end(),[](GUI_BUTTON* b){return b!=nullptr;}),
                  "panel teardown removes every control in each context");
            check(std::none_of(ButtonPictures.begin(),ButtonPictures.end(),[](const BUTTON_PICS& p){return p.vobj!=nullptr;}),
                  "panel teardown releases every image");
            for (INT32 id:iSMPanelButtons) check(id==-1,"retired inventory IDs reset");
            RemoveSMPanelButtons();
        }

        // A tactical redraw can race the post-load panel rebuild. The retired
        // button IDs must make this update a no-op instead of indexing -1.
        SOLDIERTYPE panelMerc;
        panelMerc.initialize();
        panelMerc.ubID=SoldierID(0);
        panelMerc.bActive=TRUE;
        panelMerc.sGridNo=1;
        panelMerc.ubDesiredHeight=ANIM_STAND;
        panelMerc.usAnimState=STANDING;
        panelMerc.bWeaponMode=WM_NORMAL;
        panelMerc.stats.bLife=100;
        MercPtrs[0]=&panelMerc;
        gpSMCurrentMerc=&panelMerc;
        gfUIStanceDifferent=FALSE;
        gfAllDisabled=FALSE;
        UpdateSMPanel();
        check(std::all_of(std::begin(iSMPanelButtons),std::end(iSMPanelButtons),
                          [](INT32 id){return id==-1;}),
              "post-load redraw ignores a retired single-merc panel");

        panelMerc.flags.uiStatusFlags=SOLDIER_ROBOT;
        gfInKeyRingPopup=FALSE;
        gfInItemStackPopup=FALSE;
        MSYS_DefineRegion(&gSM_SELMERCBarsRegion,0,0,10,10,MSYS_PRIORITY_NORMAL,
                          MSYS_NO_CURSOR,nullptr,nullptr);
        MSYS_AddRegion(&gSM_SELMERCBarsRegion);
        BOOLEAN dirty=DIRTYLEVEL1;
        RenderSMPanel(&dirty);
        check(gSM_SELMERCBarsRegion.FastHelpText!=nullptr,
              "robot vital statistics target the single-merc bars region");
        panelMerc.flags.uiStatusFlags=0;
        gpSMCurrentMerc=nullptr;
        MercPtrs[0]=nullptr;

        // A panel switch can retire the selected merc after its mouse events
        // were queued. Inventory callbacks must reject those events before
        // reading the old slot or cursor owner.
        MOUSE_REGION retiredInventoryRegion{};
        MSYS_SetRegionUserData(&retiredInventoryRegion,0,0);
        guiTacticalInterfaceFlags=0;
        gfCheckForMouseOverItem=TRUE;
        gbCheckForMouseOverItemPos=0;
        SMInvMoveCallback(&retiredInventoryRegion,MSYS_CALLBACK_REASON_GAIN_MOUSE);
        SMInvMoveCallback(&retiredInventoryRegion,MSYS_CALLBACK_REASON_LOST_MOUSE);
        SMInvMoveCammoCallback(&retiredInventoryRegion,MSYS_CALLBACK_REASON_LOST_MOUSE);
        SMInvClickCallback(&retiredInventoryRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        OBJECTTYPE retiredCursor;
        retiredCursor.usItem=401;
        retiredCursor.ubNumberOfObjects=1;
        gpItemPointer=&retiredCursor;
        gpItemPointerSoldier=nullptr;
        SMInvClickCamoCallback(&retiredInventoryRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        MergeMessageBoxCallBack(MSG_BOX_RETURN_YES);
        MergeMessageBoxCallBack(MSG_BOX_RETURN_NO);
        gpItemPointer=nullptr;
        check(!gfCheckForMouseOverItem && gbCheckForMouseOverItemPos==NO_SLOT,
              "retired inventory callbacks clear hover state without an owner");

        // Single-merc buttons and team-panel regions can also deliver an event
        // after the panel owner or slot has been retired. Every callback must
        // reject that event before its first soldier or panel-array access.
        GUI_BUTTON retiredButton{};
        retiredButton.uiFlags=BUTTON_ENABLED;
        BtnStanceUpCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnDropPackCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnZipperCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnUpdownCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnClimbCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnStanceDownCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnStealthModeCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnMuteCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnPrevMercCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnNextMercCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnBurstModeCallback(&retiredButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        BtnStanceUpCallback(nullptr,MSYS_CALLBACK_REASON_LBUTTON_UP);

        MOUSE_REGION retiredTeamRegion{};
        MSYS_SetRegionUserData(&retiredTeamRegion,0,NUM_TEAM_SLOTS);
        MercFacePanelMoveCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_MOVE);
        EnemyIndicatorClickCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        MercFacePanelCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        TMFirstHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        TMClickFirstHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_RBUTTON_UP);
        TMClickSecondHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_RBUTTON_UP);
        gTeamPanel[0].fOccupied=TRUE;
        gTeamPanel[0].ubID=SoldierID(0);
        MSYS_SetRegionUserData(&retiredTeamRegion,0,0);
        MercFacePanelMoveCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_MOVE);
        EnemyIndicatorClickCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        MercFacePanelCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        TMFirstHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        TMClickFirstHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_RBUTTON_UP);
        TMClickSecondHandInvCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_RBUTTON_UP);
        MercFacePanelCallback(nullptr,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        gTeamPanel[0].fOccupied=FALSE;
        gTeamPanel[0].ubID=NOBODY;
        check(gpSMCurrentMerc==nullptr && MercPtrs[0]==nullptr,
              "retired tactical-panel callbacks preserve missing owners");

        // Occupied team-panel slots can outlive their MercPtrs owner while a
        // panel rebuild or queued squad action is still in progress. Slot
        // management, rendering, and next-merc selection must treat that as a
        // retired slot rather than dereferencing the stale numeric ID.
        const UINT8 previousSquadSize=gGameOptions.ubSquadSize;
        gGameOptions.ubSquadSize=2;
        gTeamPanel[0].fOccupied=TRUE;
        gTeamPanel[0].ubID=SoldierID(0);
        check(RemovePlayerFromInterfaceTeamSlot(0),
              "stale occupied slot can be retired without its soldier");
        check(!gTeamPanel[0].fOccupied && gTeamPanel[0].ubID==NOBODY,
              "stale slot retirement clears panel ownership");

        panelMerc.initialize();
        panelMerc.ubID=SoldierID(0);
        panelMerc.bActive=TRUE;
        panelMerc.bInSector=TRUE;
        panelMerc.bTeam=gbPlayerNum;
        panelMerc.bAssignment=0;
        panelMerc.stats.bLife=100;
        MercPtrs[0]=&panelMerc;
        MercPtrs[1]=nullptr;
        gTeamPanel[0].fOccupied=TRUE;
        gTeamPanel[0].ubID=panelMerc.ubID;
        gTeamPanel[1].fOccupied=TRUE;
        gTeamPanel[1].ubID=SoldierID(1);
        check(FindNextMercInTeamPanel(&panelMerc,FALSE,FALSE)==panelMerc.ubID,
              "next-merc selection skips a stale occupied slot");
        RenderTEAMPanel(DIRTYLEVEL0);
        check(!GetPlayerIDFromInterfaceTeamSlot(1,nullptr),
              "team-slot lookup rejects null output storage");
        SoldierID staleID=NOBODY;
        check(!GetPlayerIDFromInterfaceTeamSlot(1,&staleID),
              "team-slot lookup rejects an ownerless occupied slot");
        AddPlayerToInterfaceTeamSlot(NOBODY);
        check(FindNextMercInTeamPanel(nullptr,FALSE,FALSE)==NOBODY,
              "next-merc selection rejects a missing current merc");
        gTeamPanel[0].fOccupied=FALSE;
        gTeamPanel[0].ubID=NOBODY;
        gTeamPanel[1].fOccupied=FALSE;
        gTeamPanel[1].ubID=NOBODY;
        MercPtrs[0]=nullptr;
        gGameOptions.ubSquadSize=previousSquadSize;

        // A queued portrait selection can arrive after its soldier retired,
        // while give-item and redraw work is still pending. The single-merc
        // panel must keep a disabled, ownerless state rather than converting
        // NOBODY or an empty MercPtrs slot into a pointer.
        gpSMCurrentMerc=nullptr;
        gusSMCurrentMerc=NOBODY;
        gfSMDisableForItems=FALSE;
        SetSMPanelCurrentMerc(NOBODY);
        check(gpSMCurrentMerc==nullptr && gusSMCurrentMerc==NOBODY,
              "invalid portrait selection preserves an ownerless panel");
        check(gfSMDisableForItems,
              "ownerless single-merc panel disables item interaction");
        CheckForDisabledForGiveItem();
        check(gfSMDisableForItems,
              "give-item state remains disabled without a panel owner");
        BOOLEAN ownerlessDirty=DIRTYLEVEL2;
        gubSelectSMPanelToMerc=SoldierID(1);
        RenderSMPanel(&ownerlessDirty);
        check(gubSelectSMPanelToMerc==NOBODY && gpSMCurrentMerc==nullptr,
              "redraw retires a queued selection whose soldier disappeared");
        HandleMouseOverTeamFaceForContMove(TRUE);
        HandleMouseOverTeamFaceForContMove(FALSE);
        const SoldierID previousFirstID=gTacticalStatus.Team[gbPlayerNum].bFirstID;
        const SoldierID previousLastID=gTacticalStatus.Team[gbPlayerNum].bLastID;
        gTacticalStatus.Team[gbPlayerNum].bFirstID=SoldierID(0);
        gTacticalStatus.Team[gbPlayerNum].bLastID=SoldierID(1);
        FinishAnySkullPanelAnimations();
        gTacticalStatus.Team[gbPlayerNum].bFirstID=previousFirstID;
        gTacticalStatus.Team[gbPlayerNum].bLastID=previousLastID;

        // Dialogue, map focus, and queued portrait work can request a locate
        // or radio locator after the target soldier has retired. These entry
        // points must reject both NOBODY and an empty in-range MercPtrs slot
        // before converting the numeric ID into a soldier pointer.
        HandleLocateSelectMerc(NOBODY,0);
        ShowRadioLocator(NOBODY,SHOW_LOCATOR_NORMAL);
        EndRadioLocator(NOBODY);
        HandleLocateSelectMerc(SoldierID(1),0);
        ShowRadioLocator(SoldierID(1),SHOW_LOCATOR_FAST);
        EndRadioLocator(SoldierID(1));
        check(MercPtrs[1]==nullptr,
              "retired locate and radio-locator requests preserve missing owners");

        panelMerc.initialize();
        panelMerc.ubID=SoldierID(0);
        panelMerc.bActive=TRUE;
        MercPtrs[0]=&panelMerc;
        ShowRadioLocator(panelMerc.ubID,SHOW_LOCATOR_NORMAL);
        check(panelMerc.flags.fFlashLocator && panelMerc.sLocatorFrame==0 &&
              panelMerc.ubNumLocateCycles==5,
              "normal radio locator retains its live-owner behavior");
        EndRadioLocator(panelMerc.ubID);
        check(!panelMerc.flags.fFlashLocator && !panelMerc.flags.fShowLocator,
              "radio locator ends for a live owner");
        ShowRadioLocator(panelMerc.ubID,SHOW_LOCATOR_FAST);
        check(panelMerc.ubNumLocateCycles==3,
              "fast radio locator retains its live-owner cycle count");
        EndRadioLocator(panelMerc.ubID);

        // A shift-click can be queued after the former selected merc retired,
        // even though the clicked team-panel owner is still live.
        panelMerc.bInSector=TRUE;
        panelMerc.bTeam=gbPlayerNum;
        panelMerc.bAssignment=0;
        panelMerc.stats.bLife=100;
        panelMerc.flags.uiStatusFlags=0;
        gTeamPanel[0].fOccupied=TRUE;
        gTeamPanel[0].ubID=panelMerc.ubID;
        MSYS_SetRegionUserData(&retiredTeamRegion,0,0);
        const SoldierID previousSelectedSoldier=gusSelectedSoldier;
        const UINT32 previousTacticalFlags=gTacticalStatus.uiFlags;
        gusSelectedSoldier=NOBODY;
        gTacticalStatus.uiFlags&=~INCOMBAT;
        gfKeyState[SHIFT]=TRUE;
        MercFacePanelCallback(&retiredTeamRegion,MSYS_CALLBACK_REASON_LBUTTON_DWN);
        gfKeyState[SHIFT]=FALSE;
        check(!(panelMerc.flags.uiStatusFlags&SOLDIER_MULTI_SELECTED),
              "shift-click rejects a retired primary selection");
        gusSelectedSoldier=previousSelectedSoldier;
        gTacticalStatus.uiFlags=previousTacticalFlags;
        gTeamPanel[0].fOccupied=FALSE;
        gTeamPanel[0].ubID=NOBODY;
        MercPtrs[0]=nullptr;

        // Post-load and dialogue transitions can request a squad that became
        // empty before the queued selection runs. It must not assume slot zero
        // still owns a soldier.
        check(FindSoldierByProfileID(7,TRUE)==nullptr,
              "profile lookup skips empty runtime soldier slots");
        panelMerc.initialize();
        panelMerc.ubID=SoldierID(0);
        panelMerc.ubProfile=7;
        panelMerc.bActive=TRUE;
        const SoldierID previousPlayerLastID=gTacticalStatus.Team[0].bLastID;
        gTacticalStatus.Team[0].bLastID=SoldierID(0);
        MercPtrs[0]=&panelMerc;
        check(FindSoldierByProfileID(7,TRUE)==&panelMerc,
              "profile lookup retains live-owner behavior");
        MercPtrs[0]=nullptr;
        gTacticalStatus.Team[0].bLastID=previousPlayerLastID;
        const UINT32 previousScreen=guiCurrentScreen;
        guiCurrentScreen=GAME_SCREEN;
        for (INT32 member=0;member<NUMBER_OF_SOLDIERS_PER_SQUAD;++member)
            Squad[0][member]=nullptr;
        gusSelectedSoldier=NOBODY;
        check(!SetCurrentSquad(0,TRUE) && CurrentSquad()==NO_CURRENT_SQUAD,
              "empty queued squad selection retires the current squad");

        panelMerc.initialize();
        panelMerc.ubID=SoldierID(0);
        panelMerc.bActive=TRUE;
        panelMerc.bInSector=TRUE;
        panelMerc.bTeam=gbPlayerNum;
        panelMerc.bAssignment=0;
        panelMerc.stats.bLife=100;
        panelMerc.sSectorX=gWorldSectorX;
        panelMerc.sSectorY=gWorldSectorY;
        panelMerc.bSectorZ=gbWorldSectorZ;
        panelMerc.flags.fBetweenSectors=FALSE;
        const UINT8 previousLiveSquadSize=gGameOptions.ubSquadSize;
        gGameOptions.ubSquadSize=2;
        MercPtrs[0]=&panelMerc;
        Squad[0][0]=&panelMerc;
        gusSelectedSoldier=panelMerc.ubID;
        check(SetCurrentSquad(0,TRUE) && CurrentSquad()==0,
              "live squad selection retains its current-squad behavior");
        check(PlayerExistsInSlot(panelMerc.ubID),
              "live squad selection rebuilds its team-panel owner");

        // A post-load rebuild can retain a squad pointer after its runtime
        // MercPtrs owner was retired. Map detection, light restoration, and
        // panel reconstruction must all ignore that stale owner.
        MercPtrs[0]=nullptr;
        MercPtrs[1]=nullptr;
        const SoldierID previousSquadLightFirstID=gTacticalStatus.Team[gbPlayerNum].bFirstID;
        const SoldierID previousSquadLightLastID=gTacticalStatus.Team[gbPlayerNum].bLastID;
        gTacticalStatus.Team[gbPlayerNum].bFirstID=SoldierID(0);
        gTacticalStatus.Team[gbPlayerNum].bLastID=SoldierID(1);
        ExamineCurrentSquadLights();
        gTacticalStatus.Team[gbPlayerNum].bFirstID=previousSquadLightFirstID;
        gTacticalStatus.Team[gbPlayerNum].bLastID=previousSquadLightLastID;
        check(!IsSquadOnCurrentTacticalMap(0),
              "map detection rejects a squad whose runtime owner retired");
        RebuildCurrentSquad();
        check(CurrentSquad()==NO_CURRENT_SQUAD &&
              !PlayerExistsInSlot(panelMerc.ubID),
              "post-load rebuild retires a squad with no live owners");

        SetCurrentSquad(NO_CURRENT_SQUAD,TRUE);
        Squad[0][0]=nullptr;
        MercPtrs[0]=nullptr;
        gusSelectedSoldier=NOBODY;
        gGameOptions.ubSquadSize=previousLiveSquadSize;
        guiCurrentScreen=previousScreen;

        // Dead-squad state is retained across saves and can be queried while
        // soldier and squad ownership is being rebuilt. Invalid callers must
        // fail closed, while the final squad slot and the dead-entry cursor
        // retain their intended live behavior.
        check(!IsDeadGuyOnAnySquad(nullptr),
              "dead-squad lookup rejects a missing soldier");
        check(!AddDeadCharacterToSquadDeadGuys(nullptr,0),
              "dead-squad insertion rejects a missing soldier");
        check(!AddDeadCharacterToSquadDeadGuys(&panelMerc,-1) &&
              !AddDeadCharacterToSquadDeadGuys(&panelMerc,NUMBER_OF_SQUADS),
              "dead-squad insertion rejects invalid squad indices");
        INT8 deadCursor=0;
        check(!IsDeadGuyInThisSquadSlot(0,-1,&deadCursor) &&
              !IsDeadGuyInThisSquadSlot(0,0,nullptr),
              "dead-squad slot lookup rejects invalid ownership");
        check(!SoldierIsDeadAndWasOnSquad(nullptr,0) &&
              !SoldierIsDeadAndWasOnSquad(&panelMerc,-1),
              "dead-squad membership rejects invalid ownership");
        check(!ResetDeadSquadMemberList(-1) &&
              !ResetDeadSquadMemberList(NUMBER_OF_SQUADS),
              "dead-squad reset rejects invalid squad indices");
        check(!IsAnyMercOnSquadAsleep(static_cast<UINT8>(NUMBER_OF_SQUADS)),
              "squad sleep lookup rejects an invalid squad index");

        check(ResetDeadSquadMemberList(0),
              "valid dead-squad list resets");
        panelMerc.ubProfile=42;
        check(AddDeadCharacterToSquadDeadGuys(&panelMerc,0) &&
              IsDeadGuyOnAnySquad(&panelMerc) &&
              SoldierIsDeadAndWasOnSquad(&panelMerc,0),
              "valid dead-squad entry remains discoverable");
        deadCursor=0;
        check(IsDeadGuyInThisSquadSlot(0,0,&deadCursor) && deadCursor==1,
              "dead-squad slot lookup advances its caller-owned cursor");
        check(ResetDeadSquadMemberList(0),
              "dead-squad fixture resets after use");

        panelMerc.flags.fMercAsleep=TRUE;
        MercPtrs[0]=&panelMerc;
        Squad[1][NUMBER_OF_SOLDIERS_PER_SQUAD-1]=&panelMerc;
        check(IsAnyMercOnSquadAsleep(1),
              "squad sleep lookup includes the final squad slot");
        Squad[1][NUMBER_OF_SOLDIERS_PER_SQUAD-1]=nullptr;
        MercPtrs[0]=nullptr;
        panelMerc.flags.fMercAsleep=FALSE;

        // The strategic item-description close path must retire its controls
        // before unloading their images, even outside a button callback.
        gsCurInterfacePanel=TEAM_PANEL; gfInKeyRingPopup=FALSE;
        guiCurrentItemDescriptionScreen=MAP_SCREEN;
        OBJECTTYPE described;
        described.usItem=401; described.ubNumberOfObjects=1;
        Item[401].usItemClass=IC_AMMO;
        gpItemDescObject=&described;
        dynamicButton(giMapInvDescButtonImage,giMapInvDescButton);
        dynamicButton(giItemDescAmmoButtonImages,giItemDescAmmoButton);
        gfInItemDescBox=TRUE;
        DeleteItemDescriptionBox();
        check(!gfInItemDescBox && !gpItemDescObject,"description closure clears its state");
        check(std::none_of(ButtonList.begin(),ButtonList.end(),[](GUI_BUTTON* b){return b!=nullptr;}),
              "description closure removes its controls");
        check(std::none_of(ButtonPictures.begin(),ButtonPictures.end(),[](const BUTTON_PICS& p){return p.vobj!=nullptr;}),
              "description closure releases its images");
        cleanup();
        std::cout<<"PASS: "<<checks<<" production inventory-panel lifecycle assertions\n";
        return 0;
    } catch (const std::exception& e) {
        cleanup(); std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; return 1;
    }
}
