/*
 * P710 native notice preparation: sgp/NativeSurface.cpp
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: implement native software-surface API subset.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include "native_surface.h"
#include "clip_state.h"
#include "himage.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

struct NativeSurface
{
    UINT32 id = 0, width = 0, height = 0, pitch = 0;
    UINT16 key = 0;
    bool alive = false, locked = false, reserved = false;
    std::vector<BYTE> bytes;
};
namespace
{
// Keep capability tombstones, not pixel allocations: allocator address reuse
// must never turn a stale public HVSURFACE into a valid new surface.
std::vector<std::unique_ptr<NativeSurface>> surfaces;
NativeSurface* aliases[3]{};
std::uint64_t nextId = 1;
bool initialized = false;
using I = std::int64_t;
struct Rect { I l, t, r, b; };
Rect rect(const SGPRect& r) { return {r.iLeft, r.iTop, r.iRight, r.iBottom}; }
bool ordered(Rect r) { return r.l < r.r && r.t < r.b; }
Rect intersect(Rect a, Rect b)
{ return {std::max(a.l,b.l),std::max(a.t,b.t),std::min(a.r,b.r),std::min(a.b,b.b)}; }
NativeSurface* valid(HVSURFACE p)
{
    if (initialized && p)
        for (auto& s : surfaces) if (s.get() == p) return s->alive ? p : nullptr;
    return nullptr;
}
NativeSurface* find(UINT32 id)
{
    if (!initialized || !id) return nullptr;
    if (id >= PRIMARY_SURFACE && id <= FRAME_BUFFER) return aliases[id-PRIMARY_SURFACE];
    if (id >= PRIMARY_SURFACE) return nullptr;
    for (auto& s : surfaces) if (s->id == id && s->alive) return s.get();
    return nullptr;
}
Rect bounds(const NativeSurface* s) { return {0,0,s->width,s->height}; }
UINT16 rgb(COLORVAL c)
{ return static_cast<UINT16>(((c & 0xf8u) << 8) | ((c & 0xfc00u) >> 5) | ((c >> 19) & 31u)); }
void put(NativeSurface* s, I x, I y, UINT16 color)
{ std::memcpy(s->bytes.data()+static_cast<std::size_t>(y)*s->pitch+static_cast<std::size_t>(x)*2,&color,2); }
BOOLEAN fill(NativeSurface* s, Rect r, UINT16 color, bool clipped)
{
    if (!s || s->locked || !ordered(r)) return FALSE;
    r = intersect(r, bounds(s));
    if (clipped) r = intersect(r, rect(ClippingRect));
    for (I y=r.t; y<r.b; ++y) for (I x=r.l; x<r.r; ++x) put(s,x,y,color);
    return TRUE;
}
BOOLEAN blit(NativeSurface* d, NativeSurface* s, UINT16 index, INT32 x, INT32 y, UINT32 flags, blt_vs_fx* fx)
{
    constexpr UINT32 allowed = VS_BLT_COLORFILL | VS_BLT_COLORFILLRECT | VS_BLT_USECOLORKEY | VS_BLT_SRCSUBRECT | VS_BLT_CLIPPED;
    if (!d || d->locked || index || (flags & ~allowed)) return FALSE;
    const UINT32 fills = flags & (VS_BLT_COLORFILL | VS_BLT_COLORFILLRECT);
    if (fills)
    {
        if (!fx || fx->DestRegion || fills == (VS_BLT_COLORFILL | VS_BLT_COLORFILLRECT) ||
            (flags & (VS_BLT_USECOLORKEY | VS_BLT_SRCSUBRECT))) return FALSE;
        return fill(d, fills == VS_BLT_COLORFILL ? bounds(d) : rect(fx->FillRect),
                    static_cast<UINT16>(fx->ColorFill), (flags & VS_BLT_CLIPPED) != 0);
    }
    if (!s || s->locked || (fx && fx->DestRegion)) return FALSE;
    Rect src = bounds(s);
    if (flags & VS_BLT_SRCSUBRECT)
    {
        if (!fx || !ordered(rect(fx->SrcRect))) return FALSE;
        src = rect(fx->SrcRect);
    }
    // Parameterize the copy in original source coordinates. Clipping negative
    // subrect origins must also move the destination, not stretch the image.
    const I dx = static_cast<I>(x)-src.l, dy = static_cast<I>(y)-src.t;
    Rect destClip = bounds(d);
    if (flags & VS_BLT_CLIPPED) destClip = intersect(destClip,rect(ClippingRect));
    Rect visible = intersect(intersect(src,bounds(s)),
        {destClip.l-dx,destClip.t-dy,destClip.r-dx,destClip.b-dy});
    if (!ordered(visible)) return TRUE;
    std::vector<BYTE> snapshot;
    const BYTE* source = s->bytes.data();
    if (s == d)
    {
        try { snapshot = s->bytes; } catch (...) { return FALSE; }
        source = snapshot.data();
    }
    for (I sy=visible.t; sy<visible.b; ++sy)
        for (I sx=visible.l; sx<visible.r; ++sx)
        {
            UINT16 color;
            std::memcpy(&color,source+static_cast<std::size_t>(sy)*s->pitch+static_cast<std::size_t>(sx)*2,2);
            if (!(flags & VS_BLT_USECOLORKEY) || color != s->key) put(d,sx+dx,sy+dy,color);
        }
    return TRUE;
}
}
BOOLEAN InitializeVideoSurfaceManager()
{ initialized = true; ConfigureRGB565ImageFormat(); return TRUE; }
BOOLEAN ShutdownVideoSurfaceManager()
{
    if (aliases[0]) return FALSE;
    for (auto& s : surfaces) if (s->alive && s->locked) return FALSE;
    for (auto& s : surfaces) { s->alive = false; std::vector<BYTE>().swap(s->bytes); }
    initialized = false;
    return TRUE;
}
HVSURFACE CreateVideoSurface(VSURFACE_DESC* desc)
{
    if (!initialized || !desc || desc->fCreateFlags != VSURFACE_CREATE_DEFAULT ||
        desc->ubBitDepth != 16 || !desc->usWidth || !desc->usHeight ||
        desc->usWidth > 65535 || desc->usHeight > 65535 || nextId >= PRIMARY_SURFACE) return nullptr;
    const std::uint64_t pitch = ((static_cast<std::uint64_t>(desc->usWidth)*2+7)&~std::uint64_t(7))+8;
    const std::uint64_t size = pitch*desc->usHeight;
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::ptrdiff_t>::max()) ||
        size > std::numeric_limits<std::size_t>::max()) return nullptr;
    try
    {
        auto s = std::make_unique<NativeSurface>();
        s->width=desc->usWidth; s->height=desc->usHeight; s->pitch=static_cast<UINT32>(pitch);
        s->bytes.assign(static_cast<std::size_t>(size),0xa5);
        for (UINT32 y=0; y<s->height; ++y) std::memset(s->bytes.data()+static_cast<std::size_t>(y)*s->pitch,0,s->width*2);
        auto* result=s.get();
        surfaces.push_back(std::move(s));
        result->id=static_cast<UINT32>(nextId++); result->alive=true;
        return result;
    }
    catch (...) { return nullptr; }
}
BOOLEAN AddStandardVideoSurface(VSURFACE_DESC* d, UINT32* id)
{
    if (!id) return FALSE;
    *id=0;
    auto* s=CreateVideoSurface(d);
    if (!s) return FALSE;
    *id=s->id;
    return TRUE;
}
BOOLEAN GetVideoSurface(HVSURFACE* out, UINT32 id)
{
    if (!out) return FALSE;
    *out=find(id);
    return *out ? TRUE : FALSE;
}
BOOLEAN DeleteVideoSurface(HVSURFACE p)
{
    auto* s=valid(p);
    if (!s || s->locked || s->reserved) return FALSE;
    s->alive=false; std::vector<BYTE>().swap(s->bytes);
    return TRUE;
}
BOOLEAN DeleteVideoSurfaceFromIndex(UINT32 id) { return DeleteVideoSurface(find(id)); }
BYTE* LockVideoSurfaceBuffer(HVSURFACE p, UINT32* pitch)
{
    if (pitch) *pitch=0;
    auto* s=valid(p);
    if (!pitch || !s || s->locked) return nullptr;
    *pitch=s->pitch; s->locked=true;
    return s->bytes.data();
}
BYTE* LockVideoSurface(UINT32 id, UINT32* pitch) { return LockVideoSurfaceBuffer(find(id),pitch); }
void UnLockVideoSurfaceBuffer(HVSURFACE p) { if (auto* s=valid(p)) s->locked=false; }
void UnLockVideoSurface(UINT32 id) { UnLockVideoSurfaceBuffer(find(id)); }
BOOLEAN GetVideoSurfaceDescription(UINT32 id, UINT16* w, UINT16* h, UINT8* bpp)
{
    auto* s=find(id);
    if (!s || !w || !h || !bpp) return FALSE;
    *w=static_cast<UINT16>(s->width); *h=static_cast<UINT16>(s->height); *bpp=16;
    return TRUE;
}
BOOLEAN SetVideoSurfaceTransparencyColor(HVSURFACE p, COLORVAL c)
{
    auto* s=valid(p);
    if (!s || s->locked) return FALSE;
    s->key=rgb(c); return TRUE;
}
BOOLEAN SetVideoSurfaceTransparency(UINT32 id, COLORVAL c)
{ return SetVideoSurfaceTransparencyColor(find(id),c); }
BOOLEAN ColorFillVideoSurfaceArea(UINT32 id, INT32 l, INT32 t, INT32 r, INT32 b, UINT16 c)
{ return fill(find(id),{l,t,r,b},c,true); }
BOOLEAN BltVideoSurface(UINT32 d, UINT32 s, UINT16 i, INT32 x, INT32 y, UINT32 f, blt_vs_fx* fx)
{ return blit(find(d),find(s),i,x,y,f,fx); }
BOOLEAN BltVideoSurfaceToVideoSurface(HVSURFACE d, HVSURFACE s, UINT16 i, INT32 x, INT32 y, INT32 f, blt_vs_fx* fx)
{ return blit(valid(d),valid(s),i,x,y,static_cast<UINT32>(f),fx); }

namespace sgp
{
bool BindReservedVideoSurfaces(const UINT32 (&ids)[3])
{
    if (aliases[0]) return false;
    NativeSurface* candidates[3]{};
    for (std::size_t i=0; i<3; ++i)
    {
        if (ids[i]>=PRIMARY_SURFACE) return false;
        auto* s=find(ids[i]);
        if (!s || s->locked || s->reserved) return false;
        if (i && (s->width!=candidates[0]->width || s->height!=candidates[0]->height)) return false;
        for (std::size_t j=0; j<i; ++j) if (s==candidates[j]) return false;
        candidates[i]=s;
    }
    for (std::size_t i=0; i<3; ++i) { aliases[i]=candidates[i]; aliases[i]->reserved=true; }
    return true;
}
bool ReservedVideoSurfacesReady()
{
    for (auto* s : aliases) if (!s || !s->alive || s->locked) return false;
    return true;
}
bool ReleaseReservedVideoSurfaces()
{
    if (!aliases[0]) return true;
    if (!ReservedVideoSurfacesReady()) return false;
    for (auto& s : aliases)
    {
        auto* owned=s; s=nullptr; owned->reserved=false;
        DeleteVideoSurface(owned);
    }
    return true;
}
UINT32 ReservedVideoSurfaceBackingId(UINT32 alias)
{
    if (alias<PRIMARY_SURFACE || alias>FRAME_BUFFER) return 0;
    auto* s=aliases[alias-PRIMARY_SURFACE]; return s ? s->id : 0;
}
}
