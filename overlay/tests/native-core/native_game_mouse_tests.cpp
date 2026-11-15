/*
 * P710 native snapshot update: tests/native-core/native_game_mouse_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native GameLoop mouse bridge contracts, 2026-11-13. Actual SDL injection,
// owned video window and production input/MSYS/button services; no game data.
#include "native_buttons.h"
#include "native_video.h"
#include <algorithm>
#include <array>
#include <cwchar>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool value,const char* why) { ++checks; if (!value) throw std::runtime_error(why); }
struct Region : MOUSE_REGION { Region():MOUSE_REGION{} {} ~Region() { MSYS_RemoveRegion(this); } };
struct Observation { NativeUIPayload tag; INT32 reason; INT16 x,y,wheel; UINT16 buttons; };
std::vector<Observation> trace;
void record(MOUSE_REGION* r,INT32 why) { trace.push_back({r->UserData[0],why,r->MouseXPos,r->MouseYPos,r->WheelState,r->ButtonState}); }
void define(MOUSE_REGION& r,INT32 tag,UINT16 l=0,UINT16 right=31,INT8 priority=31,MOUSE_CALLBACK movement=record) {
    MSYS_DefineRegion(&r,l,0,right,19,priority,MSYS_NO_CURSOR,movement,record);
    MSYS_SetRegionUserData(&r,0,tag);
}
struct Geometry { SDL_WindowID id; int w,h,ww,wh; };
Geometry geometry() { Geometry g{}; check(sgp::NativeVideoInputInfo(g.id,g.w,g.h,g.ww,g.wh),"owned input window"); return g; }
void push(SDL_Event e) { check(SDL_PushEvent(&e),"push synthetic SDL event"); }
void windowEvent(Uint32 type) { SDL_Event e{}; e.type=type; e.window.windowID=geometry().id; push(e); }
void motion(int x,int y) {
    const auto g=geometry(); SDL_Event e{}; e.type=SDL_EVENT_MOUSE_MOTION; e.motion.windowID=g.id;
    e.motion.x=static_cast<float>((static_cast<double>(x)+0.25)*g.ww/g.w);
    e.motion.y=static_cast<float>((static_cast<double>(y)+0.25)*g.wh/g.h); push(e);
}
void button(Uint8 which,bool down,int x,int y) {
    const auto g=geometry(); SDL_Event e{}; e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.windowID=g.id; e.button.button=which; e.button.down=down;
    e.button.x=static_cast<float>((static_cast<double>(x)+0.25)*g.ww/g.w);
    e.button.y=static_cast<float>((static_cast<double>(y)+0.25)*g.wh/g.h); push(e);
}
void wheel(float delta,int x=4,int y=5) {
    const auto g=geometry(); SDL_Event e{}; e.type=SDL_EVENT_MOUSE_WHEEL; e.wheel.windowID=g.id;
    e.wheel.y=delta; e.wheel.direction=SDL_MOUSEWHEEL_NORMAL;
    e.wheel.mouse_x=static_cast<float>((static_cast<double>(x)+0.25)*g.ww/g.w);
    e.wheel.mouse_y=static_cast<float>((static_cast<double>(y)+0.25)*g.wh/g.h); push(e);
}
void key(bool down) { SDL_Event e{}; e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP; e.key.windowID=geometry().id; e.key.scancode=SDL_SCANCODE_A; e.key.down=down; push(e); }
void frame(BOOLEAN extended=TRUE) { sgp::DispatchQueuedNativeGameMouseEvents(extended); }
void fresh() {
    windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
    sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents();
    ShutdownButtonSystem(); MSYS_Shutdown(); check(MSYS_Init()!=0 && InitButtonSystem(),"fresh production UI managers");
    gfTrackMousePos=FALSE; trace.clear();
}
std::vector<Observation> clicks() {
    std::vector<Observation> out;
    for (const auto& o:trace) if (o.reason & (MSYS_CALLBACK_REASON_BUTTONS|MSYS_CALLBACK_REASON_LBUTTON_REPEAT|MSYS_CALLBACK_REASON_RBUTTON_REPEAT)) out.push_back(o);
    return out;
}
void queuedHistory() {
    fresh(); Region r; define(r,1); gfTrackMousePos=TRUE;
    motion(2,3); button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_RIGHT,true,6,7);
    button(SDL_BUTTON_LEFT,false,8,9); button(SDL_BUTTON_RIGHT,false,10,11);
    sgp::PumpNativeVideoEvents(); check(!gfLeftButtonState && !gfRightButtonState,"polling globals already reflect final release"); frame();
    const auto events=clicks(); check(events.size()==4,"frame bridge delivers all four queued transitions");
    const INT32 reasons[]={4,16,8,32}; const UINT16 states[]={1,3,2,0};
    for (std::size_t i=0;i<4;++i) check(events[i].reason==reasons[i] && events[i].buttons==states[i] && events[i].x==4+static_cast<int>(i)*2 && events[i].y==5+static_cast<int>(i)*2,"queued coordinates and held states retain history");
    check(sgp::NativeInputQueueEmpty(),"mouse prefix consumed completely");
    check(sizeof(InputAtom)==16 && offsetof(InputAtom,uiParam)==12,"original input atom ABI");
    check(!sgp::IsNativeGameMouseEvent(KEY_DOWN) && !sgp::IsNativeGameMouseEvent(0x8011) && sgp::IsNativeGameMouseEvent(X1_BUTTON_DOWN),"mouse classification compares exact overlapping enum tags");
}
void keyboardBoundary() {
    fresh(); Region r; define(r,2); gfTrackMousePos=TRUE;
    motion(2,3); key(true); button(SDL_BUTTON_LEFT,true,4,5); key(false); button(SDL_BUTTON_LEFT,false,6,7);
    frame(); check(clicks().empty() && MSYS_CurrentMX==2 && MSYS_CurrentMY==3,"keyboard head stops prefix and final hover polling");
    InputAtom atom{1,2,3,4,5}; check(!sgp::DequeueNativeGameMouseEvent(&atom) && atom.uiTimeStamp==1 && atom.usEvent==3,"mouse head miss leaves caller output intact");
    check(!sgp::DequeueNativeGameMouseEvent(nullptr),"null prefix output never consumes");
    check(DequeueEvent(&atom) && atom.usEvent==KEY_DOWN && atom.usParam==97,"first keyboard atom retained");
    frame(); const auto first=clicks(); check(first.size()==1 && first[0].reason==4 && first[0].x==4 && first[0].buttons==1,"next prefix replays press before later keyboard head");
    check(DequeueEvent(&atom) && atom.usEvent==KEY_UP,"second keyboard atom retained in order");
    frame(); const auto both=clicks(); check(both.size()==2 && both[1].reason==8 && both[1].x==6 && both[1].buttons==0,"final prefix replays release");
}
void priorityTies() {
    fresh(); Region older,newer; define(older,10); define(newer,11);
    button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_LEFT,false,4,5); frame();
    auto seen=clicks(); check(seen.size()==2 && seen[0].tag==11 && seen[1].tag==11,"newest equal-priority region receives queued click");
    MSYS_ChangeRegionPriority(&older,127); trace.clear();
    button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_LEFT,false,4,5); frame();
    seen=clicks(); check(seen.size()==2 && seen[0].tag==11,"priority setter preserves original existing list order");
}
void hoverReset() {
    fresh(); Region r; define(r,3); motion(4,5); frame();
    check((r.uiFlags & MSYS_MOUSE_IN_AREA) && r.MouseXPos==4 && r.MouseYPos==5,"untracked hover coalesces after empty FIFO");
    const auto reset=sgp::NativeInputResetGeneration(); trace.clear(); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); frame();
    check(sgp::NativeInputResetGeneration()!=reset && sgp::NativeInputQueueEmpty(),"hover-only focus loss has reset but no mouse atom");
    check(!(r.uiFlags & MSYS_MOUSE_IN_AREA) && !r.ButtonState && !r.WheelState && !MSYS_PrevRegion && !MSYS_CurrRegion && trace.empty(),"empty frame cancels hover without callbacks");
    windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); frame();
    check((r.uiFlags & MSYS_MOUSE_IN_AREA) && !trace.empty() && trace.front().reason==MSYS_CALLBACK_REASON_GAIN_MOUSE,"fresh hover enters after focus returns");
}
void extendedState() {
    fresh(); Region r; define(r,4); motion(4,5); frame(); trace.clear();
    button(SDL_BUTTON_MIDDLE,true,4,5); frame(); check(r.ButtonState==MSYS_MIDDLE_BUTTON && trace.empty(),"middle state has original no-callback behavior");
    button(SDL_BUTTON_X1,true,4,5); frame(); check(r.ButtonState==(MSYS_MIDDLE_BUTTON|MSYS_X1_BUTTON) && trace.empty(),"X1 exact-tag state retained");
    button(SDL_BUTTON_X2,true,4,5); frame(); check(r.ButtonState==(MSYS_MIDDLE_BUTTON|MSYS_X1_BUTTON|MSYS_X2_BUTTON) && trace.empty(),"X2 exact-tag state retained");
    button(SDL_BUTTON_X1,false,4,5); frame(); check(r.ButtonState==(MSYS_MIDDLE_BUTTON|MSYS_X2_BUTTON),"X1 up clears only its bit");
    button(SDL_BUTTON_MIDDLE,false,4,5); button(SDL_BUTTON_X2,false,4,5); frame(); check(r.ButtonState==0,"extended release history clears state");
    wheel(9.75f); frame(); check(r.WheelState==1 && gsMouseWheelDeltaValue==9 && trace.empty(),"wheel region counts one original event independent of SDL delta magnitude");
    wheel(-3.0f); frame(); check(r.WheelState==-1 && trace.empty(),"wheel down updates polling field without invented callback");
    ResetWheelState(&r); check(r.WheelState==0,"public region wheel reset");
    button(SDL_BUTTON_X1,true,4,5); button(SDL_BUTTON_X2,true,4,5); frame();
    windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); frame(); check(!r.ButtonState && !r.WheelState && clicks().empty(),"focus loss cancels extended state and suppresses unpaired releases");
    windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); button(SDL_BUTTON_X1,true,4,5); button(SDL_BUTTON_X1,false,4,5); wheel(1); frame(FALSE);
    check(!r.ButtonState && !r.WheelState && clicks().empty(),"disabled extended option drains atoms without changing region state");
}
void clickedWheel() {
    fresh(); Region left,right; define(left,5,0,9); define(right,6,10,31);
    button(SDL_BUTTON_LEFT,true,4,5); frame();
    wheel(1,14,5); wheel(1,14,5); frame(); check(right.WheelState==0,"foreign clicked-ID gate retains pending wheel units");
    button(SDL_BUTTON_MIDDLE,true,14,5); button(SDL_BUTTON_MIDDLE,false,14,5); frame();
    check(right.WheelState==0 && clicks().size()==1,"extended releases preserve original L/R clicked gate");
    button(SDL_BUTTON_LEFT,false,14,5); frame(); check(clicks().size()==1,"foreign L/R release clears gate without callback");
    motion(15,5); frame(); check(right.WheelState==2 && !right.ButtonState,"next allowed movement receives accumulated wheel units");
    ResetWheelState(&right);
    button(SDL_BUTTON_LEFT,true,14,5); button(SDL_BUTTON_LEFT,false,14,5); frame();
    const auto seen=clicks(); check(seen.size()==3 && seen[1].tag==6 && seen[1].reason==4 && seen[2].reason==8,"fresh click works after foreign release");
}
void wheelBounds() {
    for (const int direction : {1,-1}) {
        fresh(); Region left,right; define(left,12,0,9); define(right,13,10,31);
        button(SDL_BUTTON_LEFT,true,4,5); frame();
        // Fixed 256-atom input FIFO: pump bounded batches, retaining wheel in
        // MSYS behind a foreign clicked-ID gate until its INT16 limit is crossed.
        const auto g=geometry(); SDL_Event e{}; e.type=SDL_EVENT_MOUSE_WHEEL;
        e.wheel.windowID=g.id; e.wheel.y=static_cast<float>(direction);
        e.wheel.direction=SDL_MOUSEWHEEL_NORMAL;
        e.wheel.mouse_x=static_cast<float>(14.25*g.ww/g.w);
        e.wheel.mouse_y=static_cast<float>(5.25*g.wh/g.h);
        for (unsigned i=0;i<32770;++i) {
            push(e);
            if (i%128==127) frame();
        }
        frame(); check(!right.WheelState,"overflow boundary keeps pending wheel behind clicked gate");
        button(SDL_BUTTON_LEFT,false,14,5); frame(); motion(15,5); frame();
        check(right.WheelState==(direction>0?32767:-32768),"pending wheel accumulation saturates both INT16 boundaries");
    }
}
unsigned buttonDowns=0,buttonUps=0;
void buttonRecord(GUI_BUTTON*,INT32 reason) { if (reason & 4) ++buttonDowns; if (reason & 8) ++buttonUps; }
void anchors() {
    fresh(); buttonDowns=buttonUps=0;
    const auto id=CreateHotSpot(2,3,7,5,31,DEFAULT_MOVE_CALLBACK,buttonRecord);
    check(id>=0,"real hotspot without proprietary image");
    button(SDL_BUTTON_LEFT,true,4,5); frame(); check(GetButtonPtr(id) && (GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),"queued press anchors real button");
    motion(20,15); button(SDL_BUTTON_LEFT,false,20,15); frame();
    check(buttonDowns==1 && buttonUps==0 && !(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),"outside queued release restores anchor without activation");
    button(SDL_BUTTON_LEFT,true,4,5); frame(); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); frame();
    check(buttonDowns==2 && buttonUps==0 && !(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),"focus reset cancels real anchored control");
    windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_LEFT,false,4,5); frame();
    check(buttonDowns==3 && buttonUps==1,"fresh inside click activates after reset");
}
void fittedDispatch() {
    auto* window=SDL_GetWindowFromID(geometry().id);
    check(SDL_SetWindowSize(window,256,192) && SDL_SyncWindow(window),"resize production dispatch window to letterbox");
    fresh(); buttonDowns=buttonUps=0;
    const auto id=CreateHotSpot(2,3,7,5,31,DEFAULT_MOVE_CALLBACK,buttonRecord);
    check(id>=0,"fitted real hotspot");
    // This visible target models a momentary command. Ordinary hotspots are
    // toggles; explicitly select momentary behavior before testing an off-state
    // restoration rather than changing the original anchor/toggle contract.
    GetButtonPtr(id)->uiFlags |= BUTTON_NO_TOGGLE;
    check(ColorFillVideoSurfaceArea(FRAME_BUFFER,0,0,32,20,0x2104) &&
          ColorFillVideoSurfaceArea(FRAME_BUFFER,2,3,9,8,0x001f),"draw authored blue control through original framebuffer");
    InvalidateScreen(); check(sgp::RefreshNativeVideo(),"present authored fitted control");
    // Literal oracle: fitted image is (0,16,256,160). Window (36,60)
    // lies on the visible blue control and maps to logical (4,5).
    auto* surface=SDL_GetWindowSurface(window); Uint8 red=0,green=0,blue=0,alpha=0;
    check(surface && SDL_ReadSurfacePixel(surface,36,60,&red,&green,&blue,&alpha) &&
          red==0 && green==0 && blue==255,"actual fitted control pixel matches blue target");
    check(SDL_ReadSurfacePixel(surface,36,8,&red,&green,&blue,&alpha) && red==0 && green==0 && blue==0,
          "actual presentation bar is black");
    const auto physicalButton=[&](bool down,float x,float y) {
        SDL_Event e{}; e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;
        e.button.windowID=geometry().id; e.button.button=SDL_BUTTON_LEFT; e.button.down=down;
        e.button.x=x; e.button.y=y; push(e);
    };
    physicalButton(true,36,60); physicalButton(false,36,60); frame();
    check(buttonDowns==1 && buttonUps==1 && GetButtonPtr(id)->Area.MouseXPos==4 &&
          GetButtonPtr(id)->Area.MouseYPos==5,"visible fitted target activates once through SDL input and GameLoop dispatch");
    check(!(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),"momentary target is off before cancellation regression");
    physicalButton(true,92,60); physicalButton(false,92,60); frame();
    check(buttonDowns==1 && buttonUps==1,"visible neighboring background misses fitted control");
    physicalButton(true,36,60); frame();
    check(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON,"fitted inside press anchors control");
    physicalButton(false,36,8); frame();
    check(!gfLeftButtonState && !(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON) &&
          buttonDowns==2 && buttonUps==1,"release into bar clears original anchor without activation");
    physicalButton(true,36,8); physicalButton(false,36,8); frame();
    check(buttonDowns==2 && buttonUps==1 && !gfLeftButtonState,"bar click cannot activate or hold control");
    physicalButton(true,36,60); physicalButton(false,36,60); frame();
    check(buttonDowns==3 && buttonUps==2,"fresh fitted click works after bar cancellation");
    // Retain the ordinary-toggle counterpart: outside release must restore its
    // prior on state, which is deliberately different from the momentary case.
    GetButtonPtr(id)->uiFlags &= ~static_cast<UINT32>(BUTTON_NO_TOGGLE);
    physicalButton(true,36,60); physicalButton(false,36,60); frame();
    check(buttonDowns==4 && buttonUps==3 && (GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),
          "ordinary fitted toggle remains on after accepted click");
    physicalButton(true,36,60); frame(); physicalButton(false,36,8); frame();
    check(buttonDowns==5 && buttonUps==3 && !gfLeftButtonState && (GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),
          "bar release restores prior on state of ordinary toggle without activation");
    // Untracked motion still updates final polling, allowing the real UI hover
    // to leave the control when no motion atom is requested.
    SDL_Event e{}; e.type=SDL_EVENT_MOUSE_MOTION; e.motion.windowID=geometry().id;
    e.motion.x=36; e.motion.y=8; push(e); frame();
    check(!(GetButtonPtr(id)->Area.uiFlags & MSYS_MOUSE_IN_AREA) && gusMouseYPos==-1,
          "bar motion clears production hover through empty-queue polling");
    check(SDL_SetWindowSize(window,256,160) && SDL_SyncWindow(window),"restore exact-aspect dispatch window");
    sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents();
}
void resolutionDispatch() {
    // Literal fitted rectangles for the existing authored 32x20 UI scene.
    const int cases[][6]={
        {1024,768,0,64,1024,640},{1280,720,64,0,1152,720},
        {1280,1024,0,112,1280,800},{1366,768,69,0,1228,768},
        {1600,900,80,0,1440,900},{1920,1080,96,0,1728,1080},
        {2560,1080,416,0,1728,1080},{2560,1440,128,0,2304,1440},
        {3440,1440,568,0,2304,1440},{3840,2160,192,0,3456,2160}
    };
    auto* window=SDL_GetWindowFromID(geometry().id);
    for (const auto& c:cases) {
        check(SDL_SetWindowSize(window,c[0],c[1]) && SDL_SyncWindow(window),"resize production UI to matrix output");
        fresh(); buttonDowns=buttonUps=0;
        const auto id=CreateHotSpot(2,3,7,5,31,DEFAULT_MOVE_CALLBACK,buttonRecord);
        check(id>=0,"matrix production hotspot"); GetButtonPtr(id)->uiFlags |= BUTTON_NO_TOGGLE;
        check(ColorFillVideoSurfaceArea(FRAME_BUFFER,0,0,32,20,0x2104) &&
              ColorFillVideoSurfaceArea(FRAME_BUFFER,2,3,9,8,0x001f),"draw matrix control using original framebuffer");
        InvalidateScreen();check(sgp::RefreshNativeVideo(),"present matrix production control");
        const float left=static_cast<float>(c[2]),top=static_cast<float>(c[3]);
        const float fittedWidth=static_cast<float>(c[4]),fittedHeight=static_cast<float>(c[5]);
        const float targetX=left+4.25f*fittedWidth/32,targetY=top+5.25f*fittedHeight/20;
        const float missX=left+11.25f*fittedWidth/32;
        auto* surface=SDL_GetWindowSurface(window);Uint8 red=0,green=0,blue=0,alpha=0;
        check(surface && surface->w==c[0] && surface->h==c[1] &&
              SDL_ReadSurfacePixel(surface,static_cast<int>(targetX),static_cast<int>(targetY),&red,&green,&blue,&alpha) &&
              red==0 && green==0 && blue==255,"matrix event point is visibly on the authored blue control");
        check(SDL_ReadSurfacePixel(surface,0,0,&red,&green,&blue,&alpha) && !red && !green && !blue,
              "matrix cancellation point is visibly in a black bar");
        const auto physicalButton=[&](bool down,float x,float y) {
            SDL_Event e{};e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;
            e.button.windowID=geometry().id;e.button.button=SDL_BUTTON_LEFT;e.button.down=down;
            e.button.x=x;e.button.y=y;push(e);
        };
        physicalButton(true,targetX,targetY);physicalButton(false,targetX,targetY);frame();
        check(buttonDowns==1 && buttonUps==1 && GetButtonPtr(id)->Area.MouseXPos==4 &&
              GetButtonPtr(id)->Area.MouseYPos==5,"matrix visible control activates at expected logical coordinates");
        physicalButton(true,missX,targetY);physicalButton(false,missX,targetY);frame();
        check(buttonDowns==1 && buttonUps==1,"matrix neighboring background does not activate control");
        physicalButton(true,targetX,targetY);frame();physicalButton(false,0,0);frame();
        check(buttonDowns==2 && buttonUps==1 && !gfLeftButtonState &&
              !(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),"matrix bar release cancels original anchor without activation");
        std::cout<<"UI MATRIX "<<c[0]<<'x'<<c[1]<<" PASS\n";
    }
    check(SDL_SetWindowSize(window,256,160) && SDL_SyncWindow(window),"restore dispatch window after resolution matrix");
    sgp::PumpNativeVideoEvents();DequeueAllKeyBoardEvents();
}
unsigned mutationCount=0; bool nested=false;
void mutate(MOUSE_REGION* r,INT32 reason) {
    if (reason!=MSYS_CALLBACK_REASON_GAIN_MOUSE) return;
    ++mutationCount; sgp::DispatchQueuedNativeGameMouseEvents(TRUE);
    InputAtom atom{}; atom.usEvent=MOUSE_POS; nested=sgp::DispatchNativeMouseEvent(atom);
    MSYS_RemoveRegion(r); define(*r,99);
}
void callbackLifetime() {
    fresh(); Region r; define(r,7,0,31,31,mutate); mutationCount=0; nested=true;
    motion(4,5); frame();
    check(mutationCount==1 && !nested && r.UserData[0]==99,"GAIN replacement rejects nested bridge/direct dispatch safely");
    for (const auto& o:trace) check(o.tag==99,"callback address reuse never invokes stale registration");
    trace.clear(); button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_LEFT,false,4,5); frame();
    const auto seen=clicks(); check(seen.size()==2 && seen[0].tag==99 && seen[1].tag==99,"later queued transitions target replacement registration");
}
unsigned helpDoneCalls=0;
void helpDone() { ++helpDoneCalls; }
void fastHelpMetadata() {
    fresh(); Region r; define(r,20); helpDoneCalls=0;
    CHAR16 source[]=L"alpha beta";
    SetRegionFastHelpText(&r,source); source[0]=L'X';
    check(r.FastHelpText && std::wcscmp(r.FastHelpText,L"alpha beta")==0,"fast-help setter owns a copy independent of caller text");
    SetRegionFastHelpText(&r,r.FastHelpText);
    check(std::wcscmp(r.FastHelpText,L"alpha beta")==0,"fast-help self replacement copies before release");
    SetRegionFastHelpText(&r,r.FastHelpText+6);
    check(std::wcscmp(r.FastHelpText,L"beta")==0,"fast-help substring replacement copies before release");
    auto* stable=r.FastHelpText;
    std::array<Region,128> additions;
    for (auto& added:additions) {
        define(added,21); CHAR16 shortText[]=L"x"; SetRegionFastHelpText(&added,shortText);
    }
    check(r.FastHelpText==stable && std::wcscmp(stable,L"beta")==0,"per-region heap text survives region-vector growth and insertion");
    for (auto& added:additions) check(added.FastHelpText && std::wcscmp(added.FastHelpText,L"x")==0,"relocated entries retain their independent short text buffers");
    SetRegionHelpEndCallback(&r,helpDone);
    check(r.HelpDoneCallback==helpDone && !helpDoneCalls,"help-end setter stores callback without executing it");
    SetRegionHelpEndCallback(&r,nullptr); check(!r.HelpDoneCallback,"help-end null replacement clears callback");
    SetRegionHelpEndCallback(nullptr,helpDone);
    SetRegionFastHelpText(&r,r.FastHelpText+4); check(!r.FastHelpText,"empty suffix clears owned fast-help text");
    CHAR16 again[]=L"replacement"; SetRegionFastHelpText(&r,again); SetRegionFastHelpText(&r,nullptr);
    check(!r.FastHelpText,"null fast-help replacement clears owned text");
    SetRegionFastHelpText(nullptr,again);
    Region unregistered; SetRegionFastHelpText(&unregistered,again);
    check(!unregistered.FastHelpText,"unregistered fast-help text is ignored safely");
    SetRegionHelpEndCallback(&unregistered,helpDone);
    check(unregistered.HelpDoneCallback==helpDone,"original help-end setter accepts any nonnull caller region");
    SetRegionFastHelpText(&r,again); SetRegionHelpEndCallback(&r,helpDone); MSYS_RemoveRegion(&r);
    check(!r.FastHelpText && !r.HelpDoneCallback,"region removal clears text and callback metadata and releases ownership");
    define(r,22); check(!r.FastHelpText && !r.HelpDoneCallback,"reused region address starts without old help metadata");
    SetRegionFastHelpText(&r,again); SetRegionHelpEndCallback(&r,helpDone); MSYS_Shutdown();
    check(!r.FastHelpText && !r.HelpDoneCallback && !helpDoneCalls,"MSYS shutdown releases metadata without executing help callbacks");
    for (const auto& added:additions) check(!added.FastHelpText,"shutdown clears each caller text pointer");
}
void configureHelpOnGain(MOUSE_REGION* r,INT32 reason) {
    record(r,reason);
    if (reason==MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        CHAR16 text[]=L"gain metadata";
        SetRegionFastHelpText(r,text); SetRegionHelpEndCallback(r,helpDone);
    }
}
void fastHelpGain() {
    fresh(); Region r; define(r,23,0,31,31,configureHelpOnGain); helpDoneCalls=0;
    button(SDL_BUTTON_LEFT,true,4,5); frame(); button(SDL_BUTTON_LEFT,false,4,5); frame();
    check(trace.size()==4 && trace[0].reason==128 && trace[1].reason==2 && trace[2].reason==4 && trace[3].reason==8,"cosmetic help metadata inside GAIN preserves MOVE and queued DOWN/UP progress");
    check(r.FastHelpText && std::wcscmp(r.FastHelpText,L"gain metadata")==0 && r.HelpDoneCallback==helpDone && !helpDoneCalls,"GAIN metadata retains owned text and callback without rendering or completion execution");
}
void terminalBacklog() {
    fresh(); Region r; define(r,8); motion(4,5); frame(); trace.clear();
    button(SDL_BUTTON_LEFT,true,4,5); button(SDL_BUTTON_LEFT,false,4,5); windowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED); frame();
    check(sgp::NativeInputTerminated() && trace.empty() && !r.ButtonState && !(r.uiFlags & MSYS_MOUSE_IN_AREA),"close before frame rejects retained mouse backlog and hover");
    QueueEvent(LEFT_BUTTON_DOWN,0,0x00050004u); frame(); check(trace.empty() && !r.ButtonState,"terminal queue remains nonactivating");
    QueueEvent(KEY_DOWN,97,0x00050004u); QueueEvent(X1_BUTTON_DOWN,0,0x00050004u); frame();
    InputAtom atom{}; check(DequeueEvent(&atom) && atom.usEvent==KEY_DOWN && trace.empty(),"terminal reset preserves keyboard head while cancelling UI");
    frame(); check(sgp::NativeInputQueueEmpty() && trace.empty() && !r.ButtonState,"terminal extended backlog never activates after keyboard dequeue");
    ShutdownInputManager(); check(sgp::ShutdownNativeVideo(),"terminal video shutdown");
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownInputManager(); (void)sgp::ShutdownNativeVideo();
    ShutdownVideoSurfaceManager(); SDL_Quit();
}
}
int main() {
    try {
        check(sgp::ConfigureNativeVideo(32,20,16,8,"P710 original GameLoop mouse contracts") && InitializeVideoManager(nullptr,0,nullptr),"open native owned window");
        check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"real X11 window required");
        check(InitializeInputManager(),"bind actual input service");
        queuedHistory(); keyboardBoundary(); priorityTies(); hoverReset(); extendedState(); clickedWheel(); wheelBounds(); anchors(); fittedDispatch(); resolutionDispatch(); callbackLifetime(); fastHelpMetadata(); fastHelpGain(); terminalBacklog(); cleanup();
        std::cout<<"PASS: "<<checks<<" original GameLoop mouse bridge assertions; real X11 owned window; synthetic SDL input, no human-input claim\n"; return 0;
    } catch (const std::exception& e) { cleanup(); std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; return 1; }
}
