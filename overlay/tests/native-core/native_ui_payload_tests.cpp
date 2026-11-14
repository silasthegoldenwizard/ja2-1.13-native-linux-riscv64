/*
 * P710 native snapshot update: tests/native-core/native_ui_payload_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Process-local UI ABI and original cursor selection; no game assets/window.
#include "native_buttons.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

static_assert(sizeof(InputAtom)==16, "input resource/event ABI remains fixed");
static_assert(std::is_same<std::remove_reference<decltype(MOUSE_REGION{}.UserData[0])>::type, NativeUIPayload>::value,
              "region payload is explicitly pointer-sized");
namespace {
unsigned checks=0, moves=0, clicks=0;
struct Region : MOUSE_REGION { Region() : MOUSE_REGION{} {} ~Region() { MSYS_RemoveRegion(this); } };
std::vector<UINT32> selected;
MOUSE_REGION* selectorVictim=nullptr;
bool replaceOnSelect=false;
NativeUIPayload expected=0;
INT32 created=-1;
void check(bool value,const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
void movement(MOUSE_REGION* r,INT32 reason) {
    if (reason==MSYS_CALLBACK_REASON_MOVE) { ++moves; check(MSYS_GetRegionUserData(r,0)==expected,"callback sees full region pointer"); }
}
BOOLEAN selector(UINT32 cursor) {
    selected.push_back(cursor);
    if (replaceOnSelect) {
        replaceOnSelect=false;
        MSYS_RemoveRegion(selectorVictim);
        MSYS_DefineRegion(selectorVictim,2,2,9,9,63,91,movement,nullptr);
        MSYS_SetRegionUserData(selectorVictim,0,expected);
    }
    return TRUE;
}
void send(UINT16 type,UINT16 x=4,UINT16 y=4) {
    InputAtom atom{}; atom.usEvent=type; atom.uiParam=x|(static_cast<UINT32>(y)<<16);
    check(sgp::DispatchNativeMouseEvent(atom),"native atom accepted");
}
void click(GUI_BUTTON* b,INT32 reason) {
    if (!(reason&MSYS_CALLBACK_REASON_LBUTTON_DWN)) return;
    ++clicks;
    check(MSYS_GetBtnUserData(b,0)==expected,"callback sees full button pointer");
    const INT32 removed=b->IDNum;
    RemoveButton(removed);
    created=CreateHotSpot(2,2,8,8,63,nullptr,nullptr);
    check(created>=0 && created!=removed,"deleted callback slot unavailable until dispatch ends");
    check(MSYS_GetBtnUserData(GetButtonPtr(created),0)==0,"fresh callback button starts with zero payload");
}
void alternateHook(UINT16,UINT16,UINT16,BOOLEAN,BOOLEAN) {}
void fresh() {
    ShutdownButtonSystem(); MSYS_Shutdown(); check(MSYS_Init()==1,"mouse initialization");
    check(InitButtonSystem(),"button initialization");
    sgp::SetNativeMouseCursorSelector(selector); selected.clear(); moves=clicks=0;
}
void hooks() {
    ShutdownButtonSystem(); MSYS_Shutdown(); check(MSYS_Init()==1,"hook initialization");
    check(MouseSystemHook==&MSYS_SGP_Mouse_Handler_Hook,"MSYS init binds actual original handler");
    SetMouseInputHook(&alternateHook); MSYS_Shutdown();
    check(MouseSystemHook==&alternateHook,"MSYS shutdown preserves replacement hook owner");
    check(MSYS_Init()==1 && MouseSystemHook==&MSYS_SGP_Mouse_Handler_Hook,"MSYS reinit binds actual handler");
    MSYS_Shutdown(); check(MouseSystemHook==nullptr,"MSYS shutdown clears its owned hook");
}
void payloads() {
    fresh(); int object=7;
    expected=reinterpret_cast<NativeUIPayload>(&object);
    check(reinterpret_cast<std::uintptr_t>(&object)>std::numeric_limits<UINT32>::max(),"real pointer exercises high bits above bit31");
    Region region{};
    MSYS_DefineRegion(&region,2,2,9,9,63,MSYS_NO_CURSOR,movement,nullptr);
    MSYS_SetRegionUserData(&region,0,expected);
    check(reinterpret_cast<int*>(MSYS_GetRegionUserData(&region,0))==&object,"exact region pointer round trip");
    MSYS_SetRegionUserData(&region,1,std::numeric_limits<INT32>::min());
    MSYS_SetRegionUserData(&region,2,std::numeric_limits<INT32>::max());
    MSYS_SetRegionUserData(&region,3,std::numeric_limits<UINT32>::max());
    check(MSYS_GetRegionUserData(&region,1)==std::numeric_limits<INT32>::min(),"signed32 minimum sign extends");
    check(MSYS_GetRegionUserData(&region,2)==std::numeric_limits<INT32>::max(),"signed32 maximum unchanged");
    check(MSYS_GetRegionUserData(&region,3)==static_cast<NativeUIPayload>(std::numeric_limits<UINT32>::max()),"unsigned32 maximum retains positive value");
    MSYS_SetRegionUserData(&region,-1,22); MSYS_SetRegionUserData(&region,4,22);
    check(MSYS_GetRegionUserData(&region,-1)==0 && MSYS_GetRegionUserData(&region,4)==0 && MSYS_GetRegionUserData(nullptr,0)==0,"invalid region slots/null are bounded");
    send(MOUSE_POS); check(moves==1,"region callback receives wide state");
    MSYS_RemoveRegion(&region);
    check(MSYS_GetRegionUserData(&region,0)==0,"removed region payload unavailable");
    const INT32 id=CreateHotSpot(2,2,8,8,63,nullptr,click); check(id>=0,"asset-free original hotspot");
    auto* b=GetButtonPtr(id); MSYS_SetBtnUserData(id,0,expected);
    check(reinterpret_cast<int*>(MSYS_GetBtnUserData(b,0))==&object,"exact button pointer round trip");
    MSYS_SetBtnUserData(id,1,std::numeric_limits<INT32>::min());
    MSYS_SetBtnUserData(id,2,std::numeric_limits<UINT32>::max());
    check(MSYS_GetBtnUserData(b,1)==std::numeric_limits<INT32>::min() && MSYS_GetBtnUserData(b,2)==static_cast<NativeUIPayload>(std::numeric_limits<UINT32>::max()),"button signed and unsigned scalar values");
    MSYS_SetBtnUserData(id,4,99); check(MSYS_GetBtnUserData(b,4)==0 && MSYS_GetBtnUserData(nullptr,0)==0,"invalid button slots/null bounded");
    // A forged wide region payload must not narrow back to a valid button slot.
    MSYS_SetRegionUserData(&b->Area,0,(NativeUIPayload{1}<<40)+id);
    send(LEFT_BUTTON_DOWN); check(clicks==0,"wide invalid internal button ID cannot alias low32 slot");
    send(LEFT_BUTTON_UP); MSYS_SetRegionUserData(&b->Area,0,id);
    send(LEFT_BUTTON_DOWN); check(clicks==1,"valid internal button ID dispatches once");
    send(LEFT_BUTTON_UP); RemoveButton(created);
    const auto reused=CreateHotSpot(2,2,8,8,63,nullptr,nullptr);
    check(reused==id && MSYS_GetBtnUserData(GetButtonPtr(reused),0)==0,"later reused slot does not inherit pointer state");
    RemoveButton(reused);
}
void cursors() {
    fresh(); Region lower{},upper{};
    MSYS_DefineRegion(&lower,2,2,9,9,31,42,nullptr,nullptr);
    MSYS_DefineRegion(&upper,2,2,9,9,63,MSYS_NO_CURSOR,nullptr,nullptr);
    send(MOUSE_POS); check(selected.back()==42,"no-cursor hit inherits lower enabled overlapping region cursor");
    MSYS_DisableRegion(&lower); RefreshMouseRegions(); check(selected.back()==0,"disabled lower region falls back to original base cursor0");
    MSYS_ChangeRegionCursor(&upper,73); check(selected.back()==73,"active region cursor changes immediately");
    upper.Cursor=MSYS_NO_CURSOR; RefreshMouseRegions();
    check(selected.back()==0,"source direct no-cursor field change also falls through to lower cursor");
    MSYS_SetCurrentCursor(65535); check(selected.back()==65535,"direct selector preserves complete UINT16 argument into UINT32 service");
    MSYS_ChangeRegionCursor(&upper,MSYS_NO_CURSOR); RefreshMouseRegions(); check(selected.back()==0,"removing cursor restores eligible lower fallback");
    MSYS_EnableRegion(&lower); RefreshMouseRegions(); check(selected.back()==42,"reenabled lower cursor restored");
    const auto before=selected.size(); MSYS_ChangeRegionCursor(&lower,84); check(selected.size()==before,"noncurrent fallback region mutation defers to refresh");
    RefreshMouseRegions(); check(selected.back()==84,"refresh observes changed fallback cursor");
    const INT32 button=CreateHotSpot(20,2,8,8,63,nullptr,nullptr); check(button>=0,"cursor hotspot");
    check(SetButtonCursor(button,93),"button cursor setter"); send(MOUSE_POS,22,4); check(selected.back()==93,"button cursor reaches real bound selector");
    RemoveButton(button); MSYS_RemoveRegion(&upper); MSYS_RemoveRegion(&lower);
    // A selector-triggered removal/address reuse must end the old update.
    int object=9; expected=reinterpret_cast<NativeUIPayload>(&object);
    Region victim{}; selectorVictim=&victim;
    MSYS_DefineRegion(&victim,2,2,9,9,63,90,movement,nullptr); MSYS_SetRegionUserData(&victim,0,expected);
    replaceOnSelect=true; moves=0; send(MOUSE_POS);
    check(moves==0 && victim.Cursor==91,"selector replacement aborts before stale MOVE");
    RefreshMouseRegions(); check(moves==1 && selected.back()==91,"later refresh selects replacement generation");
    MSYS_RemoveRegion(&victim); selectorVictim=nullptr;
    sgp::SetNativeMouseCursorSelector(nullptr); const auto count=selected.size();
    MSYS_SetCurrentCursor(12); check(selected.size()==count,"cleared selector cannot call stale database");
}
}
int main() {
    try { hooks(); payloads(); cursors(); ShutdownButtonSystem(); MSYS_Shutdown();
        std::cout<<"PASS: "<<checks<<" UI payload/cursor checks\n"; return 0;
    } catch (const std::exception& error) {
        ShutdownButtonSystem(); MSYS_Shutdown(); sgp::SetNativeMouseCursorSelector(nullptr);
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n'; return 1;
    }
}
