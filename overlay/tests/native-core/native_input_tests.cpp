/*
 * P710 native snapshot update: tests/native-core/native_input_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "input.h"
#include "native_video.h"
#include "english.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
namespace {
unsigned assertions=0;
void check(bool b,const char* why) { ++assertions; if (!b) throw std::runtime_error(why); }
SDL_WindowID owned() { SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0; check(sgp::NativeVideoInputInfo(id,w,h,ww,wh),"owned geometry"); return id; }
void push(SDL_Event e) { check(SDL_PushEvent(&e),"SDL_PushEvent"); }
void windowEvent(Uint32 type,SDL_WindowID id=0) { SDL_Event e{}; e.type=type; e.window.windowID=id?id:owned(); push(e); }
void key(SDL_Scancode s,bool down,SDL_Keymod mod=SDL_KMOD_NONE,SDL_WindowID id=0) { SDL_Event e{}; e.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP; e.key.windowID=id?id:owned(); e.key.scancode=s; e.key.mod=mod; e.key.down=down; push(e); }
void motion(float x,float y,SDL_WindowID id=0) { SDL_Event e{}; e.type=SDL_EVENT_MOUSE_MOTION; e.motion.windowID=id?id:owned(); e.motion.x=x; e.motion.y=y; push(e); }
void button(Uint8 b,bool down,float x=104,float y=120,SDL_WindowID id=0) { SDL_Event e{}; e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP; e.button.windowID=id?id:owned(); e.button.button=b; e.button.down=down; e.button.x=x; e.button.y=y; push(e); }
void wheel(float y,bool flipped=false) { SDL_Event e{}; e.type=SDL_EVENT_MOUSE_WHEEL; e.wheel.windowID=owned(); e.wheel.y=y; e.wheel.direction=flipped?SDL_MOUSEWHEEL_FLIPPED:SDL_MOUSEWHEEL_NORMAL; e.wheel.mouse_x=104; e.wheel.mouse_y=120; push(e); }
InputAtom atom(UINT16 type,UINT32 param,UINT16 mods=0,UINT32 pos=0x000a0010u) { InputAtom a{}; check(DequeueEvent(&a),"atom exists"); if (!(a.usEvent==type && a.usParam==param && a.usKeyState==mods && a.uiParam==pos)) std::cerr<<"got event/key/mods/pos="<<a.usEvent<<"/"<<a.usParam<<"/"<<a.usKeyState<<"/"<<a.uiParam<<" expected="<<type<<"/"<<param<<"/"<<mods<<"/"<<pos<<"\n"; check(a.usEvent==type && a.usParam==param && a.usKeyState==mods && a.uiParam==pos,"literal atom fields"); return a; }
void empty() { InputAtom a{1,2,3,4,5}; check(!DequeueEvent(&a),"empty queue"); check(a.uiTimeStamp==1 && a.usKeyState==2 && a.usEvent==3 && a.usParam==4 && a.uiParam==5,"empty leaves output"); }
void reset() { windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents(); }
void clearHeld() { for (auto b:gfKeyState) check(!b,"no stuck key"); check(!gfLeftButtonState && !gfRightButtonState && !gfMiddleButtonState && !gsMouseWheelDeltaValue,"no stuck buttons/wheel"); }
unsigned callbacks=0;
void observer(void*,const SDL_Event* e) { if (e) { ++callbacks; sgp::PumpNativeVideoEvents(); (void)sgp::NativeVideoQuitRequested(); } }
void open() { check(sgp::ConfigureNativeVideo(32,20,16,8,"P710 input contracts"),"configure"); check(InitializeVideoManager(nullptr,0,nullptr),"video init"); check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"actual X11 required"); sgp::PumpNativeVideoEvents(); }
}
int main()
{
    try {
        ShutdownInputManager(); ShutdownInputManager(); check(!InitializeInputManager(),"input before video fails"); InputAtom untouched{1,2,3,4,5}; check(!DequeueEvent(&untouched) && !PeekSpecificEvent(0xffff),"inactive APIs"); QueueEvent(KEY_DOWN,97,0); GetMousePos(nullptr);
        open(); check(InitializeInputManager(),"input init"); check(!gfTrackMousePos,"movement defaults off"); reset(); clearHeld(); check(!_KeyDown(256) && !_KeyDown(-1),"poll macro bounds");
        int competitor=0; check(!sgp::AttachNativeVideoInput(observer,&competitor),"competing sink rejected");
        const auto before=static_cast<UINT32>(SDL_GetTicks()); QueueEvent(KEY_DOWN,0x12345678u,0xabcdef01u); check(InitializeInputManager(),"repeat init"); const auto first=atom(KEY_DOWN,0x12345678u,0,0xabcdef01u); const auto after=static_cast<UINT32>(SDL_GetTicks()); check(static_cast<UINT32>(first.uiTimeStamp-before)<=static_cast<UINT32>(after-before),"monotonic millisecond snapshot"); check(!gfKeyState[65],"QueueEvent does not change held state");
        QueueEvent(MOUSE_POS,12,34); QueueEvent(KEY_UP,56,78); InputAtom miss{}; check(!PeekSpecificEvent(KEY_UP),"peek only head"); check(!DequeueSpecificEvent(&miss,KEY_UP) && miss.usEvent==MOUSE_POS && miss.usParam==12 && miss.uiParam==34,"mask miss copies head"); check(!DequeueEvent(nullptr) && !DequeueSpecificEvent(nullptr,0xffff),"null does not drain"); check(PeekSpecificEvent(MOUSE_POS|KEY_DOWN),"head bitwise mask"); atom(MOUSE_POS,12,0,34); check(DequeueSpecificEvent(&miss,KEY_UP) && miss.usParam==56,"specific consumes matching head"); empty();
        for (unsigned i=0;i<256;++i) QueueEvent(KEY_DOWN,1000+i,2000+i);
        QueueEvent(KEY_DOWN,9999,9999);
        for (unsigned i=0;i<128;++i) atom(KEY_DOWN,1000+i,0,2000+i);
        for (unsigned i=0;i<128;++i) QueueEvent(KEY_UP,3000+i,4000+i);
        for (unsigned i=128;i<256;++i) atom(KEY_DOWN,1000+i,0,2000+i);
        for (unsigned i=0;i<128;++i) atom(KEY_UP,3000+i,0,4000+i);
        empty();
        struct Mapping { SDL_Scancode scan; unsigned index,value; SDL_Keymod mod; };
        const Mapping mappings[]={{SDL_SCANCODE_A,65,97,SDL_KMOD_NONE},{SDL_SCANCODE_Z,90,122,SDL_KMOD_NONE},{SDL_SCANCODE_0,48,48,SDL_KMOD_NONE},{SDL_SCANCODE_9,57,57,SDL_KMOD_NONE},{SDL_SCANCODE_F1,112,124,SDL_KMOD_NONE},{SDL_SCANCODE_F12,123,135,SDL_KMOD_NONE},{SDL_SCANCODE_INSERT,245,245,SDL_KMOD_NONE},{SDL_SCANCODE_DELETE,246,246,SDL_KMOD_NONE},{SDL_SCANCODE_END,247,247,SDL_KMOD_NONE},{SDL_SCANCODE_DOWN,248,248,SDL_KMOD_NONE},{SDL_SCANCODE_PAGEDOWN,249,249,SDL_KMOD_NONE},{SDL_SCANCODE_LEFT,250,250,SDL_KMOD_NONE},{SDL_SCANCODE_RIGHT,251,251,SDL_KMOD_NONE},{SDL_SCANCODE_HOME,252,252,SDL_KMOD_NONE},{SDL_SCANCODE_UP,253,253,SDL_KMOD_NONE},{SDL_SCANCODE_PAGEUP,254,254,SDL_KMOD_NONE},{SDL_SCANCODE_KP_0,223,48,SDL_KMOD_NUM},{SDL_SCANCODE_KP_PERIOD,224,46,SDL_KMOD_NUM},{SDL_SCANCODE_KP_1,225,49,SDL_KMOD_NUM},{SDL_SCANCODE_KP_9,233,57,SDL_KMOD_NUM},{SDL_SCANCODE_KP_0,234,245,SDL_KMOD_NONE},{SDL_SCANCODE_KP_PERIOD,235,246,SDL_KMOD_NONE},{SDL_SCANCODE_KP_1,236,247,SDL_KMOD_NONE},{SDL_SCANCODE_KP_5,240,0,SDL_KMOD_NONE},{SDL_SCANCODE_KP_9,244,254,SDL_KMOD_NONE},{SDL_SCANCODE_KP_PLUS,107,43,SDL_KMOD_NONE},{SDL_SCANCODE_KP_DIVIDE,111,47,SDL_KMOD_NONE},{SDL_SCANCODE_ESCAPE,27,27,SDL_KMOD_NONE},{SDL_SCANCODE_RETURN,13,13,SDL_KMOD_NONE},{SDL_SCANCODE_TAB,9,9,SDL_KMOD_NONE},{SDL_SCANCODE_BACKSPACE,8,8,SDL_KMOD_NONE},{SDL_SCANCODE_SPACE,32,32,SDL_KMOD_NONE},{SDL_SCANCODE_SEMICOLON,186,59,SDL_KMOD_NONE},{SDL_SCANCODE_APOSTROPHE,222,39,SDL_KMOD_NONE}};
        for (const auto& m:mappings) { key(m.scan,true,m.mod); sgp::PumpNativeVideoEvents(); check(gfKeyState[m.index],"normalized polling index"); atom(KEY_DOWN,m.value); key(m.scan,true,m.mod); atom(KEY_REPEAT,m.value); key(m.scan,false,SDL_KMOD_NONE); atom(KEY_UP,m.value); check(!gfKeyState[m.index],"held release index retained"); key(m.scan,false,m.mod); empty(); }
        key(SDL_SCANCODE_LSHIFT,true); key(SDL_SCANCODE_RSHIFT,true); key(SDL_SCANCODE_LSHIFT,false); key(SDL_SCANCODE_A,true); atom(KEY_DOWN,65,1); check(gfKeyState[SHIFT],"second Shift side held"); key(SDL_SCANCODE_RSHIFT,false); key(SDL_SCANCODE_A,false); atom(KEY_UP,97); check(!gfKeyState[SHIFT],"last Shift release");
        key(SDL_SCANCODE_LCTRL,true); key(SDL_SCANCODE_RCTRL,true); key(SDL_SCANCODE_LCTRL,false); key(SDL_SCANCODE_LALT,true); key(SDL_SCANCODE_RALT,true); key(SDL_SCANCODE_LALT,false); key(SDL_SCANCODE_A,true); atom(KEY_DOWN,97,6); check(gfKeyState[CTRL] && gfKeyState[ALT],"Ctrl/Alt independent sides and normal-bank quirk"); QueueEvent(MOUSE_POS,7,8); key(SDL_SCANCODE_RCTRL,false); key(SDL_SCANCODE_RALT,false); key(SDL_SCANCODE_A,false); atom(MOUSE_POS,7,6,8); atom(KEY_UP,97); empty();
        key(SDL_SCANCODE_LSHIFT,true); key(SDL_SCANCODE_F1,true); atom(KEY_DOWN,368,1); key(SDL_SCANCODE_F1,false); atom(KEY_UP,368,1); key(SDL_SCANCODE_TAB,true); atom(KEY_DOWN,265,1); key(SDL_SCANCODE_TAB,false); atom(KEY_UP,265,1); key(SDL_SCANCODE_LSHIFT,false); sgp::PumpNativeVideoEvents(); empty();
        key(SDL_SCANCODE_RETURN,true); key(SDL_SCANCODE_KP_ENTER,true); atom(KEY_DOWN,13); atom(KEY_REPEAT,13); key(SDL_SCANCODE_RETURN,false); sgp::PumpNativeVideoEvents(); check(gfKeyState[13],"shared normalized key remains held"); empty(); key(SDL_SCANCODE_KP_ENTER,false); atom(KEY_UP,13);
        key(SDL_SCANCODE_UNKNOWN,true); key(static_cast<SDL_Scancode>(SDL_SCANCODE_COUNT),true); key(SDL_SCANCODE_F13,true); SDL_Event text{}; text.type=SDL_EVENT_TEXT_INPUT; text.text.windowID=owned(); text.text.text="ignored"; push(text); empty(); clearHeld();
        motion(104,120); sgp::PumpNativeVideoEvents(); check(gusMouseXPos==13 && gusMouseYPos==15,"scaled movement polling without atom"); empty(); SGPPoint p{}; GetMousePos(&p); check(p.iX==13 && p.iY==15,"existing SGPPoint"); GetMousePos(nullptr);
        gfTrackMousePos=TRUE; motion(104,120); auto pos=atom(MOUSE_POS,0,0,0x000f000du); check(GETXPOS(&pos)==13 && GETYPOS(&pos)==15 && _EvMouseX(&pos)==13 && _EvMouseY(&pos)==15,"portable position macros"); check(InputEventInside(&pos,13,15,13,15) && !InputEventInside(&pos,14,15,20,19) && !InputEventInside(nullptr,0,0,31,19),"inclusive edges/null");
        motion(-100,-200); atom(MOUSE_POS,0,0,0xffe7fff3u); motion(1e30f,1e30f); atom(MOUSE_POS,0,0,0x7fff7fffu); motion(std::numeric_limits<float>::infinity(),1); motion(1,std::numeric_limits<float>::quiet_NaN()); empty(); check(gusMouseXPos==32767 && gusMouseYPos==32767,"nonfinite movement ignored");
        button(SDL_BUTTON_LEFT,true); button(SDL_BUTTON_RIGHT,true); button(SDL_BUTTON_MIDDLE,true); atom(LEFT_BUTTON_DOWN,0,0,0x000f000du); atom(RIGHT_BUTTON_DOWN,0,0,0x000f000du); atom(MIDDLE_BUTTON_DOWN,0,0,0x000f000du); check(gfLeftButtonState && gfRightButtonState && gfMiddleButtonState,"all three held buttons");
        button(99,true,0,0); empty(); check(gusMouseXPos==13 && gusMouseYPos==15,"unknown button ignored including position"); button(SDL_BUTTON_LEFT,false); button(SDL_BUTTON_RIGHT,false); button(SDL_BUTTON_MIDDLE,false); atom(LEFT_BUTTON_UP,0,0,0x000f000du); atom(RIGHT_BUTTON_UP,0,0,0x000f000du); atom(MIDDLE_BUTTON_UP,0,0,0x000f000du); clearHeld();
        wheel(1e30f); atom(MOUSE_WHEEL_UP,0,0,0x000f000du); check(gsMouseWheelDeltaValue==32767,"wheel positive saturation"); wheel(1e30f,true); atom(MOUSE_WHEEL_DOWN,0,0,0x000f000du); check(gsMouseWheelDeltaValue==-32768,"wheel flipped negative saturation"); wheel(2.75f); atom(MOUSE_WHEEL_UP,0,0,0x000f000du); check(gsMouseWheelDeltaValue==2,"wheel truncation"); wheel(std::numeric_limits<float>::infinity()); wheel(std::numeric_limits<float>::quiet_NaN()); empty(); check(gsMouseWheelDeltaValue==2,"nonfinite wheel ignored");
        const auto foreign=owned()+1000; key(SDL_SCANCODE_A,true,SDL_KMOD_NONE,foreign); motion(0,0,foreign); button(SDL_BUTTON_LEFT,true,0,0,foreign); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST,foreign); windowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED,foreign); check(!sgp::NativeVideoQuitRequested(),"foreign close not quit"); empty(); check(!gfKeyState[65] && !gfLeftButtonState && gusMouseXPos==13,"foreign input excluded");
        key(SDL_SCANCODE_A,true); button(SDL_BUTTON_LEFT,true); sgp::PumpNativeVideoEvents(); QueueEvent(RIGHT_BUTTON_DOWN,123,456); DequeueAllKeyBoardEvents(); empty(); check(gfKeyState[65] && gfLeftButtonState,"whole queue drain preserves held polling state");
        // A physical release can enter SDL immediately before focus loss in one pump.
        key(SDL_SCANCODE_A,false); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); sgp::PumpNativeVideoEvents(); clearHeld();
        atom(LEFT_BUTTON_UP,0,0,0x000f000du); atom(KEY_UP,97,0,0x000f000du); empty();
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); key(SDL_SCANCODE_A,true); button(SDL_BUTTON_LEFT,true); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents();
        key(SDL_SCANCODE_LSHIFT,true); button(SDL_BUTTON_RIGHT,true); button(SDL_BUTTON_MIDDLE,true); sgp::PumpNativeVideoEvents(); QueueEvent(KEY_REPEAT,65,0); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); windowEvent(SDL_EVENT_WINDOW_HIDDEN); sgp::PumpNativeVideoEvents(); clearHeld(); atom(KEY_UP,97,0,0x000f000du); atom(LEFT_BUTTON_UP,0,0,0x000f000du); atom(RIGHT_BUTTON_UP,0,0,0x000f000du); atom(MIDDLE_BUTTON_UP,0,0,0x000f000du); empty(); key(SDL_SCANCODE_B,true); button(SDL_BUTTON_LEFT,true); empty(); clearHeld();
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); key(SDL_SCANCODE_A,true); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents(); for (unsigned i=0;i<256;++i) QueueEvent(KEY_REPEAT,i,i); key(SDL_SCANCODE_B,true); sgp::PumpNativeVideoEvents(); check(gfKeyState[65] && gfKeyState[66],"poll state updates at full FIFO"); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); sgp::PumpNativeVideoEvents(); clearHeld(); atom(KEY_UP,97,0,0x000f000du); atom(KEY_UP,98,0,0x000f000du); empty();
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); sgp::PumpNativeVideoEvents();
        // Independent literal fit oracles: 32x20 in 256x192 has 16-pixel
        // top/bottom bars; 320x160 has 32-pixel left/right bars.
        check(SDL_SetWindowSize(SDL_GetWindowFromID(owned()),256,192) && SDL_SyncWindow(SDL_GetWindowFromID(owned())),"letterbox actual window");
        reset();
        sgp::NativePresentationGeometry fit{};
        check(sgp::NativeVideoPresentationGeometry(fit) && fit.windowWidth==256 && fit.windowHeight==192 &&
              fit.pixelWidth==256 && fit.pixelHeight==192 && fit.contentX==0 && fit.contentY==16 &&
              fit.contentWidth==256 && fit.contentHeight==160,"literal letterbox fit readback");
        motion(104,136); atom(MOUSE_POS,0,0,0x000f000du);
        motion(0,16); atom(MOUSE_POS,0,0,0);
        motion(255.5f,175.5f); atom(MOUSE_POS,0,0,0x0013001fu);
        motion(104,8); atom(MOUSE_POS,0,0,0xffff000du);
        button(SDL_BUTTON_LEFT,true,104,8); button(SDL_BUTTON_LEFT,false,104,8); empty();
        check(!gfLeftButtonState,"letterbox press never becomes held");
        SDL_Event barWheel{}; barWheel.type=SDL_EVENT_MOUSE_WHEEL; barWheel.wheel.windowID=owned();
        barWheel.wheel.y=1; barWheel.wheel.mouse_x=104; barWheel.wheel.mouse_y=8; push(barWheel); empty();
        button(SDL_BUTTON_LEFT,true,104,136); atom(LEFT_BUTTON_DOWN,0,0,0x000f000du);
        button(SDL_BUTTON_LEFT,false,104,8); atom(LEFT_BUTTON_UP,0,0,0xffff000du);
        check(!gfLeftButtonState,"held inside press releases through letterbox");
        button(SDL_BUTTON_RIGHT,true,104,176); empty(); check(!gfRightButtonState,"exclusive fitted bottom cannot press");
        check(SDL_SetWindowSize(SDL_GetWindowFromID(owned()),320,160) && SDL_SyncWindow(SDL_GetWindowFromID(owned())),"pillarbox actual window");
        reset();
        check(sgp::NativeVideoPresentationGeometry(fit) && fit.contentX==32 && fit.contentY==0 &&
              fit.contentWidth==256 && fit.contentHeight==160,"literal pillarbox fit readback");
        motion(136,120); atom(MOUSE_POS,0,0,0x000f000du);
        motion(24,120); atom(MOUSE_POS,0,0,0x000fffffu);
        button(SDL_BUTTON_LEFT,true,24,120); button(SDL_BUTTON_LEFT,false,24,120); empty();
        button(SDL_BUTTON_MIDDLE,true,136,120); atom(MIDDLE_BUTTON_DOWN,0,0,0x000f000du);
        button(SDL_BUTTON_MIDDLE,false,288,120); atom(MIDDLE_BUTTON_UP,0,0,0x000f0020u);
        check(!gfMiddleButtonState,"held middle releases through exclusive pillarbox right edge");
        check(SDL_SetWindowSize(SDL_GetWindowFromID(owned()),512,320) && SDL_SyncWindow(SDL_GetWindowFromID(owned())),"resize actual window"); motion(208,240); atom(MOUSE_POS,0,0,0x000f000du);
        // Subsequent lifecycle cases close/reopen; do not assume the WM accepts an immediate shrink.
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); key(SDL_SCANCODE_A,true); key(SDL_SCANCODE_A,false); SDL_Event quit{}; quit.type=SDL_EVENT_QUIT; push(quit); key(SDL_SCANCODE_B,true); check(sgp::NativeVideoQuitRequested() && sgp::NativeVideoQuitRequested(),"global quit latched, complete pump"); atom(KEY_DOWN,97,0,0x000f000du); atom(KEY_UP,97,0,0x000f000du); empty(); clearHeld();
        ShutdownInputManager(); ShutdownInputManager(); check(!DequeueEvent(&untouched) && untouched.usParam==4,"shutdown safe/untouched"); check(sgp::ShutdownNativeVideo(),"quit video close");
        open(); check(!sgp::NativeVideoQuitRequested(),"reopen clears quit latch"); check(InitializeInputManager(),"reinitialize manager"); reset(); key(SDL_SCANCODE_A,true); button(SDL_BUTTON_LEFT,true); sgp::PumpNativeVideoEvents(); check(gfKeyState[65] && gfLeftButtonState,"held before destruction"); check(sgp::ShutdownNativeVideo(),"video destroyed before input"); clearHeld(); open(); check(!InitializeInputManager(),"old binding cannot resurrect on reopen"); key(SDL_SCANCODE_B,true); sgp::PumpNativeVideoEvents(); check(!gfKeyState[66],"no stale callback on reopened window"); ShutdownInputManager(); check(InitializeInputManager(),"explicit fresh binding"); reset();
        for (unsigned i=0;i<256;++i) QueueEvent(KEY_REPEAT,i,i);
        key(SDL_SCANCODE_A,true); sgp::PumpNativeVideoEvents(); windowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED); check(sgp::NativeVideoQuitRequested() && sgp::NativeVideoQuitRequested(),"owned close latch at full FIFO"); clearHeld(); ShutdownInputManager(); check(sgp::ShutdownNativeVideo(),"close teardown");
        open(); callbacks=0; check(sgp::AttachNativeVideoInput(observer,&competitor) && sgp::AttachNativeVideoInput(observer,&competitor),"idempotent observer attach"); int other=0; check(!sgp::AttachNativeVideoInput(observer,&other),"context competitor refused"); motion(104,120); sgp::PumpNativeVideoEvents(); check(callbacks==1,"recursive pump guarded"); sgp::DetachNativeVideoInput(observer,&other); motion(104,120); sgp::PumpNativeVideoEvents(); check(callbacks==2,"wrong owner cannot detach"); sgp::DetachNativeVideoInput(observer,&competitor); sgp::DetachNativeVideoInput(observer,&competitor); motion(104,120); sgp::PumpNativeVideoEvents(); check(callbacks==2,"detached callback never invoked");
        check(sgp::ShutdownNativeVideo(),"final video shutdown"); check(ShutdownVideoSurfaceManager(),"final surfaces shutdown"); std::cout<<"PASS: "<<assertions<<" assertions; real X11 window; SDL-queue injection (not human input)\n"; return 0;
    } catch (const std::exception& e) {
        ShutdownInputManager(); (void)sgp::ShutdownNativeVideo(); ShutdownVideoSurfaceManager(); std::cerr<<"FAIL after "<<assertions<<" assertions: "<<e.what()<<'\n'; return 1;
    }
}
