/*
 * P710 native notice preparation: sgp/NativeVideo.cpp
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: implement native reserved framebuffer/video-manager subset.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/NativeVideo.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_video.h"
#include "SDLPresentation.h"
#include "vobject.h"
#include "clip_state.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
UINT32 guiMouseBufferState=BUFFER_DISABLED;
BOOLEAN gfNextRefreshFullScreen=FALSE;
INT32 giNumFrames=0; // original shared Air Raid/recording counter; game code owns increments
namespace
{
sgp::SDLPresentation window;
bool active=false, suspended=false, full=false, renderPending=false;
UINT32 width=640, height=480, scale=1;
std::string title="JA2 v1.13 native video";
sgp::NativePresentationOptions presentationOptions{};
std::array<SGPRect,128> dirty{};
std::size_t count=0;
std::array<SGPRect,128> extendedDirty{};
std::size_t extendedCount=0;
sgp::NativeVideoRegionSource regionSource=nullptr;
sgp::NativeVideoScrollHooks scrollHooks{};
std::uint64_t generation=0;
SDL_Cursor* cursor=nullptr;
SDL_Cursor* previousCursor=nullptr;
bool previousVisible=true;
HVOBJECT cursorStore=nullptr;
INT16 cursorX=0,cursorY=0;
UINT16 cursorW=0,cursorH=0;
sgp::NativePresentationGeometry cursorGeometry{};
void closeCursor()
{
    // Restore the borrowed cursor before destroying our currently selected one.
    if (cursor && SDL_GetCursor()==cursor) SDL_SetCursor(previousCursor ? previousCursor : SDL_GetDefaultCursor());
    if (cursor) SDL_DestroyCursor(cursor);
    cursor=nullptr; previousCursor=nullptr;
    if (previousVisible) SDL_ShowCursor(); else SDL_HideCursor();
    if (cursorStore) DeleteVideoObject(cursorStore);
    cursorStore=nullptr; cursorW=cursorH=0; cursorX=cursorY=0;
    cursorGeometry={}; guiMouseBufferState=BUFFER_DISABLED;
}
bool dimensions(UINT32 w, UINT32 h, UINT32 depth, UINT32 s)
{
    if (!w || !h || w>65535 || h>65535 || depth!=16 || !s || s>static_cast<UINT32>(std::numeric_limits<int>::max())/w || s>static_cast<UINT32>(std::numeric_limits<int>::max())/h) return false;
    const std::uint64_t pitch=((static_cast<std::uint64_t>(w)*2+7)&~std::uint64_t(7))+8;
    const std::uint64_t sw=static_cast<std::uint64_t>(w)*s, sh=static_cast<std::uint64_t>(h)*s;
    const auto limit=std::min(static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()),static_cast<std::uint64_t>(std::numeric_limits<std::ptrdiff_t>::max()));
    return pitch*h<=limit/3 && sw*sh<=limit/4;
}
void copy(BYTE* d, UINT32 dp, const BYTE* s, UINT32 sp, const SGPRect& r)
{
    for (INT32 y=r.iTop; y<r.iBottom; ++y)
        std::memcpy(d+static_cast<std::size_t>(y)*dp+static_cast<std::size_t>(r.iLeft)*2,s+static_cast<std::size_t>(y)*sp+static_cast<std::size_t>(r.iLeft)*2,static_cast<std::size_t>(r.iRight-r.iLeft)*2);
}
sgp::NativeVideoRegionState regionState()
{
    auto state=regionSource ? regionSource() : sgp::NativeVideoRegionState{static_cast<INT32>(height),false};
    state.viewportBottom=std::clamp(state.viewportBottom,INT32{0},static_cast<INT32>(height));
    return state;
}
void addExtended(INT32 l,INT32 t,INT32 r,INT32 b)
{
    const auto left=std::max<INT32>(0,l), top=std::max<INT32>(0,t);
    const auto right=std::min<INT32>(static_cast<INT32>(width),r);
    const auto bottom=std::min<INT32>(static_cast<INT32>(height),b);
    if (left>=right || top>=bottom) return;
    if (extendedCount==extendedDirty.size()) { InvalidateScreen(); return; }
    extendedDirty[extendedCount++]={left,top,right,bottom};
}
}
void sgp::SetNativeVideoRegionSource(NativeVideoRegionSource source) { regionSource=source; }
void sgp::SetNativeVideoScrollHooks(NativeVideoScrollHooks hooks) { scrollHooks=hooks; }
bool sgp::ConfigureNativeVideo(UINT32 w, UINT32 h, UINT32 depth, UINT32 s, const char* name,
                               NativePresentationOptions options)
{
    if (active || !dimensions(w,h,depth,s) ||
        (options.display!=NativeDisplayTarget::Primary && options.display!=NativeDisplayTarget::Upper)) return false;
    try { std::string owned(name ? name : "JA2 v1.13 native video"); title.swap(owned); } catch (...) { return false; }
    width=w; height=h; scale=s; presentationOptions=options; return true;
}
BOOLEAN InitializeVideoManager(void* instance, UINT16 show, void* windowproc)
{
    if (instance || show || windowproc) return FALSE;
    if (active) return TRUE;
    if (!dimensions(width,height,16,scale)) return FALSE;
    if (!window.Open(title.c_str(),static_cast<int>(width),static_cast<int>(height),static_cast<int>(scale),presentationOptions)) return FALSE;
    InitializeVideoSurfaceManager();
    UINT32 ids[4]{};
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=width; d.usHeight=height; d.ubBitDepth=16;
    bool ok=true;
    for (std::size_t i=0; i<4; ++i) {
        if (i==3) { d.usWidth=MAX_CURSOR_WIDTH; d.usHeight=MAX_CURSOR_HEIGHT; }
        if (!AddVideoSurface(&d,&ids[i])) { ok=false; break; }
    }
    if (ok) ok=sgp::BindReservedVideoSurfaces(ids);
    if (!ok) { for (auto id : ids) if (id) DeleteVideoSurfaceFromIndex(id); window.Close(); return FALSE; }
    // Indexed objects and buttons need a drawing clip before any screen runs.
    // Repeated active initialization remains a no-op and keeps caller clipping.
    SGPRect clip{0,0,static_cast<INT32>(width),static_cast<INT32>(height)};
    SetClippingRect(&clip);
    previousCursor=SDL_GetCursor(); previousVisible=SDL_CursorVisible();
    active=true; suspended=false; full=true; renderPending=false; count=0; extendedCount=0; gfNextRefreshFullScreen=FALSE;
    guiMouseBufferState=BUFFER_DISABLED;
    if (generation!=std::numeric_limits<std::uint64_t>::max()) ++generation;
    return TRUE;
}
bool sgp::ShutdownNativeVideo()
{
    if (!active) return true;
    if (!sgp::ReleaseReservedVideoSurfaces()) return false;
    closeCursor();
    window.Close(); active=false; suspended=false; full=false; renderPending=false; count=0; extendedCount=0; gfNextRefreshFullScreen=FALSE;
    if (generation!=std::numeric_limits<std::uint64_t>::max()) ++generation;
    return true;
}
void ShutdownVideoManager() { (void)sgp::ShutdownNativeVideo(); }
void SuspendVideoManager() { if (active) suspended=true; }
BOOLEAN RestoreVideoManager() { if (!active) return FALSE; suspended=false; return TRUE; }
void GetCurrentVideoSettings(UINT16* w, UINT16* h, UINT8* depth)
{
    if (w) *w=static_cast<UINT16>(width);
    if (h) *h=static_cast<UINT16>(height);
    if (depth) *depth=16;
}
PTR LockPrimarySurface(UINT32* p) { return LockVideoSurface(PRIMARY_SURFACE,p); }
void UnlockPrimarySurface() { UnLockVideoSurface(PRIMARY_SURFACE); }
PTR LockBackBuffer(UINT32* p) { return LockVideoSurface(BACKBUFFER,p); }
void UnlockBackBuffer() { UnLockVideoSurface(BACKBUFFER); }
PTR LockFrameBuffer(UINT32* p) { return LockVideoSurface(FRAME_BUFFER,p); }
void UnlockFrameBuffer() { UnLockVideoSurface(FRAME_BUFFER); }
BOOLEAN CanBlitToFrameBuffer()
{
    if (!active || suspended || full || renderPending || count!=0 || extendedCount!=0) return FALSE;
    UINT32 pitch=0; if (!LockVideoSurface(FRAME_BUFFER,&pitch)) return FALSE;
    UnLockVideoSurface(FRAME_BUFFER); return TRUE;
}
void InvalidateScreen() { if (active) { full=true; count=0; extendedCount=0; } }
void InvalidateFrameBuffer() { InvalidateScreen(); }
void StartFrameBufferRender() {}
void EndFrameBufferRender()
{
    // Original EndFrameBufferRender marks BUFFER_DIRTY; it does not request a
    // full copy. During scrolling FRAME_BUFFER holds only freshly drawn strips
    // and dirty work, while BACKBUFFER retains the accumulated world view.
    if (active) renderPending=true;
}
void InvalidateRegion(INT32 l, INT32 t, INT32 r, INT32 b)
{
    if (!active || full) return;
    const std::int64_t left=std::max<std::int64_t>(0,l), top=std::max<std::int64_t>(0,t);
    const std::int64_t right=std::min<std::int64_t>(width,r), bottom=std::min<std::int64_t>(height,b);
    if (left>=right || top>=bottom) return;
    if (count==dirty.size()) { InvalidateScreen(); return; }
    dirty[count++]={static_cast<INT32>(left),static_cast<INT32>(top),static_cast<INT32>(right),static_cast<INT32>(bottom)};
}
void InvalidateRegionEx(INT32 l,INT32 t,INT32 r,INT32 b,UINT32 /*flags*/)
{
    if (!active || full) return;
    const auto state=regionState();
    // Preserve the original viewport split without a dependency on the engine
    // globals or a second rendering implementation.
    if (t<=state.viewportBottom && b>state.viewportBottom) {
        addExtended(l,t,r,state.viewportBottom);
        if (!full) addExtended(l,state.viewportBottom,r,b);
    } else addExtended(l,t,r,b);
}
void InvalidateRegions(SGPRect* regions, UINT32 n)
{
    if (!active || full || !regions) return;
    if (n>dirty.size()) { InvalidateScreen(); return; }
    for (UINT32 i=0; i<n; ++i) InvalidateRegion(regions[i].iLeft,regions[i].iTop,regions[i].iRight,regions[i].iBottom);
}
bool sgp::RefreshNativeVideo()
{
    if (!active || suspended || !sgp::ReservedVideoSurfacesReady()) return false;
    auto state=regionState();
    // Original RefreshScreen requests full frame transfer only on GAME_SCREEN.
    // Suppress scrolling for that retry; commit clears the original scroll flags.
    if (gfNextRefreshFullScreen && state.gameScreen) { InvalidateScreen();state.scrolling=false; }
    const auto hooks=scrollHooks;
    if (state.scrolling && (!hooks.renderStrip || !hooks.beginOverlays ||
        !hooks.finishOverlays || !hooks.endOverlays)) return false;
    struct Locks {
        const UINT32 aliases[3]{PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER};
        std::array<sgp::BackgroundView,3> views{};
        std::size_t held=0;
        void release() { while (held) UnLockVideoSurface(aliases[--held]); }
        bool acquire() {
            if (held==3) return true;
            release();
            for (;held<3;) {
                UINT32 pitch=0;
                // Manager locks and their metadata lookup do not allocate.
                // Rollback must reacquire after callbacks even under low memory.
                auto* data=LockVideoSurface(aliases[held],&pitch);
                if (!data) { release();return false; }
                ++held;
                sgp::NativeSurfaceView actual;
                if (!sgp::LockedNativeSurfaceView(data,&actual) || actual.pitch!=pitch) {
                    release();return false;
                }
                views[held-1]={actual.data,actual.bytes,actual.width,actual.height,actual.pitch};
            }
            return true;
        }
        ~Locks() { release(); }
    } locks;
    if (!locks.acquire()) return false;
    sgp::BackgroundView z{};sgp::NativeScrollPlan scrollPlan;
    std::array<std::vector<BYTE>,4> saved;
    bool overlayTransaction=false;
    const auto pendingDirty=dirty,pendingExtended=extendedDirty;
    const auto pendingCount=count,pendingExtendedCount=extendedCount;
    const bool pendingFull=full;
    if (state.scrolling) {
        if (!sgp::OwnedZBufferView(state.zBuffer,&z) ||
            !sgp::PlanNativeScroll(locks.views[1],z,state.scroll,scrollPlan)) return false;
        try {
            for (std::size_t i=0;i<3;++i)
                saved[i].assign(locks.views[i].data,locks.views[i].data+locks.views[i].bytes);
            saved[3].assign(z.data,z.data+z.bytes);
            // Overlay snapshot preparation must succeed before dirty copies
            // or scroll mutation. Original callbacks acquire their own locks.
            locks.release();
            if (!hooks.beginOverlays()) return false;
            overlayTransaction=true;
            if (!locks.acquire()) { hooks.endOverlays(false);return false; }
        } catch (...) {
            if (overlayTransaction) hooks.endOverlays(false);
            return false;
        }
    }
    const auto rollback=[&]() {
        // The real engine hooks balance their locks. A hook retaining an outer
        // lock is a contract violation; never unlock somebody else's lock.
        const bool acquired=locks.acquire();
        if (acquired) {
            for (std::size_t i=0;i<3;++i)
                std::memcpy(locks.views[i].data,saved[i].data(),saved[i].size());
            std::memcpy(z.data,saved[3].data(),saved[3].size());
        }
        locks.release();
        if (overlayTransaction) { hooks.endOverlays(false);overlayTransaction=false; }
        dirty=pendingDirty;extendedDirty=pendingExtended;
        count=pendingCount;extendedCount=pendingExtendedCount;full=pendingFull;
        return false;
    };
    const SGPRect all{0,0,static_cast<INT32>(width),static_cast<INT32>(height)};
    auto transfer=[&](const SGPRect& rect) {
        copy(locks.views[1].data,locks.views[1].pitch,locks.views[2].data,locks.views[2].pitch,rect);
    };
    if (full) transfer(all);
    else {
        for (std::size_t i=0;i<count;++i) transfer(dirty[i]);
        for (std::size_t i=0;i<extendedCount;++i) {
            // Original extended world rectangles are redrawn through scroll
            // strips; the separately split interface portion still transfers.
            if (state.scrolling && extendedDirty[i].iBottom<=state.viewportBottom) continue;
            transfer(extendedDirty[i]);
        }
    }
    if (state.scrolling) {
        if (!sgp::ApplyNativeScroll(locks.views[1],z,state.scroll)) return rollback();
        locks.release();
        try {
            for (UINT32 i=0;i<scrollPlan.stripCount;++i) {
                const auto& strip=scrollPlan.strips[i];
                if (!hooks.renderStrip(strip) || !locks.acquire()) return rollback();
                transfer(strip);
                locks.release();
            }
            if (!hooks.finishOverlays(scrollPlan.shiftX,scrollPlan.shiftY) || !locks.acquire()) return rollback();
        } catch (...) { return rollback(); }
    }
    copy(locks.views[0].data,locks.views[0].pitch,locks.views[1].data,locks.views[1].pitch,all);
    const bool ok=window.Present565(locks.views[0].data,locks.views[0].bytes,locks.views[0].pitch);
    if (!ok && state.scrolling) return rollback();
    locks.release();
    if (overlayTransaction) { hooks.endOverlays(true);overlayTransaction=false; }
    if (ok) {
        full=false;renderPending=false;count=0;extendedCount=0;gfNextRefreshFullScreen=FALSE;
        if (hooks.committedFrame) hooks.committedFrame();
        (void)sgp::RefreshNativeCursor();
    }
    return ok;
}
void RefreshScreen(void*) { (void)sgp::RefreshNativeVideo(); }
bool sgp::SaveNativeVideoWindow(const char* path) { return active && window.SaveWindowSurface(path); }
const char* sgp::NativeVideoDriver() { return active ? window.VideoDriver() : "uninitialized"; }
bool sgp::NativeVideoQuitRequested() { return !active || window.QuitRequested(); }
bool sgp::NativeVideoInputInfo(SDL_WindowID& id, int& w, int& h, int& ww, int& wh) { return active && window.InputInfo(id,w,h,ww,wh); }
bool sgp::NativeVideoPresentationGeometry(NativePresentationGeometry& geometry) { return active && window.Geometry(geometry); }
bool sgp::AttachNativeVideoInput(PresentationEventSink fn, void* context) { return active && window.AttachInput(fn,context); }
void sgp::DetachNativeVideoInput(PresentationEventSink fn, void* context) { window.DetachInput(fn,context); }
void sgp::PumpNativeVideoEvents() { if (active) window.PumpEvents(); }

