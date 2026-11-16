/*
 * P710 native snapshot update: tests/native-core/native_weapon_tooltip_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production weapon tooltip regions with authored weapon state and real text.
#include "Interface.h"
#include "Interface Panels.h"
#include "Interface Items.h"
#include "Interface Enhanced.h"
#include "GameSettings.h"
#include "Items.h"
#include "Weapons.h"
#include "Text.h"
#include "mousesystem.h"
#include <language.hpp>
#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <iostream>

extern MOUSE_REGION gUDBFasthelpRegions[103];
extern UINT32 guiCurrentScreen;
extern bool is_networked;

namespace {
unsigned checks=0;
void check(bool value,const char* why) {
    ++checks;
    if (!value) { std::cerr<<"FAIL: "<<why<<'\n'; std::abort(); }
}
}
int main() {
    MSYS_Init();
    OBJECTTYPE gun;
    gun.usItem=400; gun.ubNumberOfObjects=1;
    Item[400].usItemClass=IC_GUN; Item[400].ubClassIndex=400;
    Weapon[400].ubMagSize=30;
    is_networked=false;
    gGameSettings.fOptions[TOPTION_ENHANCED_DESC_BOX]=TRUE;
    gGameExternalOptions.iEnhancedDescriptionBox=2;
    gubDescGenPage=0; gubDescBoxPage=1;
    for (auto lang:{i18n::Lang::en,i18n::Lang::de,i18n::Lang::ru,i18n::Lang::nl,
                    i18n::Lang::pl,i18n::Lang::fr,i18n::Lang::it,i18n::Lang::zh}) {
        BindLanguageStrings(lang);
        for (UINT32 screen:{MAP_SCREEN,GAME_SCREEN}) {
            guiCurrentScreen=screen;
            for (bool ncth:{true,false}) {
                gGameExternalOptions.fUseNCTH=ncth;
                InternalInitEDBTooltipRegion(&gun,screen);
                const auto count=std::count_if(std::begin(gUDBFasthelpRegions),std::end(gUDBFasthelpRegions),
                    [](const MOUSE_REGION& r){return r.IDNumber!=0;});
                check(count==(ncth?37:38),"exact weapon tooltip region count for selected aiming system");
                const auto& last=gUDBFasthelpRegions[ncth?36:37];
                check(last.FastHelpText && *last.FastHelpText,"last applicable weapon tooltip has text");
                const auto expected=ncth?szUDBGenWeaponsStatsTooltipText[21]:gzWeaponStatsFasthelpTactical[22];
                check(std::wcsstr(last.FastHelpText,expected)!=nullptr,"last tooltip uses the selected aiming-system table");
                if (ncth) check(!gUDBFasthelpRegions[37].IDNumber,"old-system-only penalty region is absent under NCTH");
            }
        }
    }
    MSYS_Shutdown(); BindLanguageStrings(i18n::Lang::en);
    std::cout<<"PASS: "<<checks<<" actual weapon tooltip assertions across eight languages and both screens/aiming systems\n";
}
