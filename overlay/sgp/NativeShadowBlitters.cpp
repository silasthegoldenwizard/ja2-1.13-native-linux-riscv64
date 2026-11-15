/*
 * P710 native snapshot update: sgp/NativeShadowBlitters.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeShadowBlitters.h"
#include "NativeRawBlitters.h"
#include "native_background.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <new>
#include <vector>

// Direct scalar translations of sgp/vobject_blitters.cpp:3610-7720 at
// 3394695a387a14c78e92b158cfdcf50e4068c96b. Original engine license applies.
// No implementation from another JA2 project is incorporated.
extern UINT16 ShadeTable[65536];
namespace {
using I=std::int64_t;
using sgp::etrle::Sprite;
enum class Policy { Plain, WriteZ, NoWriteZ, Obscured, ObscuredTest };
bool overlap(const void* a,std::size_t an,const void* b,std::size_t bn)
{
    if(!an || !bn) return false;
    const auto av=reinterpret_cast<std::uintptr_t>(a),bv=reinterpret_cast<std::uintptr_t>(b);
    return av<=bv ? bv-av<an : av-bv<bn;
}
bool bindData(HVOBJECT source,Sprite& sprite)
{
    const auto& o=*sprite.object;
    if(!source->pPixData || !o.uiDataLength || o.uiDataOffset>source->uiSizePixData ||
       o.uiDataLength>source->uiSizePixData-o.uiDataOffset) return false;
    sprite.data=static_cast<const UINT8*>(source->pPixData)+o.uiDataOffset;
    sprite.length=o.uiDataLength;return true;
}
struct Draw {
    sgp::BackgroundView color,z;
    const UINT16* palette;
    const Sprite* source;
    const UINT8* alpha;
    const std::vector<std::size_t>* alphaShift;
    I left,top,right,bottom;
    UINT16 depth;
    Policy policy;
    bool ignore;
};
void drawRun(void* context,UINT32 row,UINT32 col,const UINT8* pixels,UINT32 count)
{
    const auto& d=*static_cast<Draw*>(context);
    const I y=d.source->y+row,runLeft=d.source->x+col;
    if(y<d.top || y>=d.bottom) return;
    for(I x=std::max(d.left,runLeft),end=std::min(d.right,runLeft+count);x<end;++x)
    {
        const auto offset=std::size_t(y)*d.color.pitch+std::size_t(x)*2;
        UINT8* dest=d.color.data+offset;
        const auto literal=std::size_t(x-runLeft);
        const UINT8 index=pixels[literal];
        UINT16 oldZ=0;
        if(d.policy!=Policy::Plain) std::memcpy(&oldZ,d.z.data+offset,2);
        if(d.policy==Policy::WriteZ)
        {
            if(oldZ>=d.depth) continue; // Original JAE, including shadow and alpha.
            std::memcpy(d.z.data+offset,&d.depth,2); // Even ignored shadows claim Z.
        }
        else if(d.policy!=Policy::Plain && oldZ>d.depth)
        {
            if((d.policy!=Policy::Obscured && d.policy!=Policy::ObscuredTest) || index==254)
                continue;
            // The original tests EDI bit 1, not logical x parity.
            if(bool(reinterpret_cast<std::uintptr_t>(dest)&2)!=bool(y&1)) continue;
        }
        if(index==254)
        {
            if(d.ignore || (d.policy!=Policy::Plain && d.policy!=Policy::WriteZ && oldZ>=d.depth))
                continue;
            UINT16 previous;std::memcpy(&previous,dest,2);
            const UINT16 shaded=ShadeTable[previous];std::memcpy(dest,&shaded,2);continue;
        }
        UINT16 value=d.palette[index];
        if(d.alpha || d.policy==Policy::ObscuredTest)
        {
            UINT16 previous;std::memcpy(&previous,dest,2);
            const unsigned int alpha=d.alpha ? d.alpha[std::size_t(pixels-d.source->data)+literal-(*d.alphaShift)[row]] : 127;
            value=blendWithAlpha(value,previous,alpha);
        }
        std::memcpy(dest,&value,2);
    }
}
// Validate the actual alpha cursor the assembly uses. Alpha control slots are
// ignored bytes, not independently interpreted ETRLE controls. Every source
// control normally consumes one alpha byte; LeftSkipLoop omits that increment.
bool alphaAlignment(const Sprite& source,const Sprite& alpha,const SGPRect* clip,
                    I top,I bottom,I left,std::vector<std::size_t>& shifts)
{
    shifts.resize(source.object->usHeight);
    std::size_t cursor=0,shift=0;
    for(UINT32 row=0;row<source.object->usHeight;++row)
    {
        UINT32 column=0;
        const bool visible=source.y+row>=top && source.y+row<bottom;
        const I skip=clip && visible ? left-source.x : 0;
        for(;;)
        {
            const auto control=source.data[cursor++];
            if(!control) break;
            const UINT32 run=control&127;
            if(I(column)<skip) ++shift;
            column+=run;
            if(!(control&128)) cursor+=run;
        }
        shifts[row]=shift;
        // Real alpha allocation may contain padding, but cannot be inferred
        // from source dimensions or from the color stream's allocation size.
        if(cursor<shift || cursor-shift>alpha.length) return false;
    }
    return true;
}
BOOLEAN blit(UINT16* buffer,UINT32 pitch,UINT16* zbuffer,UINT16 depth,
             HVOBJECT source,HVOBJECT alpha,INT32 x,INT32 y,UINT16 index,
             const SGPRect* clip,const UINT16* palette,BOOLEAN ignore,Policy policy)
{
    sgp::BackgroundView color,z;Sprite sprite{},alphaSprite{};
    if(!palette || !sgp::BackgroundBufferView(buffer,&color) || !sgp::ValidBackgroundView(color) ||
       color.pitch!=pitch || !sgp::etrle::DescribeSprite(source,index,x,y,sprite,clip!=nullptr)) return FALSE;
    if(policy!=Policy::Plain &&
       (!sgp::BackgroundBufferView(zbuffer,&z) || !sgp::ValidBackgroundView(z) ||
        z.pitch!=pitch || color.width!=z.width || color.height!=z.height ||
        overlap(color.data,color.bytes,z.data,z.bytes))) return FALSE;
    I l=sprite.x,t=sprite.y,r=l+sprite.object->usWidth,b=t+sprite.object->usHeight;
    if(clip)
    {
        l=std::max<I>(l,clip->iLeft);t=std::max<I>(t,clip->iTop);
        r=std::min<I>(r,clip->iRight);b=std::min<I>(b,clip->iBottom);
        if(l>=r || t>=b) return TRUE;
    }
    if(l<0 || t<0 || r>color.width || b>color.height ||
       !sgp::etrle::ValidDestination(pitch,l,t,r,b) || !bindData(source,sprite)) return FALSE;
    if(alpha && (!sgp::etrle::DescribeSprite(alpha,index,0,0,alphaSprite) || !bindData(alpha,alphaSprite) ||
       alphaSprite.object->usWidth!=sprite.object->usWidth || alphaSprite.object->usHeight!=sprite.object->usHeight)) return FALSE;
    auto alias=[&](const void* p,std::size_t n) {
        return overlap(p,n,color.data,color.bytes) || overlap(p,n,z.data,z.bytes);
    };
    if(alias(source,sizeof(*source)) || alias(source->pETRLEObject,std::size_t(source->usNumberOfObjects)*sizeof(ETRLEObject)) ||
       alias(sprite.data,sprite.length) || alias(palette,256*sizeof(UINT16)) || alias(ShadeTable,sizeof(ShadeTable))) return FALSE;
    if(alpha && (alias(alpha,sizeof(*alpha)) || alias(alpha->pETRLEObject,std::size_t(alpha->usNumberOfObjects)*sizeof(ETRLEObject)) ||
                 alias(alphaSprite.data,alphaSprite.length))) return FALSE;
    if(!sgp::etrle::ValidateEncoding(sprite)) return FALSE;
    std::vector<std::size_t> shifts;
    try { if(alpha && !alphaAlignment(sprite,alphaSprite,clip,t,b,l,shifts)) return FALSE; }
    catch(const std::bad_alloc&) { return FALSE; }
    Draw draw{color,z,palette,&sprite,alpha?alphaSprite.data:nullptr,&shifts,l,t,r,b,depth,policy,ignore!=0};
    sgp::etrle::ForEachLiteralRun(sprite,drawRun,&draw);return TRUE;
}
}
#define P_ARGS UINT16* b,UINT32 p,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define A_ARGS UINT16* b,UINT32 p,HVOBJECT s,HVOBJECT a,INT32 x,INT32 y,UINT16 i
#define Z_ARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define ZA_ARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,HVOBJECT a,INT32 x,INT32 y,UINT16 i
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadow(P_ARGS,UINT16* pal,BOOLEAN ig) { return blit(b,p,nullptr,0,s,nullptr,x,y,i,nullptr,pal,ig,Policy::Plain); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowAlpha(A_ARGS,UINT16* pal,BOOLEAN ig) { if(!a) return FALSE; return blit(b,p,nullptr,0,s,a,x,y,i,nullptr,pal,ig,Policy::Plain); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowClip(P_ARGS,SGPRect* c,UINT16* pal,BOOLEAN ig) { return blit(b,p,nullptr,0,s,nullptr,x,y,i,c?c:&ClippingRect,pal,ig,Policy::Plain); }
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowClipAlpha(A_ARGS,SGPRect* c,UINT16* pal,BOOLEAN ig) { if(!a) return FALSE; return blit(b,p,nullptr,0,s,a,x,y,i,c?c:&ClippingRect,pal,ig,Policy::Plain); }
#define Z_IMPL(name,policy) \
BOOLEAN name(Z_ARGS,UINT16* pal,BOOLEAN ig) { return blit(b,p,z,d,s,nullptr,x,y,i,nullptr,pal,ig,policy); }
#define ZA_IMPL(name,policy) \
BOOLEAN name(ZA_ARGS,UINT16* pal,BOOLEAN ig) { if(!a) return FALSE; return blit(b,p,z,d,s,a,x,y,i,nullptr,pal,ig,policy); }
#define ZC_IMPL(name,policy) \
BOOLEAN name(Z_ARGS,SGPRect* c,UINT16* pal,BOOLEAN ig) { return blit(b,p,z,d,s,nullptr,x,y,i,c?c:&ClippingRect,pal,ig,policy); }
#define ZAC_IMPL(name,policy) \
BOOLEAN name(ZA_ARGS,SGPRect* c,UINT16* pal,BOOLEAN ig) { if(!a) return FALSE; return blit(b,p,z,d,s,a,x,y,i,c?c:&ClippingRect,pal,ig,policy); }
Z_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZ,Policy::WriteZ)
ZA_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZAlpha,Policy::WriteZ)
ZC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZClip,Policy::WriteZ)
ZAC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZClipAlpha,Policy::WriteZ)
Z_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNB,Policy::NoWriteZ)
ZA_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBAlpha,Policy::NoWriteZ)
ZC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBClip,Policy::NoWriteZ)
ZAC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBClipAlpha,Policy::NoWriteZ)
Z_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBObscured,Policy::Obscured)
Z_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredTest,Policy::ObscuredTest)
ZA_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredAlpha,Policy::Obscured)
ZC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip,Policy::Obscured)
ZAC_IMPL(Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClipAlpha,Policy::Obscured)
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowBelowOrEqualZNBClip(Z_ARGS,SGPRect* c,UINT16* pal) { return blit(b,p,z,d,s,nullptr,x,y,i,c?c:&ClippingRect,pal,FALSE,Policy::NoWriteZ); }
#undef P_ARGS
#undef A_ARGS
#undef Z_ARGS
#undef ZA_ARGS
#undef Z_IMPL
#undef ZA_IMPL
#undef ZC_IMPL
#undef ZAC_IMPL
