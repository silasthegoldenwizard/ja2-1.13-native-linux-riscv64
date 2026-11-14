/*
 * P710 native snapshot update: tests/native-core/native_world_blitter_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Literal outputs through production ETRLE/Z/surface implementations. No stubs.
#include "NativeWorldBlitters.h"
#include "native_background.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool value,const char* reason) { ++checks;if(!value)throw std::runtime_error(reason); }
UINT16 read(const BYTE* p,std::size_t off) { UINT16 v;std::memcpy(&v,p+off,2);return v; }
void write(BYTE* p,std::size_t off,UINT16 v) { std::memcpy(p+off,&v,2); }
UINT32 surface(UINT32 w,UINT32 h) {
    VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=w;d.usHeight=h;d.ubBitDepth=16;
    UINT32 id=0;check(AddVideoSurface(&d,&id),"actual manager surface");return id;
}
struct Fixture {
    std::array<UINT16,256> palette{};
    // Row 0: literal zero, skip, literal 2/254/3. Row 1 starts with zero
    // inside a literal payload, then outlines, so clipped row skipping is tested.
    std::vector<UINT8> bytes{1,0,0x81,3,2,254,3,0,5,4,0,254,5,6,0};
    ETRLEObject object{};SGPVObject source{};
    Fixture() {
        for(unsigned i=0;i<256;++i)palette[i]=UINT16(0x4000+i);
        object.usWidth=5;object.usHeight=2;object.uiDataLength=bytes.size();
        source.ubBitDepth=8;source.usNumberOfObjects=1;source.pETRLEObject=&object;
        source.pPixData=bytes.data();source.uiSizePixData=bytes.size();source.pShadeCurrent=palette.data();
    }
};
void run() {
    check(InitializeVideoSurfaceManager(),"production manager init");
    const UINT32 ids[4]{surface(8,4),surface(8,4),surface(8,4),surface(2,2)};
    check(sgp::BindReservedVideoSurfaces(ids),"real reserved framebuffer");
    UINT32 pitch=0;auto* raw=LockVideoSurface(FRAME_BUFFER,&pitch);check(raw,"actual frame pitch");UnLockVideoSurface(FRAME_BUFFER);
    auto* z=InitZBuffer(pitch,4);check(z,"production owned Z allocation");
    auto* color=sgp::LockBackgroundSurface(FRAME_BUFFER,&pitch);check(color,"real helper surface lock");
    sgp::BackgroundView cv,zv;check(sgp::BackgroundBufferView(color,&cv)&&sgp::BackgroundBufferView(z,&zv),"real allocation views");
    auto* b=reinterpret_cast<UINT16*>(color);Fixture f;
    const UINT16 depth=0x1234;
    auto reset=[&]() {
        std::memset(color,0x5a,cv.bytes);std::memset(zv.data,0xa5,zv.bytes);
        for(unsigned y=0;y<4;++y)for(unsigned x=0;x<8;++x)write(zv.data,y*pitch+x*2,depth);
        // In sprite row zero: equal, transparent, greater, equal outline, lower.
        write(zv.data,pitch+3*2,depth+1);write(zv.data,pitch+5*2,depth-1);
    };
    auto pixel=[&](unsigned x,unsigned y) {return read(color,y*pitch+x*2);};
    auto zp=[&](unsigned x,unsigned y) {return read(zv.data,y*pitch+x*2);};
    for(unsigned policy=0;policy<6;++policy) {
        reset();BOOLEAN ok=FALSE;SGPRect full{0,0,8,4};
        switch(policy) {
        case 0:ok=Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,0);break;
        case 1:ok=Blt8BPPDataTo16BPPBufferTransZNB(b,pitch,z,depth,&f.source,1,1,0);break;
        case 2:ok=Blt8BPPDataTo16BPPBufferTransZNBColor(b,pitch,z,depth,&f.source,1,1,0,0xdead);break;
        case 3:ok=Blt8BPPDataTo16BPPBufferTransZClip(b,pitch,z,depth,&f.source,1,1,0,&full);break;
        case 4:ok=Blt8BPPDataTo16BPPBufferTransZNBClip(b,pitch,z,depth,&f.source,1,1,0,&full);break;
        case 5:ok=Blt8BPPDataTo16BPPBufferTransZNBClipColor(b,pitch,z,depth,&f.source,1,1,0,&full,0xdead);break;
        }
        check(ok,"each transparent policy real dispatch");
        check(pixel(1,1)==0x4000&&pixel(2,1)==0x5a5a,"literal zero draws; transparent skip preserves");
        check(pixel(3,1)==((policy==2||policy==5)?0xdead:0x5a5a),"obscured-color policy only on literal depth failure");
        check(pixel(4,1)==0x40fe&&pixel(5,1)==0x4003,"transparent family uses palette 254 and lower Z");
        check(zp(3,1)==depth+1&&zp(5,1)==((policy==0||policy==3)?depth:depth-1),"correct Z write policy");
        check(zp(2,1)==depth,"transparent run does not claim Z");
        for(unsigned y=0;y<4;++y)for(unsigned off=16;off<pitch;++off)
            check(color[y*pitch+off]==0x5a&&zv.data[y*pitch+off]==0xa5,"all color/Z pitch padding preserved");
    }
    for(unsigned policy=0;policy<3;++policy)for(BOOLEAN flag:{BOOLEAN(FALSE),BOOLEAN(TRUE),BOOLEAN(2)}) {
        reset();BOOLEAN ok=FALSE;SGPRect full{0,0,8,4};
        if(policy==0)ok=Blt8BPPDataTo16BPPBufferOutlineZ(b,pitch,z,depth,&f.source,1,1,0,INT16(0xbeef),flag);
        if(policy==1)ok=Blt8BPPDataTo16BPPBufferOutlineZNB(b,pitch,z,depth,&f.source,1,1,0,INT16(0xbeef),flag);
        if(policy==2)ok=Blt8BPPDataTo16BPPBufferOutlineZClip(b,pitch,z,depth,&f.source,1,1,0,INT16(0xbeef),flag,&full);
        check(ok,"each outline policy real dispatch");
        check(pixel(4,1)==(flag==TRUE?0xbeef:0x5a5a)&&zp(4,1)==depth,"outline exact TRUE and no Z write");
        check(pixel(1,1)==0x4000&&zp(1,1)==(policy==1?depth:0x1200),"literal zero ordinary outline pixel; AX low byte preservation");
        check(pixel(3,1)==0x5a5a&&zp(3,1)==depth+1,"outline depth occlusion");
        check(pixel(5,1)==0x4003&&zp(5,1)==(policy==1?depth-1:0x1203),"ordinary outline Z equals original mutated AX");
    }
    reset();SGPRect clipped{2,2,5,3};SetClippingRect(&clipped);
    check(Blt8BPPDataTo16BPPBufferTransZClip(b,pitch,z,depth,&f.source,1,1,0,nullptr),"global clip honored");
    check(pixel(2,2)==0x4000&&pixel(3,2)==0x40fe&&pixel(4,2)==0x4005,"top/left/right clipping retains literal zero row structure");
    check(pixel(1,2)==0x5a5a&&pixel(5,2)==0x5a5a&&pixel(1,1)==0x5a5a,"four clip edges preserve outside");
    reset();f.object.sOffsetX=-2;f.object.sOffsetY=-1;SGPRect edges{0,0,3,1};
    check(Blt8BPPDataTo16BPPBufferTransZNBClip(b,pitch,z,depth,&f.source,1,1,0,&edges),"negative signed offsets clip safely");
    check(pixel(0,0)==0x5a5a&&pixel(1,0)==0x4002&&pixel(2,0)==0x40fe,"literal negative-offset visible samples");
    f.object.sOffsetX=f.object.sOffsetY=0;
    reset();const std::vector<BYTE> before(color,color+cv.bytes),beforeZ(zv.data,zv.data+zv.bytes);
    auto unchanged=[&]() {check(std::memcmp(color,before.data(),cv.bytes)==0&&std::memcmp(zv.data,beforeZ.data(),zv.bytes)==0,"invalid input no-write preflight");};
    f.bytes.back()=5;check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,0),"malformed tail rejected before early rows write");unchanged();f.bytes.back()=0;
    f.object.uiDataLength--;check(!Blt8BPPDataTo16BPPBufferOutlineZ(b,pitch,z,depth,&f.source,1,1,0,1,TRUE),"truncated declared length rejected");unchanged();f.object.uiDataLength++;
    f.object.usWidth=4;check(!Blt8BPPDataTo16BPPBufferTransZNB(b,pitch,z,depth,&f.source,1,1,0),"run exceeds declared width");unchanged();f.object.usWidth=5;
    f.source.uiSizePixData=1;check(!Blt8BPPDataTo16BPPBufferTransZNBColor(b,pitch,z,depth,&f.source,1,1,0,7),"declared source extent rejected");unchanged();f.source.uiSizePixData=f.bytes.size();
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,1),"invalid subimage index rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch+2,z,depth,&f.source,1,1,0),"wrong actual pitch rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,7,3,0),"allocation geometry rejected");unchanged();
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,b,depth,&f.source,1,1,0),"overlapping color and depth rejected");unchanged();
    auto* savedData=f.source.pPixData;f.source.pPixData=color;f.source.uiSizePixData=cv.bytes;
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,0),"source/destination overlap rejected");unchanged();f.source.pPixData=savedData;f.source.uiSizePixData=f.bytes.size();
    auto* savedPalette=f.source.pShadeCurrent;f.source.pShadeCurrent=b;
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,0),"palette/destination overlap rejected");unchanged();f.source.pShadeCurrent=savedPalette;
    UINT16 foreign[32]{};check(!Blt8BPPDataTo16BPPBufferTransZ(foreign,16,z,depth,&f.source,1,1,0),"unowned raw destination rejected");unchanged();
    SGPRect off{50,50,55,55};f.bytes.back()=5;
    check(Blt8BPPDataTo16BPPBufferTransZClip(b,pitch,z,depth,&f.source,1,1,0,&off),"fully clipped sprite keeps original early no-decode success");unchanged();f.bytes.back()=0;
    auto* blankData=f.source.pPixData;auto* blankPalette=f.source.pShadeCurrent;
    f.source.pPixData=nullptr;f.source.pShadeCurrent=nullptr;
    for(const auto dimensions:{std::array<UINT16,2>{0,0},std::array<UINT16,2>{0,2},std::array<UINT16,2>{5,0}}) {
        f.object.usWidth=dimensions[0];f.object.usHeight=dimensions[1];SGPRect full{0,0,8,4};
        check(Blt8BPPDataTo16BPPBufferTransZClip(b,pitch,z,depth,&f.source,1,1,0,&full),"blank world frame clipped no-op before payload/palette bind");unchanged();
    }
    f.object.usWidth=5;f.object.usHeight=2;f.source.pPixData=blankData;f.source.pShadeCurrent=blankPalette;
    sgp::UnlockBackgroundSurface(FRAME_BUFFER);
    color=LockVideoSurface(FRAME_BUFFER,&pitch);check(color,"ordinary render manager lock");b=reinterpret_cast<UINT16*>(color);
    check(Blt8BPPDataTo16BPPBufferTransZNB(b,pitch,z,depth,&f.source,1,1,0),"ordinary manager lock reaches production world kernel");
    UnLockVideoSurface(FRAME_BUFFER);
    check(!Blt8BPPDataTo16BPPBufferTransZ(b,pitch,z,depth,&f.source,1,1,0),"retired surface view refused");
    check(ShutdownZBuffer(z)&&sgp::ReleaseReservedVideoSurfaces()&&ShutdownVideoSurfaceManager(),"real production cleanup");
}
}
int main() {try {run();std::cout<<checks<<" world-blitter checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
