/*
 * P710 native snapshot update: tests/native-core/native_item_ammo_callback_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Real map-inventory ammunition callback, with authored weapon/ammo state.
#include "Button System.h"
#include "Interface Items.h"
#include "Interface.h"
#include "Items.h"
#include "Weapons.h"
#include "Soldier Control.h"
#include <cstdlib>
#include <iostream>
#include <memory>

extern OBJECTTYPE* gpItemDescObject;
extern SOLDIERTYPE* gpItemDescSoldier;
extern OBJECTTYPE gTempObject;
extern UINT32 guiCurrentItemDescriptionScreen;
extern INT32 giItemDescAmmoButton;
extern BOOLEAN fShowMapInventoryPool, fShowInventoryFlag;
extern void ItemDescAmmoCallback(GUI_BUTTON*,INT32);

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<why<<'\n'; std::abort(); }
}
constexpr UINT16 gunItem=400,ammoItem=401;
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
    gpItemPointer=nullptr;
    gpItemDescObject=nullptr; gpItemDescSoldier=nullptr; gpItemPointerSoldier=nullptr;
    std::cout<<"PASS: "<<checks<<" actual map-inventory ammunition callback assertions\n";
}