std::uint64_t sgp::NativeVideoGeneration() { return generation; }
PTR LockMouseBuffer(UINT32* pitch) { return LockVideoSurface(MOUSE_BUFFER,pitch); }
void UnlockMouseBuffer() { UnLockVideoSurface(MOUSE_BUFFER); }
BOOLEAN CanBlitToMouseBuffer()
{
    if (!active || suspended || guiMouseBufferState!=BUFFER_READY) return FALSE;
    UINT32 pitch=0; if (!LockMouseBuffer(&pitch)) return FALSE;
    UnlockMouseBuffer(); return TRUE;
}
BOOLEAN EraseMouseCursor()
{
    UINT32 pitch=0; auto* p=static_cast<BYTE*>(LockMouseBuffer(&pitch));
    if (!p) return FALSE;
    for (unsigned y=0; y<MAX_CURSOR_HEIGHT; ++y) std::memset(p+std::size_t(y)*pitch,0,MAX_CURSOR_WIDTH*2);
    UnlockMouseBuffer(); return TRUE;
}
BOOLEAN SetMouseCursorProperties(INT16 x,INT16 y,UINT16 h,UINT16 w)
{
    if (!active || h>MAX_CURSOR_HEIGHT || w>MAX_CURSOR_WIDTH) return FALSE;
    UINT32 pitch=0; if (!LockMouseBuffer(&pitch)) return FALSE;
    UnlockMouseBuffer(); cursorX=x; cursorY=y; cursorH=h; cursorW=w; return TRUE;
}
BOOLEAN BltToMouseCursor(UINT32 id,UINT16 frame,UINT16 x,UINT16 y)
{ return BltVideoObjectFromIndex(MOUSE_BUFFER,id,frame,x,y,VO_BLT_SRCTRANSPARENCY,nullptr); }
namespace {
BOOLEAN setCursorObject(HVOBJECT object,UINT16 frame,UINT16 x,UINT16 y)
{
    ETRLEObject properties{};
    if (!active || !GetVideoObjectETRLEProperties(object,&properties,frame)) return FALSE;
    const int w=int(properties.usWidth)+properties.sOffsetX;
    const int h=int(properties.usHeight)+properties.sOffsetY;
    if (w<=0 || h<=0 || w>MAX_CURSOR_WIDTH || h>MAX_CURSOR_HEIGHT || x>32767 || y>32767) return FALSE;
    std::array<BYTE,MAX_CURSOR_WIDTH*MAX_CURSOR_HEIGHT*2> saved{};
    UINT32 pitch=0; auto* bitmap=static_cast<BYTE*>(LockMouseBuffer(&pitch));
    if (!bitmap) return FALSE;
    for (unsigned row=0; row<MAX_CURSOR_HEIGHT; ++row)
        std::memcpy(saved.data()+std::size_t(row)*MAX_CURSOR_WIDTH*2,bitmap+std::size_t(row)*pitch,MAX_CURSOR_WIDTH*2);
    UnlockMouseBuffer();
    if (!EraseMouseCursor()) return FALSE;
    if (!BltVideoObject(MOUSE_BUFFER,object,frame,0,0,VO_BLT_SRCTRANSPARENCY,nullptr)) {
        bitmap=static_cast<BYTE*>(LockMouseBuffer(&pitch));
        if (bitmap) {
            for (unsigned row=0; row<MAX_CURSOR_HEIGHT; ++row)
                std::memcpy(bitmap+std::size_t(row)*pitch,saved.data()+std::size_t(row)*MAX_CURSOR_WIDTH*2,MAX_CURSOR_WIDTH*2);
            UnlockMouseBuffer();
        }
        return FALSE;
    }
    cursorX=static_cast<INT16>(x); cursorY=static_cast<INT16>(y);
    cursorH=static_cast<UINT16>(h); cursorW=static_cast<UINT16>(w);
    DirtyCursor(); return TRUE;
}
}
BOOLEAN SetMouseCursorFromObject(UINT32 id,UINT16 frame,UINT16 x,UINT16 y)
{
    HVOBJECT object=nullptr;
    if (!GetVideoObject(&object,id)) return FALSE;
    return setCursorObject(object,frame,x,y);
}
BOOLEAN LoadCursorFile(STR8 filename)
{
    if (!active || !filename || std::strlen(filename)>=sizeof(VOBJECT_DESC{}.ImageFile)) return FALSE;
    VOBJECT_DESC descriptor{}; descriptor.fCreateFlags=VOBJECT_CREATE_FROMFILE;
    std::strcpy(descriptor.ImageFile,filename);
    HVOBJECT loaded=CreateVideoObject(&descriptor);
    if (!loaded) return FALSE;
    if (cursorStore) DeleteVideoObject(cursorStore);
    cursorStore=loaded; return TRUE;
}
BOOLEAN SetCurrentCursor(UINT16 frame,UINT16 x,UINT16 y) { return setCursorObject(cursorStore,frame,x,y); }
void DirtyCursor() { if (active) guiMouseBufferState=BUFFER_DIRTY; }
void EnableCursor(BOOLEAN disable)
{
    if (!active) return;
    guiMouseBufferState=disable ? BUFFER_DISABLED : BUFFER_DIRTY;
    (void)sgp::RefreshNativeCursor();
}
BOOLEAN HideMouseCursor()
{
    if (!active) return FALSE;
    guiMouseBufferState=BUFFER_DISABLED;
    return static_cast<BOOLEAN>(sgp::RefreshNativeCursor());
}
bool sgp::RefreshNativeCursor()
{
    if (!active || suspended || !SDL_IsMainThread()) return false;
    NativePresentationGeometry geometry{};
    if (!window.Geometry(geometry)) return false;
    if (guiMouseBufferState==BUFFER_DISABLED) return SDL_HideCursor();
    if (guiMouseBufferState==BUFFER_READY && geometry==cursorGeometry) return true;
    if (guiMouseBufferState!=BUFFER_DIRTY && guiMouseBufferState!=BUFFER_READY) return false;
    UINT32 pitch=0; const auto* source=static_cast<const BYTE*>(LockMouseBuffer(&pitch));
    if (!source) return false;
    // SDL hotspots must be inside their bitmap. Transparent padding represents
    // signed original offsets, including hotspots beyond the sprite rectangle.
    const int left=std::min(0,int(cursorX)), top=std::min(0,int(cursorY));
    const int right=std::max(int(cursorW),int(cursorX)+1), bottom=std::max(int(cursorH),int(cursorY)+1);
    const double sxScale=double(geometry.contentWidth)*geometry.windowWidth/(double(geometry.pixelWidth)*geometry.logicalWidth);
    const double syScale=double(geometry.contentHeight)*geometry.windowHeight/(double(geometry.pixelHeight)*geometry.logicalHeight);
    const auto dw=std::ceil((right-left)*sxScale), dh=std::ceil((bottom-top)*syScale);
    if (!cursorW || !cursorH) { UnlockMouseBuffer(); if (!SDL_HideCursor()) return false; guiMouseBufferState=BUFFER_READY; cursorGeometry=geometry; return true; }
    // Bound pathological offset/resize allocations; keep the previous cursor
    // and pending bitmap intact if the native driver cannot represent it.
    if (dw<=0 || dh<=0 || dw>4096 || dh>4096) { UnlockMouseBuffer(); return false; }
    SDL_Surface* rgba=SDL_CreateSurface(static_cast<int>(dw),static_cast<int>(dh),SDL_PIXELFORMAT_ARGB8888);
    if (!rgba) { UnlockMouseBuffer(); return false; }
    for (int py=0; py<dh; ++py) for (int px=0; px<dw; ++px) {
        const int sx=static_cast<int>(std::floor(px/sxScale))+left;
        const int sy=static_cast<int>(std::floor(py/syScale))+top;
        UINT16 color=0;
        if (sx>=0 && sy>=0 && sx<cursorW && sy<cursorH) std::memcpy(&color,source+std::size_t(sy)*pitch+std::size_t(sx)*2,2);
        const unsigned r=(color>>11)&31u,g=(color>>5)&63u,b=color&31u;
        const Uint32 pixel=color ? 0xff000000u | ((r*255u/31u)<<16) | ((g*255u/63u)<<8) | (b*255u/31u) : 0u;
        std::memcpy(static_cast<BYTE*>(rgba->pixels)+std::size_t(py)*static_cast<std::size_t>(rgba->pitch)+std::size_t(px)*4,&pixel,4);
    }
    UnlockMouseBuffer();
    const int hx=static_cast<int>(std::floor((cursorX-left)*sxScale)), hy=static_cast<int>(std::floor((cursorY-top)*syScale));
    SDL_Cursor* replacement=SDL_CreateColorCursor(rgba,hx,hy); SDL_DestroySurface(rgba);
    if (!replacement) return false;
    if (!SDL_SetCursor(replacement)) { SDL_DestroyCursor(replacement); return false; }
    if (!SDL_ShowCursor()) {
        SDL_SetCursor(cursor ? cursor : previousCursor);
        SDL_DestroyCursor(replacement); return false;
    }
    if (cursor) SDL_DestroyCursor(cursor);
    cursor=replacement; guiMouseBufferState=BUFFER_READY; cursorGeometry=geometry;
    return true;
}
