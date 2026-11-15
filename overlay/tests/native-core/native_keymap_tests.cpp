/*
 * P710 native snapshot update: tests/native-core/native_keymap_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original KeyMap + original Text Utils + production native input, no game data.
#include "KeyMap.h"
#include "native_input.h"
#include "native_video.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
unsigned checks=0;
void check(bool ok,const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
int parse(const char* text) { return ParseKeyString(const_cast<char*>(text)); }
UINT32 bits(int value) { UINT32 out; std::memcpy(&out,&value,sizeof(out)); return out; }
SDL_WindowID owned() { SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0; check(sgp::NativeVideoInputInfo(id,w,h,ww,wh),"owned window"); return id; }
void push(SDL_Event event) { check(SDL_PushEvent(&event),"inject SDL queue event"); }
void focus(Uint32 type,SDL_WindowID id=0) { SDL_Event event{}; event.type=type; event.window.windowID=id?id:owned(); push(event); }
void key(SDL_Scancode scan,bool down,SDL_Keymod mod=SDL_KMOD_NONE,SDL_WindowID id=0) { SDL_Event event{}; event.type=down?SDL_EVENT_KEY_DOWN:SDL_EVENT_KEY_UP; event.key.windowID=id?id:owned(); event.key.scancode=scan; event.key.mod=mod; event.key.down=down; push(event); }
void mouse(Uint8 button,bool down) { SDL_Event event{}; event.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP; event.button.windowID=owned(); event.button.button=button; event.button.down=down; event.button.x=1; event.button.y=1; push(event); }
void reset() { focus(SDL_EVENT_WINDOW_FOCUS_LOST); focus(SDL_EVENT_WINDOW_FOCUS_GAINED); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents(); }
void parser() {
    check(parse(nullptr)==0 && parse("")==0 && parse("  ")==0,"empty/null/whitespace refused safely");
    check(bits(parse(" control + SHIFT | a "))==0x00411011u,"original names, case, trim and low-byte order");
    check(bits(parse("0x25|38|0x27|40"))==0x28272625u,"real StringToEnum numeric fallback and four keys");
    check(bits(parse("0101|0x41trailing|65trailing|+65"))==0x41414141u,"original base-0 syntax, sign delimiter and numeric trailing text");
    check(bits(parse("0|1|8|9"))==0x39383130u,"named digit aliases take precedence over numeric values");
    check(bits(parse("0xFF|0377|255"))==0x00FFFFFFu,"largest historical byte accepted in all numeric bases");
    const char* oversized[] = {
        "256", "-1", "2147483647", "2147483648", "-2147483648", "-2147483649",
        "4294967361", "-4294967231", "0x100000041", "-0xFFFFFFBF",
        "9223372036854775807", "9223372036854775808", "-9223372036854775808", "-9223372036854775809",
        "0x7FFFFFFFFFFFFFFF", "0x8000000000000000", "-0x8000000000000000", "-0x8000000000000001",
        "0xFFFFFFFFFFFFFFFF", "0x10000000000000041", "-0x10000000000000041", "4294967361trailing"
    };
    for (const char* token : oversized) {
        check(parse(token)==0,"out-of-byte numeric magnitude cannot narrow into a binding");
        const std::string withKeys=std::string("B|")+token+"|C";
        check(bits(parse(withKeys.c_str()))==0x00004342u,"rejected magnitude is ignored without consuming a packed key slot");
    }
    check(bits(parse("OEM_CLEAR|PA1|NONAME|ZOOM"))==0xFBFCFDFEu,"fourth high byte preserves original integer bits");
    check(parse("A|B|C|D|E")==0,"fifth valid key refuses whole binding");
    check(bits(parse("A|unknown|B|0x00|256|C|D"))==0x44434241u,"original unknown/out-of-byte values ignored within domain");
    check(parse(std::string(512,'A').c_str())==0,"overlong source is refused instead of truncating");
    check(bits(parse("0"))==0x30u,"literal digit zero remains a named key rather than numeric zero");
    check(bits(parse("LBUTTON|XBUTTON1|XBUTTON2|RBUTTON"))==0x02060501u,"mouse binding file values retained");
}
}
int main() {
    try {
        parser(); check(!IsKeyPressed(0) && !IsKeyPressed(0x100),"empty/zero-prefix packed state is never held");
        check(InitializeVideoSurfaceManager(),"surface manager");
        check(sgp::ConfigureNativeVideo(32,20,16,1,"P710 authored key binding fixture") && InitializeVideoManager(nullptr,0,nullptr),"native owned video");
        check(InitializeInputManager(),"production input"); reset();
        key(SDL_SCANCODE_A,true); check(IsKeyPressed(parse("A")),"original binding polls actual native held A");
        InputAtom atom{}; check(DequeueEvent(&atom) && atom.usEvent==KEY_DOWN,"binding polling preserves input queue");
        key(SDL_SCANCODE_A,false); check(!IsKeyPressed(parse("A")),"release clears binding"); reset();
        key(SDL_SCANCODE_LCTRL,true); key(SDL_SCANCODE_RSHIFT,true); key(SDL_SCANCODE_A,true);
        check(IsKeyPressed(parse("CONTROL|SHIFT|A")) && IsKeyPressed(parse("LCONTROL|RSHIFT|A")),"aggregate and side-specific modifiers");
        check(!IsKeyPressed(parse("RCONTROL")) && !IsKeyPressed(parse("LSHIFT")),"opposite modifier side is not invented"); reset();
        key(SDL_SCANCODE_LEFT,true); check(IsKeyPressed(parse("LEFT")) && !IsKeyPressed(parse("RIGHT")),"historical navigation code maps to normalized native state"); reset();
        key(SDL_SCANCODE_KP_1,true,SDL_KMOD_NUM); check(IsKeyPressed(parse("NUMPAD1")) && !IsKeyPressed(parse("END")),"keypad physical identity with NumLock");
        key(SDL_SCANCODE_KP_1,false); check(!IsKeyPressed(parse("NUMPAD1")),"keypad release across NumLock state"); reset();
        key(SDL_SCANCODE_KP_1,true); check(!IsKeyPressed(parse("NUMPAD1")) && IsKeyPressed(parse("END")),"keypad navigation binding follows the stored NumLock bank"); reset();
        mouse(SDL_BUTTON_X1,true); mouse(SDL_BUTTON_X2,true); mouse(SDL_BUTTON_LEFT,true);
        check(IsKeyPressed(parse("LBUTTON|XBUTTON1|XBUTTON2")),"actual native extended mouse held state");
        focus(SDL_EVENT_WINDOW_FOCUS_LOST); check(!IsKeyPressed(parse("LBUTTON")) && !IsKeyPressed(parse("XBUTTON1")),"focus loss clears binding state"); reset();
        const auto foreign=owned()+1000; key(SDL_SCANCODE_A,true,SDL_KMOD_NONE,foreign); focus(SDL_EVENT_WINDOW_FOCUS_LOST,foreign);
        check(!IsKeyPressed(parse("A")) && sgp::NativeInputFocused(),"foreign events neither capture keys nor change owned focus");
        check(!IsKeyPressed(parse("BROWSER_BACK")),"unsupported native key remains up");
        key(SDL_SCANCODE_A,true); check(IsKeyPressed(parse("A")),"held before close"); focus(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
        check(!IsKeyPressed(parse("A")),"owned close rejects held binding");
        ShutdownInputManager(); check(sgp::ShutdownNativeVideo() && ShutdownVideoSurfaceManager(),"teardown");
        std::cout<<"PASS: "<<checks<<" original key binding checks; actual SDL queue input, no human/global capture\n"; return 0;
    } catch(const std::exception& error) { ShutdownInputManager(); (void)sgp::ShutdownNativeVideo(); ShutdownVideoSurfaceManager(); std::cerr<<error.what()<<'\n'; return 1; }
}
