/*
 * P710 native snapshot update: sgp/NativeWorldBlitters.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeWorldBlitters.h"
#include "native_background.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// Scalar translations of the original v1.13 assembly; original license applies.
// Other Z/shadow/gradient/pixelate families remain original unported blockers.
namespace {
using I=std::int64_t;
using sgp::BackgroundView;
using sgp::etrle::Sprite;
enum class Policy { WriteZ, NoWriteZ, ObscuredColor, OutlineWriteZ, OutlineNoWriteZ };
bool overlaps(const void* a,std::size_t an,const void* b,std::size_t bn)
{
    const auto av=reinterpret_cast<std::uintptr_t>(a),bv=reinterpret_cast<std::uintptr_t>(b);
    return av<=bv ? bv-av<an : av-bv<bn;
}
struct Draw {
    BackgroundView color,z;
    const UINT16* palette;
    I x,y,left,top,right,bottom;
    UINT16 depth,special;
    Policy policy;
    bool outline;
};
void drawRun(void* opaque,UINT32 row,UINT32 column,const UINT8* pixels,UINT32 count)
{
    auto& d=*static_cast<Draw*>(opaque);
    const I y=d.y+row,runLeft=d.x+column;
    if (y<d.top || y>=d.bottom) return;
    const I first=std::max(d.left,runLeft),last=std::min(d.right,runLeft+count);
    for(I x=first;x<last;++x)
    {
        const auto offset=std::size_t(y)*d.color.pitch+std::size_t(x)*2;
        const UINT8 index=pixels[std::size_t(x-runLeft)];
        UINT16 oldZ;std::memcpy(&oldZ,d.z.data+offset,2);
        // Unsigned original JB / JA comparisons admit equality in every kernel.
        if (oldZ>d.depth)
        {
            if(d.policy==Policy::ObscuredColor) std::memcpy(d.color.data+offset,&d.special,2);
            continue;
        }
        const bool outlinePolicy=d.policy==Policy::OutlineWriteZ || d.policy==Policy::OutlineNoWriteZ;
        if (outlinePolicy && index==254)
        {
            if(d.outline) std::memcpy(d.color.data+offset,&d.special,2);
            continue; // Outline pixels never claim Z, even when drawn.
        }
        if(d.policy==Policy::WriteZ || d.policy==Policy::OutlineWriteZ)
        {
            // The original MOV AL,[ESI] precedes MOV [EBX],AX in OutlineZ.
            const UINT16 nextZ=d.policy==Policy::OutlineWriteZ ? UINT16((d.depth&0xff00u)|index) : d.depth;
            std::memcpy(d.z.data+offset,&nextZ,2);
        }
        const UINT16 pixel=d.palette[index];std::memcpy(d.color.data+offset,&pixel,2);
    }
}
BOOLEAN blit(UINT16* buffer,UINT32 pitch,UINT16* zbuffer,UINT16 depth,HVOBJECT source,
    INT32 x,INT32 y,UINT16 index,const SGPRect* clip,Policy policy,UINT16 special=0,BOOLEAN outline=FALSE)
{
    BackgroundView color,z;Sprite sprite{};
    if(!sgp::BackgroundBufferView(buffer,&color) || !sgp::BackgroundBufferView(zbuffer,&z) ||
       !sgp::ValidBackgroundView(color) || !sgp::ValidBackgroundView(z) ||
       color.pitch!=pitch || z.pitch!=pitch || color.width!=z.width || color.height!=z.height ||
       overlaps(color.data,color.bytes,z.data,z.bytes) ||
       !sgp::etrle::DescribeSprite(source,index,x,y,sprite,clip!=nullptr)) return FALSE;
    I l=sprite.x,t=sprite.y,r=l+sprite.object->usWidth,b=t+sprite.object->usHeight;
    if(clip)
    {
        l=std::max<I>(l,clip->iLeft);t=std::max<I>(t,clip->iTop);
        r=std::min<I>(r,clip->iRight);b=std::min<I>(b,clip->iBottom);
        if(l>=r || t>=b) return TRUE;
    }
    // A caller clip cannot turn padding or out-of-allocation coordinates into pixels.
    if(l<0 || t<0 || r>color.width || b>color.height ||
       !sgp::etrle::ValidDestination(pitch,l,t,r,b) ||
       !sgp::etrle::BindSource(source,sprite)) return FALSE;
    // Reject aliasing before decoding or drawing. In-place encoded streams or
    // palette/metadata aliases cannot alter a later run after preflight.
    const void* inputs[]{source,source->pETRLEObject,sprite.data,source->pShadeCurrent};
    const std::size_t sizes[]{sizeof(*source),std::size_t(source->usNumberOfObjects)*sizeof(ETRLEObject),sprite.length,256*sizeof(UINT16)};
    for(unsigned i=0;i<4;++i)
        if(overlaps(inputs[i],sizes[i],color.data,color.bytes) || overlaps(inputs[i],sizes[i],z.data,z.bytes)) return FALSE;
    if(!sgp::etrle::ValidateEncoding(sprite)) return FALSE;
    Draw draw{color,z,source->pShadeCurrent,sprite.x,sprite.y,l,t,r,b,depth,special,policy,outline==TRUE};
    sgp::etrle::ForEachLiteralRun(sprite,drawRun,&draw);return TRUE;
}
}
#define WORLD_ARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define WORLD_PASS b,p,z,d,s,x,y,i
BOOLEAN Blt8BPPDataTo16BPPBufferTransZ(WORLD_ARGS) { return blit(WORLD_PASS,nullptr,Policy::WriteZ); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNB(WORLD_ARGS) { return blit(WORLD_PASS,nullptr,Policy::NoWriteZ); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClip(WORLD_ARGS,SGPRect* c) { return blit(WORLD_PASS,c?c:&ClippingRect,Policy::WriteZ); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClip(WORLD_ARGS,SGPRect* c) { return blit(WORLD_PASS,c?c:&ClippingRect,Policy::NoWriteZ); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBColor(WORLD_ARGS,UINT16 color) { return blit(WORLD_PASS,nullptr,Policy::ObscuredColor,color); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClipColor(WORLD_ARGS,SGPRect* c,UINT16 color) { return blit(WORLD_PASS,c?c:&ClippingRect,Policy::ObscuredColor,color); }
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZ(WORLD_ARGS,INT16 color,BOOLEAN outline) { return blit(WORLD_PASS,nullptr,Policy::OutlineWriteZ,UINT16(color),outline); }
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZNB(WORLD_ARGS,INT16 color,BOOLEAN outline) { return blit(WORLD_PASS,nullptr,Policy::OutlineNoWriteZ,UINT16(color),outline); }
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZClip(WORLD_ARGS,INT16 color,BOOLEAN outline,SGPRect* c) { return blit(WORLD_PASS,c?c:&ClippingRect,Policy::OutlineWriteZ,UINT16(color),outline); }
#undef WORLD_ARGS
#undef WORLD_PASS
