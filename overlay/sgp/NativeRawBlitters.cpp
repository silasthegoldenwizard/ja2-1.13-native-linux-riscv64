/*
 * P710 native snapshot update: sgp/NativeRawBlitters.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeRawBlitters.h"
#include "vobject_blitters.h"
#include "native_background.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

// Native translations of the original vobject_blitters.cpp scalar/rectangle
// interfaces. The Windows translation unit remains byte-for-byte retained.
UINT32 guiTranslucentMask=0x3def;
INT32 gLeftSkip=0, gRightSkip=0, gTopSkip=0, gBottomSkip=0;
BOOLEAN gfUsePreCalcSkips=FALSE;

unsigned short blendWithAlpha(unsigned int fresh, unsigned int old, unsigned int alpha)
{
    const unsigned narrowed=static_cast<UINT16>(alpha);
    // FILD reads a signed sixteen-bit memory operand despite the C++ unsigned
    // declaration. Ordinary alpha callers are bytes; retain the wider API quirk.
    const int signedAlpha=narrowed<32768?int(narrowed):int(narrowed)-65536;
    const long double ratio=signedAlpha/255.0L;
    // x87 FST leaves its extended ratio on the stack, but the new-color term
    // reloads the single-precision memory copy. Preserve that asymmetry.
    const volatile float stored=static_cast<float>(ratio);
    const long double newRatio=stored;
    const auto channel=[&](unsigned shift,unsigned mask) {
        const long double value=(1-ratio)*((old>>shift)&mask)+newRatio*((fresh>>shift)&mask);
        return static_cast<UINT16>(static_cast<INT32>(std::nearbyintl(value)));
    };
    return static_cast<UINT16>(channel(0,31)+static_cast<UINT16>(channel(5,63)<<5)+
                               static_cast<UINT16>(channel(11,31)<<11));
}

namespace {
using I=std::int64_t;
struct Span { std::uintptr_t begin=0,end=0; };
bool span(void* data,UINT32 pitch,UINT32 pixelBytes,I x,I y,UINT32 w,UINT32 h,Span& result)
{
    if (!data || !pixelBytes || pitch%pixelBytes ||
        reinterpret_cast<std::uintptr_t>(data)%pixelBytes || x<0 || y<0 ||
        std::uint64_t(x)+w>pitch/pixelBytes) return false;
    const auto offset=std::uint64_t(y)*pitch+std::uint64_t(x)*pixelBytes;
    const auto end=h ? (std::uint64_t(y)+h-1)*pitch+(std::uint64_t(x)+w)*pixelBytes : offset;
    if (end>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max())) return false;
    const auto base=reinterpret_cast<std::uintptr_t>(data);
    if (base>std::numeric_limits<std::uintptr_t>::max()-end) return false;
    sgp::BackgroundView owned;
    if (sgp::BackgroundBufferView(data,&owned)) {
        if (pixelBytes!=2 || pitch!=owned.pitch || std::uint64_t(x)+w>owned.width ||
            std::uint64_t(y)+h>owned.height || end>owned.bytes) return false;
    }
    result={base+offset,base+end};
    return true;
}
bool overlap(const Span& a,const Span& b) { return a.begin<b.end && b.begin<a.end; }
enum class CopyMode { Plain,Key,Shadow,Mirror,Alpha,AlphaShadow };
bool copy(void* dest,UINT32 dp,void* src,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,
          UINT32 w,UINT32 h,unsigned bytes,CopyMode mode,UINT16 key=0)
{
    Span d,s;
    const unsigned sourceBytes=(mode==CopyMode::Alpha || mode==CopyMode::AlphaShadow)?4:bytes;
    if (!span(dest,dp,bytes,dx,dy,w,h,d) || !span(src,sp,sourceBytes,sx,sy,w,h,s)) return false;
    if (!w || !h) return true;
    if (overlap(d,s)) return false; // Legacy forward-copy aliases are unsafe.
    auto* out=static_cast<BYTE*>(dest); auto* in=static_cast<BYTE*>(src);
    static const auto cache=[] {
        std::array<std::array<UINT8,256>,256> table{};
        for (unsigned a=0;a<256;++a) for (unsigned v=0;v<256;++v)
            table[a][v]=static_cast<UINT8>((double(a)/255.0)*v);
        return table;
    }();
    for (UINT32 row=0;row<h;++row) for (UINT32 col=0;col<w;++col) {
        const auto sourceCol=mode==CopyMode::Mirror?w-1-col:col;
        auto* to=out+(std::size_t(dy)+row)*dp+(std::size_t(dx)+col)*bytes;
        const auto* from=in+(std::size_t(sy)+row)*sp+(std::size_t(sx)+sourceCol)*sourceBytes;
        if (bytes==1) { *to=*from; continue; }
        UINT16 before=0,value=0; std::memcpy(&before,to,2);
        if (sourceBytes==4) {
            UINT32 pixel=0; std::memcpy(&pixel,from,4);
            const UINT8 a=static_cast<UINT8>(pixel>>24);
            if (!a) continue;
            const UINT16 shade=ShadeTable[before];
            const bool shadow=mode==CopyMode::AlphaShadow;
            const unsigned oldR=(before&31)<<3,oldG=(before&0x7e0)>>3,oldB=(before&0xf800)>>8;
            const unsigned r=shadow?(shade&31)<<3:pixel&255;
            const unsigned g=shadow?(shade&0x7e0)>>3:(pixel>>8)&255;
            const unsigned b=shadow?(shade&0xf800)>>8:(pixel>>16)&255;
            // Retain original low-565-channel to FROMRGB red ordering.
            value=Get16BPPColor(FROMRGB(cache[255-a][oldR]+cache[a][r],
                cache[255-a][oldG]+cache[a][g],cache[255-a][oldB]+cache[a][b]));
        } else {
            std::memcpy(&value,from,2);
            if ((mode==CopyMode::Key || mode==CopyMode::Shadow) && value==key) continue;
            if (mode==CopyMode::Shadow) value=ShadeTable[before];
        }
        std::memcpy(to,&value,2);
    }
    return true;
}
bool rectangle(UINT16* data,UINT32 pitch,SGPRect* area,SGPRect& r)
{
    if (!area) return false;
    r={std::max(area->iLeft,ClippingRect.iLeft),std::max(area->iTop,ClippingRect.iTop),
       std::min(area->iRight,ClippingRect.iRight-1),std::min(area->iBottom,ClippingRect.iBottom-1)};
    if (r.iRight<r.iLeft || r.iBottom<r.iTop) return false;
    Span view;
    return span(data,pitch,2,r.iLeft,r.iTop,UINT32(I(r.iRight)-r.iLeft+1),
                UINT32(I(r.iBottom)-r.iTop+1),view);
}
BOOLEAN shadeRect(UINT16* data,UINT32 pitch,SGPRect* area,const UINT16* table)
{
    SGPRect r; if (!rectangle(data,pitch,area,r)) return FALSE;
    *area=r; // Original API clips the caller's rectangle in place.
    for (I y=r.iTop;y<=r.iBottom;++y) for (I x=r.iLeft;x<=r.iRight;++x) {
        auto& pixel=data[std::size_t(y)*(pitch/2)+std::size_t(x)]; pixel=table[pixel];
    }
    return TRUE;
}
}

BOOLEAN Blt16BPPTo16BPP(UINT16*d,UINT32 dp,UINT16*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,2,CopyMode::Plain); }
BOOLEAN Blt16BPPTo16BPPTrans(UINT16*d,UINT32 dp,UINT16*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h,UINT16 key)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,2,CopyMode::Key,key); }
BOOLEAN Blt16BPPTo16BPPTransShadow(UINT16*d,UINT32 dp,UINT16*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h,UINT16 key)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,2,CopyMode::Shadow,key); }
BOOLEAN Blt16BPPTo16BPPMirror(UINT16*d,UINT32 dp,UINT16*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h)
{
    // Original ignores both source coordinates; clipping advances from source
    // row zero and the right skipped columns before reversing the visible run.
    (void)sx; (void)sy;
    const I l=std::max<I>(dx,ClippingRect.iLeft),t=std::max<I>(dy,ClippingRect.iTop);
    const I r=std::min<I>(I(dx)+w,ClippingRect.iRight),b=std::min<I>(I(dy)+h,ClippingRect.iBottom);
    if (r<=l || b<=t) return TRUE;
    const I sourceX=I(dx)+w-r,sourceY=t-dy;
    if (sourceX>INT32_MAX || sourceY>INT32_MAX) return FALSE;
    return copy(d,dp,s,sp,INT32(l),INT32(t),INT32(sourceX),INT32(sourceY),UINT32(r-l),UINT32(b-t),2,CopyMode::Mirror);
}
BOOLEAN Blt8BPPTo8BPP(UINT8*d,UINT32 dp,UINT8*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,1,CopyMode::Plain); }
BOOLEAN Blt32BPPTo16BPPTrans(UINT16*d,UINT32 dp,UINT32*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,2,CopyMode::Alpha); }
BOOLEAN Blt32BPPTo16BPPTransShadow(UINT16*d,UINT32 dp,UINT32*s,UINT32 sp,INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 w,UINT32 h)
{ return copy(d,dp,s,sp,dx,dy,sx,sy,w,h,2,CopyMode::AlphaShadow); }
BOOLEAN Blt16BPPBufferShadowRect(UINT16*d,UINT32 p,SGPRect*r) { return shadeRect(d,p,r,ShadeTable); }
BOOLEAN Blt16BPPBufferShadowRectAlternateTable(UINT16*d,UINT32 p,SGPRect*r) { return shadeRect(d,p,r,IntensityTable); }

BOOLEAN Blt16BPPBufferPixelateRectWithColor(UINT16* data,UINT32 pitch,SGPRect* area,UINT8 pattern[8][8],UINT16 color)
{
    SGPRect r; if (!pattern || !rectangle(data,pitch,area,r)) return FALSE;
    for (I y=r.iTop;y<=r.iBottom;++y) for (I x=r.iLeft;x<=r.iRight;++x)
        // Original resets EBX to zero each row: the first column always uses
        // Pattern[0][0], then the remaining columns use that row's pattern.
        if (x==r.iLeft ? pattern[0][0] : pattern[(y-r.iTop)%8][(x-r.iLeft)%8])
            data[std::size_t(y)*(pitch/2)+std::size_t(x)]=color;
    return TRUE;
}
BOOLEAN Blt16BPPBufferPixelateRect(UINT16*d,UINT32 p,SGPRect*r,UINT8 pattern[8][8])
{ return Blt16BPPBufferPixelateRectWithColor(d,p,r,pattern,0); }
BOOLEAN Blt16BPPBufferHatchRectWithColor(UINT16*d,UINT32 p,SGPRect*r,UINT16 c)
{
    UINT8 pattern[8][8]{}; for (int y=0;y<8;++y) for (int x=0;x<8;++x) pattern[y][x]=(x+y)%2==0;
    return Blt16BPPBufferPixelateRectWithColor(d,p,r,pattern,c);
}
BOOLEAN Blt16BPPBufferHatchRect(UINT16*d,UINT32 p,SGPRect*r) { return Blt16BPPBufferHatchRectWithColor(d,p,r,0); }
BOOLEAN Blt16BPPBufferLooseHatchRectWithColor(UINT16*d,UINT32 p,SGPRect*r,UINT16 c)
{
    UINT8 pattern[8][8]{}; for (int y=0;y<8;y+=2) for (int x=0;x<8;++x) pattern[y][x]=(x%4)==(y%4);
    return Blt16BPPBufferPixelateRectWithColor(d,p,r,pattern,c);
}
BOOLEAN Blt16BPPBufferLooseHatchRect(UINT16*d,UINT32 p,SGPRect*r) { return Blt16BPPBufferLooseHatchRectWithColor(d,p,r,0); }
BOOLEAN FillRect16BPP(UINT16* data,UINT32 pitch,INT32 x1,INT32 y1,INT32 x2,INT32 y2,UINT16 color)
{
    if (x2<=x1 || y2<=y1) return FALSE;
    const I l=std::max(0,x1),t=std::max(0,y1),r=std::min(639,x2),b=std::min(479,y2);
    Span view; if (r<l || b<t || !span(data,pitch,2,l,t,UINT32(r-l+1),UINT32(b-t+1),view)) return FALSE;
    for (I y=t;y<=b;++y) std::fill_n(data+std::size_t(y)*(pitch/2)+l,r-l+1,color);
    return TRUE;
}
BOOLEAN BlitZRect(UINT16* data,UINT32 pitch,INT16 l,INT16 t,INT16 r,INT16 b,UINT16 value)
{
    sgp::BackgroundView owned;
    if (!sgp::OwnedZBufferView(data,&owned) || pitch!=owned.pitch) return FALSE;
    const I left=std::max<INT32>(l,ClippingRect.iLeft),top=std::max<INT32>(t,ClippingRect.iTop);
    const I right=std::min<INT32>(r,ClippingRect.iRight),bottom=std::min<INT32>(b,ClippingRect.iBottom);
    Span view; if (right<=left || bottom<=top || !span(data,pitch,2,left,top,UINT32(right-left),UINT32(bottom-top),view)) return FALSE;
    for (I y=top;y<bottom;++y) std::fill_n(data+std::size_t(y)*(pitch/2)+left,right-left,value);
    return TRUE;
}

CHAR8 BltIsClippedOrOffScreen(HVOBJECT object,INT32 x,INT32 y,UINT16 index,SGPRect* clipping)
{
    sgp::etrle::Sprite sprite;
    if (!sgp::etrle::DescribeSprite(object,index,x,y,sprite)) return -1;
    const auto& c=clipping?*clipping:ClippingRect;
    const I width=sprite.object->usWidth,height=sprite.object->usHeight;
    if (c.iRight<c.iLeft || c.iBottom<c.iTop) return -1;
    gLeftSkip=INT32(std::min(width,std::max<I>(0,I(c.iLeft)-sprite.x)));
    gRightSkip=INT32(std::min(width,std::max<I>(0,sprite.x+width-c.iRight)));
    gTopSkip=INT32(std::min(height,std::max<I>(0,I(c.iTop)-sprite.y)));
    gBottomSkip=INT32(std::min(height,std::max<I>(0,sprite.y+height-c.iBottom)));
    gfUsePreCalcSkips=TRUE;
    if (gLeftSkip>=width || gRightSkip>=width || gTopSkip>=height || gBottomSkip>=height) return -1;
    return gLeftSkip || gRightSkip || gTopSkip || gBottomSkip;
}
BOOLEAN BltIsClipped(HVOBJECT object,INT32 x,INT32 y,UINT16 index,SGPRect* clipping)
{
    const auto oldLeft=gLeftSkip,oldRight=gRightSkip,oldTop=gTopSkip,oldBottom=gBottomSkip;
    const auto oldUse=gfUsePreCalcSkips;
    const auto result=BltIsClippedOrOffScreen(object,x,y,index,clipping);
    gLeftSkip=oldLeft;gRightSkip=oldRight;gTopSkip=oldTop;gBottomSkip=oldBottom;gfUsePreCalcSkips=oldUse;
    return result!=0;
}
