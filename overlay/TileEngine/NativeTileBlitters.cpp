/*
 * P710 native snapshot update: TileEngine/NativeTileBlitters.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeTileBlitters.h"
#include "native_background.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

// Translated from TileEngine/renderworld.cpp at engine
// 3394695a387a14c78e92b158cfdcf50e4068c96b; original engine license applies.
// No external implementation copied. See TILE-IMPLEMENTATION.md for the old
// clipped EOL scan bug and unused final strip-read safety adjustment.
extern UINT16 ShadeTable[65536];
unsigned short blendWithAlpha(unsigned int,unsigned int,unsigned int);
namespace {
using I=std::int64_t;
using sgp::BackgroundView;
using sgp::etrle::Sprite;
enum class Kind { Inc, Same, Obscure, Shadow, ShadowObscure, Alpha, AlphaObscure, Zero, Inverse, Redundant };
bool overlap(const void* a,std::size_t an,const void* b,std::size_t bn)
{
    if(!a || !b || !an || !bn) return false;
    const auto av=reinterpret_cast<std::uintptr_t>(a),bv=reinterpret_cast<std::uintptr_t>(b);
    return av<=bv ? bv-av<an : av-bv<bn;
}
UINT16 load(const BYTE* p) { UINT16 v;std::memcpy(&v,p,2);return v; }
void store(BYTE* p,UINT16 v) {std::memcpy(p,&v,2);}
bool shadow(Kind k) { return k==Kind::Shadow || k==Kind::ShadowObscure || k==Kind::Alpha || k==Kind::AlphaObscure; }
bool alpha(Kind k) { return k==Kind::Alpha || k==Kind::AlphaObscure; }
bool strips(Kind k) {return k!=Kind::Zero && k!=Kind::Inverse && k!=Kind::Redundant;}
int signedDepth(UINT16 v) { return v<0x8000u ? int(v) : int(v)-65536; }
struct Draw {
    BackgroundView color{},z{};
    Sprite sprite{};
    const UINT16* palette=nullptr;
    const UINT8* alphaData=nullptr;
    I left=0,top=0,right=0,bottom=0;
    std::vector<UINT16> levels;
    UINT16 depth=0;
    Kind kind=Kind::Inc;
    bool ignore=false,hidden=true;
};
void drawRun(void* context,UINT32 row,UINT32 column,const UINT8* pixels,UINT32 count)
{
    auto& d=*static_cast<Draw*>(context);
    const I y=d.sprite.y+row,runLeft=d.sprite.x+column;
    if(y<d.top || y>=d.bottom) return;
    const I first=std::max(d.left,runLeft),last=std::min(d.right,runLeft+count);
    for(I x=first;x<last;++x)
    {
        const auto offset=std::size_t(y)*d.z.pitch+std::size_t(x)*2;
        const UINT8 index=pixels[std::size_t(x-runLeft)];
        const UINT16 level=strips(d.kind)?d.levels[std::size_t(x-d.left)]:d.depth;
        if(d.kind==Kind::Redundant)
        {
            // Original JLE is signed, unlike the drawing kernels' JA/JAE.
            if(signedDepth(level)>signedDepth(load(d.z.data+offset))) d.hidden=false;
            continue;
        }
        BYTE* destination=d.color.data+offset;
        if(d.kind==Kind::Zero) {store(destination,0);continue;}
        const UINT16 oldZ=load(d.z.data+offset);
        if(d.kind==Kind::Inverse)
        {
            if(oldZ==level) store(destination,d.palette[index]);
            continue; // Never changes Z.
        }
        bool obscured=false;
        if(d.kind==Kind::Inc || d.kind==Kind::Obscure || d.kind==Kind::AlphaObscure)
            obscured=oldZ>=level;
        else obscured=oldZ>level;
        if(obscured)
        {
            if(d.kind!=Kind::Obscure && d.kind!=Kind::ShadowObscure && d.kind!=Kind::AlphaObscure) continue;
            // Preserve TEST EDI,2, including an odd pitch or a base at +2 mod 4.
            const bool addressBit=(reinterpret_cast<std::uintptr_t>(destination)&2u)!=0;
            if(addressBit!=((y&1)!=0)) continue;
        }
        // Original writes depth even for ignored shadows and obscured pixels.
        store(d.z.data+offset,level);
        if(shadow(d.kind) && index==254)
        {
            if(!d.ignore) store(destination,ShadeTable[load(destination)]);
        }
        else if(alpha(d.kind))
        {
            // Original AlphaPtr advances in lockstep with the *color stream*,
            // including controls. Alpha control values are never interpreted.
            const auto sourceOffset=std::size_t(pixels-d.sprite.data)+std::size_t(x-runLeft);
            store(destination,blendWithAlpha(d.palette[index],load(destination),d.alphaData[sourceOffset]));
        }
        else store(destination,d.palette[index]);
    }
}
BOOLEAN blit(UINT16* buffer,UINT32 pitch,UINT16* zbuffer,UINT16 depth,HVOBJECT source,
    HVOBJECT a,INT32 x,INT32 y,UINT16 index,const SGPRect* clip,INT32 stripIndex,
    UINT16* palette,BOOLEAN ignore,Kind kind)
{
    Draw d;d.kind=kind;d.depth=depth;d.ignore=ignore!=FALSE;
    if(!sgp::etrle::DescribeSprite(source,index,x,y,d.sprite)) return FALSE;
    const bool onlyZ=kind==Kind::Redundant,noZ=kind==Kind::Zero;
    if(!onlyZ && (!sgp::BackgroundBufferView(buffer,&d.color) || !sgp::ValidBackgroundView(d.color) || d.color.pitch!=pitch)) return FALSE;
    if(!noZ && (!sgp::BackgroundBufferView(zbuffer,&d.z) || !sgp::ValidBackgroundView(d.z) || d.z.pitch!=pitch)) return FALSE;
    if(noZ) d.z=d.color; // Geometry/offset carrier; never read/written as Z.
    if(!onlyZ && !noZ && (d.color.width!=d.z.width || d.color.height!=d.z.height ||
       overlap(d.color.data,d.color.bytes,d.z.data,d.z.bytes))) return FALSE;
    d.left=d.sprite.x;d.top=d.sprite.y;
    d.right=d.left+d.sprite.object->usWidth;d.bottom=d.top+d.sprite.object->usHeight;
    if(clip)
    {
        d.left=std::max<I>(d.left,clip->iLeft);d.top=std::max<I>(d.top,clip->iTop);
        d.right=std::min<I>(d.right,clip->iRight);d.bottom=std::min<I>(d.bottom,clip->iBottom);
        if(d.left>=d.right || d.top>=d.bottom) return TRUE;
    }
    if(d.left<0 || d.top<0 || d.right>d.z.width || d.bottom>d.z.height ||
       !sgp::etrle::ValidDestination(pitch,d.left,d.top,d.right,d.bottom)) return FALSE;
    // Zero and the visibility query do not use a palette in the original code.
    const auto& object=*d.sprite.object;
    if(!source->pPixData || !object.uiDataLength || object.uiDataOffset>source->uiSizePixData ||
       object.uiDataLength>source->uiSizePixData-object.uiDataOffset) return FALSE;
    d.sprite.data=static_cast<const UINT8*>(source->pPixData)+object.uiDataOffset;
    d.sprite.length=object.uiDataLength;
    d.palette=shadow(kind)?palette:source->pShadeCurrent;
    if(!noZ && !onlyZ && !d.palette) return FALSE;
    auto aliases=[&](const void* p,std::size_t n) {
        return (!onlyZ && overlap(p,n,d.color.data,d.color.bytes)) || (!noZ && overlap(p,n,d.z.data,d.z.bytes));
    };
    if(aliases(source,sizeof(*source)) || aliases(source->pETRLEObject,std::size_t(source->usNumberOfObjects)*sizeof(ETRLEObject)) ||
       aliases(d.sprite.data,d.sprite.length) || (!noZ && !onlyZ && aliases(d.palette,256*sizeof(UINT16)))) return FALSE;
    if(alpha(kind))
    {
        if(!a || !a->pETRLEObject || !a->pPixData || index>=a->usNumberOfObjects || a->ubBitDepth!=8) return FALSE;
        const auto& ao=a->pETRLEObject[index];
        if(ao.uiDataOffset>a->uiSizePixData || ao.uiDataLength>a->uiSizePixData-ao.uiDataOffset || ao.uiDataLength<d.sprite.length) return FALSE;
        d.alphaData=static_cast<const UINT8*>(a->pPixData)+ao.uiDataOffset;
        if(aliases(a,sizeof(*a)) || aliases(a->pETRLEObject,std::size_t(a->usNumberOfObjects)*sizeof(ETRLEObject)) ||
           aliases(d.alphaData,ao.uiDataLength)) return FALSE;
    }
    if(strips(kind))
    {
        // structure.cpp allocates exactly usNumberOfObjects pointer slots.
        if(stripIndex<0 || stripIndex>=source->usNumberOfObjects || !source->ppZStripInfo ||
           aliases(source->ppZStripInfo,std::size_t(source->usNumberOfObjects)*sizeof(ZStripInfo*))) return FALSE;
        const ZStripInfo* info=source->ppZStripInfo[stripIndex];
        if(!info || aliases(info,sizeof(*info)) || info->ubFirstZStripWidth>20 || !info->pbZChange) return FALSE;
        // The producer explicitly allocates one zero byte for count==0.
        const unsigned capacity=std::max(1u,unsigned(info->ubNumberOfZChanges));
        if(aliases(info->pbZChange,capacity)) return FALSE;
        const unsigned leftSkip=unsigned(d.left-d.sprite.x),first=info->ubFirstZStripWidth;
        unsigned next=0,remaining=first;
        UINT16 level=UINT16(unsigned(depth)+int(info->bInitialZChange)*80);
        const int delta=shadow(kind)?8:80;
        if(leftSkip>=first)
        {
            next=1+(leftSkip-first)/20;
            if(next>capacity) return FALSE;
            for(unsigned i=0;i<next;++i)
            {
                // Original C++ left-clipping switch recognizes only -1/+1.
                const int change=info->pbZChange[i];
                if(change==-1 || change==1) level=UINT16(unsigned(level)+change*delta);
            }
            remaining=20-(leftSkip-first)%20;
        }
        else remaining=first-leftSkip;
        const auto width=std::size_t(d.right-d.left);
        d.levels.reserve(width);
        for(std::size_t col=0;col<width;++col)
        {
            d.levels.push_back(level);
            if(--remaining==0 && col+1<width)
            {
                if(next>=capacity) return FALSE;
                const int change=info->pbZChange[next++];
                level=UINT16(unsigned(level)+(change<0?-delta:change>0?delta:0));
                remaining=20;
            }
        }
    }
    if(!sgp::etrle::ValidateEncoding(d.sprite)) return FALSE;
    sgp::etrle::ForEachLiteralRun(d.sprite,drawRun,&d);
    return onlyZ ? BOOLEAN(d.hidden) : TRUE;
}
}
#define ARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define PASS b,p,z,d,s,nullptr,x,y,i
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClip(ARGS,SGPRect* c) {return blit(PASS,c?c:&ClippingRect,i,nullptr,FALSE,Kind::Inc);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(ARGS,SGPRect* c,INT16 si) {return blit(PASS,c?c:&ClippingRect,si,nullptr,FALSE,Kind::Same);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncObscureClip(ARGS,SGPRect* c) {return blit(PASS,c?c:&ClippingRect,i,nullptr,FALSE,Kind::Obscure);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(ARGS,SGPRect* c,INT16 si,UINT16* pal,BOOLEAN ignore) {return blit(PASS,c?c:&ClippingRect,si,pal,ignore,Kind::ShadowObscure);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(ARGS,SGPRect* c,INT16 si,UINT16* pal,BOOLEAN ignore) {return blit(PASS,c?c:&ClippingRect,si,pal,ignore,Kind::Shadow);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransInvZ(ARGS) {return blit(PASS,nullptr,0,nullptr,FALSE,Kind::Inverse);}
#undef PASS
#undef ARGS
#define AARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,HVOBJECT a,INT32 x,INT32 y,UINT16 i,SGPRect* c,INT16 si,UINT16* pal,BOOLEAN ignore
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClipAlpha(AARGS) {return blit(b,p,z,d,s,a,x,y,i,c?c:&ClippingRect,si,pal,ignore,Kind::AlphaObscure);}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(AARGS) {return blit(b,p,z,d,s,a,x,y,i,c?c:&ClippingRect,si,pal,ignore,Kind::Alpha);}
#undef AARGS
BOOLEAN Zero8BPPDataTo16BPPBufferTransparent(UINT16* b,UINT32 p,HVOBJECT s,INT32 x,INT32 y,UINT16 i) {return blit(b,p,nullptr,0,s,nullptr,x,y,i,nullptr,0,nullptr,FALSE,Kind::Zero);}
BOOLEAN IsTileRedundent(UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,INT32 x,INT32 y,UINT16 i) {return blit(nullptr,p,z,d,s,nullptr,x,y,i,nullptr,0,nullptr,FALSE,Kind::Redundant);}
