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

#include "native_video.h"
#include "SDLPresentation.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
namespace
{
sgp::SDLPresentation window;
bool active=false, suspended=false, full=false;
UINT32 width=640, height=480, scale=1;
std::string title="JA2 v1.13 native video";
std::array<SGPRect,128> dirty{};
std::size_t count=0;
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
}
bool sgp::ConfigureNativeVideo(UINT32 w, UINT32 h, UINT32 depth, UINT32 s, const char* name)
{
    if (active || !dimensions(w,h,depth,s)) return false;
    try { std::string owned(name ? name : "JA2 v1.13 native video"); title.swap(owned); } catch (...) { return false; }
    width=w; height=h; scale=s; return true;
}
BOOLEAN InitializeVideoManager(void* instance, UINT16 show, void* windowproc)
{
    if (instance || show || windowproc) return FALSE;
    if (active) return TRUE;
    if (!dimensions(width,height,16,scale)) return FALSE;
    if (!window.Open(title.c_str(),static_cast<int>(width),static_cast<int>(height),static_cast<int>(scale))) return FALSE;
    InitializeVideoSurfaceManager();
    UINT32 ids[3]{};
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=width; d.usHeight=height; d.ubBitDepth=16;
    bool ok=true;
    for (auto& id : ids) if (!AddVideoSurface(&d,&id)) { ok=false; break; }
    if (ok) ok=sgp::BindReservedVideoSurfaces(ids);
    if (!ok) { for (auto id : ids) if (id) DeleteVideoSurfaceFromIndex(id); window.Close(); return FALSE; }
    active=true; suspended=false; full=true; count=0; return TRUE;
}
bool sgp::ShutdownNativeVideo()
{
    if (!active) return true;
    if (!sgp::ReleaseReservedVideoSurfaces()) return false;
    window.Close(); active=false; suspended=false; full=false; count=0;
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
    if (!active || suspended || full || count!=0) return FALSE;
    UINT32 pitch=0; if (!LockVideoSurface(FRAME_BUFFER,&pitch)) return FALSE;
    UnLockVideoSurface(FRAME_BUFFER); return TRUE;
}
void InvalidateScreen() { if (active) { full=true; count=0; } }
void InvalidateFrameBuffer() { InvalidateScreen(); }
void StartFrameBufferRender() {}
void EndFrameBufferRender() { InvalidateScreen(); }
void InvalidateRegion(INT32 l, INT32 t, INT32 r, INT32 b)
{
    if (!active || full) return;
    const std::int64_t left=std::max<std::int64_t>(0,l), top=std::max<std::int64_t>(0,t);
    const std::int64_t right=std::min<std::int64_t>(width,r), bottom=std::min<std::int64_t>(height,b);
    if (left>=right || top>=bottom) return;
    if (count==dirty.size()) { InvalidateScreen(); return; }
    dirty[count++]={static_cast<INT32>(left),static_cast<INT32>(top),static_cast<INT32>(right),static_cast<INT32>(bottom)};
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
    BYTE* pixels[3]{}; UINT32 pitch[3]{};
    const UINT32 aliases[]={PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER};
    for (std::size_t i=0; i<3; ++i)
    {
        pixels[i]=LockVideoSurface(aliases[i],&pitch[i]);
        if (!pixels[i])
        {
            for (std::size_t j=0; j<i; ++j) UnLockVideoSurface(aliases[j]);
            return false;
        }
    }
    const SGPRect all{0,0,static_cast<INT32>(width),static_cast<INT32>(height)};
    if (full) copy(pixels[1],pitch[1],pixels[2],pitch[2],all);
    else for (std::size_t i=0; i<count; ++i) copy(pixels[1],pitch[1],pixels[2],pitch[2],dirty[i]);
    copy(pixels[0],pitch[0],pixels[1],pitch[1],all);
    const bool ok=window.Present565(pixels[0],static_cast<std::size_t>(pitch[0])*height,pitch[0]);
    for (auto id : aliases) UnLockVideoSurface(id);
    if (ok) { full=false; count=0; }
    return ok;
}
void RefreshScreen(void*) { (void)sgp::RefreshNativeVideo(); }
bool sgp::SaveNativeVideoWindow(const char* path) { return active && window.SaveWindowSurface(path); }
const char* sgp::NativeVideoDriver() { return active ? window.VideoDriver() : "uninitialized"; }
bool sgp::NativeVideoQuitRequested() { return !active || window.QuitRequested(); }
