/*
 * P710 native snapshot update: tests/native-core/native_attachment_tooltip_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored attachment state through the production tooltip and popup routines.
#include "Interface.h"
#include "Interface Items.h"
#include "GameSettings.h"
#include "Items.h"
#include "Overhead.h"
#include "Soldier Control.h"
#include "Map Screen Interface.h"
#include "Map Screen Interface Map.h"
#include "Map Screen Interface Map Inventory.h"
#include "Text.h"
#include "mousesystem.h"
#include "popup_class.h"
#include <language.hpp>
#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <iostream>
#include <string>
#include <vector>

class PopupAttachmentInfo;
extern std::vector<PopupAttachmentInfo*> gPopupAttachmentInfos;
extern MOUSE_REGION gItemDescAttachmentRegions[MAX_ATTACHMENTS];
extern INV_ATTACHXY gItemDescAttachmentsXY[MAX_ATTACHMENTS];
extern BOOLEAN gItemDescAttachmentPopupsInitialized;
extern UINT32 guiCurrentItemDescriptionScreen;
extern OBJECTTYPE* gpItemDescObject;

namespace {
unsigned checks=0;
constexpr UINT16 rootItem=10, carrierItem=20, firstOption=100;

void check(bool value,const char* reason) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<reason<<'\n'; std::abort(); }
}

template<std::size_t N>
void name(CHAR16 (&destination)[N],const std::wstring& text) {
    check(text.size()<N,"authored name fits its production field");
    std::copy(text.begin(),text.end(),destination);
    destination[text.size()]=L'\0';
}

OBJECTTYPE object(UINT16 item) {
    OBJECTTYPE value;
    value.usItem=item;
    value.ubNumberOfObjects=1;
    return value;
}

void slot(UINT16 id,UINT64 attachmentClass) {
    AttachmentSlots[id]=AttachmentSlotStruct{};
    AttachmentSlots[id].uiSlotIndex=id;
    AttachmentSlots[id].nasAttachmentClass=attachmentClass;
    AttachmentSlots[id].nasLayoutClass=2;
}

void fixture(unsigned optionCount=40) {
    UpdateAttachmentTooltips(nullptr,0);
    AttachmentBackmap.clear();
    gMAXITEMS_READ=260;
    gMAXATTACHMENTS_READ=0;
    gMAXLAUNCHABLES_READ=0;
    for (UINT16 i=0;i<260;++i) {
        Item[i]=INVTYPE{};
        Item[i].usItemClass=IC_MISC;
        Item[i].ubCoolness=1;
        Item[i].nasLayoutClass=2;
        AttachmentSlots[i]=AttachmentSlotStruct{};
    }
    for (UINT32 i=0;i<MAX_ATTACHMENTS;++i) {
        gItemDescAttachmentsXY[i]=INV_ATTACHXY{};
        gItemDescAttachmentsXY[i].sX=20+10*i;
        gItemDescAttachmentsXY[i].sY=50;
        gItemDescAttachmentsXY[i].sWidth=8;
        gItemDescAttachmentsXY[i].sHeight=8;
    }
    Item[rootItem].ulAvailableAttachmentPoint=1;
    for (UINT16 i=firstOption;i<firstOption+optionCount;++i) {
        Item[i].usItemFlag=ITEM_attachment;
        Item[i].ulAttachmentPoint=1;
        Item[i].nasAttachmentClass=1;
        name(Item[i].szItemName,std::wstring(79,wchar_t(L'A'+i-firstOption)));
    }
    AttachmentStruct attachment{};
    attachment.itemIndex=rootItem;
    attachment.attachmentIndex=firstOption;
    AttachmentBackmap.emplace(rootItem,attachment);
    slot(1,1);
    bSelectedInfoChar=-1;
    guiCurrentItemDescriptionScreen=MAP_SCREEN;
    fShowMapInventoryPool=TRUE;
}

std::wstring tooltip(UINT32 index) {
    check(gItemDescAttachmentRegions[index].IDNumber!=0,"expected production region exists");
    check(gItemDescAttachmentRegions[index].FastHelpText!=nullptr,"production region owns tooltip text");
    return gItemDescAttachmentRegions[index].FastHelpText;
}

std::size_t regionCount() {
    return std::count_if(std::begin(gItemDescAttachmentRegions),std::end(gItemDescAttachmentRegions),
        [](const MOUSE_REGION& region){return region.IDNumber!=0;});
}

void longListsAndSelection() {
    fixture();
    auto root=object(rootItem); // Deliberately no attachment placeholders.
    static SOLDIERTYPE soldier;
    soldier.sSectorX=sSelMapX;
    soldier.sSectorY=sSelMapY;
    soldier.bSectorZ=iCurrentMapSectorZ;
    MercPtrs[7]=&soldier;
    gCharactersList[0].usSolID=SoldierID(7);
    gCharactersList[0].fValid=TRUE;
    bSelectedInfoChar=0;
    UpdateAttachmentTooltips(&root,0);
    auto text=tooltip(0);
    check(text.size()<=2499 && text.find(L"\n...")!=std::wstring::npos,
          "long names produce a bounded complete tooltip with ellipsis");
    check(text.find(Item[firstOption].szItemName)!=std::wstring::npos,"first name remains visible");
    check(gPopupAttachmentInfos.size()==40,"tooltip limit does not remove popup options");
    auto* popup=gItemDescAttachmentPopups[0];
    check(popup && popup->optionCount==11 && popup->subPopupOptionCount==1,
          "long list retains the production main/submenu split");
    check(popup->getSubPopupOption(0)->subPopup->optionCount==29,"submenu contains every remaining option");
    UpdateAttachmentTooltips(&root,0);
    check(gPopupAttachmentInfos.size()==40 && gPopupIndex.size()==2,
          "rebuild replaces popup metadata and owners without accumulating entries");

    const std::wstring originalHeading=Message[STR_ATTACHMENTS];
    const std::wstring heading(254,L'H');
    name(Message[STR_ATTACHMENTS],heading);
    UpdateAttachmentTooltips(&root,0);
    text=tooltip(0);
    check(text.size()<=2499 && text.compare(0,heading.size(),heading)==0,
          "localized heading participates in the complete tooltip limit");
    check(gPopupAttachmentInfos.size()==40,"long localized heading preserves all popup options");
    name(Message[STR_ATTACHMENTS],originalHeading);

    for (INT16 selected:{INT16(-1),INT16(CODE_MAXIMUM_NUMBER_OF_PLAYER_SLOTS)}) {
        bSelectedInfoChar=selected;
        UpdateAttachmentTooltips(&root,0);
        check(!tooltip(0).empty() && gPopupAttachmentInfos.empty(),"invalid selection keeps text without a popup");
    }
    bSelectedInfoChar=0;
    gCharactersList[0].fValid=FALSE;
    UpdateAttachmentTooltips(&root,0);
    check(gPopupAttachmentInfos.empty(),"invalid map entry does not create a popup");
    gCharactersList[0].fValid=TRUE;
    gCharactersList[0].usSolID=SoldierID{};
    UpdateAttachmentTooltips(&root,0);
    check(gPopupAttachmentInfos.empty(),"invalid soldier ID is checked before pointer conversion");
    gCharactersList[0].usSolID=SoldierID(7);
    MercPtrs[7]=nullptr;
    UpdateAttachmentTooltips(&root,0);
    check(gPopupAttachmentInfos.empty(),"null soldier does not create a popup");
    guiCurrentItemDescriptionScreen=GAME_SCREEN;
    bSelectedInfoChar=-1;
    UpdateAttachmentTooltips(&root,0);
    check(!tooltip(0).empty(),"tactical tooltip does not require a map selection");
}

void slotAndStackBounds() {
    fixture();
    auto root=object(rootItem);
    Item[rootItem].ulAvailableAttachmentPoint=0;
    name(AttachmentSlots[1].szSlotName,std::wstring(199,L'S'));
    UpdateAttachmentTooltips(&root,0);
    check(tooltip(0)==L"\n"+std::wstring(199,L'S'),"199-character slot name remains complete");

    for (UINT16 i=2;i<=33;++i) slot(i,1);
    check(GetItemSlots(&root,0).size()==33,"production layout returns more than 32 slots");
    UpdateAttachmentTooltips(&root,0);
    check(regionCount()==MAX_ATTACHMENTS,"UI limits the authored 33-slot layout to its actual capacity");

    fixture();
    root=object(rootItem);
    root.objectStack.resize(2);
    root.ubNumberOfObjects=2;
    root[0]->attachments.resize(1);
    auto carrier=object(carrierItem);
    carrier[0]->attachments.resize(1);
    Item[carrierItem].usItemFlag=ITEM_attachment;
    Item[carrierItem].ulAvailableAttachmentPoint=2;
    name(ItemNames[carrierItem],L"Authored carrier");
    Item[140].usItemFlag=ITEM_attachment;
    Item[140].ulAttachmentPoint=2;
    Item[140].nasAttachmentClass=2;
    name(Item[140].szItemName,L"Selected subobject attachment");
    AttachmentStruct attachment{};
    attachment.itemIndex=carrierItem;
    attachment.attachmentIndex=140;
    AttachmentBackmap.emplace(carrierItem,attachment);
    slot(2,2);
    root[1]->attachments.push_back(carrier);
    check(GetItemSlots(&root,0).size()==1 && GetItemSlots(&root,1).size()==2,
          "authored stacked subobjects have distinct production layouts");
    UpdateAttachmentTooltips(&root,1);
    check(regionCount()==2 && tooltip(1).find(Item[140].szItemName)!=std::wstring::npos,
          "selected subobject supplies its extra slot and available attachment point");
    check(MSYS_GetRegionUserData(&gItemDescAttachmentRegions[1],1)==1,
          "selected subobject remains bound to the attachment region");
    UpdateAttachmentTooltips(&root,0);
    check(regionCount()==1,"switching subobjects retires the extra region");
    UpdateAttachmentTooltips(&root,2);
    check(regionCount()==0 && gPopupAttachmentInfos.empty(),"invalid subobject clears retired UI state");
}

void rejectedOptionOwnership() {
    fixture(110);
    auto root=object(rootItem);
    static SOLDIERTYPE soldier;
    soldier.sSectorX=sSelMapX;
    soldier.sSectorY=sSelMapY;
    soldier.bSectorZ=iCurrentMapSectorZ;
    MercPtrs[7]=&soldier;
    gCharactersList[0].usSolID=SoldierID(7);
    gCharactersList[0].fValid=TRUE;
    bSelectedInfoChar=0;
    for (unsigned rebuild=0;rebuild<3;++rebuild) {
        UpdateAttachmentTooltips(&root,0);
        auto* popup=gItemDescAttachmentPopups[0];
        check(popup && popup->optionCount==11 && popup->subPopupOptionCount==1,
              "capacity fixture retains its production main menu");
        check(popup->getSubPopupOption(0)->subPopup->optionCount==POPUP_MAX_OPTIONS,
              "production submenu preserves its existing capacity");
        check(gPopupAttachmentInfos.size()==11+POPUP_MAX_OPTIONS,
              "only accepted options retain borrowed metadata");
    }
    UpdateAttachmentTooltips(nullptr,0);
    check(gPopupAttachmentInfos.empty() && gPopupIndex.empty(),
          "over-capacity rebuilds release all owners; strict LSan also checks rejected allocations");
}

void descriptionClosure() {
    fixture();
    auto root=object(rootItem);
    static SOLDIERTYPE soldier;
    soldier.sSectorX=sSelMapX;
    soldier.sSectorY=sSelMapY;
    soldier.bSectorZ=iCurrentMapSectorZ;
    MercPtrs[7]=&soldier;
    gCharactersList[0].usSolID=SoldierID(7);
    gCharactersList[0].fValid=TRUE;
    bSelectedInfoChar=0;
    UpdateAttachmentTooltips(&root,0);
    check(gPopupAttachmentInfos.size()==40,"description closure starts with real popup metadata");
    gpItemDescObject=&root;
    gfInItemDescBox=TRUE;
    gTacticalStatus.uiFlags=0;
    DeleteItemDescriptionBox();
    check(!gfInItemDescBox && regionCount()==0 && gPopupAttachmentInfos.empty() && gPopupIndex.empty(),
          "actual description closure releases regions, popups, and borrowed metadata");
    check(!gItemDescAttachmentPopupsInitialized && giActiveAttachmentPopup==-1,
          "description closure resets popup handles before callbacks retire");
}
}

int main() {
    MSYS_Init();
    BindLanguageStrings(i18n::Lang::en);
    gGameOptions.ubAttachmentSystem=ATTACHMENT_NEW;
    gGameOptions.ubInventorySystem=INVENTORY_OLD;
    gGameSettings.fOptions[TOPTION_ENHANCED_DESC_BOX]=FALSE;
    gGameExternalOptions.iEnhancedDescriptionBox=0;
    longListsAndSelection();
    slotAndStackBounds();
    rejectedOptionOwnership();
    descriptionClosure();
    UpdateAttachmentTooltips(nullptr,0);
    MSYS_Shutdown();
    std::cout<<"PASS: "<<checks<<" actual attachment tooltip and popup assertions\n";
}
