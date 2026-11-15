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

/*
 * P710 native snapshot update: sgp/NativeSurface.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_surface.h"
#include "clip_state.h"
#include "himage.h"
#include "shading.h"
#include "vobject.h"
#include "etrle_blitters.h"
#include <array>
#include <zlib.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

HVSURFACE ghFrameBuffer=nullptr; // borrowed actual reserved frame identity

struct NativeSurface
{
    UINT32 id = 0, width = 0, height = 0, pitch = 0;
    UINT16 key = 0;
    UINT8 depth = 16;
    bool alive = false, locked = false, reserved = false;
    std::vector<BYTE> bytes;
    std::unique_ptr<std::array<SGPPaletteEntry, 256>> palette;
    std::unique_ptr<std::array<UINT16, 256>> colors;
};
namespace
{
// Keep capability tombstones, not pixel allocations: allocator address reuse
// must never turn a stale public HVSURFACE into a valid new surface.
std::vector<std::unique_ptr<NativeSurface>> surfaces;
NativeSurface* aliases[4]{};
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
    if (id >= PRIMARY_SURFACE && id <= MOUSE_BUFFER) return aliases[id-PRIMARY_SURFACE];
    if (id >= PRIMARY_SURFACE) return nullptr;
    for (auto& s : surfaces) if (s->id == id && s->alive) return s.get();
    return nullptr;
}
Rect bounds(const NativeSurface* s) { return {0,0,s->width,s->height}; }
UINT16 rgb(COLORVAL c)
{ return static_cast<UINT16>(((c & 0xf8u) << 8) | ((c & 0xfc00u) >> 5) | ((c >> 19) & 31u)); }
void put(NativeSurface* s, I x, I y, UINT16 color)
{
    auto* pixel=s->bytes.data()+static_cast<std::size_t>(y)*s->pitch+static_cast<std::size_t>(x)*(s->depth/8);
    if (s->depth==8) *pixel=static_cast<BYTE>(color);
    else std::memcpy(pixel,&color,2);
}
BOOLEAN fill(NativeSurface* s, Rect r, UINT16 color, bool clipped)
{
    if (!s || s->locked || !ordered(r) || (s->depth==8 && color>255)) return FALSE;
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
        if (!fx || fills == (VS_BLT_COLORFILL | VS_BLT_COLORFILLRECT) ||
            (flags & (VS_BLT_USECOLORKEY | VS_BLT_SRCSUBRECT))) return FALSE;
        // Validate the original COLORVAL before RGB565's legacy narrowing.
        // Otherwise high-word index values such as0x10000 become index zero.
        if (d->depth==8 && fx->ColorFill>255) return FALSE;
        return fill(d, fills == VS_BLT_COLORFILL ? bounds(d) : rect(fx->FillRect),
                    static_cast<UINT16>(fx->ColorFill), (flags & VS_BLT_CLIPPED) != 0);
    }
    // Effects fields are selected by flags. Original callers can initialize
    // SrcRect alone; VS_BLT_DESTREGION remains rejected by the flag mask above.
    if (!s || s->locked ||
        (d->depth==8 && s->depth!=8) || (d->depth==16 && s->depth==8 && !s->colors)) return FALSE;
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
            if (s->depth==8) color=source[static_cast<std::size_t>(sy)*s->pitch+static_cast<std::size_t>(sx)];
            else std::memcpy(&color,source+static_cast<std::size_t>(sy)*s->pitch+static_cast<std::size_t>(sx)*2,2);
            if (!(flags & VS_BLT_USECOLORKEY) || color != s->key)
                put(d,sx+dx,sy+dy,s->depth==8 && d->depth==16 ? (*s->colors)[color] : color);
        }
    return TRUE;
}
// Native port, 2026-11-12. Keep the resource loader's ownership local until
// every pixel validates and converts; failed creations never publish a handle.
bool imagePixels(NativeSurface& surface, HIMAGE image)
{
    if (!image || !(image->fFlags & IMAGE_BITMAPDATA) || !image->pImageData ||
        !image->usWidth || !image->usHeight) return false;
    const std::size_t count = std::size_t(image->usWidth) * image->usHeight;
    const unsigned depth = image->ubBitDepth;
    if ((depth != 8 && depth != 16 && depth != 24 && depth != 32) ||
        (surface.depth==8 && depth!=8)) return false;
    if (depth == 8)
    {
        if (!(image->fFlags & IMAGE_PALETTE) || !image->pPalette) return false;
        surface.palette=std::make_unique<std::array<SGPPaletteEntry, 256>>();
        surface.colors=std::make_unique<std::array<UINT16, 256>>();
        std::copy_n(image->pPalette, 256, surface.palette->begin());
        // Use the production palette helper, including BLACK_SUBSTITUTE.
        auto* converted = Create16BPPPalette(image->pPalette);
        if (!converted) return false;
        std::copy_n(converted, 256, surface.colors->begin());
        MemFree(converted);
    }
    const auto* data = static_cast<const BYTE*>(image->pImageData);
    std::size_t length = image->uiSizePixData;
    std::vector<BYTE> expanded;
    if (image->fFlags & IMAGE_COMPRESSED)
    {
        if (image->fFlags & IMAGE_TRLECOMPRESSED) return false;
        const std::size_t required = count * (depth / 8);
        expanded.resize(required);
        uLongf actual = static_cast<uLongf>(required);
        if (uncompress(expanded.data(), &actual, data, static_cast<uLong>(length)) != Z_OK || actual != required)
            return false;
        data = expanded.data(); length = required;
    }
    if (image->fFlags & IMAGE_TRLECOMPRESSED)
    {
        if (depth != 8 || image->usNumberOfObjects != 1 || !image->pETRLEObject) return false;
        const auto& frame = image->pETRLEObject[0];
        if (frame.usWidth != image->usWidth || frame.usHeight != image->usHeight ||
            frame.uiDataOffset > length || frame.uiDataLength > length - frame.uiDataOffset) return false;
        const BYTE* cursor = data + frame.uiDataOffset;
        const BYTE* end = cursor + frame.uiDataLength;
        for (UINT32 y=0; y<surface.height; ++y)
        {
            UINT32 x=0;
            bool ended=false;
            while (cursor<end)
            {
                const BYTE control=*cursor++;
                if (!control) { ended=true; break; }
                const UINT32 run=control & 127u;
                if (run>surface.width-x) return false;
                if (control & 128u)
                    // ETRLE skip runs are transparency, independently of the
                    // color stored in palette entry zero. Preserve the new
                    // surface's zero/key pixels rather than painting palette[0].
                    x+=run;
                else
                {
                    if (static_cast<std::size_t>(end-cursor)<run) return false;
                    for (UINT32 i=0; i<run; ++i) { const BYTE index=*cursor++; put(&surface,x++,y,surface.depth==8 ? index : (*surface.colors)[index]); }
                }
            }
            if (!ended || x != surface.width) return false;
        }
        return cursor == end;
    }
    std::size_t offset=0;
    // PNG true-color data has one bounded descriptor; a JPC collection is not
    // one surface and must not silently expose only the first object.
    if (image->pETRLEObject || image->usNumberOfObjects)
    {
        if (!image->pETRLEObject || image->usNumberOfObjects != 1) return false;
        const auto& frame=image->pETRLEObject[0];
        if (frame.usWidth != image->usWidth || frame.usHeight != image->usHeight ||
            frame.uiDataOffset > length || frame.uiDataLength > length-frame.uiDataOffset ||
            count*(depth/8) > frame.uiDataLength) return false;
        offset=frame.uiDataOffset;
    }
    const unsigned pixelBytes=depth/8;
    if (offset > length || count*pixelBytes > length-offset) return false;
    data += offset;
    for (UINT32 y=0; y<surface.height; ++y)
        for (UINT32 x=0; x<surface.width; ++x)
        {
            const BYTE* pixel=data+(std::size_t(y)*surface.width+x)*pixelBytes;
            UINT16 color=0;
            if (depth==8) color=surface.depth==8 ? *pixel : (*surface.colors)[*pixel];
            else if (depth==16)
            {
                std::memcpy(&color,pixel,2);
                // The production TGA reader preserves 5:5:5 samples. Expand
                // green's fifth bit into six bits for this RGB565 surface.
                if (image->iFileLoader==TGA_FILE_READER)
                    color=static_cast<UINT16>(((color&0x7c00u)<<1)|((color&0x03e0u)<<1)|((color&0x0200u)>>4)|(color&31u));
            }
            else if (depth==24 && image->iFileLoader==STCI_FILE_READER)
                color=Get16BPPColor(FROMRGB(pixel[2],pixel[1],pixel[0]));
            else if (depth==32 && image->iFileLoader==STCI_FILE_READER)
            {
                UINT32 value; std::memcpy(&value,pixel,4);
                color=Get16BPPColor(FROMRGB((value>>16)&255u,(value>>8)&255u,value&255u));
            }
            else
            {
                // PNG/JPC retain RGBA byte order. Original himage.cpp routes
                // 32bpp through Blt32BPPTo16BPPTrans: alpha zero skips the pixel;
                // its cache truncates (double(alpha)/255.0)*channel. A newly
                // created native surface is black, so the destination terms
                // are zero. Preserve that exact cache arithmetic over black.
                if (depth==32)
                {
                    const UINT8 alpha=pixel[3];
                    if (!alpha) continue;
                    const auto scaled=[alpha](UINT8 channel) {
                        return static_cast<UINT8>((static_cast<double>(alpha)/255.0)*channel);
                    };
                    color=Get16BPPColor(FROMRGB(scaled(pixel[0]),scaled(pixel[1]),scaled(pixel[2])));
                }
                else color=Get16BPPColor(FROMRGB(pixel[0],pixel[1],pixel[2]));
            }
            put(&surface,x,y,color);
        }
    return true;
}

}
BOOLEAN InitializeVideoSurfaceManager()
{ initialized = true; ConfigureRGB565ImageFormat(); return TRUE; }
BOOLEAN ShutdownVideoSurfaceManager()
{
    if (aliases[0]) return FALSE;
    for (auto& s : surfaces) if (s->alive && s->locked) return FALSE;
    for (auto& s : surfaces)
    {
        s->alive = false; std::vector<BYTE>().swap(s->bytes);
        s->palette.reset(); s->colors.reset();
    }
    initialized = false; ghFrameBuffer=nullptr;
    return TRUE;
}
HVSURFACE CreateVideoSurface(VSURFACE_DESC* desc)
{
    if (!initialized || !desc || nextId >= PRIMARY_SURFACE) return nullptr;
    constexpr UINT32 memory = VSURFACE_DEFAULT_MEM_USAGE | VSURFACE_SYSTEM_MEM_USAGE;
    constexpr UINT32 choices = VSURFACE_CREATE_FROMJPC | VSURFACE_CREATE_FROMJPC_FALLBACK |
        VSURFACE_CREATE_FROMPNG | VSURFACE_CREATE_FROMPNG_FALLBACK;
    const UINT32 flags=desc->fCreateFlags;
    const bool fromFile=(flags & VSURFACE_CREATE_FROMFILE)!=0;
    const UINT32 mode=flags & (VSURFACE_CREATE_DEFAULT | VSURFACE_CREATE_FROMFILE);
    const UINT32 choice=flags & choices;
    if ((flags & ~(memory|choices|VSURFACE_NATIVE_INDEXED8|VSURFACE_CREATE_DEFAULT|VSURFACE_CREATE_FROMFILE)) ||
        mode != (fromFile ? VSURFACE_CREATE_FROMFILE : VSURFACE_CREATE_DEFAULT) ||
        (choice && ((choice & (choice-1)) || !fromFile)) ||
        ((flags & memory)==memory) || ((flags & VSURFACE_NATIVE_INDEXED8) && !fromFile)) return nullptr;
    try
    {
        std::unique_ptr<image_type, decltype(&DestroyImage)> image(nullptr,&DestroyImage);
        UINT32 width=0, height=0;
        UINT8 depth=16;
        if (fromFile)
        {
            ImageFileType::TestOrder order=ImageFileType::JPC_FALLBACK;
            if (choice==VSURFACE_CREATE_FROMJPC) order=ImageFileType::JPC;
            if (choice==VSURFACE_CREATE_FROMJPC_FALLBACK) order=ImageFileType::JPC_FALLBACK;
            if (choice==VSURFACE_CREATE_FROMPNG) order=ImageFileType::PNG;
            if (choice==VSURFACE_CREATE_FROMPNG_FALLBACK) order=ImageFileType::PNG_FALLBACK;
            image.reset(CreateImage(desc->ImageFile,IMAGE_ALLIMAGEDATA,order));
            if (!image) return nullptr;
            width=image->usWidth; height=image->usHeight;
            if (flags & VSURFACE_NATIVE_INDEXED8)
            { if (image->ubBitDepth!=8) return nullptr; depth=8; }
        }
        else
        {
            width=desc->usWidth; height=desc->usHeight; depth=desc->ubBitDepth;
            if (depth!=8 && depth!=16) return nullptr;
        }
        if (!width || !height || width>65535 || height>65535) return nullptr;
        const std::uint64_t pitch=((std::uint64_t(width)*(depth/8)+7)&~std::uint64_t(7))+8;
        const std::uint64_t size=pitch*height;
        if (size>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max()) ||
            size>std::numeric_limits<std::size_t>::max()) return nullptr;
        auto s=std::make_unique<NativeSurface>();
        s->width=width; s->height=height; s->pitch=static_cast<UINT32>(pitch); s->depth=depth;
        s->bytes.assign(static_cast<std::size_t>(size),0xa5);
        for (UINT32 y=0; y<height; ++y)
            std::memset(s->bytes.data()+std::size_t(y)*s->pitch,0,width*(depth/8));
        if (fromFile && !imagePixels(*s,image.get())) return nullptr;
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
    s->palette.reset(); s->colors.reset();
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
    *w=static_cast<UINT16>(s->width); *h=static_cast<UINT16>(s->height); *bpp=s->depth;
    return TRUE;
}
UINT16 GetVideoSurfaceWidth(HVSURFACE p)
{ auto* s=valid(p); return s && !s->locked ? static_cast<UINT16>(s->width) : 0; }
UINT16 GetVideoSurfaceHeight(HVSURFACE p)
{ auto* s=valid(p); return s && !s->locked ? static_cast<UINT16>(s->height) : 0; }
BOOLEAN SetVideoSurfacePalette(HVSURFACE p, SGPPaletteEntry* entries)
{
    auto* s=valid(p);
    if (!s || s->locked || !entries) return FALSE;
    try
    {
        auto palette=std::make_unique<std::array<SGPPaletteEntry,256>>();
        auto colors=std::make_unique<std::array<UINT16,256>>();
        std::copy_n(entries,256,palette->begin());
        auto* converted=Create16BPPPalette(palette->data());
        if (!converted) return FALSE;
        std::copy_n(converted,256,colors->begin()); MemFree(converted);
        s->palette=std::move(palette); s->colors=std::move(colors);
        return TRUE;
    }
    catch (...) { return FALSE; }
}
BOOLEAN GetVSurfacePaletteEntries(HVSURFACE p, SGPPaletteEntry* entries)
{
    auto* s=valid(p);
    if (!s || s->locked || !entries || !s->palette) return FALSE;
    std::copy_n(s->palette->begin(),256,entries); return TRUE;
}
BOOLEAN GetVideoSurfacePalette16(HVSURFACE p, UINT16* colors)
{
    auto* s=valid(p);
    if (!s || s->locked || !colors || !s->colors) return FALSE;
    std::copy_n(s->colors->begin(),256,colors); return TRUE;
}
BOOLEAN SetVideoSurfacePalette16(HVSURFACE p, const UINT16* colors)
{
    auto* s=valid(p);
    if (!s || s->depth!=8 || !colors || !s->colors) return FALSE;
    // Map shading selects a palette while its own indexed bytes are locked.
    // Copying into the owned table retains no caller pointer and frees nothing.
    std::copy_n(colors,256,s->colors->begin()); return TRUE;
}
BOOLEAN GetPrimaryRGBDistributionMasks(UINT32* red,UINT32* green,UINT32* blue)
{
    auto* primary=find(PRIMARY_SURFACE);
    if (!primary || primary->depth!=16 || !red || !green || !blue) return FALSE;
    // All native sixteen-bit surfaces actually store RGB565, including primary.
    *red=0xf800;*green=0x07e0;*blue=0x001f;return TRUE;
}
BOOLEAN Set8BPPPalette(SGPPaletteEntry*)
{
    // Native display modes are RGB565 only. Indexed offscreen surfaces have
    // genuine per-surface palettes through SetVideoSurfacePalette instead.
    return FALSE;
}
static BOOLEAN bltIndexed(UINT16*,UINT32,HVSURFACE,UINT8*,UINT32,INT32,INT32,SGPRect*,unsigned);
BOOLEAN Blt8BPPDataTo16BPPBuffer(UINT16* destination,UINT32 pitch,HVSURFACE source,UINT8* pixels,INT32 x,INT32 y)
{
    auto* s=valid(source);if (!s) return FALSE;
    SGPRect region{0,0,static_cast<INT32>(s->width),static_cast<INT32>(s->height)};
    return bltIndexed(destination,pitch,source,pixels,s->pitch,x,y,&region,1);
}
BOOLEAN Blt8BPPDataSubTo16BPPBuffer(UINT16* destination,UINT32 pitch,HVSURFACE source,UINT8* pixels,
    UINT32 sourcePitch,INT32 x,INT32 y,SGPRect* region)
{ return bltIndexed(destination,pitch,source,pixels,sourcePitch,x,y,region,1); }
BOOLEAN Blt8BPPDataTo16BPPBufferHalf(UINT16* destination, UINT32 pitch,
    HVSURFACE source, UINT8* pixels, UINT32 sourcePitch, INT32 x, INT32 y)
{
    auto* s=valid(source);
    if (!s) return FALSE;
    SGPRect region{0,0,static_cast<INT32>(s->width),static_cast<INT32>(s->height)};
    return Blt8BPPDataTo16BPPBufferHalfRect(destination,pitch,source,pixels,sourcePitch,x,y,&region);
}
BOOLEAN Blt8BPPDataTo16BPPBufferHalfRect(UINT16* destination, UINT32 pitch,
    HVSURFACE source, UINT8* pixels, UINT32 sourcePitch, INT32 x, INT32 y, SGPRect* region)
{ return bltIndexed(destination,pitch,source,pixels,sourcePitch,x,y,region,2); }
static BOOLEAN bltIndexed(UINT16* destination,UINT32 pitch,HVSURFACE source,UINT8* pixels,
    UINT32 sourcePitch,INT32 x,INT32 y,SGPRect* region,unsigned step)
{
    auto* s=valid(source);
    if (!s || s->depth!=8 || !s->locked || !s->colors || !destination || !region ||
        pixels!=s->bytes.data() || sourcePitch!=s->pitch || x<0 || y<0 || (pitch&1u)) return FALSE;
    const Rect r=rect(*region);
    if (!ordered(r) || r.l<0 || r.t<0 || r.r>s->width || r.b>s->height) return FALSE;
    const I width=(r.r-r.l)/step, height=(r.b-r.t)/step;
    // Unlike the assembly's zero-count loops, a sub-two-pixel region is a
    // valid empty copy. No row, sample or padding is read or written.
    if (!width || !height) return TRUE;
    if (std::uint64_t(x)+std::uint64_t(width)>pitch/2) return FALSE;
    const auto end=(std::uint64_t(y)+height-1)*pitch+(std::uint64_t(x)+width)*2;
    if (end>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max())) return FALSE;
    // Raw-buffer callers keep the legacy extent responsibility. If this is a
    // managed allocation, enforce its lock/depth/width/height as well.
    const auto destAddress=reinterpret_cast<std::uintptr_t>(destination);
    for (const auto& candidate : surfaces)
    {
        if (!candidate->alive || candidate->bytes.empty()) continue;
        const auto start=reinterpret_cast<std::uintptr_t>(candidate->bytes.data());
        if (destAddress>=start && destAddress-start<candidate->bytes.size())
        {
            if (destAddress!=start || candidate->depth!=16 || !candidate->locked ||
                pitch!=candidate->pitch || I(x)+width>candidate->width || I(y)+height>candidate->height)
                return FALSE;
        }
    }
    auto* out=reinterpret_cast<BYTE*>(destination);
    for (I row=0; row<height; ++row)
        for (I col=0; col<width; ++col)
        {
            const BYTE index=pixels[std::size_t(r.t+row*step)*sourcePitch+std::size_t(r.l+col*step)];
            const UINT16 color=(*s->colors)[index];
            std::memcpy(out+std::size_t(y+row)*pitch+std::size_t(x+col)*2,&color,2);
        }
    return TRUE;
}

BOOLEAN SetVideoSurfaceTransparencyColor(HVSURFACE p, COLORVAL c)
{
    auto* s=valid(p);
    if (!s || s->locked) return FALSE;
    if (s->depth==8 && c>255) return FALSE;
    s->key=s->depth==8 ? static_cast<UINT16>(c) : rgb(c); return TRUE;
}
BOOLEAN SetVideoSurfaceTransparency(UINT32 id, COLORVAL c)
{ return SetVideoSurfaceTransparencyColor(find(id),c); }
BOOLEAN ColorFillVideoSurfaceArea(UINT32 id, INT32 l, INT32 t, INT32 r, INT32 b, UINT16 c)
{ return fill(find(id),{l,t,r,b},c,true); }
BOOLEAN BltVideoSurface(UINT32 d, UINT32 s, UINT16 i, INT32 x, INT32 y, UINT32 f, blt_vs_fx* fx)
{ return blit(find(d),find(s),i,x,y,f,fx); }
BOOLEAN BltVideoSurfaceToVideoSurface(HVSURFACE d, HVSURFACE s, UINT16 i, INT32 x, INT32 y, INT32 f, blt_vs_fx* fx)
{ return blit(valid(d),valid(s),i,x,y,static_cast<UINT32>(f),fx); }

BOOLEAN BltVSurfaceUsingDD(HVSURFACE destination, HVSURFACE source, UINT32 flags,
                          INT32 x, INT32 y, SGPRect* sourceRect)
{
    if (flags & ~UINT32(VS_BLT_USECOLORKEY)) return FALSE;
    blt_vs_fx effects{};
    if (sourceRect) { effects.SrcRect=*sourceRect; flags|=VS_BLT_SRCSUBRECT; }
    return BltVideoSurfaceToVideoSurface(destination,source,0,x,y,
        static_cast<INT32>(flags),sourceRect ? &effects : nullptr);
}

namespace {
BOOLEAN objectBlit(UINT32 destination, HVOBJECT source, UINT16 index, INT32 x, INT32 y,
                   bool outline, INT16 color, BOOLEAN drawOutline)
{
    auto* surface=find(destination);
    if (!surface || surface->locked || surface->depth!=16 || ClippingRect.iLeft>ClippingRect.iRight ||
        ClippingRect.iTop>ClippingRect.iBottom) return FALSE;
    SGPRect clip{
        std::clamp<INT32>(ClippingRect.iLeft,0,static_cast<INT32>(surface->width)),
        std::clamp<INT32>(ClippingRect.iTop,0,static_cast<INT32>(surface->height)),
        std::clamp<INT32>(ClippingRect.iRight,0,static_cast<INT32>(surface->width)),
        std::clamp<INT32>(ClippingRect.iBottom,0,static_cast<INT32>(surface->height))};
    // Reuse the validated kernels with a local surface-bound clip. No public
    // lock is acquired or released, so an existing caller lock stays intact.
    auto* pixels=reinterpret_cast<UINT16*>(surface->bytes.data());
    return outline ? Blt8BPPDataTo16BPPBufferOutlineClip(pixels,surface->pitch,source,
        x,y,index,color,drawOutline,&clip) :
        Blt8BPPDataTo16BPPBufferTransparentClip(pixels,surface->pitch,source,x,y,index,&clip);
}
}
BOOLEAN BltVideoObject(UINT32 destination, HVOBJECT source, UINT16 index,
                       INT32 x, INT32 y, UINT32 flags, blt_fx*)
{
    // Other sprite families remain explicit port work; never report a
    // successful empty draw for an unsupported operation.
    if (!(flags&VO_BLT_SRCTRANSPARENCY) || (flags&~UINT32(VO_BLT_SRCTRANSPARENCY|VO_BLT_CLIP))) return FALSE;
    return objectBlit(destination,source,index,x,y,false,0,FALSE);
}
BOOLEAN BltVideoObjectFromIndex(UINT32 destination, UINT32 object, UINT16 index,
                                INT32 x, INT32 y, UINT32 flags, blt_fx* effects)
{
    HVOBJECT source=nullptr;
    return GetVideoObject(&source,object) && BltVideoObject(destination,source,index,x,y,flags,effects);
}
BOOLEAN BltVideoObjectOutline(UINT32 destination, HVOBJECT source, UINT16 index,
                            INT32 x, INT32 y, INT16 color, BOOLEAN drawOutline)
{ return objectBlit(destination,source,index,x,y,true,color,drawOutline); }
BOOLEAN BltVideoObjectOutlineFromIndex(UINT32 destination, UINT32 object, UINT16 index,
                                     INT32 x, INT32 y, INT16 color, BOOLEAN drawOutline)
{
    HVOBJECT source=nullptr;
    return GetVideoObject(&source,object) && BltVideoObjectOutline(destination,source,index,x,y,color,drawOutline);
}

BOOLEAN BltStretchVideoSurface(UINT32 dest, UINT32 source, INT32, INT32, UINT32 flags,
                              SGPRect* sourceRect, SGPRect* destRect)
{
    auto* d=find(dest); auto* s=find(source);
    if (!d || !s || d->locked || s->locked || !sourceRect || !destRect ||
        (flags & ~(VS_BLT_USECOLORKEY|VS_BLT_CLIPPED)) ||
        (d->depth==8 && s->depth!=8) || (d->depth==16 && s->depth==8 && !s->colors)) return FALSE;
    const Rect src=rect(*sourceRect), dst=rect(*destRect);
    if (!ordered(src) || !ordered(dst)) return FALSE;
    Rect visible=intersect(dst,bounds(d));
    if (flags & VS_BLT_CLIPPED) visible=intersect(visible,rect(ClippingRect));
    if (!ordered(visible)) return TRUE;
    std::vector<BYTE> snapshot;
    const BYTE* bytes=s->bytes.data();
    if (s==d)
    {
        try { snapshot=s->bytes; } catch (...) { return FALSE; }
        bytes=snapshot.data();
    }
    // Coordinates can span all INT32 values. Unsigned multiplication prevents
    // signed overflow: each factor is <= UINT32_MAX, so UINT64 is sufficient.
    for (I y=visible.t; y<visible.b; ++y)
    {
        const I sy=src.t+static_cast<I>(std::uint64_t(y-dst.t)*std::uint64_t(src.b-src.t)/std::uint64_t(dst.b-dst.t));
        if (sy<0 || sy>=s->height) continue;
        for (I x=visible.l; x<visible.r; ++x)
        {
            const I sx=src.l+static_cast<I>(std::uint64_t(x-dst.l)*std::uint64_t(src.r-src.l)/std::uint64_t(dst.r-dst.l));
            if (sx<0 || sx>=s->width) continue;
            UINT16 color;
            if (s->depth==8) color=bytes[std::size_t(sy)*s->pitch+std::size_t(sx)];
            else std::memcpy(&color,bytes+std::size_t(sy)*s->pitch+std::size_t(sx)*2,2);
            if (!(flags & VS_BLT_USECOLORKEY) || color!=s->key)
                put(d,x,y,s->depth==8 && d->depth==16 ? (*s->colors)[color] : color);
        }
    }
    return TRUE;
}
namespace {
BOOLEAN shadowRect(UINT32 dest, INT32 x1, INT32 y1, INT32 x2, INT32 y2, const UINT16* table)
{
    auto* s=find(dest);
    if (!s || s->locked || s->depth!=16) return FALSE;
    // Original InternalShadowVideoSurfaceRect clamps inclusive endpoints and
    // rejects spans <=0 before either source table blitter clips globally.
    const I l=std::max<I>(x1,0), t=std::max<I>(y1,0);
    const I r=std::min<I>(x2,I(s->width)-1), b=std::min<I>(y2,I(s->height)-1);
    if (r<=l || b<=t) return FALSE;
    const Rect visible=intersect({l,t,r+1,b+1},rect(ClippingRect));
    if (!ordered(visible)) return TRUE;
    for (I y=visible.t; y<visible.b; ++y)
        for (I x=visible.l; x<visible.r; ++x)
        {
            UINT16 color;
            std::memcpy(&color,s->bytes.data()+std::size_t(y)*s->pitch+std::size_t(x)*2,2);
            put(s,x,y,table[color]);
        }
    return TRUE;
}

}
BOOLEAN ShadowVideoSurfaceRect(UINT32 dest, INT32 x1, INT32 y1, INT32 x2, INT32 y2)
{ return shadowRect(dest,x1,y1,x2,y2,ShadeTable); }
BOOLEAN ShadowVideoSurfaceRectUsingLowPercentTable(UINT32 dest, INT32 x1, INT32 y1, INT32 x2, INT32 y2)
{ return shadowRect(dest,x1,y1,x2,y2,IntensityTable); }

BOOLEAN ShadowVideoSurfaceImage(UINT32 dest, HVOBJECT image, INT32 x, INT32 y)
{
    auto* s=find(dest);
    if (!s || s->locked || s->depth!=16 || !image || !image->pETRLEObject || !image->usNumberOfObjects) return FALSE;
    const auto& first=image->pETRLEObject[0];
    if (!first.usWidth || !first.usHeight) return FALSE;
    const I right=I(x)+first.usWidth, bottom=I(y)+first.usHeight;
    // The original uses INT32 additions; reject their undefined overflow before
    // either strip writes. Width/height are positive, so only upper overflow is possible.
    if (I(x)+3>std::numeric_limits<INT32>::max() || I(y)+3>std::numeric_limits<INT32>::max() ||
        right+3>std::numeric_limits<INT32>::max() || bottom+3>std::numeric_limits<INT32>::max()) return FALSE;
    // vsurface.cpp2588: bottom first, then right. Both inclusive strips include
    // (right,bottom), whose pixel must pass through ShadeTable twice. The source
    // helper succeeds even when clipping removes a strip; retain that convention.
    ShadowVideoSurfaceRect(dest,x+3,static_cast<INT32>(bottom),static_cast<INT32>(right),static_cast<INT32>(bottom+3));
    ShadowVideoSurfaceRect(dest,static_cast<INT32>(right),y+3,static_cast<INT32>(right+3),static_cast<INT32>(bottom));
    return TRUE;
}

namespace sgp
{
bool BindReservedVideoSurfaces(const UINT32 (&ids)[4])
{
    if (aliases[0]) return false;
    NativeSurface* candidates[4]{};
    for (std::size_t i=0; i<4; ++i)
    {
        if (ids[i]>=PRIMARY_SURFACE) return false;
        auto* s=find(ids[i]);
        if (!s || s->locked || s->reserved || s->depth!=16) return false;
        if (i && i<3 && (s->width!=candidates[0]->width || s->height!=candidates[0]->height)) return false;
        for (std::size_t j=0; j<i; ++j) if (s==candidates[j]) return false;
        candidates[i]=s;
    }
    for (std::size_t i=0; i<4; ++i) { aliases[i]=candidates[i]; aliases[i]->reserved=true; }
    ghFrameBuffer=candidates[2];
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
    ghFrameBuffer=nullptr;
    for (auto& s : aliases)
    {
        auto* owned=s; s=nullptr; owned->reserved=false;
        DeleteVideoSurface(owned);
    }
    return true;
}
UINT32 ReservedVideoSurfaceBackingId(UINT32 alias)
{
    if (alias<PRIMARY_SURFACE || alias>MOUSE_BUFFER) return 0;
    auto* s=aliases[alias-PRIMARY_SURFACE]; return s ? s->id : 0;
}
}

namespace sgp {
bool GetNativeSurfacePitch(HVSURFACE surface, UINT32* pitch)
{
    const auto* s=valid(surface);
    if (!s || !pitch) return false;
    *pitch=s->pitch;
    return true;
}
bool LockedNativeSurfaceView(void* pixels, NativeSurfaceView* output)
{
    if (!initialized || !pixels || !output) return false;
    for (const auto& entry : surfaces) {
        auto& s = *entry;
        if (s.alive && s.locked && s.depth == 16 && s.bytes.data() == pixels) {
            *output = {s.bytes.data(), s.bytes.size(), s.width, s.height, s.pitch};
            return true;
        }
    }
    return false;
}
}
