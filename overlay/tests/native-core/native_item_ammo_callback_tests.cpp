/*
 * P710 native snapshot update: tests/native-core/native_item_ammo_callback_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_item_ammo_callback_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-17.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_item_ammo_callback_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Real map-inventory ammunition callback, with authored weapon/ammo state.
#include "Button System.h"
#include "CampaignStats.h"
#include "Interface Items.h"
#include "Interface Control.h"
#include "Interface.h"
#include "Items.h"
#include "Map Screen Interface Map Inventory.h"
#include "Weapons.h"
#include "Soldier Control.h"
#include "jascreens.h"
#include <cstdlib>
#include <iostream>
#include <memory>

extern OBJECTTYPE* gpItemDescObject;
extern SOLDIERTYPE* gpItemDescSoldier;
extern OBJECTTYPE gTempObject;
extern UINT32 guiCurrentItemDescriptionScreen;
extern UINT8 gubItemDescStatusIndex;
extern INT32 giItemDescAmmoButton;
extern BOOLEAN fShowMapInventoryPool, fShowInventoryFlag;
extern BOOLEAN fMapInventoryItem;
extern BOOLEAN gfItemDescObjectIsAttachment;
extern BOOLEAN gfAddingMoneyToMercFromPlayersAccount;
extern BOOLEAN gfInItemStackPopup;
extern BOOLEAN gfInSectorStackPopup;
extern BOOLEAN gfInKeyRingPopup;
extern BOOLEAN gfInItemPickupMenu;
extern BOOLEAN gfItemPopupRegionCallbackEndFix;
extern REMOVE_MONEY gRemoveMoney;
extern OBJECTTYPE* gpItemPopupObject;
extern SOLDIERTYPE* gpItemPopupSoldier;
extern void ItemDescAmmoCallback(GUI_BUTTON*,INT32);
extern void popupCallbackAmmo(UINT16,UINT16,SOLDIERTYPE*);
extern void BtnMoneyButtonCallback(GUI_BUTTON*,INT32);
extern void RemoveMoney();
extern void ItemPopupRegionCallback(MOUSE_REGION*,INT32);
extern void ItemPopupFullRegionCallback(MOUSE_REGION*,INT32);
extern void KeyRingSlotInvClickCallback(MOUSE_REGION*,INT32);
extern void ItemPickMenuMouseClickCallback(MOUSE_REGION*,INT32);
extern void ItemPickMenuMouseMoveCallback(MOUSE_REGION*,INT32);
extern void ItemPickupBackgroundClick(MOUSE_REGION*,INT32);
extern void ItemPickupAll(GUI_BUTTON*,INT32);
extern void ItemPickupOK(GUI_BUTTON*,INT32);
extern void ItemPickupCancel(GUI_BUTTON*,INT32);
extern void ItemPickupScrollUp(GUI_BUTTON*,INT32);
extern void ItemPickupScrollDown(GUI_BUTTON*,INT32);
extern void fillLBEWithSectorItems(OBJECTTYPE*);
extern void DoAttachment(UINT8,INT32);
extern void PermanantAttachmentMessageBoxCallBack(UINT8);
extern void ItemDescAttachmentsCallback(MOUSE_REGION*,INT32);
extern void ItemDescCallback(MOUSE_REGION*,INT32);
extern void ItemDescDoneButtonCallback(GUI_BUTTON*,INT32);
extern void ItemDescTabButtonCallback(GUI_BUTTON*,INT32);
extern void ItemDescAdvButtonCallback(GUI_BUTTON*,INT32);
extern void ItemDescTransformRegionCallback(MOUSE_REGION*,INT32);
extern void TransformationMenuPopup_Transform(TransformInfoStruct*);
extern BOOLEAN TransformationMenuPopup_TestValid(TransformInfoStruct*);
extern void TransformationMenuPopup_Arm(OBJECTTYPE*);
extern BOOLEAN TransformationMenuPopup_Arm_TestValid(OBJECTTYPE*);
extern void BombInventoryMessageBoxCallBack(UINT8);
extern void BombInventoryDisArmMessageBoxCallBack(UINT8);
extern void TransformationMenuPopup_SplitCrate(UINT16);
extern void TransformationMenuPopup_SplitCrateInInventory();
extern void TransformFromItemDescBox(TransformInfoStruct*);
extern void ConfirmTransformationMessageBoxCallBack(UINT8);
extern BOOLEAN TransformationMenuPopup_DelayedGrenadeExplosion_TestValid(OBJECTTYPE*);
extern void TransformationMenuPopup_DelayedGrenadeExplosion();
extern TransformInfoStruct* gTransformInProgress;
extern UINT32 guiTransformInProgressPrevScreen;

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<why<<'\n'; std::abort(); }
}
constexpr UINT16 gunItem=400,ammoItem=401,kitItem=402;
constexpr UINT16 attachmentHostItem=403,launcherItem=404;
constexpr UINT16 mergeHostItem=405,mergeToolItem=406,mergeResultItem=407;
constexpr UINT16 missingLbeItem=408;
constexpr UINT16 lbePayloadItem=409;
void callback() {
    GUI_BUTTON button{};
    ItemDescAmmoCallback(&button,MSYS_CALLBACK_REASON_LBUTTON_DWN);
    ItemDescAmmoCallback(&button,MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(!(button.uiFlags&BUTTON_CLICKED_ON),"ammo button releases");
}
}
int main() {
    auto soldier=std::make_unique<SOLDIERTYPE>();
    soldier->bTeam=ENEMY_TEAM; soldier->bActionPoints=100;
    soldier->stats.bLife=100; soldier->bBreath=100;

    // Popup actions can remain queued after their pocket selection has been
    // retired.  An invalid slot must cancel before indexing the merc inventory.
    popupCallbackAmmo(ammoItem,static_cast<UINT16>(NO_SLOT),soldier.get());
    popupCallbackAmmo(ammoItem,NUM_INV_SLOTS,soldier.get());
    popupCallbackAmmo(ammoItem,0,nullptr);
    check(!soldier->inv[HANDPOS].exists(),
          "retired ammunition-popup slots are rejected before inventory access");

    // Mouse and button events can remain queued after the tactical pickup menu
    // has destroyed its regions, selection array, and owning soldier. Every
    // callback must reject that retired state before reading event payloads.
    gfInItemPickupMenu=FALSE;
    ItemPickMenuMouseClickCallback(nullptr,MSYS_CALLBACK_REASON_LBUTTON_UP);
    ItemPickMenuMouseMoveCallback(nullptr,MSYS_CALLBACK_REASON_MOVE);
    ItemPickupBackgroundClick(nullptr,MSYS_CALLBACK_REASON_RBUTTON_UP);
    using PickupButtonCallback = void (*)(GUI_BUTTON*,INT32);
    const PickupButtonCallback stalePickupCallbacks[] = {
        ItemPickupAll, ItemPickupOK, ItemPickupCancel,
        ItemPickupScrollUp, ItemPickupScrollDown,
    };
    for (PickupButtonCallback pickupCallback:stalePickupCallbacks) {
        GUI_BUTTON staleButton{};
        staleButton.uiFlags=BUTTON_CLICKED_ON;
        pickupCallback(&staleButton,MSYS_CALLBACK_REASON_LBUTTON_UP);
        check(!(staleButton.uiFlags&BUTTON_CLICKED_ON),
              "retired pickup-menu button releases without using destroyed state");
    }
    Item[gunItem].usItemClass=IC_GUN; Item[gunItem].ubClassIndex=gunItem;
    Item[ammoItem].usItemClass=IC_AMMO; Item[ammoItem].ubClassIndex=ammoItem;
    Weapon[gunItem].ubCalibre=1; Weapon[gunItem].ubMagSize=30;
    Weapon[gunItem].swapClips=1;
    Magazine[ammoItem].ubCalibre=1; Magazine[ammoItem].ubMagSize=30;
    Magazine[ammoItem].ubMagType=0;
    OBJECTTYPE gun;
    gun.usItem=gunItem; gun.ubNumberOfObjects=1;
    gpItemDescObject=&gun; gpItemDescSoldier=soldier.get();
    gubItemDescStatusIndex=0;
    guiCurrentItemDescriptionScreen=MAP_SCREEN;
    giItemDescAmmoButton=-1; gfInItemDescBox=FALSE;
    fShowMapInventoryPool=FALSE; fShowInventoryFlag=FALSE;

    // Malformed or incomplete XML can expose an ammunition entry with no
    // magazine capacity.  Crate splitting must reject it before division.
    OBJECTTYPE zeroSizeCrate;
    zeroSizeCrate.usItem=ammoItem;
    zeroSizeCrate.ubNumberOfObjects=1;
    zeroSizeCrate[0]->data.ubShotsLeft=30;
    gpItemDescObject=&zeroSizeCrate;
    Magazine[ammoItem].ubMagSize=0;
    TransformationMenuPopup_SplitCrate(ammoItem);
    check(zeroSizeCrate.exists() && zeroSizeCrate[0]->data.ubShotsLeft==30,
          "zero-size magazine definition cancels crate splitting safely");
    Magazine[ammoItem].ubMagSize=30;
    gpItemDescObject=&gun;

    // A whole magazine moves into an empty rifle and clears the cursor.
    gItemPointer.initialize(); gItemPointer.usItem=ammoItem; gItemPointer.ubNumberOfObjects=1;
    gItemPointer[0]->data.ubShotsLeft=30; gpItemPointer=&gItemPointer;
    callback();
    check(gun[0]->data.gun.ubGunShotsLeft==30,"all thirty rounds reload");
    check(gpItemPointer==nullptr && !gItemPointer.exists(),"consumed magazine clears cursor");

    // Unload the same rifle, then reload its magazine through the same UI path.
    callback();
    check(gpItemPointer && gpItemPointer->exists(),"unload puts magazine on cursor");
    check(gun[0]->data.gun.ubGunShotsLeft==0 && (*gpItemPointer)[0]->data.ubShotsLeft==30,
          "unload conserves rounds");
    callback();
    check(!gpItemPointer && gun[0]->data.gun.ubGunShotsLeft==30,"reload after unload clears cursor");

    // An empty rifle with no magazine leaves the cursor empty.
    gun[0]->data.gun.ubGunShotsLeft=0; gTempObject.initialize(); gpItemPointer=nullptr;
    callback();
    check(gpItemPointer==nullptr && !gTempObject.exists(),"empty unload leaves empty cursor");

    // One magazine remains on the cursor when reloading from a stack.
    gItemPointer.initialize(); gItemPointer.usItem=ammoItem;
    gItemPointer.ubNumberOfObjects=2; gItemPointer.objectStack.resize(2);
    gItemPointer[0]->data.ubShotsLeft=30; gItemPointer[1]->data.ubShotsLeft=30;
    gpItemPointer=&gItemPointer;
    callback();
    check(gun[0]->data.gun.ubGunShotsLeft==30,"stack reload fills rifle");
    check(gpItemPointer==&gItemPointer && gpItemPointer->ubNumberOfObjects==1 &&
          (*gpItemPointer)[0]->data.ubShotsLeft==30,"stack reload retains remaining magazine");

    // An incompatible calibre must preserve the cursor magazine.
    gun[0]->data.gun.ubGunShotsLeft=0; Magazine[ammoItem].ubCalibre=2;
    callback();
    check(gun[0]->data.gun.ubGunShotsLeft==0 && gpItemPointer==&gItemPointer &&
          (*gpItemPointer)[0]->data.ubShotsLeft==30,"incompatible magazine remains unchanged");

    // Splitting money begins with an empty cursor in the normal map-inventory flow.
    const UINT32 previousMaxItems = gMAXITEMS_READ;
    gMAXITEMS_READ = MAXITEMS;
    OBJECTTYPE money;
    CreateMoney(1000,&money);
    gpItemDescObject=&money;
    gpItemPointer=nullptr;
    gItemPointer.initialize();
    guiTacticalInterfaceFlags=INTERFACE_MAPSCREEN;
    guiCurrentItemDescriptionScreen=MAP_SCREEN;
    gfAddingMoneyToMercFromPlayersAccount=FALSE;
    gRemoveMoney.uiTotalAmount=1000;
    gRemoveMoney.uiMoneyRemaining=700;
    gRemoveMoney.uiMoneyRemoving=300;
    fMapInventoryItem=FALSE;
    RemoveMoney();
    check(gpItemPointer==&gItemPointer && gpItemPointer->usItem==MONEY &&
          (*gpItemPointer)[0]->data.money.uiMoneyAmount==300,
          "splitting money creates the requested cursor amount from an empty cursor");
    check((*gpItemDescObject)[0]->data.money.uiMoneyAmount==700 && fMapInventoryItem,
          "splitting money preserves the remaining amount and begins the map cursor");

    // A drain-reduction item that is exhausted must convert its remaining
    // status to usable points before dividing. The legacy expression divided
    // by (80 / 100), which is zero in integer arithmetic.
    Item[kitItem].usItemClass=IC_KIT;
    Item[kitItem].percentstatusdrainreduction=20;
    OBJECTTYPE kit;
    kit.usItem=kitItem;
    kit.ubNumberOfObjects=1;
    kit[0]->data.objectStatus=50;
    const UINT16 usedKitPoints=UseKitPoints(&kit,100,soldier.get());
    check(usedKitPoints==62,"drain reduction converts fifty status to sixty-two usable points");
    check(!kit.exists(),"fully consumed reduced-drain kit is removed");

    // Full drain reduction is still a successful tracked use. It preserves
    // the kit condition but must follow the normal campaign-accounting path.
    Item[kitItem].usItemClass=IC_KIT;
    Item[kitItem].usItemFlag=CLEANING_KIT;
    Item[kitItem].ubWeight=200;
    Item[kitItem].percentstatusdrainreduction=100;
    OBJECTTYPE zeroDrainKit;
    zeroDrainKit.usItem=kitItem;
    zeroDrainKit.ubNumberOfObjects=1;
    zeroDrainKit[0]->data.objectStatus=50;
    gCampaignStats.usConsumed[CAMPAIGN_CONSUMED_REPAIR]=0.0f;
    check(UseKitPoints(&zeroDrainKit,25,soldier.get())==25,
          "complete drain reduction returns every requested cleaning point");
    check(zeroDrainKit.exists() && zeroDrainKit[0]->data.objectStatus==50,
          "complete drain reduction preserves cleaning-kit condition");
    check(gCampaignStats.usConsumed[CAMPAIGN_CONSUMED_REPAIR]==50.0f,
          "complete drain reduction records successful repair consumption");
    OBJECTTYPE exhaustedZeroDrainKit;
    exhaustedZeroDrainKit.usItem=kitItem;
    exhaustedZeroDrainKit.ubNumberOfObjects=1;
    exhaustedZeroDrainKit[0]->data.objectStatus=0;
    check(UseKitPoints(&exhaustedZeroDrainKit,25,soldier.get())==0,
          "exhausted complete-reduction kit supplies no points without division");
    check(!exhaustedZeroDrainKit.exists() &&
          gCampaignStats.usConsumed[CAMPAIGN_CONSUMED_REPAIR]==50.0f,
          "exhausted complete-reduction kit retires without changing accounting");

    // The aggregate medical-kit path uses the same conversion and must report
    // that the remaining sixty-two points cannot satisfy a request for one hundred.
    Item[kitItem].usItemClass=IC_MEDKIT;
    Item[kitItem].usItemFlag=0;
    Item[kitItem].percentstatusdrainreduction=20;
    Item[kitItem].usItemFlag2=ITEM_medicalkit;
    soldier->inv[HANDPOS].usItem=kitItem;
    soldier->inv[HANDPOS].ubNumberOfObjects=1;
    soldier->inv[HANDPOS][0]->data.objectStatus=50;
    check(!UseTotalMedicalKitPoints(soldier.get(),100),
          "aggregate medical-kit use reports an unsatisfied remainder");
    check(!soldier->inv[HANDPOS].exists(),"aggregate use removes the exhausted kit");

    // A one-hundred-percent reduction is an explicit no-drain case, not a zero divisor.
    Item[kitItem].percentstatusdrainreduction=100;
    soldier->inv[HANDPOS].usItem=kitItem;
    soldier->inv[HANDPOS].ubNumberOfObjects=1;
    soldier->inv[HANDPOS][0]->data.objectStatus=50;
    check(UseTotalMedicalKitPoints(soldier.get(),100),
          "complete drain reduction satisfies the aggregate request");
    check(soldier->inv[HANDPOS].exists() &&
          soldier->inv[HANDPOS][0]->data.objectStatus==50,
          "complete drain reduction preserves kit status");

    // RemoveAttachment explicitly accepts a null output pointer to delete the
    // removed object. A grenade-launcher attachment must follow that contract
    // without querying the absent output object afterward.
    Item[attachmentHostItem].usItemClass=IC_MISC;
    Item[launcherItem].usItemClass=IC_LAUNCHER;
    Item[launcherItem].usItemFlag=ITEM_grenadelauncher;
    OBJECTTYPE attachmentHost;
    attachmentHost.usItem=attachmentHostItem;
    attachmentHost.ubNumberOfObjects=1;
    OBJECTTYPE launcher;
    launcher.usItem=launcherItem;
    launcher.ubNumberOfObjects=1;
    attachmentHost[0]->attachments.push_back(launcher);
    OBJECTTYPE* attachedLauncher=&attachmentHost[0]->attachments.back();
    check(attachmentHost.RemoveAttachment(attachedLauncher),
          "null-output attachment removal succeeds");
    check(attachmentHost[0]->attachments.empty(),
          "null-output attachment removal deletes the launcher");

    // Returning a grenade launcher is also valid when the host has no grenade.
    // FindAttachmentByClass then returns null and must not be dereferenced.
    OBJECTTYPE launcherHostWithoutGrenade;
    launcherHostWithoutGrenade.usItem=attachmentHostItem;
    launcherHostWithoutGrenade.ubNumberOfObjects=1;
    launcherHostWithoutGrenade[0]->attachments.push_back(launcher);
    OBJECTTYPE* launcherWithoutGrenade=
        &launcherHostWithoutGrenade[0]->attachments.back();
    OBJECTTYPE returnedLauncher;
    check(launcherHostWithoutGrenade.RemoveAttachment(
              launcherWithoutGrenade,&returnedLauncher),
          "grenade launcher removal succeeds without a grenade attachment");
    check(returnedLauncher.exists() && returnedLauncher.usItem==launcherItem,
          "grenade launcher removal returns the launcher object");
    check(launcherHostWithoutGrenade[0]->attachments.empty(),
          "grenade launcher removal leaves the host attachment list empty");

    // USE_ITEM_NEW is valid without a mercenary in sector inventory. If the
    // produced item cannot enter a merc inventory, the merge must not read a
    // grid number through the absent soldier pointer.
    Item[mergeHostItem].usItemClass=IC_MISC;
    Item[mergeToolItem].usItemClass=IC_MISC;
    Item[mergeResultItem].usItemClass=IC_MISC;
    Merge[0][0]=mergeToolItem;
    Merge[0][1]=mergeHostItem;
    Merge[0][2]=mergeResultItem;
    Merge[0][3]=NOTHING;
    Merge[0][4]=USE_ITEM_NEW;
    Merge[0][5]=0;
    Merge[1][0]=0;
    guiCurrentScreen=MAP_SCREEN;
    fShowMapInventoryPool=TRUE;
    MAP_INVENTORY_POOL_SLOT_COUNT=40;
    pInventoryPoolList.clear();
    OBJECTTYPE mergeHost;
    mergeHost.usItem=mergeHostItem;
    mergeHost.ubNumberOfObjects=1;
    mergeHost[0]->data.objectStatus=2;
    OBJECTTYPE mergeTool;
    mergeTool.usItem=mergeToolItem;
    mergeTool.ubNumberOfObjects=1;
    check(mergeHost.AttachObjectNAS(nullptr,&mergeTool,FALSE),
          "soldierless sector merge creates its result safely");
    check(mergeHost[0]->data.objectStatus==1,
          "soldierless sector merge consumes one source point");
    bool retainedMergeResult=false;
    for (const WORLDITEM& worldItem:pInventoryPoolList) {
        if (worldItem.fExists && worldItem.object.usItem==mergeResultItem) {
            retainedMergeResult=true;
            break;
        }
    }
    check(retainedMergeResult,"soldierless sector merge retains its generated item");

    // A full tripwire roll reports failure without trying to play a mercenary
    // sound when the merge came from sector inventory and has no soldier.
    Item[mergeHostItem].usItemFlag=TRIPWIREROLL;
    Item[mergeHostItem].usItemFlag2=ITEM_tripwire;
    Item[mergeToolItem].usItemFlag2=ITEM_tripwire;
    Merge[0][2]=mergeHostItem;
    Merge[0][4]=TRIPWIRE_ROLL;
    mergeHost[0]->data.objectStatus=100;
    mergeTool.usItem=mergeToolItem;
    mergeTool.ubNumberOfObjects=1;
    check(!mergeHost.AttachObjectNAS(nullptr,&mergeTool,FALSE),
          "full soldierless tripwire roll rejects the merge safely");
    check(mergeHost[0]->data.objectStatus==100 && mergeTool.exists(),
          "rejected soldierless tripwire merge preserves both objects");

    // Implicit same-item gun cannibalization bypasses the XML merge table.
    // Every output must still be initialized before AttachObject consumes it.
    UINT16 implicitResult=NOTHING,implicitResult2=0xFFFF;
    UINT8 implicitType=0xFF,implicitApCost=0xA5;
    check(EvaluateValidMerge(gunItem,gunItem,&implicitResult,&implicitResult2,
                             &implicitType,&implicitApCost),
          "same-gun cannibalization is a valid implicit merge");
    check(implicitResult==gunItem && implicitResult2==NOTHING &&
          implicitType==CANNIBALIZE && implicitApCost==0,
          "implicit merge initializes both results, type, and AP cost");

    // A stale active-LBE marker can survive without its matching global
    // LBENODE after malformed or partial state is loaded. The transfer path
    // must reject that state instead of dereferencing the missing node.
    Item[missingLbeItem].usItemClass=IC_LBEGEAR;
    Item[missingLbeItem].ubClassIndex=missingLbeItem;
    OBJECTTYPE& missingLbe=soldier->inv[VESTPOCKPOS];
    missingLbe.initialize();
    missingLbe.usItem=missingLbeItem;
    missingLbe.ubNumberOfObjects=1;
    missingLbe[0]->data.lbe.bLBE=-1;
    missingLbe[0]->data.lbe.uniqueID=9001;
    LBEArray.clear();
    LBEArray.push_back(LBENODE());
    LBEArray.back().uniqueID=9002;
    LBEArray.back().inv.resize(1);
    check(!MoveItemFromLBEItem(soldier.get(),VESTPOCKPOS,&missingLbe),
          "missing LBE backing node rejects the transfer");
    check(!missingLbe.IsActiveLBE(0),
          "missing LBE backing node retires the stale active marker");
    LBEArray.clear();

    missingLbe[0]->data.lbe.bLBE=-1;
    missingLbe[0]->data.lbe.uniqueID=9003;
    check(!MoveItemFromLBEItem(soldier.get(),VESTPOCKPOS,&missingLbe),
          "empty LBE table rejects the transfer");
    check(LBEArray.empty() && !missingLbe.IsActiveLBE(0),
          "empty LBE table does not synthesize an invalid backing node");

    missingLbe[0]->data.lbe.bLBE=-1;
    missingLbe[0]->data.lbe.uniqueID=9004;
    LBEArray.push_back(LBENODE());
    LBEArray.back().uniqueID=9004;
    LBEArray.back().inv.resize(1);
    check(MoveItemFromLBEItem(soldier.get(),VESTPOCKPOS,&missingLbe),
          "short LBE pocket table is handled within its live bounds");
    check(LBEArray.empty() && !missingLbe.IsActiveLBE(0),
          "empty short LBE backing node is released");

    // The strategic vehicle helper consumes sector items into a newly created
    // LBE. A stale marker can reach this helper after partial state recovery;
    // it must reject the missing backing node before moving any sector item.
    Item[missingLbeItem].ubClassIndex=0;
    Item[lbePayloadItem].usItemClass=IC_MISC;
    Item[lbePayloadItem].ItemSize=0;
    LoadBearingEquipment.resize(1);
    LoadBearingEquipment[0].lbePocketIndex[0]=1;
    LBEPocketType.resize(2);
    LBEPocketType[1].ItemCapacityPerSize.resize(1);
    LBEPocketType[1].ItemCapacityPerSize[0]=1;
    pInventoryPoolList.clear();
    pInventoryPoolList.resize(1);
    pInventoryPoolList[0].fExists=TRUE;
    pInventoryPoolList[0].usFlags=WORLD_ITEM_REACHABLE;
    pInventoryPoolList[0].object.usItem=lbePayloadItem;
    pInventoryPoolList[0].object.ubNumberOfObjects=1;
    missingLbe[0]->data.lbe.bLBE=-1;
    missingLbe[0]->data.lbe.uniqueID=9005;
    LBEArray.clear();
    fillLBEWithSectorItems(&missingLbe);
    check(!missingLbe.IsActiveLBE(0),
          "sector-fill rejects a missing LBE backing node");
    check(pInventoryPoolList[0].object.exists(),
          "rejected sector-fill preserves the sector item");

    check(!missingLbe.IsActiveLBE(1) && missingLbe.GetLBEPointer(1)==nullptr,
          "out-of-range LBE stack access is rejected instead of aliasing stack zero");
    check(!missingLbe.HasAnyActiveLBEs(nullptr,1),
          "out-of-range LBE inspection does not report stack zero as active");

    missingLbe[0]->data.lbe.bLBE=-1;
    missingLbe[0]->data.lbe.uniqueID=9006;
    LBEArray.clear();
    CreateLBE(&missingLbe,soldier->ubID,1);
    LBENODE* recoveredLbe=missingLbe.GetLBEPointer(0);
    check(recoveredLbe && recoveredLbe->inv.size()==1,
          "LBE creation replaces a stale active marker with a valid node");

    LoadBearingEquipment[0].lbePocketIndex[1]=1;
    pInventoryPoolList.clear();
    pInventoryPoolList.resize(2);
    for (WORLDITEM& worldItem:pInventoryPoolList) {
        worldItem.fExists=TRUE;
        worldItem.usFlags=WORLD_ITEM_REACHABLE;
        worldItem.object.usItem=lbePayloadItem;
        worldItem.object.ubNumberOfObjects=1;
    }
    fillLBEWithSectorItems(&missingLbe);
    recoveredLbe=missingLbe.GetLBEPointer(0);
    check(recoveredLbe && recoveredLbe->inv.size()==1 && recoveredLbe->inv[0].exists(),
          "sector-fill uses the live LBE pocket bound");
    check(pInventoryPoolList[1].object.exists(),
          "sector-fill preserves items that have no live LBE pocket");
    DestroyLBE(&missingLbe);
    pInventoryPoolList.clear();
    check(!MoveItemToLBEItem(nullptr,VESTPOCKPOS),
          "LBE transfer rejects a missing soldier");

    // Mouse callbacks can already be queued when a description panel closes,
    // and the permanent-attachment confirmation can outlive that panel. Both
    // paths must treat the retired globals as a cancelled attachment action.
    MOUSE_REGION staleAttachmentRegion{};
    gfItemDescObjectIsAttachment=FALSE;
    gpItemDescObject=nullptr;
    gpItemPointer=nullptr;
    ItemDescAttachmentsCallback(&staleAttachmentRegion,
                                MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(gpItemDescObject==nullptr && gpItemPointer==nullptr,
          "retired attachment region is an inert callback");
    PermanantAttachmentMessageBoxCallBack(MSG_BOX_RETURN_YES);
    check(gpItemDescObject==nullptr && gpItemPointer==nullptr,
          "delayed permanent-attachment confirmation is cancelled");

    gpItemDescObject=&gun;
    gpItemPointer=nullptr;
    DoAttachment(0,0);
    check(gpItemDescObject==&gun && gpItemPointer==nullptr,
          "attachment completion rejects a missing cursor object");

    gItemPointer.initialize();
    gItemPointer.usItem=lbePayloadItem;
    gItemPointer.ubNumberOfObjects=1;
    gpItemPointer=&gItemPointer;
    const UINT16 originalGunItem=gun.usItem;
    DoAttachment(1,0);
    check(gun.usItem==originalGunItem && gun.ubNumberOfObjects==1 &&
          gpItemPointer==&gItemPointer && gpItemPointer->exists(),
          "stale attachment stack index cannot alias object zero");

    // A description region can be retired between mouse-down and mouse-up.
    // Every callback owned by that panel must cancel rather than touching the
    // object or controls that belonged to the old panel.
    MOUSE_REGION staleDescriptionRegion{};
    gpItemDescObject=&gun;
    ItemDescCallback(&staleDescriptionRegion,
                     MSYS_CALLBACK_REASON_LBUTTON_DWN);
    gpItemDescObject=nullptr;
    ItemDescCallback(&staleDescriptionRegion,
                     MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(gpItemDescObject==nullptr,
          "retired description region cancels its pending click");

    GUI_BUTTON staleDescriptionButton{};
    staleDescriptionButton.uiFlags=BUTTON_CLICKED_ON;
    ItemDescDoneButtonCallback(&staleDescriptionButton,
                               MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(!(staleDescriptionButton.uiFlags&BUTTON_CLICKED_ON),
          "retired description done button releases without dereference");

    gpItemDescObject=&gun;
    ItemDescAmmoCallback(&staleDescriptionButton,
                         MSYS_CALLBACK_REASON_LBUTTON_DWN);
    gpItemDescObject=nullptr;
    ItemDescAmmoCallback(&staleDescriptionButton,
                         MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(!(staleDescriptionButton.uiFlags&BUTTON_CLICKED_ON),
          "retired ammunition button releases without dereference");

    ItemDescTabButtonCallback(&staleDescriptionButton,
                              MSYS_CALLBACK_REASON_LBUTTON_UP);
    ItemDescAdvButtonCallback(&staleDescriptionButton,
                              MSYS_CALLBACK_REASON_LBUTTON_UP);
    ItemDescTransformRegionCallback(&staleDescriptionRegion,
                                    MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(gpItemDescObject==nullptr,
          "retired tab, advanced, and transform controls are inert");

    TransformInfoStruct staleTransform{};
    TransformationMenuPopup_Transform(&staleTransform);
    check(gpItemDescObject==nullptr,
          "retired transformation popup action is inert");
    check(!TransformationMenuPopup_TestValid(&staleTransform),
          "retired transformation popup option is unavailable");

    const UINT16 staleCursorItem=gItemPointer.usItem;
    TransformationMenuPopup_Arm(&gItemPointer);
    check(!TransformationMenuPopup_Arm_TestValid(&gItemPointer) &&
          gItemPointer.exists() && gItemPointer.usItem==staleCursorItem,
          "retired arm action preserves its borrowed object");

    BombInventoryMessageBoxCallBack(1);
    BombInventoryDisArmMessageBoxCallBack(MSG_BOX_RETURN_YES);
    TransformationMenuPopup_SplitCrate(ammoItem);
    TransformationMenuPopup_SplitCrateInInventory();
    TransformFromItemDescBox(&staleTransform);
    check(gpItemDescObject==nullptr && gItemPointer.exists(),
          "retired bomb, crate, and direct transform callbacks are inert");

    check(!TransformationMenuPopup_DelayedGrenadeExplosion_TestValid(&gItemPointer),
          "retired delayed-grenade option is unavailable");
    TransformationMenuPopup_DelayedGrenadeExplosion();
    check(gItemPointer.exists() && gItemPointer.usItem==staleCursorItem,
          "retired delayed-grenade action preserves its borrowed object");

    gTransformInProgress=&staleTransform;
    guiTransformInProgressPrevScreen=MAP_SCREEN;
    const UINT32 staleCallbackScreen=guiCurrentScreen;
    ConfirmTransformationMessageBoxCallBack(MSG_BOX_RETURN_YES);
    check(gTransformInProgress==nullptr && guiTransformInProgressPrevScreen==0 &&
          guiCurrentScreen==staleCallbackScreen,
          "retired transform confirmation cancels and releases pending state");

    gTransformInProgress=&staleTransform;
    guiTransformInProgressPrevScreen=MAP_SCREEN;
    ConfirmTransformationMessageBoxCallBack(MSG_BOX_RETURN_NO);
    check(gTransformInProgress==nullptr && guiTransformInProgressPrevScreen==0,
          "declined transform confirmation releases pending state");

    // A queued money-button callback and the deferred split action can run
    // after their description panel has retired. They must release the old
    // button and cancel its pending amount without creating cursor money or
    // dereferencing the former description object.
    GUI_BUTTON staleMoneyButton{};
    staleMoneyButton.uiFlags=BUTTON_CLICKED_ON;
    staleMoneyButton.UserData[0]=3;
    gRemoveMoney.uiTotalAmount=1000;
    gRemoveMoney.uiMoneyRemaining=700;
    gRemoveMoney.uiMoneyRemoving=300;
    gpItemDescObject=nullptr;
    gpItemPointer=nullptr;
    gItemPointer.initialize();
    fMapInventoryItem=FALSE;
    BtnMoneyButtonCallback(&staleMoneyButton,
                           MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(!(staleMoneyButton.uiFlags&BUTTON_CLICKED_ON) &&
          gRemoveMoney.uiMoneyRemoving==300,
          "retired money button releases without consuming pending state");
    RemoveMoney();
    check(gRemoveMoney.uiMoneyRemaining==gRemoveMoney.uiTotalAmount &&
          gRemoveMoney.uiMoneyRemoving==0 && gpItemPointer==nullptr &&
          !gItemPointer.exists() && !fMapInventoryItem,
          "retired money split cancels without creating a cursor object");

    // Removed item-stack regions can still have a queued mouse-up event. A
    // closed popup must reject it before consulting the retired object and
    // soldier globals.
    MOUSE_REGION stalePopupRegion{};
    gfInItemStackPopup=FALSE;
    gfInSectorStackPopup=FALSE;
    gfItemPopupRegionCallbackEndFix=FALSE;
    gpItemPopupObject=nullptr;
    gpItemPopupSoldier=nullptr;
    ItemPopupRegionCallback(&stalePopupRegion,
                            MSYS_CALLBACK_REASON_RBUTTON_UP);
    ItemPopupFullRegionCallback(&stalePopupRegion,
                                MSYS_CALLBACK_REASON_RBUTTON_UP);
    check(!gfInItemStackPopup && !gfInSectorStackPopup &&
          gpItemPopupObject==nullptr && gpItemPopupSoldier==nullptr,
          "retired item-stack region is an inert callback");

    // Key-ring regions share the popup soldier but have an independent open
    // flag. A queued key click after closure must not consult that retired
    // soldier.
    MOUSE_REGION staleKeyRingRegion{};
    gfInKeyRingPopup=FALSE;
    gpItemPopupSoldier=nullptr;
    gpItemPointer=nullptr;
    KeyRingSlotInvClickCallback(&staleKeyRingRegion,
                                MSYS_CALLBACK_REASON_LBUTTON_DWN);
    check(!gfInKeyRingPopup && gpItemPopupSoldier==nullptr &&
          gpItemPointer==nullptr,
          "retired key-ring region is an inert callback");

    gpItemPointer=nullptr;
    gItemPointer.initialize();
    fMapInventoryItem=FALSE;
    guiTacticalInterfaceFlags=0;
    gMAXITEMS_READ=previousMaxItems;
    gpItemDescObject=nullptr; gpItemDescSoldier=nullptr; gpItemPointerSoldier=nullptr;
    std::cout<<"PASS: "<<checks<<" actual map-inventory ammunition callback assertions\n";
}
