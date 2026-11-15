/*
 * P710 native snapshot update: tests/native-core/native_input_viewer.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "input.h"
#include "native_video.h"
#include "clip_state.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
unsigned assertions=0;
void check(bool b,const char* why) { ++assertions; if (!b) throw std::runtime_error(why); }
SDL_WindowID id=0;
bool aHeld=false, leftHeld=false, marker=false;
bool sawDown=false,sawUp=false,sawMotion=false,sawMouseDown=false,sawMouseUp=false;
void push(SDL_Event e) { check(SDL_PushEvent(&e),"push scripted event"); }
void windowEvent(Uint32 type) { SDL_Event e{}; e.type=type; e.window.windowID=id; push(e); }
void draw()
{
    check(ColorFillVideoSurfaceArea(FRAME_BUFFER,0,0,32,20,0x2104),"background");
    check(ColorFillVideoSurfaceArea(FRAME_BUFFER,2,2,10,10,aHeld?0xf800:0x001f),"A panel");
    check(ColorFillVideoSurfaceArea(FRAME_BUFFER,20,2,28,10,leftHeld?0x07e0:0x001f),"mouse panel");
    if (marker) check(ColorFillVideoSurfaceArea(FRAME_BUFFER,13,15,14,16,0xffff),"marker");
    InvalidateScreen(); check(sgp::RefreshNativeVideo(),"present authored scene");
}
void capture(const std::string& stem)
{
    check(sgp::SaveNativeVideoWindow((stem+".bmp").c_str()),"save actual window");
    UINT32 pitch=0; const auto* pixels=static_cast<const BYTE*>(LockPrimarySurface(&pitch)); check(pixels!=nullptr,"lock primary");
    std::ofstream raw(stem+".rgb565",std::ios::binary); raw.write(reinterpret_cast<const char*>(pixels),static_cast<std::streamsize>(pitch)*20); raw.close(); const bool ok=static_cast<bool>(raw); UnlockPrimarySurface(); check(ok && pitch==72,"save actual 1440-byte padded primary");
}
unsigned consume()
{
    unsigned n=0; InputAtom e{};
    while (DequeueEvent(&e)) {
        ++n; std::cout<<"ATOM time="<<e.uiTimeStamp<<" event="<<e.usEvent<<" key="<<e.usParam<<" mods="<<e.usKeyState<<" position="<<GETXPOS(&e)<<','<<GETYPOS(&e)<<'\n';
        if (e.usEvent==KEY_DOWN && (e.usParam==97 || e.usParam==65)) { aHeld=true; sawDown=true; }
        if (e.usEvent==KEY_UP && (e.usParam==97 || e.usParam==65)) { aHeld=false; sawUp=true; }
        if (e.usEvent==LEFT_BUTTON_DOWN) { leftHeld=true; sawMouseDown=true; }
        if (e.usEvent==LEFT_BUTTON_UP) { leftHeld=false; sawMouseUp=true; }
        if (e.usEvent==MOUSE_POS && GETXPOS(&e)==13 && GETYPOS(&e)==15) { marker=true; sawMotion=true; }
    }
    std::cout.flush(); return n;
}
void noHeld() { for (auto held:gfKeyState) check(!held,"no held key"); check(!gfLeftButtonState && !gfRightButtonState && !gfMiddleButtonState,"no held buttons"); }
void showBriefly() { const auto end=SDL_GetTicks()+120; while (SDL_GetTicks()<end && !sgp::NativeVideoQuitRequested()) { consume(); SDL_Delay(8); } }
void cleanup() { ShutdownInputManager(); ShutdownVideoManager(); ShutdownVideoSurfaceManager(); }
}
int main(int argc,char** argv)
{
    try {
        check(argc==2 || argc==3,"usage: output-base [--scripted|--listen]");
        const bool trace=argc==3 && std::strcmp(argv[2],"--trace")==0;
        const bool listen=trace || (argc==3 && std::strcmp(argv[2],"--listen")==0);
        check(argc==2 || listen || std::strcmp(argv[2],"--scripted")==0,"unknown mode");
        const std::string base=argv[1]; const auto parent=std::filesystem::path(base).parent_path(); if (!parent.empty()) std::filesystem::create_directories(parent);
        check(sgp::ConfigureNativeVideo(32,20,16,8,"P710 native input scene"),"configure"); check(InitializeVideoManager(nullptr,0,nullptr),"video initialize"); check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"actual X11 required");
        // Only the scripted fixture drains startup traffic. Listen never injects.
        if (!listen) sgp::PumpNativeVideoEvents();
        check(InitializeInputManager(),"input initialize"); int w=0,h=0,ww=0,wh=0; check(sgp::NativeVideoInputInfo(id,w,h,ww,wh),"owned input info"); check(w==32 && h==20 && ww==256 && wh==160,"owned geometry");
        SGPRect clip{0,0,32,20}; SetClippingRect(&clip); gfTrackMousePos=TRUE; draw();
        bool pressed=false,released=false,quit=false;
        if (listen) {
            const auto xid=SDL_GetNumberProperty(SDL_GetWindowProperties(SDL_GetWindowFromID(id)),SDL_PROP_WINDOW_X11_WINDOW_NUMBER,0);
            check(xid!=0,"X11 diagnostic window ID"); std::cout<<"READY SDL_WINDOW_ID="<<id<<" X11_WINDOW="<<xid<<" mode=listen no-injection\n"<<std::flush;
            const auto deadline=SDL_GetTicks()+8000;
            while (SDL_GetTicks()<deadline) {
                quit=sgp::NativeVideoQuitRequested(); const auto n=consume(); if (n) draw();
                if (!trace && !pressed && aHeld && leftHeld && marker && gfKeyState[65] && gfLeftButtonState && gusMouseXPos==13 && gusMouseYPos==15) { capture(base+"-pressed"); pressed=true; }
                if (!trace && pressed && !released && !aHeld && !leftHeld && !gfKeyState[65] && !gfLeftButtonState) { draw(); capture(base+"-released"); released=true; }
                if (quit) break;
                SDL_Delay(8);
            }
        } else {
            std::cout<<"mode=scripted; SDL_PushEvent queue injection, not physical-human acceptance\n";
            windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
            SDL_Event key{}; key.type=SDL_EVENT_KEY_DOWN; key.key.windowID=id; key.key.scancode=SDL_SCANCODE_A; key.key.down=true; push(key);
            SDL_Event move{}; move.type=SDL_EVENT_MOUSE_MOTION; move.motion.windowID=id; move.motion.x=104; move.motion.y=120; push(move);
            SDL_Event button{}; button.type=SDL_EVENT_MOUSE_BUTTON_DOWN; button.button.windowID=id; button.button.button=SDL_BUTTON_LEFT; button.button.down=true; button.button.x=104; button.button.y=120; push(button);
            check(!sgp::NativeVideoQuitRequested(),"quit polling pumps without losing atoms"); check(gfKeyState[65] && gfLeftButtonState && gusMouseXPos==13 && gusMouseYPos==15,"scripted pressed polling");
            InputAtom e{}; check(DequeueEvent(&e) && e.usEvent==KEY_DOWN && e.usParam==97 && e.usKeyState==0,"literal A down atom"); aHeld=true;
            check(DequeueEvent(&e) && e.usEvent==MOUSE_POS && e.usParam==0 && e.usKeyState==0 && e.uiParam==0x000f000du,"literal marker atom"); marker=true;
            check(DequeueEvent(&e) && e.usEvent==LEFT_BUTTON_DOWN && e.usParam==0 && e.usKeyState==0 && e.uiParam==0x000f000du,"literal left down atom"); leftHeld=true; check(!DequeueEvent(&e),"exact pressed atom count");
            draw(); capture(base+"-pressed"); pressed=true; showBriefly();
            windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); sgp::PumpNativeVideoEvents(); noHeld(); check(consume()==2 && !aHeld && !leftHeld,"dequeued synthetic key/button releases"); draw(); capture(base+"-released"); released=true; showBriefly();
            windowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED); quit=sgp::NativeVideoQuitRequested(); consume(); check(quit && sgp::NativeVideoQuitRequested(),"owned close quit latch");
        }
        std::cout<<"FINAL A="<<static_cast<unsigned>(gfKeyState[65])<<" left="<<static_cast<unsigned>(gfLeftButtonState)<<" right="<<static_cast<unsigned>(gfRightButtonState)<<" middle="<<static_cast<unsigned>(gfMiddleButtonState)<<" quit="<<quit<<" pressed="<<pressed<<" released="<<released<<'\n'<<std::flush;
        if (trace) { std::cout<<"TRACE down="<<sawDown<<" up="<<sawUp<<" motion="<<sawMotion<<" mouseDown="<<sawMouseDown<<" mouseUp="<<sawMouseUp<<"\n"; check(sawDown && sawUp && sawMotion && sawMouseDown && sawMouseUp && quit,"native event trace and quit"); }
        else check(pressed && released && quit,"two captures and safe quit required");
        noHeld(); ShutdownInputManager(); check(sgp::ShutdownNativeVideo(),"video teardown"); check(ShutdownVideoSurfaceManager(),"surface teardown");
        std::cout<<"PASS: "<<assertions<<" assertions; input verification complete\n"; return 0;
    } catch (const std::exception& e) { cleanup(); std::cerr<<"FAIL after "<<assertions<<" assertions: "<<e.what()<<'\n'; return 1; }
}
