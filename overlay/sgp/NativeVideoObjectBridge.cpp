/*
 * P710 native snapshot update: sgp/NativeVideoObjectBridge.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeVideoObjectBridge.h"
#include "NativeEffectBlitters.h"
#include "vobject_blitters.h"
#include "shading.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

// Native high-level dispatch translated from original sgp/vobject.cpp at
// 3394695a387a14c78e92b158cfdcf50e4068c96b. Original source license applies.
namespace {
using I = std::int64_t;
bool overlap(const void* a, std::size_t an, const void* b, std::size_t bn)
{
    const auto av=reinterpret_cast<std::uintptr_t>(a), bv=reinterpret_cast<std::uintptr_t>(b);
    return an && bn && (av<=bv ? bv-av<an : av-bv<bn);
}
struct Destination {
    UINT32 id, pitch=0;
    BYTE* data=nullptr;
    sgp::NativeSurfaceView view;
    explicit Destination(UINT32 surface) : id(surface)
    { data=LockVideoSurface(id,&pitch); }
    ~Destination() { if(data) UnLockVideoSurface(id); }
    bool valid()
    { return data && sgp::LockedNativeSurfaceView(data,&view) && pitch==view.pitch; }
    UINT16* pixels() { return reinterpret_cast<UINT16*>(data); }
    bool aliases(const void* p,std::size_t n) const
    { return overlap(p,n,data,view.bytes); }
};
bool clipValid(const SGPRect& c)
{ return c.iLeft<=c.iRight && c.iTop<=c.iBottom; }
SGPRect bounded(SGPRect c,const Destination& d)
{
    c.iLeft=std::clamp<INT32>(c.iLeft,0,static_cast<INT32>(d.view.width));
    c.iRight=std::clamp<INT32>(c.iRight,0,static_cast<INT32>(d.view.width));
    c.iTop=std::clamp<INT32>(c.iTop,0,static_cast<INT32>(d.view.height));
    c.iBottom=std::clamp<INT32>(c.iBottom,0,static_cast<INT32>(d.view.height));
    return c;
}
bool clipped(const sgp::etrle::Sprite& s,UINT16 width,UINT16 height,const SGPRect& c)
{ return s.x<c.iLeft || s.y<c.iTop || s.x+width>c.iRight || s.y+height>c.iBottom; }
bool sprite(Destination& d,HVOBJECT source,UINT16 index,INT32 x,INT32 y,
            bool palette,sgp::etrle::Sprite& s)
{
    if(!source || d.aliases(source,sizeof(*source)) ||
       !sgp::etrle::DescribeSprite(source,index,x,y,s) ||
       d.aliases(source->pETRLEObject,std::size_t(source->usNumberOfObjects)*sizeof(ETRLEObject))) return false;
    const auto& frame=*s.object;
    if(!source->pPixData || !frame.uiDataLength || frame.uiDataOffset>source->uiSizePixData ||
       frame.uiDataLength>source->uiSizePixData-frame.uiDataOffset) return false;
    s.data=static_cast<const UINT8*>(source->pPixData)+frame.uiDataOffset;
    s.length=frame.uiDataLength;
    if(d.aliases(s.data,s.length) || (palette && (!source->pShadeCurrent ||
       d.aliases(source->pShadeCurrent,256*sizeof(UINT16))))) return false;
    return sgp::etrle::ValidateEncoding(s);
}
BOOLEAN raw(Destination& d,HVOBJECT source,UINT16 index,INT32 x,INT32 y,
            UINT32 flags,UINT16 width,UINT16 height,bool shadow)
{
    if(!source || d.aliases(source,sizeof(*source)) || !source->p16BPPObject ||
       index>=source->usNumberOf16BPPObjects ||
       (source->ubBitDepth!=16 && source->ubBitDepth!=32) ||
       d.aliases(source->p16BPPObject,std::size_t(source->usNumberOf16BPPObjects)*sizeof(SixteenBPPObjectInfo))) return FALSE;
    // Original shadow-from-index checks requested index but renders object0.
    const auto& frame=source->p16BPPObject[shadow?0:index];
    const unsigned pixelBytes=source->ubBitDepth/8;
    const std::uint64_t bytes=std::uint64_t(frame.usWidth)*frame.usHeight*pixelBytes;
    const auto address=reinterpret_cast<std::uintptr_t>(frame.p16BPPData);
    if(!frame.p16BPPData || !frame.usWidth || !frame.usHeight || address%pixelBytes ||
       bytes>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max()) ||
       address>std::numeric_limits<std::uintptr_t>::max()-bytes ||
       d.aliases(frame.p16BPPData,static_cast<std::size_t>(bytes))) return FALSE;
    const I w=width?std::min(width,frame.usWidth):frame.usWidth;
    const I h=height?std::min(height,frame.usHeight):frame.usHeight;
    // Raw original calls ignore global clipping and frame offsets. Bound only
    // against the real destination; keep original source pitch after cropping.
    const I l=std::max<I>(0,x),t=std::max<I>(0,y);
    const I r=std::min<I>(d.view.width,I(x)+w),b=std::min<I>(d.view.height,I(y)+h);
    if(l>=r || t>=b) return TRUE;
    const auto sx=static_cast<INT32>(l-x),sy=static_cast<INT32>(t-y);
    const auto dx=static_cast<INT32>(l),dy=static_cast<INT32>(t);
    const auto cw=static_cast<UINT32>(r-l),ch=static_cast<UINT32>(b-t);
    const UINT32 sp=frame.usWidth*pixelBytes;
    if(source->ubBitDepth==32) {
        auto* pixels=reinterpret_cast<UINT32*>(frame.p16BPPData);
        return shadow ? Blt32BPPTo16BPPTransShadow(d.pixels(),d.pitch,pixels,sp,dx,dy,sx,sy,cw,ch) :
                        Blt32BPPTo16BPPTrans(d.pixels(),d.pitch,pixels,sp,dx,dy,sx,sy,cw,ch);
    }
    if(shadow) return Blt16BPPTo16BPPTransShadow(d.pixels(),d.pitch,frame.p16BPPData,sp,dx,dy,sx,sy,cw,ch,0x1f);
    if(flags&VO_BLT_SRCTRANSPARENCY)
        return Blt16BPPTo16BPPTrans(d.pixels(),d.pitch,frame.p16BPPData,sp,dx,dy,sx,sy,cw,ch,0x1f);
    return Blt16BPPTo16BPP(d.pixels(),d.pitch,frame.p16BPPData,sp,dx,dy,sx,sy,cw,ch);
}
struct BoundedShadow { Destination* destination; const sgp::etrle::Sprite* source; SGPRect clip; };
void shadowRun(void* context,UINT32 row,UINT32 column,const UINT8* pixels,UINT32 count)
{
    auto& s=*static_cast<BoundedShadow*>(context);
    const I y=s.source->y+row;
    if(y<s.clip.iTop || y>=s.clip.iBottom) return;
    for(UINT32 j=0;j<count;++j) {
        const I x=s.source->x+column+j;
        if(pixels[j]==254 || x<s.clip.iLeft || x>=s.clip.iRight) continue;
        BYTE* pixel=s.destination->data+std::size_t(y)*s.destination->pitch+std::size_t(x)*2;
        UINT16 value;std::memcpy(&value,pixel,2);value=ShadeTable[value];std::memcpy(pixel,&value,2);
    }
}
BOOLEAN outline(Destination& d,HVOBJECT source,UINT16 index,INT32 x,INT32 y)
{
    if(!clipValid(ClippingRect)) return FALSE;
    sgp::etrle::Sprite s;
    if(!sprite(d,source,index,x,y,false,s)) return FALSE;
    auto clip=bounded(ClippingRect,d);
    if(clipped(s,s.object->usWidth,s.object->usHeight,ClippingRect))
        return Blt8BPPDataTo16BPPBufferOutlineShadowClip(d.pixels(),d.pitch,source,x,y,index,&clip);
    if(!clipped(s,s.object->usWidth,s.object->usHeight,clip))
        return Blt8BPPDataTo16BPPBufferOutlineShadow(d.pixels(),d.pitch,source,x,y,index);
    // Global-unclipped original excludes254. Surface safety clipping must not
    // select OutlineShadowClip (which intentionally shades254). That provider
    // cannot express this case, so reuse its bounded run decoder with this mask.
    BoundedShadow draw{&d,&s,clip};
    sgp::etrle::ForEachLiteralRun(s,shadowRun,&draw);
    return TRUE;
}
}

BOOLEAN BltVideoObject(UINT32 destination,HVOBJECT source,UINT16 index,
                       SGPRectangle region,UINT32 flags,blt_fx* effects)
{
    // Original rectangle overload ignores width/height and forwards x/y.
    return BltVideoObject(destination,source,index,region.x,region.y,flags,effects);
}
BOOLEAN BltVideoObjectFromIndexWithOverride(UINT32 destination,UINT32 object,UINT16 index,
    INT32 x,INT32 y,UINT32 flags,blt_fx*,UINT16 width,UINT16 height)
{
    Destination d(destination);
    if(!d.valid()) return FALSE;
    HVOBJECT source=nullptr;
    if(!GetVideoObject(&source,object)) return FALSE;
    if(source->ubBitDepth==16 || source->ubBitDepth==32)
        return raw(d,source,index,x,y,flags,width,height,false);
    if(!clipValid(ClippingRect)) return FALSE;
    // Preserve original transparency-before-shadow dispatch. Unsupported8-bit
    // requests fail explicitly instead of original's successful empty draw.
    const bool transparent=(flags&VO_BLT_SRCTRANSPARENCY)!=0;
    if(!transparent && !(flags&VO_BLT_SHADOW)) return FALSE;
    sgp::etrle::Sprite s;
    if(!sprite(d,source,index,x,y,transparent,s)) return FALSE;
    const UINT16 w=width?std::min(width,s.object->usWidth):s.object->usWidth;
    const UINT16 h=height?std::min(height,s.object->usHeight):s.object->usHeight;
    SGPRect clip;
    if(clipped(s,w,h,ClippingRect)) clip=ClippingRect;
    else {
        // Source's subArea is anchored at x/y, without object offsets. Avoid
        // signed overflow in the original INT32 additions, then surface-bound.
        clip={x,y,static_cast<INT32>(std::min<I>(INT32_MAX,I(x)+w)),
                  static_cast<INT32>(std::min<I>(INT32_MAX,I(y)+h))};
    }
    clip=bounded(clip,d);
    return transparent ? Blt8BPPDataTo16BPPBufferTransparentClip(d.pixels(),d.pitch,source,x,y,index,&clip) :
                         Blt8BPPDataTo16BPPBufferShadowClip(d.pixels(),d.pitch,source,x,y,index,&clip);
}
BOOLEAN BltVideoObjectOutlineShadow(UINT32 destination,HVOBJECT source,UINT16 index,INT32 x,INT32 y)
{
    Destination d(destination);
    return d.valid() && outline(d,source,index,x,y);
}
BOOLEAN BltVideoObjectOutlineShadowFromIndex(UINT32 destination,UINT32 object,UINT16 index,INT32 x,INT32 y)
{
    Destination d(destination);
    if(!d.valid()) return FALSE;
    HVOBJECT source=nullptr;
    if(!GetVideoObject(&source,object)) return FALSE;
    if(source->ubBitDepth==16 || source->ubBitDepth==32)
        return raw(d,source,index,x,y,0,0,0,true);
    return outline(d,source,index,x,y);
}
