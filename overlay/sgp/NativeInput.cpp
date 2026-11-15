/*
 * P710 native snapshot update: sgp/NativeInput.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_input.h"
#include "native_video.h"
#include "english.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <limits>
extern UINT16 gsKeyTranslationTable[1024];
NativeMouseInputHook MouseSystemHook=nullptr;
void SetMouseInputHook(NativeMouseInputHook hook) { MouseSystemHook=hook; }
void sgp::ClearNativeMouseInputHook(NativeMouseInputHook expected) { if (MouseSystemHook==expected) MouseSystemHook=nullptr; }
static_assert(sizeof(InputAtom)==16 && offsetof(InputAtom,usKeyState)==4 && offsetof(InputAtom,usEvent)==6 && offsetof(InputAtom,usParam)==8 && offsetof(InputAtom,uiParam)==12, "legacy InputAtom ABI");
extern "C" {
BOOLEAN gfKeyState[256]{};
BOOLEAN gfLeftButtonState=FALSE, gfRightButtonState=FALSE, gfMiddleButtonState=FALSE;
INT16 gusMouseXPos=0, gusMouseYPos=0, gsMouseWheelDeltaValue=0;
BOOLEAN gfSGPInputReceived=FALSE, gfTrackMousePos=FALSE;
}
namespace {
// Native UI reset observation added 2026-11-12; existing FIFO unchanged.
std::uint64_t resetGeneration=0;
std::array<InputAtom,256> fifo{};
std::size_t head=0, tail=0, count=0;
bool active=false, focused=false, terminal=false;
SDL_WindowID bound=0;
UINT16 modifiers=0;
SGPRect requestedRestriction{};
bool restrictionRequested=false;
sgp::NativePresentationGeometry restrictionGeometry{};
BOOLEAN x1Button=FALSE, x2Button=FALSE;
std::array<bool,6> sides{};
std::array<int,SDL_SCANCODE_COUNT> heldScan{}; // normalized index + 1, retained across NumLock changes
std::array<unsigned,256> heldCount{}; // shared Enter/keypad Enter cannot clear each other
UINT32 position() { return static_cast<UINT16>(gusMouseXPos) | (static_cast<UINT32>(static_cast<UINT16>(gusMouseYPos))<<16); }
UINT32 translated(unsigned key) { return gsKeyTranslationTable[key+((modifiers & SHIFT_DOWN)==TRUE ? 256u : 0u)]; }
void discard() { head=tail=count=0; }
void release(bool flush)
{
    if (resetGeneration!=std::numeric_limits<std::uint64_t>::max()) ++resetGeneration;
    std::array<BOOLEAN,256> old{}; std::copy(std::begin(gfKeyState),std::end(gfKeyState),old.begin());
    const BOOLEAN left=gfLeftButtonState, right=gfRightButtonState, middle=gfMiddleButtonState, x1=x1Button, x2=x2Button;
    std::array<InputAtom,256> pending{};
    std::size_t pendingCount=0;
    if (flush) {
        for (std::size_t i=0; i<count; ++i) {
            const auto& atom=fifo[(head+i)%fifo.size()];
            if (atom.usEvent==KEY_UP || atom.usEvent==LEFT_BUTTON_UP ||
                atom.usEvent==RIGHT_BUTTON_UP || atom.usEvent==MIDDLE_BUTTON_UP ||
                atom.usEvent==X1_BUTTON_UP || atom.usEvent==X2_BUTTON_UP)
                pending[pendingCount++]=atom;
        }
        discard();
    }
    std::memset(gfKeyState,0,sizeof(gfKeyState)); sides.fill(false); heldScan.fill(0); heldCount.fill(0); modifiers=0;
    gfLeftButtonState=gfRightButtonState=gfMiddleButtonState=x1Button=x2Button=FALSE; gsMouseWheelDeltaValue=0;
    struct Release { UINT16 event; UINT32 param; };
    std::array<Release,256> emitted{};
    std::size_t emittedCount=0;
    const auto emit=[&](UINT16 event, UINT32 param) {
        if (flush) {
            for (std::size_t i=0; i<emittedCount; ++i)
                if (emitted[i].event==event && emitted[i].param==param) return;
            if (emittedCount==emitted.size()) return;
            emitted[emittedCount++]={event,param};
        }
        QueueEvent(event,param,position());
    };
    // Current held-state releases have priority under the fixed queue bound.
    for (unsigned k=0; k<256; ++k) if (old[k] && k!=SHIFT && k!=CTRL && k!=ALT) emit(KEY_UP,translated(k));
    if (left) emit(LEFT_BUTTON_UP,0);
    if (right) emit(RIGHT_BUTTON_UP,0);
    if (middle) emit(MIDDLE_BUTTON_UP,0);
    if (x1) emit(X1_BUTTON_UP,0);
    if (x2) emit(X2_BUTTON_UP,0);
    // A queued physical release may already have cleared polling state. Retain
    // it when dropping stale presses so event consumers cannot miss that release.
    for (std::size_t i=0; i<pendingCount; ++i) emit(pending[i].usEvent,pending[i].usParam);
}
int side(SDL_Scancode s)
{
    switch(s) { case SDL_SCANCODE_LSHIFT:return 0; case SDL_SCANCODE_RSHIFT:return 1; case SDL_SCANCODE_LCTRL:return 2; case SDL_SCANCODE_RCTRL:return 3; case SDL_SCANCODE_LALT:return 4; case SDL_SCANCODE_RALT:return 5; default:return -1; }
}
int normalized(SDL_Scancode s, SDL_Keymod mod)
{
    if (s>=SDL_SCANCODE_A && s<=SDL_SCANCODE_Z) return 65+static_cast<int>(s)-SDL_SCANCODE_A;
    if (s>=SDL_SCANCODE_1 && s<=SDL_SCANCODE_9) return 49+static_cast<int>(s)-SDL_SCANCODE_1;
    if (s>=SDL_SCANCODE_F1 && s<=SDL_SCANCODE_F12) return 112+static_cast<int>(s)-SDL_SCANCODE_F1;
    const int bank=(mod & SDL_KMOD_NUM) ? 0 : 11;
    if (s>=SDL_SCANCODE_KP_1 && s<=SDL_SCANCODE_KP_9) return 225+bank+static_cast<int>(s)-SDL_SCANCODE_KP_1;
    switch(s) {
    case SDL_SCANCODE_0:return 48; case SDL_SCANCODE_ESCAPE:return 27; case SDL_SCANCODE_RETURN:case SDL_SCANCODE_KP_ENTER:return 13;
    case SDL_SCANCODE_TAB:return 9; case SDL_SCANCODE_BACKSPACE:return 8; case SDL_SCANCODE_SPACE:return 32;
    case SDL_SCANCODE_INSERT:return 245; case SDL_SCANCODE_DELETE:return 246; case SDL_SCANCODE_END:return 247; case SDL_SCANCODE_DOWN:return 248; case SDL_SCANCODE_PAGEDOWN:return 249;
    case SDL_SCANCODE_LEFT:return 250; case SDL_SCANCODE_RIGHT:return 251; case SDL_SCANCODE_HOME:return 252; case SDL_SCANCODE_UP:return 253; case SDL_SCANCODE_PAGEUP:return 254;
    case SDL_SCANCODE_KP_0:return 223+bank; case SDL_SCANCODE_KP_PERIOD:return 224+bank;
    case SDL_SCANCODE_KP_MULTIPLY:return 106; case SDL_SCANCODE_KP_PLUS:return 107; case SDL_SCANCODE_KP_MINUS:return 109; case SDL_SCANCODE_KP_DIVIDE:return 111;
    case SDL_SCANCODE_SEMICOLON:return 186; case SDL_SCANCODE_EQUALS:return 187; case SDL_SCANCODE_COMMA:return 188; case SDL_SCANCODE_MINUS:return 189; case SDL_SCANCODE_PERIOD:return 190;
    case SDL_SCANCODE_SLASH:return 191; case SDL_SCANCODE_GRAVE:return 192; case SDL_SCANCODE_LEFTBRACKET:return 219; case SDL_SCANCODE_BACKSLASH:return 220; case SDL_SCANCODE_RIGHTBRACKET:return 221; case SDL_SCANCODE_APOSTROPHE:return 222;
    case SDL_SCANCODE_CAPSLOCK:return 20; case SDL_SCANCODE_NUMLOCKCLEAR:return 144; case SDL_SCANCODE_SCROLLLOCK:return 145; case SDL_SCANCODE_PAUSE:return 19; default:return -1;
    }
}
bool move(float x, float y, bool* inside=nullptr)
{
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    if (!std::isfinite(x) || !std::isfinite(y) || !sgp::NativeVideoInputInfo(id,w,h,ww,wh) || id!=bound || ww<=0 || wh<=0) return false;
    sgp::NativePresentationGeometry geometry{};
    if (!sgp::NativeVideoPresentationGeometry(geometry)) return false;
    if (inside) *inside=sgp::NativePresentationContains(geometry,x,y);
    if (restrictionRequested && restrictionGeometry!=geometry) RestoreCursorClipRect();
    // Preserve outside-client coordinates. Edge clamping belongs only to an
    // explicitly applied restriction, otherwise outside clicks hit edge buttons.
    double px=x, py=y;
    const SDL_Rect* barrier=restrictionRequested && (!inside || *inside) ? SDL_GetWindowMouseRect(SDL_GetWindowFromID(bound)) : nullptr;
    if (barrier) {
        // The SDL rectangle is in window coordinates. Subtract its exclusive edge
        // in that domain before scaling; subtracting a whole logical pixel
        // breaks narrow restrictions when the resize factor is fractional.
        px=std::clamp(px,static_cast<double>(barrier->x),static_cast<double>(barrier->x+barrier->w-1));
        py=std::clamp(py,static_cast<double>(barrier->y),static_cast<double>(barrier->y+barrier->h-1));
    }
    double lx=0,ly=0;
    if (!sgp::NativeWindowToLogical(geometry,px,py,lx,ly)) return false;
    gusMouseXPos=static_cast<INT16>(std::clamp(std::floor(lx),-32768.0,32767.0));
    gusMouseYPos=static_cast<INT16>(std::clamp(std::floor(ly),-32768.0,32767.0)); return true;
}
void sink(void*, const SDL_Event* e)
{
    if (!active) return;
    if (!e) { release(true); focused=false; terminal=true; bound=0; return; }
    if (e->type==SDL_EVENT_QUIT || e->type==SDL_EVENT_WINDOW_CLOSE_REQUESTED || e->type==SDL_EVENT_WINDOW_DESTROYED) { release(false); focused=false; terminal=true; return; }
    if (e->type==SDL_EVENT_WINDOW_FOCUS_LOST || e->type==SDL_EVENT_WINDOW_HIDDEN) { if (focused) release(true); focused=false; return; }
    if (e->type==SDL_EVENT_WINDOW_FOCUS_GAINED) { if (!terminal) focused=true; return; }
    if (!focused || terminal) return;
    if (e->type==SDL_EVENT_KEY_DOWN || e->type==SDL_EVENT_KEY_UP) {
        const int s=static_cast<int>(e->key.scancode); if (s<0 || s>=SDL_SCANCODE_COUNT) return;
        const bool down=e->type==SDL_EVENT_KEY_DOWN; const int m=side(e->key.scancode);
        if (m>=0) { sides[static_cast<std::size_t>(m)]=down; gfKeyState[SHIFT]=static_cast<BOOLEAN>(sides[0]||sides[1]); gfKeyState[CTRL]=static_cast<BOOLEAN>(sides[2]||sides[3]); gfKeyState[ALT]=static_cast<BOOLEAN>(sides[4]||sides[5]); modifiers=static_cast<UINT16>((gfKeyState[SHIFT]?SHIFT_DOWN:0)|(gfKeyState[CTRL]?CTRL_DOWN:0)|(gfKeyState[ALT]?ALT_DOWN:0)); gfSGPInputReceived=TRUE; return; }
        int& held=heldScan[static_cast<std::size_t>(s)]; const int k=held ? held-1 : normalized(e->key.scancode,e->key.mod); if (k<0 || k>=256) return;
        const auto key=static_cast<unsigned>(k); gfSGPInputReceived=TRUE;
        if (down) { const UINT16 event=gfKeyState[key] ? KEY_REPEAT : KEY_DOWN; if (!held) { held=k+1; ++heldCount[key]; } gfKeyState[key]=TRUE; QueueEvent(event,translated(key),position()); }
        else if (held) { held=0; if (heldCount[key]) --heldCount[key]; if (!heldCount[key]) { gfKeyState[key]=FALSE; QueueEvent(KEY_UP,translated(key),position()); } }
        return;
    }
    if (e->type==SDL_EVENT_MOUSE_MOTION) { if (move(e->motion.x,e->motion.y)) { gfSGPInputReceived=TRUE; if (gfTrackMousePos) QueueEvent(MOUSE_POS,0,position()); } return; }
    if (e->type==SDL_EVENT_MOUSE_BUTTON_DOWN || e->type==SDL_EVENT_MOUSE_BUTTON_UP) {
        BOOLEAN* state=nullptr; UINT16 down=0,up=0;
        switch(e->button.button) { case SDL_BUTTON_LEFT:state=&gfLeftButtonState;down=LEFT_BUTTON_DOWN;up=LEFT_BUTTON_UP;break; case SDL_BUTTON_RIGHT:state=&gfRightButtonState;down=RIGHT_BUTTON_DOWN;up=RIGHT_BUTTON_UP;break; case SDL_BUTTON_MIDDLE:state=&gfMiddleButtonState;down=MIDDLE_BUTTON_DOWN;up=MIDDLE_BUTTON_UP;break; case SDL_BUTTON_X1:state=&x1Button;down=X1_BUTTON_DOWN;up=X1_BUTTON_UP;break; case SDL_BUTTON_X2:state=&x2Button;down=X2_BUTTON_DOWN;up=X2_BUTTON_UP;break; default:return; }
        bool inside=false;
        if (!move(e->button.x,e->button.y,&inside)) return;
        const bool pressed=e->type==SDL_EVENT_MOUSE_BUTTON_DOWN; const bool was=*state!=FALSE;
        // Bars cannot start an action. A release after dragging out still clears
        // the held state and reaches the UI at its outside logical coordinate.
        if (!inside && (pressed || !was)) return;
        *state=static_cast<BOOLEAN>(pressed); gfSGPInputReceived=TRUE;
        if (pressed || was) QueueEvent(pressed?down:up,0,position());
        return;
    }
    if (e->type==SDL_EVENT_MOUSE_WHEEL) {
        bool inside=false;
        if (!std::isfinite(e->wheel.y) || !move(e->wheel.mouse_x,e->wheel.mouse_y,&inside) || !inside) return;
        const double delta=static_cast<double>(e->wheel.y)*(e->wheel.direction==SDL_MOUSEWHEEL_FLIPPED ? -1.0 : 1.0);
        gsMouseWheelDeltaValue=static_cast<INT16>(std::clamp(delta,-32768.0,32767.0)); gfSGPInputReceived=TRUE;
        if (delta!=0) QueueEvent(delta>0?MOUSE_WHEEL_UP:MOUSE_WHEEL_DOWN,0,position());
    }
}
void pump() { if (active && bound) sgp::PumpNativeVideoEvents(); }
}
namespace sgp {
std::uint64_t NativeInputResetGeneration() { return resetGeneration; }
bool NativeInputTerminated() { return terminal; }
bool NativeInputFocused() { return active && focused && !terminal; }
bool NativeBindingKeyDown(UINT8 code)
{
    if (!SDL_IsMainThread() || !active || !focused || terminal || !bound) return false;
    switch (code) {
    case 0x01: return gfLeftButtonState != FALSE;
    case 0x02: return gfRightButtonState != FALSE;
    case 0x04: return gfMiddleButtonState != FALSE;
    case 0x05: return x1Button != FALSE;
    case 0x06: return x2Button != FALSE;
    case 0x10: return sides[0] || sides[1];
    case 0x11: return sides[2] || sides[3];
    case 0x12: return sides[4] || sides[5];
    case 0xa0: return sides[0]; case 0xa1: return sides[1];
    case 0xa2: return sides[2]; case 0xa3: return sides[3];
    case 0xa4: return sides[4]; case 0xa5: return sides[5];
    default: break;
    }
    // These are binding data values, not Windows SDK identifiers. The native
    // input manager already retains the press-time keypad bank across release.
    constexpr UINT8 navigation[10]={0x2d,0x2e,0x23,0x28,0x22,0x25,0x27,0x24,0x26,0x21};
    constexpr UINT8 keypadNavigation[9]={0x23,0x28,0x22,0x25,0x0c,0x27,0x24,0x26,0x21};
    for (std::size_t scan=0; scan<heldScan.size(); ++scan) {
        if (!heldScan[scan]) continue;
        const int key=heldScan[scan]-1;
        int binding=key;
        if (key>=245 && key<=254) binding=navigation[key-245];
        else if (key>=225 && key<=233) binding=0x61+key-225;
        else if (key==223) binding=0x60;
        else if (key==224) binding=0x6e;
        else if (key>=236 && key<=244) binding=keypadNavigation[key-236];
        else if (key==234) binding=0x2d;
        else if (key==235) binding=0x2e;
        if (binding==code) return true;
    }
    return false;
}
bool IsNativeGameMouseEvent(UINT16 type) {
    switch (type) {
    case MOUSE_POS: case LEFT_BUTTON_DOWN: case LEFT_BUTTON_UP:
    case LEFT_BUTTON_REPEAT: case RIGHT_BUTTON_DOWN: case RIGHT_BUTTON_UP:
    case RIGHT_BUTTON_REPEAT: case MIDDLE_BUTTON_DOWN: case MIDDLE_BUTTON_UP:
    case MIDDLE_BUTTON_REPEAT: case X1_BUTTON_DOWN: case X1_BUTTON_UP:
    case X1_BUTTON_REPEAT: case X2_BUTTON_DOWN: case X2_BUTTON_UP:
    case X2_BUTTON_REPEAT: case MOUSE_WHEEL_UP: case MOUSE_WHEEL_DOWN: return true;
    default: return false;
    }
}
bool DequeueNativeGameMouseEvent(InputAtom* out) {
    if (!active || !out) return false;
    pump();
    if (!count || !IsNativeGameMouseEvent(fifo[head].usEvent)) return false;
    *out=fifo[head]; head=(head+1)%fifo.size(); --count; return true;
}
bool NativeInputQueueEmpty() { return count==0; }
}
extern "C" BOOLEAN InitializeInputManager(void)
{
    if (!SDL_IsMainThread()) return FALSE;
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    if (!sgp::NativeVideoInputInfo(id,w,h,ww,wh) || w>32768 || h>32768 || w<=0 || h<=0) return FALSE;
    if (active) return static_cast<BOOLEAN>(bound==id && !terminal);
    if (!sgp::AttachNativeVideoInput(sink,&active)) return FALSE;
    discard(); bound=id; active=true; terminal=false; modifiers=0; sides.fill(false); heldScan.fill(0); heldCount.fill(0);
    std::memset(gfKeyState,0,sizeof(gfKeyState)); gfLeftButtonState=gfRightButtonState=gfMiddleButtonState=x1Button=x2Button=FALSE;
    gusMouseXPos=static_cast<INT16>(w/2); gusMouseYPos=static_cast<INT16>(h/2); gsMouseWheelDeltaValue=0; gfTrackMousePos=gfSGPInputReceived=FALSE;
    const auto flags=SDL_GetWindowFlags(SDL_GetWindowFromID(id)); focused=(flags & SDL_WINDOW_INPUT_FOCUS)!=0 && (flags & SDL_WINDOW_HIDDEN)==0;
    return TRUE;
}
extern "C" void ShutdownInputManager(void)
{
    FreeMouseCursor(FALSE); restrictionRequested=false; sgp::DetachNativeVideoInput(sink,&active); active=false; release(true); bound=0; focused=false; terminal=false;
    gusMouseXPos=gusMouseYPos=gsMouseWheelDeltaValue=0; gfTrackMousePos=gfSGPInputReceived=FALSE;
}
extern "C" void QueueEvent(UINT16 event, UINT32 param, UINT32 packed)
{
    if (!active || !bound || count==fifo.size()) return;
    fifo[tail]={static_cast<UINT32>(SDL_GetTicks()),modifiers,event,param,packed}; tail=(tail+1)%fifo.size(); ++count;
}
extern "C" BOOLEAN DequeueEvent(InputAtom* out)
{
    if (!active || !out) return FALSE;
    pump();
    if (!count) return FALSE;
    *out=fifo[head]; head=(head+1)%fifo.size(); --count; return TRUE;
}
extern "C" BOOLEAN DequeueSpecificEvent(InputAtom* out, UINT32 mask)
{
    if (!active || !out) return FALSE;
    pump();
    if (!count) return FALSE;
    *out=fifo[head];
    if (!(out->usEvent & mask)) return FALSE;
    head=(head+1)%fifo.size(); --count; return TRUE;
}
extern "C" BOOLEAN PeekSpecificEvent(UINT32 mask) { if (!active) return FALSE; pump(); return static_cast<BOOLEAN>(count && (fifo[head].usEvent & mask)); }
extern "C" void DequeueAllKeyBoardEvents() { if (active) { pump(); discard(); } }
extern "C" void GetMousePos(SGPPoint* out) { if (active && out) { out->iX=gusMouseXPos; out->iY=gusMouseYPos; } }
extern "C" BOOLEAN InputEventInside(InputAtom* e, UINT32 l, UINT32 t, UINT32 r, UINT32 b) { return static_cast<BOOLEAN>(active && e && GETXPOS(e)>=l && GETXPOS(e)<=r && GETYPOS(e)>=t && GETYPOS(e)<=b); }

extern "C" void RestrictMouseCursor(SGPRect* rectangle)
{
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    if (!active || terminal || !rectangle || !sgp::NativeVideoInputInfo(id,w,h,ww,wh) || id!=bound) return;
    const int l=std::clamp(rectangle->iLeft,0,w), t=std::clamp(rectangle->iTop,0,h);
    const int r=std::clamp(rectangle->iRight,0,w), b=std::clamp(rectangle->iBottom,0,h);
    if (l>=r || t>=b || ww<=0 || wh<=0) return;
    sgp::NativePresentationGeometry geometry{};
    if (!sgp::NativeVideoPresentationGeometry(geometry)) return;
    // ClipCursor is half-open. The fitted pixel rectangle must first be
    // converted to SDL window coordinates; round inward in that domain.
    const auto edge=[](int logical,int offset,int extent,int logicalExtent,int windowExtent,int pixelExtent) {
        const long double numerator=static_cast<long double>(std::int64_t(offset)*logicalExtent+std::int64_t(logical)*extent)*windowExtent;
        return numerator/(static_cast<long double>(logicalExtent)*pixelExtent);
    };
    SDL_Rect barrier{
        static_cast<int>(std::ceil(edge(l,geometry.contentX,geometry.contentWidth,w,ww,geometry.pixelWidth))),
        static_cast<int>(std::ceil(edge(t,geometry.contentY,geometry.contentHeight,h,wh,geometry.pixelHeight))),0,0};
    barrier.w=static_cast<int>(std::floor(edge(r,geometry.contentX,geometry.contentWidth,w,ww,geometry.pixelWidth)))-barrier.x;
    barrier.h=static_cast<int>(std::floor(edge(b,geometry.contentY,geometry.contentHeight,h,wh,geometry.pixelHeight)))-barrier.y;
    if (barrier.w<=0 || barrier.h<=0) return;
    SDL_Window* owner=SDL_GetWindowFromID(bound);
    SDL_Rect previous{};
    const SDL_Rect* old=SDL_GetWindowMouseRect(owner);
    const bool hadPrevious=old!=nullptr;
    if (old) previous=*old;
    if (SDL_SetWindowMouseRect(owner,&barrier)) {
        requestedRestriction={l,t,r,b}; restrictionRequested=true; restrictionGeometry=geometry;
    } else {
        // SDL copies the proposed rectangle before invoking some backends.
        // Roll back that readback state on failure instead of claiming success.
        if (!SDL_SetWindowMouseRect(owner,hadPrevious ? &previous : nullptr)) restrictionRequested=false;
    }
}
extern "C" void RestrictMouseToXYXY(UINT16 l,UINT16 t,UINT16 r,UINT16 b) { SGPRect rectangle{l,t,r,b}; RestrictMouseCursor(&rectangle); }
extern "C" void FreeMouseCursor(BOOLEAN)
{
    // Tactical/fullscreen policy belongs to the application. This native
    // manager releases only its owned-window barrier and never grabs desktop.
    if (bound) if (SDL_Window* owner=SDL_GetWindowFromID(bound)) {
        if (!SDL_SetWindowMouseRect(owner,nullptr)) return;
    }
    restrictionRequested=false;
}
extern "C" void RestoreCursorClipRect() { if (restrictionRequested) { SGPRect r=requestedRestriction; RestrictMouseCursor(&r); } }
extern "C" BOOLEAN IsCursorRestricted()
{
    if (!active || terminal || !bound) return FALSE;
    SDL_Window* owner=SDL_GetWindowFromID(bound);
    return static_cast<BOOLEAN>(restrictionRequested && owner && SDL_GetWindowMouseRect(owner)!=nullptr);
}
extern "C" void GetRestrictedClipCursor(SGPRect* rectangle)
{
    if (!rectangle) return;
    *rectangle={0,0,0,0};
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    if (!active || terminal || !sgp::NativeVideoInputInfo(id,w,h,ww,wh) || id!=bound || ww<=0 || wh<=0) return;
    sgp::NativePresentationGeometry geometry{};
    if (!sgp::NativeVideoPresentationGeometry(geometry)) return;
    if (restrictionRequested && restrictionGeometry!=geometry) RestoreCursorClipRect();
    const SDL_Rect* barrier=SDL_GetWindowMouseRect(SDL_GetWindowFromID(bound));
    if (!barrier) { *rectangle={0,0,w,h}; return; }
    double l=0,t=0,r=0,b=0;
    if (!sgp::NativeWindowToLogical(geometry,barrier->x,barrier->y,l,t) ||
        !sgp::NativeWindowToLogical(geometry,barrier->x+barrier->w,barrier->y+barrier->h,r,b)) return;
    rectangle->iLeft=static_cast<INT32>(std::floor(l));
    rectangle->iTop=static_cast<INT32>(std::floor(t));
    rectangle->iRight=static_cast<INT32>(std::ceil(r));
    rectangle->iBottom=static_cast<INT32>(std::ceil(b));
}
extern "C" void SimulateMouseMovement(UINT32 x,UINT32 y)
{
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    if (!active || terminal || !sgp::NativeVideoInputInfo(id,w,h,ww,wh) || id!=bound || x>=static_cast<UINT32>(w) || y>=static_cast<UINT32>(h)) return;
    // SDL generates the actual window-relative motion; polling/FIFO update only
    // on receipt of that event, never on an unconfirmed desktop warp assumption.
    sgp::NativePresentationGeometry geometry{}; double wx=0,wy=0;
    if (!sgp::NativeVideoPresentationGeometry(geometry) || !sgp::NativeLogicalToWindow(geometry,double(x)+0.5,double(y)+0.5,wx,wy)) return;
    SDL_WarpMouseInWindow(SDL_GetWindowFromID(bound),static_cast<float>(wx),static_cast<float>(wy));
}
