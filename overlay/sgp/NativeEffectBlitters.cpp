/*
 * P710 native snapshot update: sgp/NativeEffectBlitters.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeEffectBlitters.h"
#include "native_background.h"
#include "shading.h"
#include <algorithm>
#include <cstdint>
#include <cstring>

// Scalar translations of sgp/vobject_blitters.cpp at 3394695a387a14c78e92b158cfdcf50e4068c96b.
// Original engine source license applies. See EFFECT-IMPLEMENTATION.md for quirks.
extern UINT32 guiTranslucentMask;
namespace {
using I=std::int64_t;
using sgp::BackgroundView;
enum class Effect { Translucent, Pixelate, Obscured, Shadow, Intensity,
    Mono, MonoClip, Mirror, OutlineShadow, OutlineShadowClip, OutlineObscured, Land };
bool overlap(const void* a,std::size_t an,const void* b,std::size_t bn) {
    auto av=reinterpret_cast<std::uintptr_t>(a),bv=reinterpret_cast<std::uintptr_t>(b);
    return an && bn && (av<=bv ? bv-av<an : av-bv<bn);
}
UINT16 load(const BYTE* p) {UINT16 v;std::memcpy(&v,p,2);return v;}
void store(BYTE* p,UINT16 v) {std::memcpy(p,&v,2);}
struct Draw {
    BackgroundView color,z;
    sgp::etrle::Sprite sprite{};
    HVOBJECT source=nullptr;
    I left=0,top=0,right=0,bottom=0;
    Effect effect=Effect::Shadow;
    UINT16 depth=0,foreground=0,background=0,shadow=0,tint=0;
    bool writeZ=false,clipped=false,outline=false,flat=false,valid=true,preflight=false;
    const INT16* patch=nullptr;
};
bool aliases(const Draw& d,const void* p,std::size_t size) {
    return overlap(p,size,d.color.data,d.color.bytes) || (d.z.data && overlap(p,size,d.z.data,d.z.bytes));
}
// Same lattice, floor division, clamping, rounded blend and bank numbering as
// original GradShadeLevel. Refuse its out-of-patch corner cases before writes.
int gradient(const INT16* p,I dx,I dy) {
    I a=dx+2*dy-40,b=2*dy-dx;
    I ix=std::clamp<I>(a>=0?a/40:(a-39)/40,-1,2);
    I iy=std::clamp<I>(b>=0?b/40:(b-39)/40,-1,2);
    I u=std::clamp<I>(a-40*ix,0,40),v=std::clamp<I>(b-40*iy,0,40);
    I corner=(ix+1)*4+(iy+1);
    if(corner+5>=16) return -1;
    I blend=(40-u)*(40-v)*p[corner]+u*(40-v)*p[corner+4]+(40-u)*v*p[corner+1]+u*v*p[corner+5];
    return int(std::clamp<I>((blend+800)/1600,0,15));
}
void run(void* ctx,UINT32 row,UINT32 column,const UINT8* pixels,UINT32 count) {
    auto& d=*static_cast<Draw*>(ctx);const I y=d.sprite.y+row;
    if(y<d.top || y>=d.bottom)return;
    for(UINT32 j=0;j<count;++j) {
        I col=I(column)+j;
        I x=d.effect==Effect::Mirror ? d.right-1-col : d.sprite.x+col;
        if(x<d.left || x>=d.right)continue;
        const UINT8 index=pixels[j];
        const auto off=std::size_t(y)*d.color.pitch+std::size_t(x)*2;
        BYTE* dest=d.color.data+off;BYTE* z=d.z.data?d.z.data+off:nullptr;
        const UINT16 oldZ=z?load(z):0;
        const UINT16* palette=d.source->pShadeCurrent;
        if(d.effect==Effect::Land) {
            int bank=d.flat?d.patch[5]:gradient(d.patch,col+d.sprite.object->sOffsetX,I(row)+d.sprite.object->sOffsetY);
            if(bank<0 || (bank|d.tint)>=HVOBJECT_SHADE_TABLES) {d.valid=false;continue;}
            palette=d.source->pShades[bank|d.tint];
            if(!palette || aliases(d,palette,256*sizeof(UINT16))) {d.valid=false;continue;}
        }
        if(d.preflight)continue;
        // Checker is actual byte-address bit 1 compared with original line flag.
        // Most clipped kernels do NOT toggle the flag for top-skipped source rows.
        I parityY=y;
        if(d.clipped && (d.effect==Effect::Pixelate || d.effect==Effect::OutlineObscured))parityY-=d.top-d.sprite.y;
        const bool checker=bool(reinterpret_cast<std::uintptr_t>(dest)&2)==bool(parityY&1);
        bool ordinary=z?oldZ<=d.depth:true;
        if(d.effect==Effect::Shadow || d.effect==Effect::Intensity)ordinary=!z || oldZ<d.depth;
        if(d.effect==Effect::Pixelate && !checker)continue;
        if(d.effect==Effect::Obscured) {
            ordinary=oldZ<d.depth;
            if(!ordinary && !checker)continue;
        } else if(d.effect==Effect::OutlineObscured) {
            ordinary=d.clipped?oldZ<=d.depth:oldZ<d.depth;
            if(!ordinary && !checker)continue;
        } else if(!ordinary)continue;
        if(z && d.writeZ && ordinary)store(z,d.depth);
        UINT16 value=0;
        switch(d.effect) {
        case Effect::Shadow: case Effect::OutlineShadowClip:value=ShadeTable[load(dest)];break;
        case Effect::Intensity:value=IntensityTable[load(dest)];break;
        case Effect::OutlineShadow:if(index==254)continue;value=ShadeTable[load(dest)];break;
        case Effect::Translucent:value=UINT16(((palette[index]>>1)&guiTranslucentMask)+((load(dest)>>1)&guiTranslucentMask));break;
        case Effect::Mono:if(index>=1)value=0;else {if(!d.background)continue;value=d.background;}break;
        case Effect::MonoClip:
            if(index==1) {if(!d.shadow)continue;value=d.shadow;}
            else if(index) value=d.foreground;
            else {if(!d.background)continue;value=d.background;}
            break;
        case Effect::OutlineObscured:
            if(index==254) {if(!d.outline)continue;value=d.foreground;}
            else {
                // Clip's second MOV [EBX],AX occurs after MOV AL,index.
                if(d.clipped)store(z,UINT16((d.depth&0xff00u)|index));
                value=palette[index];
            }
            break;
        default:value=palette[index];break;
        }
        store(dest,value);
    }
}
BOOLEAN blit(UINT16* b,UINT32 pitch,UINT16* z,UINT16 depth,HVOBJECT source,INT32 x,INT32 y,UINT16 index,
    const SGPRect* clip,Effect effect,bool writeZ=false,UINT16 foreground=0,UINT16 background=0,UINT16 shadow=0,
    BOOLEAN outline=FALSE,const INT16* patch=nullptr,UINT16 tint=0) {
    Draw d;d.source=source;d.effect=effect;d.depth=depth;d.writeZ=writeZ;d.clipped=clip!=nullptr;
    d.foreground=foreground;d.background=background;d.shadow=shadow;d.outline=outline==TRUE;d.patch=patch;d.tint=tint;
    if(!sgp::BackgroundBufferView(b,&d.color) || !sgp::ValidBackgroundView(d.color) || d.color.pitch!=pitch ||
       !sgp::etrle::DescribeSprite(source,index,x,y,d.sprite,clip!=nullptr))return FALSE;
    if(z && (!sgp::BackgroundBufferView(z,&d.z) || !sgp::ValidBackgroundView(d.z) || d.z.pitch!=pitch ||
       d.z.width!=d.color.width || d.z.height!=d.color.height || overlap(d.color.data,d.color.bytes,d.z.data,d.z.bytes)))return FALSE;
    if(!z && effect!=Effect::Land && (effect==Effect::Translucent || effect==Effect::Pixelate || effect==Effect::Obscured || effect==Effect::OutlineObscured || writeZ))return FALSE;
    if(effect==Effect::Mirror)d.sprite.x=I(x)-d.sprite.object->sOffsetX;
    d.left=d.sprite.x;d.top=d.sprite.y;d.right=d.left+d.sprite.object->usWidth;d.bottom=d.top+d.sprite.object->usHeight;
    if(clip) {
        d.left=std::max<I>(d.left,clip->iLeft);d.top=std::max<I>(d.top,clip->iTop);
        d.right=std::min<I>(d.right,clip->iRight);d.bottom=std::min<I>(d.bottom,clip->iBottom);
        if(d.left>=d.right || d.top>=d.bottom)return TRUE;
    }
    if(d.left<0 || d.top<0 || d.right>d.color.width || d.bottom>d.color.height ||
       !sgp::etrle::ValidDestination(pitch,d.left,d.top,d.right,d.bottom))return FALSE;
    const bool usesCurrent=effect==Effect::Translucent || effect==Effect::Pixelate ||
        effect==Effect::Obscured || effect==Effect::OutlineObscured || effect==Effect::Mirror;
    if(usesCurrent) {
        if(!sgp::etrle::BindSource(source,d.sprite))return FALSE;
    } else {
        // Mask/mono/land kernels do not read pShadeCurrent. Bind the same actual
        // declared byte range without imposing a palette they never consume.
        const auto& object=*d.sprite.object;
        if(!source->pPixData || !object.uiDataLength || object.uiDataOffset>source->uiSizePixData ||
           object.uiDataLength>source->uiSizePixData-object.uiDataOffset)return FALSE;
        d.sprite.data=static_cast<const UINT8*>(source->pPixData)+object.uiDataOffset;
        d.sprite.length=object.uiDataLength;
    }
    if(aliases(d,source,sizeof(*source)) || aliases(d,source->pETRLEObject,std::size_t(source->usNumberOfObjects)*sizeof(ETRLEObject)) ||
       aliases(d,d.sprite.data,d.sprite.length) || (usesCurrent && aliases(d,source->pShadeCurrent,256*sizeof(UINT16))) ||
       !sgp::etrle::ValidateEncoding(d.sprite))return FALSE;
    if(effect==Effect::Shadow || effect==Effect::OutlineShadow || effect==Effect::OutlineShadowClip) {
        if(aliases(d,ShadeTable,sizeof(ShadeTable)))return FALSE;
    }
    if(effect==Effect::Intensity && aliases(d,IntensityTable,sizeof(IntensityTable)))return FALSE;
    if(effect==Effect::Land) {
        if(!patch || aliases(d,patch,16*sizeof(INT16)))return FALSE;
        d.flat=true;for(int j=1;j<16;++j)if(patch[j]!=patch[0])d.flat=false;
        d.preflight=true;sgp::etrle::ForEachLiteralRun(d.sprite,run,&d);d.preflight=false;
        if(!d.valid)return FALSE;
    }
    // Only transparent runs fill background: a zero shadow/zero background
    // means preserve the previous destination even beside background fills.
    if((effect==Effect::Mono || effect==Effect::MonoClip) && background) {
        std::size_t cursor=0;
        for(UINT32 row=0;row<d.sprite.object->usHeight;++row) {
            I col=0;I yy=d.sprite.y+row;
            for(;;) {
                UINT8 control=d.sprite.data[cursor++];if(!control)break;
                I count=control&0x7f;
                if(control&0x80) {
                    if(yy>=d.top && yy<d.bottom)
                        for(I xx=std::max(d.left,d.sprite.x+col);xx<std::min(d.right,d.sprite.x+col+count);++xx)
                            store(d.color.data+std::size_t(yy)*pitch+std::size_t(xx)*2,background);
                } else cursor+=std::size_t(count);
                col+=count;
            }
        }
    }
    sgp::etrle::ForEachLiteralRun(d.sprite,run,&d);return TRUE;
}
const SGPRect* clipping(SGPRect* c) {return c?c:&ClippingRect;}
}
#define ZARGS UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define ARGS UINT16* b,UINT32 p,HVOBJECT s,INT32 x,INT32 y,UINT16 i
#define ZPASS b,p,z,d,s,x,y,i
#define PASS b,p,nullptr,0,s,x,y,i
#define ZFUNCTION(name,effect,write) \
BOOLEAN Blt8BPPDataTo16BPPBuffer##name(ZARGS) {return z?blit(ZPASS,nullptr,Effect::effect,write):FALSE;}
#define ZCLIPFUNCTION(name,effect,write) \
BOOLEAN Blt8BPPDataTo16BPPBuffer##name(ZARGS,SGPRect* c) {return z?blit(ZPASS,clipping(c),Effect::effect,write):FALSE;}
ZFUNCTION(TransZTranslucent,Translucent,true)
ZFUNCTION(TransZNBTranslucent,Translucent,false)
ZCLIPFUNCTION(TransZClipTranslucent,Translucent,true)
ZCLIPFUNCTION(TransZNBClipTranslucent,Translucent,false)
ZFUNCTION(TransZPixelate,Pixelate,true)
ZFUNCTION(TransZNBPixelate,Pixelate,false)
ZCLIPFUNCTION(TransZClipPixelate,Pixelate,true)
ZCLIPFUNCTION(TransZNBClipPixelate,Pixelate,false)
ZFUNCTION(TransZPixelateObscured,Obscured,true)
ZCLIPFUNCTION(TransZClipPixelateObscured,Obscured,true)
ZFUNCTION(ShadowZ,Shadow,true)
ZFUNCTION(ShadowZNB,Shadow,false)
ZCLIPFUNCTION(ShadowZClip,Shadow,true)
ZCLIPFUNCTION(ShadowZNBClip,Shadow,false)
ZFUNCTION(IntensityZ,Intensity,true)
ZFUNCTION(IntensityZNB,Intensity,false)
ZCLIPFUNCTION(IntensityZClip,Intensity,true)
ZCLIPFUNCTION(IntensityZNBClip,Intensity,false)
#define FUNCTION(name,effect) \
BOOLEAN Blt8BPPDataTo16BPPBuffer##name(ARGS) {return blit(PASS,nullptr,Effect::effect);}
#define CLIPFUNCTION(name,effect) \
BOOLEAN Blt8BPPDataTo16BPPBuffer##name(ARGS,SGPRect* c) {return blit(PASS,clipping(c),Effect::effect);}
FUNCTION(Shadow,Shadow)
CLIPFUNCTION(ShadowClip,Shadow)
FUNCTION(Intensity,Intensity)
CLIPFUNCTION(IntensityClip,Intensity)
FUNCTION(OutlineShadow,OutlineShadow)
CLIPFUNCTION(OutlineShadowClip,OutlineShadowClip)
FUNCTION(TransMirror,Mirror)
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadow(ARGS,UINT16 fg,UINT16 bg) {return blit(PASS,nullptr,Effect::Mono,false,fg,bg);}
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadowClip(ARGS,SGPRect* c,UINT16 fg,UINT16 bg,UINT16 sh) {return blit(PASS,clipping(c),Effect::MonoClip,false,fg,bg,sh);}
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(ZARGS,INT16 color,BOOLEAN outline) {return z?blit(ZPASS,nullptr,Effect::OutlineObscured,true,UINT16(color),0,0,outline):FALSE;}
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(ZARGS,INT16 color,BOOLEAN outline,SGPRect* c) {return z?blit(ZPASS,clipping(c),Effect::OutlineObscured,true,UINT16(color),0,0,outline):FALSE;}
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClipLandGrad(UINT16* b,UINT32 p,UINT16* z,UINT16 d,HVOBJECT s,const INT16* patch,UINT16 tint,INT32 x,INT32 y,UINT16 i,SGPRect* c,BOOLEAN write) {return blit(ZPASS,clipping(c),Effect::Land,write!=FALSE,0,0,0,FALSE,patch,tint);}
#undef ZFUNCTION
#undef ZCLIPFUNCTION
#undef FUNCTION
#undef CLIPFUNCTION
#undef ZARGS
#undef ZPASS
#undef ARGS
#undef PASS
