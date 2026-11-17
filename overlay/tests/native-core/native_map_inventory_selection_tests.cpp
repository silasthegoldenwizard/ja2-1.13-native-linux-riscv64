/*
 * P710 native snapshot update: tests/native-core/native_map_inventory_selection_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise sector inventory selection guards and Delete-click stack consistency.
#include "Button System.h"
#include "english.h"
#include "GameSettings.h"
#include "input.h"
#include "Interface Items.h"
#include "Item Types.h"
#include "Map Screen Interface.h"
#include "Map Screen Interface Map.h"
#include "Map Screen Interface Map Inventory.h"
#include "Overhead.h"
#include "World Items.h"
#include "mapscreen.h"
#include "mousesystem.h"
#include <iostream>
#include <stdexcept>

extern MOUSE_REGION gMPanelRegion;
extern UINT32 guiMapInvenArrowButton[2];
extern UINT32 guiMapInvenDoneButton;
extern UINT32 guiMapInvenSortButton[4];
extern BOOLEAN fShowInventoryFlag;
void MapInvenPoolSlots(MOUSE_REGION*, INT32);

namespace {
unsigned checks=0;
void check(bool value,const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
INT32 button() {
    const INT32 id=CreateHotSpot(2,2,8,8,63,nullptr,nullptr);
    check(id>=0,"create asset-free inventory control");
    return id;
}
void enableSortButtons() {
    for (UINT32 id:guiMapInvenSortButton) EnableButton(static_cast<INT32>(id));
}
void checkSortButtonsDisabled() {
    for (UINT32 id:guiMapInvenSortButton) {
        const GUI_BUTTON* control=GetButtonPtr(static_cast<INT32>(id));
        check(control && !(control->uiFlags&BUTTON_ENABLED),"no-selection inventory keeps sorting disabled");
    }
}
class HeldDelete {
    BOOLEAN oldDelete;
    BOOLEAN oldShift;
    BOOLEAN oldY;
public:
    HeldDelete()
        : oldDelete(gfKeyState[DEL]), oldShift(gfKeyState[SHIFT]), oldY(gfKeyState[89]) {
        gfKeyState[DEL]=TRUE;
        gfKeyState[SHIFT]=FALSE;
        gfKeyState[89]=FALSE;
    }
    ~HeldDelete() {
        gfKeyState[DEL]=oldDelete;
        gfKeyState[SHIFT]=oldShift;
        gfKeyState[89]=oldY;
    }
};
void checkDeleteThenAppend(MOUSE_REGION& slot) {
    OBJECTTYPE& stack=pInventoryPoolList[0].object;
    stack.objectStack.resize(2);
    stack.usItem=1;
    stack.ubNumberOfObjects=2;
    stack.objectStack.front().data.sObjectFlag=11;
    stack.objectStack.back().data.sObjectFlag=22;
    check(stack[0]->data.sObjectFlag==11 && stack[1]->data.sObjectFlag==22,
          "Delete fixture has two distinguishable objects");
    {
        HeldDelete deleteKey;
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_LBUTTON_UP);
    }
    check(stack.exists() && stack.ubNumberOfObjects==1,
          "ordinary Delete-click removes one object");
    check(static_cast<std::size_t>(stack.ubNumberOfObjects)==stack.objectStack.size(),
          "Delete-click keeps count equal to live nodes");
    check(stack[0]->data.sObjectFlag==11,
          "Delete-click preserves the first object");

    OBJECTTYPE incoming;
    incoming.objectStack.resize(1);
    incoming.usItem=stack.usItem;
    incoming.ubNumberOfObjects=1;
    incoming.objectStack.front().data.sObjectFlag=33;
    stack.ForceAddObjectsToStack(incoming,1);
    check(!incoming.exists(),"stack append consumes the incoming object");
    check(stack.ubNumberOfObjects==2 &&
          static_cast<std::size_t>(stack.ubNumberOfObjects)==stack.objectStack.size(),
          "stack append keeps count equal to live nodes");
    check(stack[0]->data.sObjectFlag==11 && stack[1]->data.sObjectFlag==33,
          "the survivor and appended object remain accessible");
    for (const auto& object:stack.objectStack) {
        check(object.data.sObjectFlag!=22,"the deleted object cannot return after append");
    }
    {
        HeldDelete deleteKey;
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_LBUTTON_UP);
    }
    check(stack.exists() && stack.ubNumberOfObjects==1 && stack.objectStack.size()==1 &&
          stack[0]->data.sObjectFlag==11,
          "Delete-click after append preserves the original survivor");
    {
        HeldDelete deleteKey;
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_LBUTTON_UP);
    }
    check(!stack.exists() && stack.ubNumberOfObjects==0 && stack.objectStack.size()==1,
          "ordinary Delete-click initializes the final singleton stack");
}
}

int main() {
    try {
        check(MSYS_Init()==1,"initialize mouse regions");
        check(InitButtonSystem(),"initialize button manager");
        for (UINT32& id:guiMapInvenArrowButton) id=static_cast<UINT32>(button());
        guiMapInvenDoneButton=static_cast<UINT32>(button());
        for (UINT32& id:guiMapInvenSortButton) id=static_cast<UINT32>(button());

        iCurrentInventoryPoolPage=0;
        iLastInventoryPoolPage=0;
        fShowMapInventoryPool=TRUE;
        gMPanelRegion.Cursor=MSYS_NO_CURSOR;
        bSelectedInfoChar=-1;
        HandleButtonStatesWhileMapInventoryActive();
        checkSortButtonsDisabled();
        check(GetButtonPtr(static_cast<INT32>(guiMapInvenDoneButton))->uiFlags&BUTTON_ENABLED,
              "no-selection inventory remains closable");

        enableSortButtons();
        bSelectedInfoChar=static_cast<INT16>(giMAXIMUM_NUMBER_OF_PLAYER_SLOTS);
        HandleButtonStatesWhileMapInventoryActive();
        checkSortButtonsDisabled();

        enableSortButtons();
        bSelectedInfoChar=0;
        gCharactersList[0].fValid=FALSE;
        HandleButtonStatesWhileMapInventoryActive();
        checkSortButtonsDisabled();

        const INT32 oldSlotCount=MAP_INVENTORY_POOL_SLOT_COUNT;
        const INT32 oldPage=iCurrentInventoryPoolPage;
        MAP_INVENTORY_POOL_SLOT_COUNT=1;
        iCurrentInventoryPoolPage=0;
        pInventoryPoolList.clear();
        pInventoryPoolList.resize(1);
        MOUSE_REGION slot{};
        slot.UserData[0]=0;
        bSelectedInfoChar=-1;
        const BOOLEAN oldSectorDescription=gGameExternalOptions.fSectorDesc;
        gGameExternalOptions.fSectorDesc=TRUE;
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_RBUTTON_UP);
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_LBUTTON_UP);
        gGameExternalOptions.fSectorDesc=oldSectorDescription;
        check(pInventoryPoolList.size()==1 && !pInventoryPoolList[0].object.exists(),
              "no-selection slot actions preserve the sector pool");

        checkDeleteThenAppend(slot);

        MAP_INVENTORY_POOL_SLOT_COUNT=40;
        iCurrentInventoryPoolPage=3;
        pInventoryPoolList.resize(10);
        bSelectedInfoChar=0;
        gCharactersList[0].fValid=TRUE;
        gCharactersList[0].usSolID=SoldierID(0);
        Menptr[0].initialize();
        Menptr[0].ubID=SoldierID(0);
        Menptr[0].bActive=TRUE;
        Menptr[0].stats.bLife=80;
        Menptr[0].sSectorX=sSelMapX=1;
        Menptr[0].sSectorY=sSelMapY=1;
        Menptr[0].bSectorZ=iCurrentMapSectorZ=0;
        MercPtrs[0]=&Menptr[0];
        check(!HandleCompatibleAmmoUIForMapInventory(&Menptr[0],NO_SLOT,0,TRUE,TRUE),
              "a missing merc inventory slot has no compatibility highlights");
        slot.UserData[0]=35;
        MapInvenPoolSlots(&slot,MSYS_CALLBACK_REASON_RBUTTON_UP);
        check(pInventoryPoolList.size()==10,
              "stale page and slot state cannot read beyond the sector pool");

        fShowInventoryFlag=TRUE;
        bSelectedInfoChar=0;
        ChangeSelectedInfoChar(-1,TRUE);
        check(!fShowInventoryFlag && bSelectedInfoChar==-1,
              "sorting can temporarily deselect the merc while closing map inventory");
        gCharactersList[0].fValid=FALSE;
        Menptr[0].initialize();
        MAP_INVENTORY_POOL_SLOT_COUNT=oldSlotCount;
        iCurrentInventoryPoolPage=oldPage;

        // Loading and sector transitions can update the public count before
        // the backing vector is rebuilt. The information hotkey must tolerate
        // that transient mismatch instead of dereferencing an empty vector.
        gWorldItems.clear();
        guiNumWorldItems=1;
        check(DisplaySectorItemsInfo(),
              "sector-item information tolerates an empty rebuilt world-item vector");
        guiNumWorldItems=0;

        ShutdownButtonSystem();
        MSYS_Shutdown();
        std::cout<<"PASS: "<<checks<<" sector-inventory assertions\n";
        return 0;
    } catch (const std::exception& error) {
        ShutdownButtonSystem();
        MSYS_Shutdown();
        std::cerr<<"FAIL after "<<checks<<" assertions: "<<error.what()<<'\n';
        return 1;
    }
}
