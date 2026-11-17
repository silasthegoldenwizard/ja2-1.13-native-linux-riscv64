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

// Real map-inventory ammunition callback, with authored weapon/ammo state.
#include "Button System.h"
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
extern INT32 giItemDescAmmoButton;
extern BOOLEAN fShowMapInventoryPool, fShowInventoryFlag;
extern BOOLEAN fMapInventoryItem;
extern BOOLEAN gfAddingMoneyToMercFromPlayersAccount;
extern REMOVE_MONEY gRemoveMoney;
extern void ItemDescAmmoCallback(GUI_BUTTON*,INT32);
extern void RemoveMoney();
extern void fillLBEWithSectorItems(OBJECTTYPE*);

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
    Item[gunItem].usItemClass=IC_GUN; Item[gunItem].ubClassIndex=gunItem;
    Item[ammoItem].usItemClass=IC_AMMO; Item[ammoItem].ubClassIndex=ammoItem;
    Weapon[gunItem].ubCalibre=1; Weapon[gunItem].ubMagSize=30;
    Weapon[gunItem].swapClips=1;
    Magazine[ammoItem].ubCalibre=1; Magazine[ammoItem].ubMagSize=30;
    Magazine[ammoItem].ubMagType=0;
    OBJECTTYPE gun;
    gun.usItem=gunItem; gun.ubNumberOfObjects=1;
    gpItemDescObject=&gun; gpItemDescSoldier=soldier.get();
    guiCurrentItemDescriptionScreen=MAP_SCREEN;
    giItemDescAmmoButton=-1; gfInItemDescBox=FALSE;
    fShowMapInventoryPool=FALSE; fShowInventoryFlag=FALSE;

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

    // The aggregate medical-kit path uses the same conversion and must report
    // that the remaining sixty-two points cannot satisfy a request for one hundred.
    Item[kitItem].usItemClass=IC_MEDKIT;
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

    gpItemPointer=nullptr;
    gItemPointer.initialize();
    fMapInventoryItem=FALSE;
    guiTacticalInterfaceFlags=0;
    gMAXITEMS_READ=previousMaxItems;
    gpItemDescObject=nullptr; gpItemDescSoldier=nullptr; gpItemPointerSoldier=nullptr;
    std::cout<<"PASS: "<<checks<<" actual map-inventory ammunition callback assertions\n";
}
