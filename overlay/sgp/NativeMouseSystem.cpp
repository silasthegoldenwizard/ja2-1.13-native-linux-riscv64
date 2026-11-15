/*
 * P710 native snapshot update: sgp/NativeMouseSystem.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation, 2026-11-12, of 1dot13/source SGP mouse-region rules at
// 3394695a387a14c78e92b158cfdcf50e4068c96b. Retains inclusive regions, priority
// ties, callback ordering and clicked-ID gate. Corrects ID wrap and stale access.
#include "native_mousesystem.h"
#include "native_font.h"
#include "native_video.h"
#include "NativeTooltipText.h"
#include "shading.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cwchar>
#include <memory>
#include <new>
#include <utility>
#include <limits>
#include <vector>

void ReleaseAnchorMode();
extern BOOLEAN gfRenderHilights;
namespace sgp { void CancelNativeButtonInput(); void FinishNativeButtonDispatch(); }
MOUSE_REGION *MSYS_RegList=nullptr, *MSYS_PrevRegion=nullptr, *MSYS_CurrRegion=nullptr;
MOUSE_REGION MSYS_SystemBaseRegion{};
INT16 MSYS_CurrentMX=0, MSYS_CurrentMY=0;
namespace {
struct Entry { MOUSE_REGION* region; std::uint64_t generation; std::unique_ptr<CHAR16[]> fastHelp; };
std::vector<Entry> regions;
std::uint64_t serial=0, mutations=0, observedReset=0;
UINT16 nextId=1, clickedId=0, buttons=0;
MOUSE_REGION* grabbed=nullptr;
bool initialized=false, dispatching=false, refresh=true, clicked=false;
UINT16 replayButtons=0;
INT16 wheel=0;
INT16 fastHelpDelay=600;
bool showFastHelp=true;
sgp::NativeFastHelpClock fastHelpClock=nullptr;
sgp::NativeMouseCursorSelector cursorSelector=nullptr;
std::uint64_t helpTick=0;
struct HelpBinding {
    INT32 regular=-1, bold=-1;
    std::uint64_t regularGeneration=0, boldGeneration=0;
    UINT32 destination=0, backing=0;
} helpBinding;
struct HelpBackground {
    UINT32 backing=0;
    INT32 x=0,y=0,w=0,h=0;
    std::vector<UINT16> before, drawn;
} helpBackground;
std::uint64_t helpNow() {
    if (fastHelpClock) return fastHelpClock();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
UINT32 backingId(UINT32 destination) {
    const UINT32 reserved=sgp::ReservedVideoSurfaceBackingId(destination);
    return reserved?reserved:destination;
}
void clearHelpFlags(MOUSE_REGION* r) {
    if (!r) return;
    r->uiFlags &= ~static_cast<UINT32>(MSYS_FASTHELP|MSYS_FASTHELP_RESET|MSYS_GOT_BACKGROUND|MSYS_HAS_BACKRECT);
    r->FastHelpRect=-1;
}
void resetHelp(MOUSE_REGION* r) {
    if (r) { clearHelpFlags(r); r->FastHelpTimer=fastHelpDelay; }
    (void)sgp::RestoreNativeFastHelpBackground();
    helpTick=helpNow();
}
std::uint64_t fontGeneration(INT32 font) {
    const auto state=sgp::CaptureNativeFontState();
    if (!SetFont(font)) return 0;
    const auto generation=sgp::CaptureNativeFontState().generation;
    sgp::RestoreNativeFontState(state);
    return generation;
}
struct FontScope {
    sgp::NativeFontState state=sgp::CaptureNativeFontState();
    ~FontScope() { sgp::RestoreNativeFontState(state); }
};
bool drawScaledHelp(MOUSE_REGION* region,UINT32 width,UINT32 height) {
    sgp::NativeTooltipLayout layout;
    if (!sgp::LayoutNativeTooltipText(region->FastHelpText,height,layout)) return false;
    const double scale=sgp::NativeTooltipScale();
    const int w=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(layout.width)+static_cast<INT32>(10*scale),4,width));
    const int h=static_cast<int>(std::clamp<std::int64_t>(std::int64_t(GetNumberOfLinesInHeight(region->FastHelpText))*(layout.lineHeight+1)+static_cast<INT32>(8*scale),4,height));
    int left=std::max(0,static_cast<int>(region->RegionTopLeftX)+static_cast<int>(10*scale));
    if (left+w>=static_cast<int>(width)) left=std::max(0,static_cast<int>(width)-w-4);
    int top=std::max(0,static_cast<int>(region->RegionTopLeftY)-h);
    if (top+h>=static_cast<int>(height)) top=std::max(0,static_cast<int>(height)-h-10);
    HelpBackground replacement;
    replacement.backing=helpBinding.backing;replacement.x=left;replacement.y=top;replacement.w=w;replacement.h=h;
    replacement.before.resize(std::size_t(w)*std::size_t(h));
    UINT32 pitch=0;auto* pixels=LockVideoSurface(replacement.backing,&pitch);if (!pixels) return false;
    for (int row=0;row<h;++row)
        std::memcpy(replacement.before.data()+std::size_t(row)*std::size_t(w),pixels+std::size_t(top+row)*pitch+std::size_t(left)*2,std::size_t(w)*2);
    UnLockVideoSurface(replacement.backing);replacement.drawn=replacement.before;
    const auto put=[&](int x,int y,UINT16 color) { replacement.drawn[std::size_t(y)*std::size_t(w)+std::size_t(x)]=color; };
    const auto rectangle=[&](int l,int t,int r,int b,UINT16 color) {
        for (int x=l;x<=r;++x) { put(x,t,color);put(x,b,color); }
        for (int y=t;y<=b;++y) { put(l,y,color);put(r,y,color); }
    };
    rectangle(1,1,w-1,h-1,Get16BPPColor(FROMRGB(65,57,15)));
    rectangle(0,0,w-2,h-2,Get16BPPColor(FROMRGB(227,198,88)));
    for (int y=2;y<=h-3;++y) for (int x=2;x<=w-3;++x) {
        auto& pixel=replacement.drawn[std::size_t(y)*std::size_t(w)+std::size_t(x)];pixel=ShadeTable[ShadeTable[pixel]];
    }
    if (!sgp::DrawNativeTooltipLayout(layout,reinterpret_cast<BYTE*>(replacement.drawn.data()),
        replacement.drawn.size()*2,static_cast<UINT32>(w),static_cast<UINT32>(h),static_cast<UINT32>(w)*2,
        static_cast<INT32>(5*scale),static_cast<INT32>(4*scale))) return false;
    pixels=LockVideoSurface(replacement.backing,&pitch);if (!pixels) return false;
    for (int row=0;row<h;++row)
        std::memcpy(pixels+std::size_t(top+row)*pitch+std::size_t(left)*2,replacement.drawn.data()+std::size_t(row)*std::size_t(w),std::size_t(w)*2);
    UnLockVideoSurface(replacement.backing);helpBackground=std::move(replacement);
    InvalidateRegion(left,top,left+w,top+h);
    region->uiFlags|=MSYS_FASTHELP|MSYS_GOT_BACKGROUND|MSYS_HAS_BACKRECT;region->FastHelpRect=0;return true;
}
bool drawHelp(MOUSE_REGION* region) {
    if (!helpBinding.backing || backingId(helpBinding.destination)!=helpBinding.backing ||
        fontGeneration(helpBinding.regular)!=helpBinding.regularGeneration ||
        fontGeneration(helpBinding.bold)!=helpBinding.boldGeneration) return false;
    UINT16 width=0,height=0; UINT8 bpp=0;
    if (!GetVideoSurfaceDescription(helpBinding.backing,&width,&height,&bpp) || bpp!=16 || width<12 || height<12) return false;
    if (sgp::NativeTooltipScalingEnabled()) return drawScaledHelp(region,width,height);
    const int fontHeight=GetFontHeight(helpBinding.regular);
    if (!fontHeight || fontHeight+9>height) return false;
    // Parse individual glyphs: '|' marks the following character bold, as in
    // original DisplayHelpTokenizedString. Explicit newlines do not wrap.
    struct Glyph { wchar_t c; int x,y; bool bold; };
    std::vector<Glyph> glyphs;
    int x=0,y=0,maxWidth=0; bool bold=false;
    constexpr std::size_t maxCharacters=4096;
    for (std::size_t i=0;i<maxCharacters && region->FastHelpText[i];++i) {
        const wchar_t c=region->FastHelpText[i];
        if (c==L'\n') {
            maxWidth=std::max(maxWidth,x); x=0; y+=fontHeight+1;
            if (y+fontHeight+9>height) { y-=fontHeight+1; break; }
            continue;
        }
        if (c==L'|') { bold=true; continue; }
        const INT32 font=bold?helpBinding.bold:helpBinding.regular;
        const auto advance=static_cast<int>(GetWidth(GetFontObject(font),GetIndex(c)));
        if (x<width-10) glyphs.push_back({c,x,y,bold});
        x=std::min<int>(width-10,x+advance); bold=false;
    }
    maxWidth=std::max(maxWidth,x);
    const int w=std::min<int>(width,maxWidth+10), h=y+fontHeight+9;
    int left=std::max(0,static_cast<int>(region->RegionTopLeftX)+10);
    if (left+w>=width) left=std::max(0,static_cast<int>(width)-w-4);
    int top=std::max(0,static_cast<int>(region->RegionTopLeftY)-h);
    if (top+h>=height) top=std::max(0,static_cast<int>(height)-h-10);
    HelpBackground replacement;
    replacement.backing=helpBinding.backing; replacement.x=left; replacement.y=top; replacement.w=w; replacement.h=h;
    replacement.before.resize(static_cast<std::size_t>(w)*static_cast<std::size_t>(h));
    replacement.drawn.resize(replacement.before.size());
    UINT32 pitch=0;
    BYTE* pixels=LockVideoSurface(replacement.backing,&pitch);
    if (!pixels) return false;
    for (int row=0;row<h;++row)
        std::memcpy(replacement.before.data()+static_cast<std::size_t>(row)*static_cast<std::size_t>(w),
            pixels+static_cast<std::size_t>(top+row)*pitch+static_cast<std::size_t>(left)*2,static_cast<std::size_t>(w)*2);
    UnLockVideoSurface(replacement.backing);
    replacement.drawn=replacement.before;
    const auto put=[&](int px,int py,UINT16 color) {
        replacement.drawn[static_cast<std::size_t>(py)*static_cast<std::size_t>(w)+static_cast<std::size_t>(px)]=color;
    };
    const auto rectangle=[&](int l,int t,int r,int b,UINT16 color) {
        for (int col=l;col<=r;++col) { put(col,t,color); put(col,b,color); }
        for (int row=t;row<=b;++row) { put(l,row,color); put(r,row,color); }
    };
    // Original two inset RGB565 borders and twice-shaded interior.
    rectangle(1,1,w-1,h-1,static_cast<UINT16>(((65>>3)<<11)|((57>>2)<<5)|(15>>3)));
    rectangle(0,0,w-2,h-2,static_cast<UINT16>(((227>>3)<<11)|((198>>2)<<5)|(88>>3)));
    for (int row=2;row<=h-3;++row) for (int col=2;col<=w-3;++col) {
        auto& pixel=replacement.drawn[static_cast<std::size_t>(row)*static_cast<std::size_t>(w)+static_cast<std::size_t>(col)];
        pixel=ShadeTable[ShadeTable[pixel]];
    }
    FontScope scope;
    auto temporary=scope.state;
    temporary.region={0,0,w,h}; temporary.bpp=16; temporary.pitch=static_cast<UINT32>(w)*2; temporary.wrap=FALSE;
    sgp::RestoreNativeFontState(temporary);
    SetFont(helpBinding.regular); // palette colors belong to the configured help font
    SetFontBackground(0); SetFontShadow(141); // FONT_NEARBLACK
    for (const auto& glyph:glyphs) {
        const auto font=glyph.bold?helpBinding.bold:helpBinding.regular;
        SetFont(font); SetFontForeground(glyph.bold?146:130); // original bold / FONT_BEIGE
        wchar_t character[]={glyph.c,0};
        if (mprintf_buffer(reinterpret_cast<BYTE*>(replacement.drawn.data()),temporary.pitch,
            static_cast<UINT32>(font),glyph.x+5,glyph.y+5,L"%ls",character)==std::numeric_limits<UINT32>::max()) return false;
    }
    // Publish only after every glyph has passed the real ETRLE validator.
    pixels=LockVideoSurface(replacement.backing,&pitch);
    if (!pixels) return false;
    for (int row=0;row<h;++row)
        std::memcpy(pixels+static_cast<std::size_t>(top+row)*pitch+static_cast<std::size_t>(left)*2,
            replacement.drawn.data()+static_cast<std::size_t>(row)*static_cast<std::size_t>(w),static_cast<std::size_t>(w)*2);
    UnLockVideoSurface(replacement.backing);
    helpBackground=std::move(replacement);
    InvalidateRegion(left,top,left+w,top+h);
    region->uiFlags |= MSYS_FASTHELP|MSYS_GOT_BACKGROUND|MSYS_HAS_BACKRECT;
    region->FastHelpRect=0; // native-owned snapshot, never a dirty-renderer index
    return true;
}
struct ButtonEvent { UINT16 mask; int direction; INT32 reason; };
ButtonEvent buttonEvent(UINT16 type) {
    switch (type) {
    case LEFT_BUTTON_DOWN: return {MSYS_LEFT_BUTTON,1,MSYS_CALLBACK_REASON_LBUTTON_DWN};
    case LEFT_BUTTON_UP: return {MSYS_LEFT_BUTTON,-1,MSYS_CALLBACK_REASON_LBUTTON_UP};
    case LEFT_BUTTON_REPEAT: return {MSYS_LEFT_BUTTON,0,MSYS_CALLBACK_REASON_LBUTTON_REPEAT};
    case RIGHT_BUTTON_DOWN: return {MSYS_RIGHT_BUTTON,1,MSYS_CALLBACK_REASON_RBUTTON_DWN};
    case RIGHT_BUTTON_UP: return {MSYS_RIGHT_BUTTON,-1,MSYS_CALLBACK_REASON_RBUTTON_UP};
    case RIGHT_BUTTON_REPEAT: return {MSYS_RIGHT_BUTTON,0,MSYS_CALLBACK_REASON_RBUTTON_REPEAT};
    // Pinned update() only produces L/R callback reasons; extended inputs
    // feed polling fields and leave the shared L/R clicked-ID gate unchanged.
    case MIDDLE_BUTTON_DOWN: return {MSYS_MIDDLE_BUTTON,1,0};
    case MIDDLE_BUTTON_UP: return {MSYS_MIDDLE_BUTTON,-1,0};
    case MIDDLE_BUTTON_REPEAT: return {MSYS_MIDDLE_BUTTON,0,0};
    case X1_BUTTON_DOWN: return {MSYS_X1_BUTTON,1,0};
    case X1_BUTTON_UP: return {MSYS_X1_BUTTON,-1,0};
    case X1_BUTTON_REPEAT: return {MSYS_X1_BUTTON,0,0};
    case X2_BUTTON_DOWN: return {MSYS_X2_BUTTON,1,0};
    case X2_BUTTON_UP: return {MSYS_X2_BUTTON,-1,0};
    case X2_BUTTON_REPEAT: return {MSYS_X2_BUTTON,0,0};
    default: return {};
    }
}
auto find(const MOUSE_REGION* p) { return std::find_if(regions.begin(),regions.end(),[p](const Entry& e){return e.region==p;}); }
bool valid(const MOUSE_REGION* p, std::uint64_t generation) { const auto it=find(p); return generation && it!=regions.end() && it->generation==generation; }
void changed() { ++mutations; refresh=true; }
void relink() {
    MSYS_RegList=regions.empty()?nullptr:regions.front().region;
    for (std::size_t i=0;i<regions.size();++i) { auto* r=regions[i].region; r->prev=i?regions[i-1].region:nullptr; r->next=i+1<regions.size()?regions[i+1].region:nullptr; }
}
UINT16 allocateId() {
    for (unsigned attempt=0;attempt<65535;++attempt) {
        const UINT16 id=nextId; nextId=nextId==65535?1:static_cast<UINT16>(nextId+1);
        if (std::none_of(regions.begin(),regions.end(),[id](const Entry& e){return e.region->IDNumber==id;})) return id;
    }
    return 0;
}
bool contains(const MOUSE_REGION* r) { return r->RegionTopLeftX<=MSYS_CurrentMX && r->RegionBottomRightX>=MSYS_CurrentMX && r->RegionTopLeftY<=MSYS_CurrentMY && r->RegionBottomRightY>=MSYS_CurrentMY; }
bool enabled(const MOUSE_REGION* r) { return (r->uiFlags & MSYS_REGION_ENABLED)!=0; }
// Mutation during a callback ends the event. A later explicit refresh/event
// reselects from the current list; no removed/address-reused region is touched.
bool call(MOUSE_REGION* r,MOUSE_CALLBACK callback,INT32 reason) {
    const auto generation=sgp::NativeMouseRegionGeneration(r); const auto epoch=mutations;
    callback(r,reason);
    return initialized && valid(r,generation) && epoch==mutations;
}
struct DispatchScope {
    DispatchScope() { dispatching=true; }
    ~DispatchScope() { dispatching=false; sgp::FinishNativeButtonDispatch(); }
};
void update(bool moved,UINT16 type) {
    auto* target=grabbed;
    if (!target) for (const auto& entry:regions) { auto* r=entry.region; if ((r->uiFlags & (MSYS_REGION_ENABLED|MSYS_ALLOW_DISABLED_FASTHELP)) && contains(r)) { target=r; break; } }
    const auto targetGeneration=sgp::NativeMouseRegionGeneration(target);
    auto* old=MSYS_PrevRegion;
    if (old!=target) {
        if (find(old)!=regions.end()) clearHelpFlags(old);
        resetHelp(target);
    }
    MSYS_CurrRegion=target;
    if (find(old)!=regions.end()) {
        old->uiFlags &= ~static_cast<UINT32>(MSYS_MOUSE_IN_AREA);
        if (old!=target && enabled(old) && (old->uiFlags & MSYS_MOVE_CALLBACK) && old->MovementCallback)
            if (!call(old,old->MovementCallback,MSYS_CALLBACK_REASON_LOST_MOUSE)) return;
    }
    if (!valid(target,targetGeneration)) { MSYS_PrevRegion=nullptr; return; }
    if (old!=target && enabled(target) && (target->uiFlags & MSYS_MOVE_CALLBACK) && target->MovementCallback)
        if (!call(target,target->MovementCallback,MSYS_CALLBACK_REASON_GAIN_MOUSE)) return;
    // Source selection: the enabled hit region wins if it supplies a cursor;
    // otherwise search lower regions covering the pointer (including base).
    auto cursorRegion=find(target);
    for (; cursorRegion!=regions.end(); ++cursorRegion) {
        auto* r=cursorRegion->region;
        if (!enabled(r) || (r!=target && !contains(r)) || !(r->uiFlags & MSYS_SET_CURSOR)) continue;
        if (r==target && r->Cursor==MSYS_NO_CURSOR) continue;
        if (r->Cursor!=MSYS_NO_CURSOR) {
            const auto epoch=mutations;
            MSYS_SetCurrentCursor(r->Cursor);
            // The original selector may execute services; never retain a stale
            // region/iterator across removal, shutdown or address reuse.
            if (!initialized || !valid(target,targetGeneration) || epoch!=mutations) return;
        }
        break;
    }
    const bool allowed=!clicked || clickedId==target->IDNumber;
    target->uiFlags |= MSYS_MOUSE_IN_AREA;
    target->MouseXPos=MSYS_CurrentMX; target->MouseYPos=MSYS_CurrentMY;
    // Explicit bounded narrowing matches original process-local INT16 fields.
    target->RelativeXPos=static_cast<INT16>(static_cast<int>(MSYS_CurrentMX)-target->RegionTopLeftX);
    target->RelativeYPos=static_cast<INT16>(static_cast<int>(MSYS_CurrentMY)-target->RegionTopLeftY);
    if (allowed) { target->ButtonState=buttons; target->WheelState=wheel; wheel=0; }
    const auto action=buttonEvent(type);
    if (action.mask && (target->uiFlags & MSYS_FASTHELP)) resetHelp(target);
    const bool clickButton=(action.mask & (MSYS_LEFT_BUTTON|MSYS_RIGHT_BUTTON))!=0;
    if (!allowed && enabled(target) && clickButton && action.direction<0) clicked=false;
    if (enabled(target) && moved && (target->uiFlags & MSYS_MOVE_CALLBACK) && target->MovementCallback)
        if (!call(target,target->MovementCallback,MSYS_CALLBACK_REASON_MOVE)) return;
    if (allowed && enabled(target) && (target->uiFlags & MSYS_BUTTON_CALLBACK) && target->ButtonCallback) {
        INT32 reason=action.reason;
        if (clickButton && action.direction>0) { clicked=true; clickedId=target->IDNumber; }
        if (clickButton && action.direction<0) clicked=false;
        if (reason && !call(target,target->ButtonCallback,reason)) return;
    }
    if (valid(target,targetGeneration)) MSYS_PrevRegion=target;
}
bool supported(UINT16 type) { return sgp::IsNativeGameMouseEvent(type); }
}
namespace sgp {
void SetNativeMouseCursorSelector(NativeMouseCursorSelector selector) { cursorSelector=selector; refresh=true; }
bool RestoreNativeFastHelpBackground() {
    if (!helpBackground.backing) return true;
    UINT16 width=0,height=0; UINT8 bpp=0;
    if (!GetVideoSurfaceDescription(helpBackground.backing,&width,&height,&bpp) || bpp!=16 ||
        helpBackground.x+helpBackground.w>width || helpBackground.y+helpBackground.h>height) {
        helpBackground={}; return true; // deleted monotonic surface cannot alias a new one
    }
    UINT32 pitch=0; auto* pixels=LockVideoSurface(helpBackground.backing,&pitch);
    if (!pixels) return false;
    for (int y=0;y<helpBackground.h;++y) for (int x=0;x<helpBackground.w;++x) {
        const auto index=static_cast<std::size_t>(y)*static_cast<std::size_t>(helpBackground.w)+static_cast<std::size_t>(x);
        auto* p=pixels+static_cast<std::size_t>(helpBackground.y+y)*pitch+static_cast<std::size_t>(helpBackground.x+x)*2;
        UINT16 current=0; std::memcpy(&current,p,2);
        if (current==helpBackground.drawn[index]) std::memcpy(p,&helpBackground.before[index],2);
    }
    UnLockVideoSurface(helpBackground.backing);
    InvalidateRegion(helpBackground.x,helpBackground.y,helpBackground.x+helpBackground.w,helpBackground.y+helpBackground.h);
    helpBackground={}; return true;
}
bool ConfigureNativeFastHelp(INT32 regular,INT32 bold,UINT32 destination) {
    const auto normalGeneration=fontGeneration(regular), boldGeneration=fontGeneration(bold);
    UINT16 w=0,h=0; UINT8 bpp=0;
    if (!normalGeneration || !boldGeneration || !GetVideoSurfaceDescription(destination,&w,&h,&bpp) || bpp!=16) return false;
    if (!RestoreNativeFastHelpBackground()) return false;
    if (!ConfigureNativeTooltipText(regular,bold,destination)) return false;
    helpBinding={regular,bold,normalGeneration,boldGeneration,destination,backingId(destination)};
    resetHelp(MSYS_CurrRegion); return true;
}
bool ConfigureNativeScalableFastHelp(INT32 regular,INT32 bold,double scale) {
    if (!RestoreNativeFastHelpBackground() || !ConfigureNativeScalableTooltipText(regular,bold,scale)) return false;
    resetHelp(MSYS_CurrRegion);return true;
}
void ClearNativeScalableFastHelp() noexcept {
    ClearNativeScalableTooltipText();resetHelp(MSYS_CurrRegion);
}
void SetNativeFastHelpClockForTesting(NativeFastHelpClock clock) {
    fastHelpClock=clock; resetHelp(MSYS_CurrRegion);
}
std::uint64_t NativeMouseRegionGeneration(const MOUSE_REGION* r) { const auto it=find(r); return it==regions.end()?0:it->generation; }
bool NativeMouseDispatchActive() { return dispatching; }
void CancelNativeMouseInput() {
    resetHelp(MSYS_CurrRegion);
    CancelNativeButtonInput();
    clicked=false; buttons=replayButtons=0; wheel=0;
    for (const auto& e:regions) { e.region->uiFlags &= ~static_cast<UINT32>(MSYS_MOUSE_IN_AREA); e.region->ButtonState=0; e.region->WheelState=0; }
    MSYS_PrevRegion=MSYS_CurrRegion=nullptr; changed(); observedReset=NativeInputResetGeneration();
}
bool DispatchNativeMouseEvent(const InputAtom& event) {
    if (dispatching) return false;
    if (observedReset!=NativeInputResetGeneration()) CancelNativeMouseInput();
    if (!initialized || !supported(event.usEvent)) return false;
    // Close retains the existing FIFO, including presses queued before close.
    // Reject that terminal backlog until the input manager is reinitialized.
    if (NativeInputTerminated()) { CancelNativeMouseInput(); return true; }
    const UINT16 type=event.usEvent;
    const auto action=buttonEvent(type);
    if (action.direction<0 && !(replayButtons & action.mask)) return true;
    if (action.direction>0) replayButtons=static_cast<UINT16>(replayButtons|action.mask);
    if (action.direction<0) replayButtons=static_cast<UINT16>(replayButtons & ~action.mask);
    MSYS_SGP_Mouse_Handler_Hook(type,GETXPOS(&event),GETYPOS(&event),
        static_cast<BOOLEAN>((replayButtons & MSYS_LEFT_BUTTON)!=0),
        static_cast<BOOLEAN>((replayButtons & MSYS_RIGHT_BUTTON)!=0));
    if (observedReset!=NativeInputResetGeneration()) CancelNativeMouseInput();
    return true;
}
void DispatchQueuedNativeGameMouseEvents(BOOLEAN extendedMouseEnabled) {
    // Dequeue performs the pump even when the FIFO is empty or keyboard-led.
    // Check reset independently of dispatch so hover-only focus loss cancels UI.
    if (dispatching || !initialized) return;
    InputAtom event{};
    while (DequeueNativeGameMouseEvent(&event)) {
        if (observedReset!=NativeInputResetGeneration()) CancelNativeMouseInput();
        const auto action=buttonEvent(event.usEvent);
        // Preserve the original option guard, including its omission of middle repeat.
        const bool extended=((action.mask & (MSYS_MIDDLE_BUTTON|MSYS_X1_BUTTON|MSYS_X2_BUTTON))!=0 &&
            event.usEvent!=MIDDLE_BUTTON_REPEAT) ||
            event.usEvent==MOUSE_WHEEL_UP || event.usEvent==MOUSE_WHEEL_DOWN;
        if (extendedMouseEnabled || !extended) (void)DispatchNativeMouseEvent(event);
        if (!initialized) break;
    }
    if (observedReset!=NativeInputResetGeneration() || NativeInputTerminated()) CancelNativeMouseInput();
    // Motion tracking is optional in the retained input API. Coalesce only
    // after the complete FIFO: never let final polling coordinates overtake a
    // keyboard head or substitute for any queued atom's historical position.
    if (initialized && NativeInputFocused() && NativeInputQueueEmpty()) {
        event={}; event.usEvent=MOUSE_POS;
        event.uiParam=static_cast<UINT16>(gusMouseXPos) |
            (static_cast<UINT32>(static_cast<UINT16>(gusMouseYPos))<<16);
        (void)DispatchNativeMouseEvent(event);
    }
}
}
extern "C" void MSYS_SGP_Mouse_Handler_Hook(UINT16 type,UINT16 x,UINT16 y,BOOLEAN left,BOOLEAN right) {
    if (!initialized || dispatching || !supported(type)) return;
    DispatchScope scope;
    const bool moved=MSYS_CurrentMX!=static_cast<INT16>(x) || MSYS_CurrentMY!=static_cast<INT16>(y) || (type==MOUSE_POS && refresh);
    MSYS_CurrentMX=static_cast<INT16>(x); MSYS_CurrentMY=static_cast<INT16>(y);
    const auto action=buttonEvent(type);
    if (action.direction>0) buttons=static_cast<UINT16>(buttons|action.mask);
    if (action.direction<0) buttons=static_cast<UINT16>(buttons & ~action.mask);
    // Original hook parameters are authoritative for L/R even on an inconsistent
    // direct-hook call; queued replay supplies the historical L/R state here.
    buttons=static_cast<UINT16>((buttons & ~(MSYS_LEFT_BUTTON|MSYS_RIGHT_BUTTON)) |
        (left?MSYS_LEFT_BUTTON:0)|(right?MSYS_RIGHT_BUTTON:0));
    // The source INT16 wheel accumulator is bounded explicitly on native.
    if (type==MOUSE_WHEEL_UP && wheel<std::numeric_limits<INT16>::max()) ++wheel;
    if (type==MOUSE_WHEEL_DOWN && wheel>std::numeric_limits<INT16>::min()) --wheel;
    if (type==LEFT_BUTTON_UP) ReleaseAnchorMode();
    if (type==MOUSE_POS && !moved) return;
    refresh=false; update(moved,type);
}
INT32 MSYS_Init() {
    if (dispatching) return 0;
    if (initialized) MSYS_Shutdown();
    if (serial==std::numeric_limits<std::uint64_t>::max()) return 0;
    MSYS_SystemBaseRegion={}; auto* r=&MSYS_SystemBaseRegion;
    r->PriorityLevel=MSYS_PRIORITY_SYSTEM; r->uiFlags=MSYS_REGION_ENABLED|MSYS_REGION_EXISTS|MSYS_SET_CURSOR;
    r->RegionTopLeftX=r->RegionTopLeftY=-32767; r->RegionBottomRightX=r->RegionBottomRightY=32767;
    r->FastHelpRect=-1; regions.push_back({r,++serial,{}}); relink();
    initialized=true; nextId=1; MSYS_CurrentMX=MSYS_CurrentMY=0; grabbed=nullptr;
    showFastHelp=true;
    sgp::CancelNativeMouseInput();
    SetMouseInputHook(&MSYS_SGP_Mouse_Handler_Hook); return 1;
}
void MSYS_Shutdown() {
    sgp::ClearNativeMouseInputHook(&MSYS_SGP_Mouse_Handler_Hook);
    sgp::CancelNativeMouseInput();
    for (const auto& e:regions) std::memset(e.region,0,sizeof(MOUSE_REGION));
    regions.clear(); MSYS_RegList=MSYS_PrevRegion=MSYS_CurrRegion=nullptr; grabbed=nullptr; initialized=false; changed();
    helpBinding={};
    sgp::ClearNativeTooltipText();
}
void MSYS_DefineRegion(MOUSE_REGION* r,UINT16 l,UINT16 t,UINT16 right,UINT16 bottom,INT8 priority,UINT16 cursor,MOUSE_CALLBACK move,MOUSE_CALLBACK click) {
    if (!initialized || !r || find(r)!=regions.end() || serial==std::numeric_limits<std::uint64_t>::max()) return;
    const UINT16 id=allocateId(); if (!id) return;
    *r={}; r->IDNumber=id; r->PriorityLevel=priority==MSYS_PRIORITY_AUTO?MSYS_PRIORITY_BASE:std::clamp<INT8>(priority,0,127);
    r->RegionTopLeftX=static_cast<INT16>(l); r->RegionTopLeftY=static_cast<INT16>(t); r->RegionBottomRightX=static_cast<INT16>(right); r->RegionBottomRightY=static_cast<INT16>(bottom);
    r->Cursor=cursor; r->MovementCallback=move; r->ButtonCallback=click;
    r->FastHelpRect=-1;
    r->uiFlags=MSYS_REGION_ENABLED|MSYS_REGION_EXISTS;
    if (move) r->uiFlags |= MSYS_MOVE_CALLBACK;
    if (click) r->uiFlags |= MSYS_BUTTON_CALLBACK;
    if (cursor!=MSYS_NO_CURSOR) r->uiFlags |= MSYS_SET_CURSOR;
    const auto at=std::find_if(regions.begin(),regions.end(),[r](const Entry& e){return e.region->PriorityLevel<=r->PriorityLevel;});
    regions.insert(at,{r,++serial,{}}); relink(); changed();
}
INT32 MSYS_AddRegion(MOUSE_REGION*) { return 1; }
void MSYS_RemoveRegion(MOUSE_REGION* r) {
    const auto it=find(r); if (it==regions.end()) return;
    if (r==MSYS_CurrRegion || (r->uiFlags & MSYS_FASTHELP)) resetHelp(r);
    if (clicked && clickedId==r->IDNumber) clicked=false;
    if (MSYS_PrevRegion==r) MSYS_PrevRegion=nullptr;
    if (MSYS_CurrRegion==r) MSYS_CurrRegion=nullptr;
    if (grabbed==r) grabbed=nullptr;
    regions.erase(it); relink(); std::memset(r,0,sizeof(MOUSE_REGION)); changed();
}
INT32 MSYS_RegionInList(MOUSE_REGION* r) { return find(r)!=regions.end()?1:0; }
void MSYS_EnableRegion(MOUSE_REGION* r) { if (MSYS_RegionInList(r) && !enabled(r)) { r->uiFlags|=MSYS_REGION_ENABLED; changed(); } }
void MSYS_DisableRegion(MOUSE_REGION* r) { if (MSYS_RegionInList(r) && enabled(r)) { r->uiFlags&=~static_cast<UINT32>(MSYS_REGION_ENABLED); if (r==MSYS_CurrRegion) resetHelp(r); changed(); } }
void MSYS_ChangeRegionPriority(MOUSE_REGION* r,INT8 priority) {
    if (!MSYS_RegionInList(r)) return;
    const INT8 normalized=priority==MSYS_PRIORITY_AUTO?MSYS_PRIORITY_NORMAL:priority;
    if (r->PriorityLevel!=normalized) { r->PriorityLevel=normalized; changed(); }
}
void MSYS_ChangeRegionCursor(MOUSE_REGION* r,UINT16 cursor) {
    if (!MSYS_RegionInList(r)) return;
    const bool set=cursor!=MSYS_NO_CURSOR;
    if (r->Cursor==cursor && ((r->uiFlags & MSYS_SET_CURSOR)!=0)==set) return;
    r->Cursor=cursor;
    if (set) r->uiFlags|=MSYS_SET_CURSOR; else r->uiFlags&=~static_cast<UINT32>(MSYS_SET_CURSOR);
    // Cosmetic metadata cannot change hit selection or callback lifetime. It
    // requests refresh without invalidating an in-flight GAIN/MOVE/button event.
    refresh=true;
    if (set && (r->uiFlags & MSYS_MOUSE_IN_AREA)) MSYS_SetCurrentCursor(cursor);
}
void MSYS_SetCurrentCursor(UINT16 cursor) {
    if (cursorSelector) (void)cursorSelector(static_cast<UINT32>(cursor));
}
void SetRegionFastHelpText(MOUSE_REGION* r,const STR16 text) {
    const auto entry=find(r);
    if (entry==regions.end()) return;
    if (!text || !*text) {
        if (r==MSYS_CurrRegion) resetHelp(r);
        entry->fastHelp.reset(); r->FastHelpText=nullptr; return;
    }
    // Allocate/copy before releasing the old buffer: text may be that buffer,
    // or any suffix within it. Heap ownership also survives Entry relocation.
    const auto length=std::wcslen(text);
    if (length>=std::numeric_limits<std::size_t>::max()/sizeof(CHAR16)) return;
    try {
        auto replacement=std::make_unique<CHAR16[]>(length+1);
        std::copy_n(text,length+1,replacement.get());
        if (r==MSYS_CurrRegion) resetHelp(r);
        entry->fastHelp=std::move(replacement);
        r->FastHelpText=entry->fastHelp.get();
    } catch (const std::bad_alloc&) {
        return; // Keep the previous valid metadata on allocation failure.
    }
    // Updating help cannot invalidate an in-flight GAIN/MOVE/button callback.
}
void SetRegionHelpEndCallback(MOUSE_REGION* r,MOUSE_HELPTEXT_DONE_CALLBACK callback) {
    if (r) r->HelpDoneCallback=callback;
}
void ExecuteMouseHelpEndCallBack(MOUSE_REGION*) {
    // Deliberately disabled by the pinned original, including when timer is zero.
}
void SetFastHelpDelay(INT16 delay) { fastHelpDelay=std::max<INT16>(0,delay); }
void EnableMouseFastHelp() { if (!showFastHelp) { showFastHelp=true; resetHelp(MSYS_CurrRegion); } }
void DisableMouseFastHelp() { showFastHelp=false; resetHelp(MSYS_CurrRegion); }
void RenderFastHelp() {
    if (!sgp::RestoreNativeFastHelpBackground()) return;
    if (!initialized || dispatching) return;
    if (observedReset!=sgp::NativeInputResetGeneration() || sgp::NativeInputTerminated()) {
        sgp::CancelNativeMouseInput(); return;
    }
    auto* r=MSYS_CurrRegion;
    const auto generation=sgp::NativeMouseRegionGeneration(r);
    const auto now=helpNow();
    const auto elapsed=now>=helpTick?now-helpTick:0;
    helpTick=now;
    if (!valid(r,generation)) return;
    clearHelpFlags(r);
    if (!showFastHelp || !gfRenderHilights || !r->FastHelpText ||
        !(r->uiFlags & MSYS_MOUSE_IN_AREA) ||
        !(r->uiFlags & (MSYS_REGION_ENABLED|MSYS_ALLOW_DISABLED_FASTHELP))) return;
    if (r->FastHelpTimer>0) {
        if (!r->ButtonState) {
            const auto remaining=static_cast<std::uint64_t>(r->FastHelpTimer);
            r->FastHelpTimer=elapsed>=remaining?0:static_cast<INT16>(remaining-elapsed);
        }
        return; // Original renders on the frame after the countdown reaches zero.
    }
    if (r->ButtonState) return;
    try { (void)drawHelp(r); }
    catch (const std::bad_alloc&) { /* no framebuffer write before staging succeeds */ }
}
void MSYS_SetRegionUserData(MOUSE_REGION* r,INT32 i,NativeUIPayload value) { if (MSYS_RegionInList(r) && i>=0 && i<4) r->UserData[i]=value; }
NativeUIPayload MSYS_GetRegionUserData(MOUSE_REGION* r,INT32 i) { return MSYS_RegionInList(r) && i>=0 && i<4?r->UserData[i]:0; }
INT32 MSYS_GrabMouse(MOUSE_REGION* r) { if (!MSYS_RegionInList(r)) return MSYS_REGION_NOT_IN_LIST; if (grabbed) return MSYS_ALREADY_GRABBED; grabbed=r; changed(); return MSYS_GRABBED_OK; }
void MSYS_ReleaseMouse(MOUSE_REGION* r) { if (r && grabbed==r) { grabbed=nullptr; changed(); RefreshMouseRegions(); } }
void MSYS_MoveMouseRegionTo(MOUSE_REGION* r,INT16 x,INT16 y) {
    if (!MSYS_RegionInList(r) || (r->RegionTopLeftX==x && r->RegionTopLeftY==y)) return;
    const int right=static_cast<int>(x)+r->RegionBottomRightX-r->RegionTopLeftX, bottom=static_cast<int>(y)+r->RegionBottomRightY-r->RegionTopLeftY;
    if (right<-32768 || right>32767 || bottom<-32768 || bottom>32767) return;
    if (r==MSYS_CurrRegion) resetHelp(r);
    r->RegionTopLeftX=x; r->RegionTopLeftY=y; r->RegionBottomRightX=static_cast<INT16>(right); r->RegionBottomRightY=static_cast<INT16>(bottom); changed();
}
void MSYS_MoveMouseRegionBy(MOUSE_REGION* r,INT16 dx,INT16 dy) {
    if (!MSYS_RegionInList(r)) return;
    const int x=static_cast<int>(r->RegionTopLeftX)+dx, y=static_cast<int>(r->RegionTopLeftY)+dy;
    if (x<-32768 || x>32767 || y<-32768 || y>32767) return;
    MSYS_MoveMouseRegionTo(r,static_cast<INT16>(x),static_cast<INT16>(y));
}
void MSYS_AllowDisabledRegionFastHelp(MOUSE_REGION* r,BOOLEAN allow) {
    if (!MSYS_RegionInList(r) || ((r->uiFlags & MSYS_ALLOW_DISABLED_FASTHELP)!=0)==(allow!=FALSE)) return;
    if (allow) r->uiFlags|=MSYS_ALLOW_DISABLED_FASTHELP; else r->uiFlags&=~static_cast<UINT32>(MSYS_ALLOW_DISABLED_FASTHELP);
    if (r==MSYS_CurrRegion) resetHelp(r);
    changed();
}
void RefreshMouseRegions() { refresh=true; if (!initialized || dispatching) return; DispatchScope scope; refresh=false; update(true,MOUSE_POS); }
void MSYS_UpdateMouseRegion() { RefreshMouseRegions(); }
void ResetClickedMode() { clicked=false; }
void ResetWheelState(MOUSE_REGION* r) { if (MSYS_RegionInList(r)) r->WheelState=0; }
MOUSE_REGION* get_first_entry_in_MSYS_RegList() { return MSYS_RegList; }
MOUSE_REGION* get_next_entry_in_MSYS_RegList(MOUSE_REGION* r) { return MSYS_RegionInList(r)?r->next:nullptr; }
