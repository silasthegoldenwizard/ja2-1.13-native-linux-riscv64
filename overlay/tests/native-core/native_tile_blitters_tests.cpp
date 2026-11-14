/*
 * P710 native snapshot update: tests/native-core/native_tile_blitters_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Independent literal expectations exercise production providers, no stubs.
#include "NativeTileBlitters.h"
#include "native_background.h"
#include "shading.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool v,const char* reason) {++checks;if(!v)throw std::runtime_error(reason);}
UINT16 read(const BYTE* p) {UINT16 v;std::memcpy(&v,p,2);return v;}
void write(BYTE* p,UINT16 v) {std::memcpy(p,&v,2);}
UINT32 surface(UINT32 w,UINT32 h) {
    VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=w;d.usHeight=h;d.ubBitDepth=16;
    UINT32 id=0;check(AddVideoSurface(&d,&id),"actual surface manager allocation");return id;
}
struct Fixture {
    std::array<UINT16,256> palette{};
    std::vector<UINT8> data;
    ETRLEObject object{};SGPVObject source{};
    std::array<INT8,8> changes{1,-1,0,1,0,-1,0,0};
    ZStripInfo strip{};ZStripInfo* stripPointer=&strip;
    Fixture(UINT16 width=43,UINT16 height=2) {
        for(unsigned i=0;i<256;++i)palette[i]=UINT16(0x4000+i);
        for(unsigned y=0;y<height;++y) {data.push_back(UINT8(width));for(unsigned x=0;x<width;++x)data.push_back(UINT8(x%10+1));data.push_back(0);}
        object.usWidth=width;object.usHeight=height;object.uiDataLength=data.size();
        source.ubBitDepth=8;source.usNumberOfObjects=1;source.pETRLEObject=&object;
        source.pPixData=data.data();source.uiSizePixData=data.size();source.pShadeCurrent=palette.data();
        strip.ubFirstZStripWidth=3;strip.ubNumberOfZChanges=changes.size();strip.pbZChange=changes.data();
        source.ppZStripInfo=&stripPointer;
    }
    void replace(std::vector<UINT8> bytes,UINT16 width,UINT16 height=1) {
        data=std::move(bytes);object.usWidth=width;object.usHeight=height;object.uiDataLength=data.size();
        source.pPixData=data.data();source.uiSizePixData=data.size();
    }
};
void run() {
    check(InitializeVideoSurfaceManager(),"real manager initialized");
    const UINT32 ids[4]{surface(48,5),surface(48,5),surface(48,5),surface(2,2)};
    check(sgp::BindReservedVideoSurfaces(ids),"real frame binding");
    UINT32 pitch=0;BYTE* query=LockVideoSurface(FRAME_BUFFER,&pitch);check(query,"real pitch query");UnLockVideoSurface(FRAME_BUFFER);
    UINT16* z=InitZBuffer(pitch,5);check(z,"actual native Z allocation");
    BYTE* color=sgp::LockBackgroundSurface(FRAME_BUFFER,&pitch);check(color,"real acquired frame");
    UINT16* b=reinterpret_cast<UINT16*>(color);
    sgp::BackgroundView cv,zv;check(sgp::BackgroundBufferView(color,&cv)&&sgp::BackgroundBufferView(z,&zv),"real bounded views");
    auto pixel=[&](unsigned x,unsigned y=0){return read(color+y*pitch+x*2);};
    auto depth=[&](unsigned x,unsigned y=0){return read(zv.data+y*pitch+x*2);};
    auto reset=[&](UINT16 zz=0,UINT16 cc=0x5a5a) {
        std::memset(color,0x5a,cv.bytes);std::memset(zv.data,0xa5,zv.bytes);
        for(unsigned y=0;y<5;++y)for(unsigned x=0;x<48;++x){write(color+y*pitch+x*2,cc);write(zv.data+y*pitch+x*2,zz);}
    };
    Fixture f;SGPRect full{0,0,48,5};
    reset();check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&full),"increment kernel runs");
    // First strip is 3 columns: 100, then +80 at col 3, then -80 at col 23.
    check(depth(1,1)==100&&depth(3,1)==100&&depth(4,1)==180&&depth(23,1)==180&&depth(24,1)==100,"exact 3/20 strip boundary depths");
    check(pixel(1,1)==0x4001&&pixel(4,1)==0x4004&&pixel(24,1)==0x4004,"literal increment pixel values");
    check(depth(4,2)==180&&depth(24,2)==100,"strip state resets each row");
    reset(100);check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&full),"strict increment compare");
    check(pixel(1,1)==0x5a5a&&pixel(4,1)==0x4004&&pixel(24,1)==0x5a5a,"JAE rejects equal unsigned depth");
    reset(100);check(Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(b,pitch,z,100,&f.source,1,1,0,&full,0),"burn-through runs");
    check(pixel(1,1)==0x4001&&pixel(24,1)==0x4004,"burn-through JA admits equality");
    reset();SGPRect leftBoundary{4,1,25,2};
    check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&leftBoundary),"clip exactly at first strip boundary");
    check(depth(3,1)==0&&depth(4,1)==180&&depth(23,1)==180&&depth(24,1)==100&&depth(25,1)==0,"left/right clip with boundary at 3 then20");
    reset();SGPRect inside{6,2,27,3};SetClippingRect(&inside);
    check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,nullptr),"global clip top and inside strip");
    check(depth(6,2)==180&&depth(24,2)==100&&depth(26,2)==100&&depth(6,1)==0,"clipping advances Z across top and left");
    reset();f.strip.bInitialZChange=-1;
    check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,10,&f.source,1,1,0,&full),"wrapping increment runs");
    check(depth(1,1)==65466&&depth(4,1)==10&&depth(24,1)==65466,"unsigned sixteen-bit wrap around zero");f.strip.bInitialZChange=0;
    // Transparent spans still advance strip position, including a boundary.
    f.replace({0x84,3,1,2,3,0},7);f.strip.ubFirstZStripWidth=3;
    reset();check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&full),"transparent strip crossing");
    check(depth(4,1)==0&&depth(5,1)==180&&pixel(5,1)==0x4001,"transparent run advances strip without claiming Z");
    // EDI bit 1 controls occluded checker selection, not relative sprite X.
    f.replace({6,1,2,3,4,5,6,0,6,1,2,3,4,5,6,0},6,2);f.strip.ubFirstZStripWidth=20;
    reset(101);check(Blt8BPPDataTo16BPPBufferTransZIncObscureClip(b,pitch,z,100,&f.source,1,1,0,&full),"obscure increment runs");
    for(unsigned y=1;y<=2;++y)for(unsigned x=1;x<=6;++x) {
        bool drawn=((reinterpret_cast<std::uintptr_t>(color+y*pitch+x*2)&2u)!=0)==((y&1)!=0);
        check(pixel(x,y)==(drawn?UINT16(0x4000+x):UINT16(0x5a5a)),"address-based checker color");
        check(depth(x,y)==(drawn?100:101),"occluded checker writes depth only when selected");
    }
    // Shadow variants use 8 at strip changes, initial Z still changes by 80.
    f.replace({6,1,254,2,3,254,4,0},6);f.strip.ubFirstZStripWidth=3;f.strip.bInitialZChange=1;
    BuildShadeTable();reset(0,0xffff);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(b,pitch,z,100,&f.source,1,1,0,&full,0,f.palette.data(),FALSE),"shadow increment runs");
    check(depth(1,1)==180&&depth(4,1)==188,"shadow initial eighty / subsequent eight deltas");
    check(pixel(2,1)==ShadeTable[0xffff]&&pixel(5,1)==ShadeTable[0xffff]&&pixel(1,1)==0x4001,"real shade lookup and palette");
    reset(0,0xffff);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(b,pitch,z,100,&f.source,1,1,0,&full,0,f.palette.data(),TRUE),"ignored shadow runs");
    check(pixel(2,1)==0xffff&&depth(2,1)==180&&pixel(5,1)==0xffff&&depth(5,1)==188,"ignored shadow still claims depth");
    f.strip.bInitialZChange=0;f.strip.ubFirstZStripWidth=20;
    reset(100);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(b,pitch,z,100,&f.source,1,1,0,&full,0,f.palette.data(),TRUE),"nonalpha shadow obscure runs");
    check(pixel(1,1)==0x4001&&pixel(3,1)==0x4002&&pixel(4,1)==0x4003,"nonalpha shadow obscure equality draws all");
    Fixture a(6,1);a.replace({6,0,255,128,255,0,255,0},6);
    f.palette[1]=0xffff;f.palette[2]=0xffff;f.palette[3]=0x001f;f.palette[4]=0xf800;
    reset(0,0);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(b,pitch,z,100,&f.source,&a.source,1,1,0,&full,0,f.palette.data(),FALSE),"alpha shadow increment runs");
    check(pixel(1,1)==0&&pixel(3,1)==0x8410&&pixel(4,1)==0x001f&&pixel(6,1)==0xf800,"literal alpha 0/128/255 RGB565 expectations");
    check(depth(1,1)==100&&depth(3,1)==100,"alpha zero still claims Z");
    reset(100,0);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClipAlpha(b,pitch,z,100,&f.source,&a.source,1,1,0,&full,0,f.palette.data(),TRUE),"alpha obscure equality runs");
    for(unsigned x:{1u,3u,4u,6u}) {
        bool drawn=((reinterpret_cast<std::uintptr_t>(color+pitch+x*2)&2u)!=0);
        const UINT16 expected=x==1?0:x==3?0x8410:x==4?0x001f:0xf800;
        check(pixel(x,1)==(drawn?expected:0),"alpha obscure JAE equality checker quirk");
    }
    // Control bytes in the alpha stream are ignored by the original; only
    // color-stream positions address alpha samples.
    a.data[0]=99;reset(0,0);
    check(Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(b,pitch,z,100,&f.source,&a.source,1,1,0,&full,0,f.palette.data(),TRUE),"raw synchronized alpha control ignored");
    check(pixel(3,1)==0x8410,"raw alpha byte at color-stream offset");a.data[0]=6;
    // Literal zero is opaque; transparent data does not zero or test Z.
    f.replace({2,0,2,0x81,2,3,4,0},5);f.source.pShadeCurrent=nullptr;
    reset(100);
    check(Zero8BPPDataTo16BPPBufferTransparent(b,pitch,&f.source,1,1,0),"zero kernel has no palette requirement");
    check(pixel(1,1)==0&&pixel(2,1)==0&&pixel(3,1)==0x5a5a&&pixel(4,1)==0&&depth(1,1)==100,"zero literal payload only, no depth writes");
    f.source.pShadeCurrent=f.palette.data();reset(100);write(zv.data+pitch+2*2,101);write(zv.data+pitch+4*2,99);
    check(Blt8BPPDataTo16BPPBufferTransInvZ(b,pitch,z,100,&f.source,1,1,0),"inverse kernel runs");
    check(pixel(1,1)==0x4000&&pixel(2,1)==0x5a5a&&pixel(3,1)==0x5a5a&&pixel(4,1)==0x5a5a&&pixel(5,1)==0xf800,"inverse matches equality only");
    check(depth(2,1)==101&&depth(4,1)==99,"inverse preserves depth");
    f.source.pShadeCurrent=nullptr;reset(100);
    check(IsTileRedundent(pitch,z,100,&f.source,1,1,0),"equal depth tile redundant");
    write(zv.data+pitch+3*2,0);check(IsTileRedundent(pitch,z,100,&f.source,1,1,0),"transparent pixel excluded from redundancy");
    write(zv.data+pitch+2*2,99);check(!IsTileRedundent(pitch,z,100,&f.source,1,1,0),"lower literal depth makes tile visible");
    reset(0x7fff);check(IsTileRedundent(pitch,z,0x8000,&f.source,1,1,0),"redundancy preserves signed JLE near high bit");
    reset(0x8000);check(!IsTileRedundent(pitch,z,0x7fff,&f.source,1,1,0),"signed redundant comparison reverse high bit");
    f.source.pShadeCurrent=f.palette.data();
    // Every rejection is checked against both entire real allocations.
    reset();const std::vector<BYTE> before(color,color+cv.bytes),beforeZ(zv.data,zv.data+zv.bytes);
    auto unchanged=[&] {check(!std::memcmp(color,before.data(),cv.bytes)&&!std::memcmp(zv.data,beforeZ.data(),zv.bytes),"failed preflight leaves color/depth unchanged");};
    auto inc=[&] {return Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&full);};
    f.data.back()=8;check(!inc(),"malformed ETRLE rejected before mutation");unchanged();f.data.back()=0;
    f.object.uiDataLength--;check(!inc(),"truncated source rejected");unchanged();f.object.uiDataLength++;
    check(!Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,1,&full),"invalid subimage rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(b,pitch,z,100,&f.source,1,1,0,&full,-1),"negative strip index rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(b,pitch,z,100,&f.source,1,1,0,&full,1),"strip index beyond actual pointer table rejected");unchanged();
    f.stripPointer=nullptr;check(!inc(),"missing strip rejected");unchanged();f.stripPointer=&f.strip;
    f.strip.ubFirstZStripWidth=21;check(!inc(),"nonproducer strip width rejected");unchanged();f.strip.ubFirstZStripWidth=20;
    f.strip.pbZChange=nullptr;check(!inc(),"missing change allocation rejected");unchanged();f.strip.pbZChange=f.changes.data();
    auto* saved=f.strip.pbZChange;f.strip.pbZChange=reinterpret_cast<INT8*>(color);check(!inc(),"strip/output alias rejected");unchanged();f.strip.pbZChange=saved;
    const auto oldPalette=f.source.pShadeCurrent;f.source.pShadeCurrent=b;check(!inc(),"palette/output alias rejected");unchanged();f.source.pShadeCurrent=oldPalette;
    const auto oldData=f.source.pPixData;f.source.pPixData=color;check(!inc(),"encoded/output alias rejected");unchanged();f.source.pPixData=oldData;
    check(!Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,b,100,&f.source,1,1,0,&full),"color/depth alias rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransInvZ(b,pitch+2,z,100,&f.source,1,1,0),"incorrect actual pitch rejected");unchanged();
    UINT16 foreign[100]{};check(!Zero8BPPDataTo16BPPBufferTransparent(foreign,20,&f.source,0,0,0),"unowned output rejected");unchanged();
    check(!Zero8BPPDataTo16BPPBufferTransparent(b,pitch,&f.source,47,4,0),"real allocation edge rejected");unchanged();
    check(!IsTileRedundent(pitch,z,100,&f.source,-1,0,0),"negative unclipped coordinate rejected");unchanged();
    a.object.uiDataLength=1;check(!Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(b,pitch,z,100,&f.source,&a.source,1,1,0,&full,0,f.palette.data(),FALSE),"insufficient alpha extent rejected");unchanged();a.object.uiDataLength=a.data.size();
    a.source.pPixData=color;check(!Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(b,pitch,z,100,&f.source,&a.source,1,1,0,&full,0,f.palette.data(),FALSE),"alpha/output alias rejected");unchanged();a.source.pPixData=a.data.data();
    // Capacity zero's producer-owned fallback is one byte; never read it if
    // every visible column fits the initial strip.
    INT8 singleFallback=0;f.strip.pbZChange=&singleFallback;
    f.strip.ubNumberOfZChanges=0;f.strip.ubFirstZStripWidth=3;
    f.replace({3,1,2,3,0},3);check(inc(),"exact final strip boundary avoids unused OOB change read");
    reset();f.replace({24,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},24);
    check(!inc(),"insufficient declared change capacity rejected");unchanged();
    // Noncanonical delta 2: runtime sign classification, but clipping switch
    // recognizes only exact +/-1, as the original two code paths do.
    f.strip.pbZChange=f.changes.data();f.strip.ubNumberOfZChanges=8;f.strip.ubFirstZStripWidth=3;f.changes[0]=2;reset();
    check(inc()&&depth(4,1)==180,"runtime positive noncanonical direction increments once");
    reset();SGPRect skip{4,1,5,2};check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&skip)&&depth(4,1)==100,"left clip switch ignores direction 2");
    reset();SGPRect off{99,99,100,100};f.data.back()=9;
    check(Blt8BPPDataTo16BPPBufferTransZIncClip(b,pitch,z,100,&f.source,1,1,0,&off),"fully clipped retains early success");unchanged();f.data.back()=0;
    for(unsigned y=0;y<5;++y)for(unsigned off=96;off<pitch;++off)
        check(color[y*pitch+off]==0x5a&&zv.data[y*pitch+off]==0xa5,"real pitch padding preserved");
    sgp::UnlockBackgroundSurface(FRAME_BUFFER);
    check(!inc(),"retired surface lock rejected");
    check(ShutdownZBuffer(z)&&sgp::ReleaseReservedVideoSurfaces()&&ShutdownVideoSurfaceManager(),"actual manager cleanup");
}
}
int main() {try {run();std::cout<<checks<<" native tile blitter checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
