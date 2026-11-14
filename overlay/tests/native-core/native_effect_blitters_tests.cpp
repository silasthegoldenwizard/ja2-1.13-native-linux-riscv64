/*
 * P710 native snapshot update: tests/native-core/native_effect_blitters_tests.cpp
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
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool b,const char* why) {++checks;if(!b)throw std::runtime_error(why);}
UINT16 read(BYTE* p,std::size_t o) {UINT16 v;std::memcpy(&v,p+o,2);return v;}
void write(BYTE* p,std::size_t o,UINT16 v) {std::memcpy(p+o,&v,2);}
UINT32 surface(UINT32 w,UINT32 h) {VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=w;d.usHeight=h;d.ubBitDepth=16;UINT32 id=0;check(AddVideoSurface(&d,&id),"actual surface create");return id;}
struct Fixture {
    std::array<UINT16,256> pal{};
    std::array<std::array<UINT16,256>,48> shades{};
    std::vector<UINT8> bytes{4,0,1,2,254,0x81,1,3,0,4,4,5,6,254,0x81,1,7,0};
    ETRLEObject object{};SGPVObject source{};
    Fixture() {
        for(unsigned i=0;i<256;++i)pal[i]=UINT16(0x4000+i);
        for(unsigned bank=0;bank<48;++bank) {
            for(unsigned i=0;i<256;++i)shades[bank][i]=UINT16(0x1000+bank*256+i);
            source.pShades[bank]=shades[bank].data();
        }
        object.usWidth=6;object.usHeight=2;object.uiDataLength=bytes.size();
        source.ubBitDepth=8;source.usNumberOfObjects=1;source.pETRLEObject=&object;
        source.pPixData=bytes.data();source.uiSizePixData=bytes.size();source.pShadeCurrent=pal.data();
    }
};
void test() {
    check(InitializeVideoSurfaceManager(),"manager init");
    UINT32 ids[]{surface(10,4),surface(10,4),surface(10,4),surface(2,2)};
    check(sgp::BindReservedVideoSurfaces(ids),"real framebuffer binding");
    UINT32 pitch=0;BYTE* initial=LockVideoSurface(FRAME_BUFFER,&pitch);check(initial,"initial pitch lock");UnLockVideoSurface(FRAME_BUFFER);
    auto* z=InitZBuffer(pitch,4);check(z,"owned Z");
    BYTE* color=sgp::LockBackgroundSurface(FRAME_BUFFER,&pitch);check(color,"frame lock");
    auto* b=reinterpret_cast<UINT16*>(color);
    sgp::BackgroundView cv,zv;check(sgp::BackgroundBufferView(b,&cv)&&sgp::BackgroundBufferView(z,&zv),"actual bounds");
    check((reinterpret_cast<std::uintptr_t>(color)&3)==0,"literal parity fixture aligned at column zero");
    Fixture f;SGPRect full{0,0,10,4};const UINT16 depth=0x1234;
    auto reset=[&] {std::memset(color,0x22,cv.bytes);std::memset(zv.data,0xa5,zv.bytes);for(unsigned yy=0;yy<4;++yy)for(unsigned xx=0;xx<10;++xx)write(zv.data,yy*pitch+xx*2,depth-1);};
    auto px=[&](unsigned x,unsigned y){return read(color,y*pitch+x*2);};
    auto zp=[&](unsigned x,unsigned y){return read(zv.data,y*pitch+x*2);};
    auto setz=[&](unsigned x,unsigned y,UINT16 v){write(zv.data,y*pitch+x*2,v);};
    using ZFn=BOOLEAN(*)(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16);
    using ZClipFn=BOOLEAN(*)(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,SGPRect*);
    ZFn trans[]{Blt8BPPDataTo16BPPBufferTransZTranslucent,Blt8BPPDataTo16BPPBufferTransZNBTranslucent};
    ZClipFn transc[]{Blt8BPPDataTo16BPPBufferTransZClipTranslucent,Blt8BPPDataTo16BPPBufferTransZNBClipTranslucent};
    for(unsigned mode=0;mode<4;++mode) {
        reset();setz(2,0,depth);setz(3,0,depth+1);
        check(mode<2?trans[mode](b,pitch,z,depth,&f.source,0,0,0):transc[mode-2](b,pitch,z,depth,&f.source,0,0,0,&full),"all translucent exports");
        // Independent halves with historical 0x3def (not conventional RGB565 average).
        check(px(0,0)==0x3101&&px(1,0)==0x3101&&px(2,0)==0x3102,"literal masked translucent colors including zero/equal depth");
        check(px(3,0)==0x2222&&px(4,0)==0x2222,"failed depth and transparent skip");
        check(zp(0,0)==((mode%2)==0?depth:depth-1),"translucent write/no-write Z");
    }
    ZFn pix[]{Blt8BPPDataTo16BPPBufferTransZPixelate,Blt8BPPDataTo16BPPBufferTransZNBPixelate};
    ZClipFn pixc[]{Blt8BPPDataTo16BPPBufferTransZClipPixelate,Blt8BPPDataTo16BPPBufferTransZNBClipPixelate};
    for(unsigned mode=0;mode<4;++mode) {
        reset();setz(2,0,depth);
        check(mode<2?pix[mode](b,pitch,z,depth,&f.source,0,0,0):pixc[mode-2](b,pitch,z,depth,&f.source,0,0,0,&full),"all pixelate exports");
        check(px(0,0)==0x4000&&px(1,0)==0x2222&&px(2,0)==0x4002&&px(3,0)==0x2222,"even physical-address checker draws equal depth");
        check(px(0,1)==0x2222&&px(1,1)==0x4005&&px(3,1)==0x40fe&&px(5,1)==0x4007,"odd row checker and index254 palette");
        check(zp(0,0)==(mode%2==0?depth:depth-1)&&zp(1,0)==depth-1,"checker Z updates only drawn positions");
    }
    for(unsigned mode=0;mode<2;++mode) {
        reset();setz(1,0,depth);setz(2,0,depth+1);setz(3,0,depth+1);
        check(mode?Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured(b,pitch,z,depth,&f.source,0,0,0,&full):Blt8BPPDataTo16BPPBufferTransZPixelateObscured(b,pitch,z,depth,&f.source,0,0,0),"both obscured exports");
        check(px(0,0)==0x4000&&px(1,0)==0x2222&&px(2,0)==0x4002&&px(3,0)==0x2222,"obscured strict depth then checker");
        check(zp(0,0)==depth&&zp(2,0)==depth+1,"obscured checker keeps old depth");
    }
    // Table content is supplied fixture input; the real production globals and
    // provider perform each lookup. No substitute provider or expected-value loop.
    ShadeTable[0x2222]=0x1111;IntensityTable[0x2222]=0x3333;
    ZFn shade[]{Blt8BPPDataTo16BPPBufferShadowZ,Blt8BPPDataTo16BPPBufferShadowZNB,Blt8BPPDataTo16BPPBufferIntensityZ,Blt8BPPDataTo16BPPBufferIntensityZNB};
    ZClipFn shadec[]{Blt8BPPDataTo16BPPBufferShadowZClip,Blt8BPPDataTo16BPPBufferShadowZNBClip,Blt8BPPDataTo16BPPBufferIntensityZClip,Blt8BPPDataTo16BPPBufferIntensityZNBClip};
    for(unsigned mode=0;mode<8;++mode) {
        reset();setz(1,0,depth);setz(2,0,depth+1);
        check(mode<4?shade[mode](b,pitch,z,depth,&f.source,0,0,0):shadec[mode-4](b,pitch,z,depth,&f.source,0,0,0,&full),"all shadow/intensity Z exports");
        check(px(0,0)==((mode%4)<2?0x1111:0x3333)&&px(1,0)==0x2222&&px(2,0)==0x2222,"mask shades literal zero; strict equality refusal");
        check(px(3,0)==((mode%4)<2?0x1111:0x3333)&&px(4,0)==0x2222,"table mask includes index254 excludes skip");
        check(zp(0,0)==((mode%2)==0?depth:depth-1),"table Z policy");
    }
    using Fn=BOOLEAN(*)(UINT16*,UINT32,HVOBJECT,INT32,INT32,UINT16);
    using ClipFn=BOOLEAN(*)(UINT16*,UINT32,HVOBJECT,INT32,INT32,UINT16,SGPRect*);
    Fn mask[]{Blt8BPPDataTo16BPPBufferShadow,Blt8BPPDataTo16BPPBufferIntensity,Blt8BPPDataTo16BPPBufferOutlineShadow};
    ClipFn maskc[]{Blt8BPPDataTo16BPPBufferShadowClip,Blt8BPPDataTo16BPPBufferIntensityClip,Blt8BPPDataTo16BPPBufferOutlineShadowClip};
    for(unsigned mode=0;mode<6;++mode) {
        reset();check(mode<3?mask[mode](b,pitch,&f.source,0,0,0):maskc[mode-3](b,pitch,&f.source,0,0,0,&full),"all no-Z mask exports");
        check(px(0,0)==(mode%3==1?0x3333:0x1111)&&px(4,0)==0x2222,"table output and transparent skip");
        check(px(3,0)==(mode==2?0x2222:mode%3==1?0x3333:0x1111),"OutlineShadowClip shades index254 unlike nonclip");
    }
    reset();check(Blt8BPPDataTo16BPPBufferMonoShadow(b,pitch,&f.source,0,0,0,0xbeef,0xabcd),"mono original");
    check(px(0,0)==0xabcd&&px(1,0)==0&&px(2,0)==0&&px(3,0)==0&&px(4,0)==0xabcd,"mono JB quirk makes every nonzero literal black, skips background filled");
    reset();check(Blt8BPPDataTo16BPPBufferMonoShadowClip(b,pitch,&f.source,0,0,0,&full,0xbeef,0xabcd,0),"mono clipped");
    check(px(0,0)==0xabcd&&px(1,0)==0x2222&&px(2,0)==0xbeef&&px(3,0)==0xbeef&&px(4,0)==0xabcd,"clipped zero shadow preserves amid background fill");
    reset();check(Blt8BPPDataTo16BPPBufferMonoShadowClip(b,pitch,&f.source,0,0,0,&full,0,0,0xdead),"mono zero foreground and background");
    check(px(0,0)==0x2222&&px(1,0)==0xdead&&px(2,0)==0&&px(4,0)==0x2222,"exact optional mono colors");
    reset();f.object.sOffsetX=1;
    check(Blt8BPPDataTo16BPPBufferTransMirror(b,pitch,&f.source,2,0,0),"mirror with original negative offset placement");
    check(px(1,0)==0x4003&&px(2,0)==0x2222&&px(3,0)==0x40fe&&px(4,0)==0x4002&&px(5,0)==0x4001&&px(6,0)==0x4000,"literal mirrored samples and transparency");f.object.sOffsetX=0;
    for(unsigned mode=0;mode<2;++mode)for(BOOLEAN outline:{BOOLEAN(FALSE),BOOLEAN(TRUE),BOOLEAN(2)}) {
        reset();setz(0,0,depth);setz(2,0,depth+1);
        check(mode?Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(b,pitch,z,depth,&f.source,0,0,0,INT16(0xbeef),outline,&full):Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(b,pitch,z,depth,&f.source,0,0,0,INT16(0xbeef),outline),"outline obscured exports flags");
        check(px(3,0)==(outline==TRUE?0xbeef:0x2222)&&zp(3,0)==depth,"near outline claims depth even when disabled");
        check(px(2,0)==0x4002&&zp(2,0)==(mode?0x1202:depth+1),"clip AX mutation also writes depth for obscured nonoutline checker");
        check(zp(0,0)==(mode?0x1200:depth),"equal outline depth clip distinction");
    }
    reset();setz(3,0,depth);
    check(Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured(b,pitch,z,depth,&f.source,0,0,0,INT16(0xbeef),TRUE),"unclipped equal outline checker miss");
    check(px(3,0)==0x2222,"unclipped equal depth is checker-only");
    reset();setz(3,0,depth);
    check(Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip(b,pitch,z,depth,&f.source,0,0,0,INT16(0xbeef),TRUE,&full),"clipped equal outline draws");
    check(px(3,0)==0xbeef,"clip admits equal depth fully");
    reset();std::array<INT16,16> patch{};patch.fill(4);
    check(Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,z,depth,&f.source,patch.data(),16,0,0,0,&full,TRUE),"flat tinted land");
    check(px(0,0)==0x2400&&px(3,0)==0x24fe&&px(4,0)==0x2222&&zp(0,0)==depth,"flat bank20 palette and depth");
    reset();patch.fill(0);patch[1]=8;patch[4]=8;patch[5]=12;
    check(Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,nullptr,depth,&f.source,patch.data(),0,0,0,0,&full,FALSE),"nonflat land without optional Z");
    check(px(0,0)==0x1800&&px(1,0)==0x1801&&px(2,0)==0x1802,"lattice at row0 uses bank8 with independent rounded corner samples");
    reset();f.object.sOffsetX=20;f.object.sOffsetY=10;
    check(Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,z,depth,&f.source,patch.data(),0,-20,-10,0,&full,FALSE),"land gradient uses object offsets from anchor");
    check(px(0,0)==0x1c00&&zp(0,0)==depth-1,"own lattice cell bank12; no-write depth");
    f.object.sOffsetX=f.object.sOffsetY=0;
    reset();auto* current=f.source.pShadeCurrent;f.source.pShadeCurrent=nullptr;
    check(Blt8BPPDataTo16BPPBufferShadow(b,pitch,&f.source,0,0,0),"mask needs no unused current palette");
    check(px(0,0)==0x1111,"palette-free mask actual output");
    reset();patch.fill(4);
    check(Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,nullptr,depth,&f.source,patch.data(),0,0,0,0,&full,FALSE),"land uses shade banks without current palette");
    check(px(0,0)==0x1400,"palette-free land bank4 output");
    f.source.pShadeCurrent=current;
    // Clipped ordinary checker retains old initial Y; obscured checker toggles
    // top-skipped rows. Literal zero in row0 prevents byte-scanning skips.
    reset();SGPRect strip{1,1,4,2};SetClippingRect(&strip);
    check(Blt8BPPDataTo16BPPBufferTransZNBClipPixelate(b,pitch,z,depth,&f.source,0,0,0,nullptr),"top/left/right clipping and global clip");
    check(px(1,1)==0x2222&&px(2,1)==0x4006&&px(3,1)==0x2222&&px(0,1)==0x2222,"ordinary clip retains parity without top skip toggle");
    reset();
    check(Blt8BPPDataTo16BPPBufferOutlineShadowClip(b,pitch,&f.source,0,0,0,&strip),"outline shadow top clipping decodes structural runs");
    check(px(1,1)==0x1111&&px(2,1)==0x1111&&px(3,1)==0x1111&&px(0,1)==0x2222&&px(1,0)==0x2222,"top-clipped row includes254 and preserves all outside edges");
    // Original OutlineShadowClip top skip replaces the run length with its
    // first payload byte. Row0's literal0 triggers its desynchronization bug;
    // native deliberately follows validated ETRLE row boundaries instead.
    reset();setz(1,1,depth+1);setz(2,1,depth+1);setz(3,1,depth+1);
    check(Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured(b,pitch,z,depth,&f.source,0,0,0,&strip),"clipped obscured parity");
    check(px(1,1)==0x4005&&px(2,1)==0x2222&&px(3,1)==0x40fe,"obscured clipped top skip toggles row");
    reset();const std::vector<BYTE> before(color,color+cv.bytes),beforeZ(zv.data,zv.data+zv.bytes);
    auto unchanged=[&] {check(!std::memcmp(color,before.data(),cv.bytes)&&!std::memcmp(zv.data,beforeZ.data(),zv.bytes),"invalid input cannot partially write");};
    f.bytes.back()=7;check(!Blt8BPPDataTo16BPPBufferShadow(b,pitch,&f.source,0,0,0),"malformed later row");unchanged();f.bytes.back()=0;
    f.object.uiDataLength--;check(!Blt8BPPDataTo16BPPBufferMonoShadow(b,pitch,&f.source,0,0,0,1,2),"truncation before transparent fill");unchanged();f.object.uiDataLength++;
    check(!Blt8BPPDataTo16BPPBufferTransZPixelate(b,pitch,b,depth,&f.source,0,0,0),"Z/color alias");unchanged();
    check(!Blt8BPPDataTo16BPPBufferIntensity(b,pitch,&f.source,8,0,0),"actual width not pitch padding");unchanged();
    check(!Blt8BPPDataTo16BPPBufferShadowZ(b,pitch+2,z,depth,&f.source,0,0,0),"actual pitch");unchanged();
    UINT16 foreign[80]{};check(!Blt8BPPDataTo16BPPBufferShadow(foreign,20,&f.source,0,0,0),"foreign destination");unchanged();
    auto* save=f.source.pPixData;f.source.pPixData=color;f.source.uiSizePixData=cv.bytes;
    check(!Blt8BPPDataTo16BPPBufferShadow(b,pitch,&f.source,0,0,0),"encoded stream alias");unchanged();f.source.pPixData=save;f.source.uiSizePixData=f.bytes.size();
    auto* pal=f.source.pShadeCurrent;f.source.pShadeCurrent=b;
    check(!Blt8BPPDataTo16BPPBufferTransMirror(b,pitch,&f.source,0,0,0),"palette alias");unchanged();f.source.pShadeCurrent=pal;
    patch.fill(47);check(!Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,z,depth,&f.source,patch.data(),16,0,0,0,&full,TRUE),"invalid bank OR rejected");unchanged();
    patch.fill(0);patch[0]=1;f.object.sOffsetX=200;SGPRect invalid{0,0,400,4};
    check(!Blt8BPPDataTo16BPPBufferTransZClipLandGrad(b,pitch,z,depth,&f.source,patch.data(),0,-200,0,0,&invalid,TRUE),"unsafe original gradient corner rejected");unchanged();f.object.sOffsetX=0;
    SGPRect off{50,50,60,60};f.bytes.back()=7;
    check(Blt8BPPDataTo16BPPBufferShadowClip(b,pitch,&f.source,0,0,0,&off),"fully clipped keeps no-decode early success");unchanged();f.bytes.back()=0;
    auto* blankData=f.source.pPixData;auto* blankPalette=f.source.pShadeCurrent;
    f.source.pPixData=nullptr;f.source.pShadeCurrent=nullptr;
    for(const auto dimensions:{std::array<UINT16,2>{0,0},std::array<UINT16,2>{0,2},std::array<UINT16,2>{5,0}}) {
        f.object.usWidth=dimensions[0];f.object.usHeight=dimensions[1];
        check(Blt8BPPDataTo16BPPBufferShadowClip(b,pitch,&f.source,0,0,0,&full),"blank clipped mask succeeds before payload bind");unchanged();
        check(Blt8BPPDataTo16BPPBufferTransZClipPixelate(b,pitch,z,depth,&f.source,0,0,0,&full),"blank clipped effect succeeds before palette bind");unchanged();
    }
    f.object.usWidth=5;f.object.usHeight=2;f.source.pPixData=blankData;f.source.pShadeCurrent=blankPalette;
    // Independent padding/boundary checks for all invalid requests.
    for(unsigned yy=0;yy<4;++yy)for(unsigned o=20;o<pitch;++o)check(color[yy*pitch+o]==0x22&&zv.data[yy*pitch+o]==0xa5,"row padding unchanged");
    sgp::UnlockBackgroundSurface(FRAME_BUFFER);
    check(!Blt8BPPDataTo16BPPBufferShadow(b,pitch,&f.source,0,0,0),"retired lock rejected");
    check(ShutdownZBuffer(z)&&sgp::ReleaseReservedVideoSurfaces()&&ShutdownVideoSurfaceManager(),"real cleanup");
}
}
int main() {try {test();std::cout<<checks<<" effect-blitter checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
