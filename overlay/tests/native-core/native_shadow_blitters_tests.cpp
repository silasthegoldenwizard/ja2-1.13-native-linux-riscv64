/*
 * P710 native snapshot update: tests/native-core/native_shadow_blitters_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeShadowBlitters.h"
#include "native_background.h"
#include "shading.h"
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool ok,const char* why) { ++checks;if(!ok) throw std::runtime_error(why); }
struct Sprite {
    std::vector<UINT8> data;
    ETRLEObject object{};
    SGPVObject value{};
    Sprite(UINT16 width,UINT16 height,std::initializer_list<UINT8> bytes):data(bytes) {
        object.usWidth=width;object.usHeight=height;object.uiDataLength=data.size();
        value.pPixData=data.data();value.uiSizePixData=data.size();value.pETRLEObject=&object;
        value.usNumberOfObjects=1;value.ubBitDepth=8;
    }
};
struct Buffers {
    UINT16* color=reinterpret_cast<UINT16*>(sgp::AllocateBackgroundBuffer(8,4));
    UINT16* z=reinterpret_cast<UINT16*>(sgp::AllocateBackgroundBuffer(8,4));
    Buffers(){check(color&&z,"production owned buffers");check(!(reinterpret_cast<std::uintptr_t>(color)&3),"aligned allocation checker origin");reset();}
    ~Buffers(){sgp::FreeBackgroundBuffer(color);sgp::FreeBackgroundBuffer(z);}
    void reset(){std::fill(color,color+32,UINT16(0xffff));std::fill(z,z+32,UINT16(11));z[0]=z[1]=9;z[2]=z[3]=10;}
};
std::array<UINT16,256> palette;
void exportsAndDepth()
{
    Buffers b;Sprite s(6,1,{6,1,254,2,254,3,0,0}),a(6,1,{6,255,1,255,2,255,255,0});
    SGPRect clip{0,0,8,4};
    using Call=std::function<BOOLEAN()>;
    struct Case{const char* name;Call draw;unsigned policy;};
#define COMMON b.color,16,b.z,10,&s.value
#define TAIL 0,0,0,palette.data(),FALSE
#define CTAIl 0,0,0,&clip,palette.data(),FALSE
    std::vector<Case> cases{
        {"plain",[&]{return Blt8BPPDataTo16BPPBufferTransShadow(b.color,16,&s.value,TAIL);},0},
        {"plain alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowAlpha(b.color,16,&s.value,&a.value,TAIL);},0},
        {"plain clip",[&]{return Blt8BPPDataTo16BPPBufferTransShadowClip(b.color,16,&s.value,CTAIl);},0},
        {"plain clip alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowClipAlpha(b.color,16,&s.value,&a.value,CTAIl);},0},
        {"Z",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZ(COMMON,TAIL);},1},
        {"Z alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZAlpha(COMMON,&a.value,TAIL);},1},
        {"Z clip",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZClip(COMMON,CTAIl);},1},
        {"Z clip alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZClipAlpha(COMMON,&a.value,CTAIl);},1},
        {"NB",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNB(COMMON,TAIL);},2},
        {"NB alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBAlpha(COMMON,&a.value,TAIL);},2},
        {"NB clip",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBClip(COMMON,CTAIl);},2},
        {"NB clip alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBClipAlpha(COMMON,&a.value,CTAIl);},2},
        {"obscured",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBObscured(COMMON,TAIL);},3},
        {"obscured alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredAlpha(COMMON,&a.value,TAIL);},3},
        {"obscured test",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredTest(COMMON,TAIL);},4},
        {"obscured clip",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(COMMON,CTAIl);},3},
        {"obscured clip alpha",[&]{return Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClipAlpha(COMMON,&a.value,CTAIl);},3},
        {"below or equal",[&]{return Blt8BPPDataTo16BPPBufferTransShadowBelowOrEqualZNBClip(COMMON,0,0,0,&clip,palette.data());},2}
    };
    const UINT16 expected[5][6]{
        {0xf800,0x7bcf,0xf800,0x7bcf,0xf800,0xf800},
        {0xf800,0x7bcf,0xffff,0xffff,0xffff,0xffff},
        {0xf800,0x7bcf,0xf800,0xffff,0xffff,0xffff},
        {0xf800,0x7bcf,0xf800,0xffff,0xf800,0xffff},
        {0xfc10,0x7bcf,0xfc10,0xffff,0xfc10,0xffff}
    };
    for(auto& c:cases) {
        b.reset();check(c.draw(),c.name);
        for(unsigned i=0;i<32;++i) {
            check(b.color[i]==(i<6?expected[c.policy][i]:0xffff),c.name);
            const UINT16 z=i<2 ? (c.policy==1?10:9) : (i<4?10:11);
            check(b.z[i]==z,"strict/non-strict original depth policy and guards");
        }
    }
    b.reset();check(Blt8BPPDataTo16BPPBufferTransShadowZ(COMMON,0,0,0,palette.data(),2),"nonzero ignore flag");
    check(b.color[1]==0xffff&&b.z[1]==10,"ignored shadow still claims depth in write-Z");
    b.reset();check(Blt8BPPDataTo16BPPBufferTransShadowZNB(COMMON,0,0,0,palette.data(),TRUE),"ignore NB");
    check(b.color[1]==0xffff&&b.z[1]==9,"ignored NB shadow leaves both buffers");
#undef COMMON
#undef TAIL
#undef CTAIl
}
void alphaAndClips()
{
    Buffers b;
    // Mixed control layout: three left-skipped controls consume no alpha
    // controls in the original assembly. Source literal offsets 6/7 therefore
    // read alpha offsets 3/4, then the omission accumulates on the next row.
    Sprite s(5,2,{0x81,1,1,1,2,2,3,4,0,0x81,1,1,1,2,2,3,4,0});
    Sprite a(5,2,{99,11,22,0,255,33,44,55,66,0,255,77,88,99,110,121,132,143});
    SGPRect clip{3,0,5,2};
    check(Blt8BPPDataTo16BPPBufferTransShadowClipAlpha(b.color,16,&s.value,&a.value,0,0,0,&clip,palette.data(),FALSE),"alpha mixed-control left clip");
    check(b.color[3]==0xffff&&b.color[4]==0xf800&&b.color[11]==0xffff&&b.color[12]==0xf800,"literal original cumulative alpha cursor omission");
    b.reset();SGPRect top{0,1,5,2};
    Sprite simple(5,2,{5,1,1,1,1,1,0,5,1,1,1,1,1,0});
    Sprite alpha(5,2,{5,255,255,255,255,255,0,5,0,255,128,127,0,0});
    check(Blt8BPPDataTo16BPPBufferTransShadowClipAlpha(b.color,16,&simple.value,&alpha.value,0,0,0,&top,palette.data(),FALSE),"top clip alpha cursor");
    const UINT16 row[5]{0xffff,0xf800,0xfbef,0xfc10,0xffff};
    for(unsigned i=0;i<32;++i) check(b.color[i]==(i>=8&&i<13?row[i-8]:0xffff),"independent alpha 0/255/128/127 samples and untouched rows");
    b.reset();SGPRect odd{0,1,5,2};
    check(Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(b.color,16,b.z,10,&simple.value,0,0,0,&odd,palette.data(),FALSE),"deterministic clipped checker top phase");
    for(unsigned i=0;i<5;++i)check(b.color[8+i]==(i%2?0xf800:0xffff),"original address bit matches odd row phase");
    // Zero literal in clipped-away right payload must not become a row marker.
    b.reset();Sprite zero(5,2,{5,1,1,1,0,1,0,5,1,1,1,1,1,0});SGPRect right{0,0,2,2};
    check(Blt8BPPDataTo16BPPBufferTransShadowClip(b.color,16,&zero.value,0,0,0,&right,palette.data(),FALSE),"bounded structural row traversal");
    check(b.color[0]==0xf800&&b.color[1]==0xf800&&b.color[8]==0xf800&&b.color[9]==0xf800&&b.color[2]==0xffff,"literal-zero right-skip regression");
}
void realSurfaceAndZ()
{
    check(InitializeVideoSurfaceManager(),"production surface manager initialization");
    UINT32 ids[4]{};
    for(auto& id:ids) {
        VSURFACE_DESC desc{};desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;
        desc.usWidth=5;desc.usHeight=3;desc.ubBitDepth=16;
        check(AddVideoSurface(&desc,&id),"real RGB565 reserved backing surface");
    }
    check(sgp::BindReservedVideoSurfaces(ids),"real reserved frame aliases");
    UINT32 pitch=0;auto* probe=LockVideoSurface(FRAME_BUFFER,&pitch);
    check(probe,"query actual frame pitch");UnLockVideoSurface(FRAME_BUFFER);
    auto* depth=InitZBuffer(pitch,3);check(depth,"production owned Z for real frame");
    auto* color=sgp::LockBackgroundSurface(FRAME_BUFFER,&pitch);check(color,"registered real padded frame lock");
    sgp::BackgroundView cv,zv;
    check(sgp::BackgroundBufferView(color,&cv)&&sgp::OwnedZBufferView(depth,&zv),"truthful color and owned depth views");
    std::memset(color,0xa5,cv.bytes);
    const UINT16 white=0xffff;
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<5;++x)std::memcpy(color+y*pitch+x*2,&white,2);
    Sprite sprite(3,2,{3,254,1,1,0,0x81,2,1,254,0});
    check(Blt8BPPDataTo16BPPBufferTransShadowZ(reinterpret_cast<UINT16*>(color),pitch,depth,7,&sprite.value,1,1,0,palette.data(),FALSE),"render into genuine padded frame and owned depth");
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<5;++x) {
        UINT16 actualColor,actualZ;
        std::memcpy(&actualColor,color+y*pitch+x*2,2);std::memcpy(&actualZ,zv.data+y*pitch+x*2,2);
        const bool written=(y==1&&x>=1&&x<=3)||(y==2&&x>=2&&x<=3);
        const bool shadow=(y==1&&x==1)||(y==2&&x==3);
        check(actualColor==(written?(shadow?0x7bcf:0xf800):0xffff),"literal production surface pixels, transparent guard and shadows");
        check(actualZ==(written?7:0),"literal owned production Z and transparent guard");
    }
    for(unsigned y=0;y<3;++y)for(unsigned x=10;x<pitch;++x) {
        check(color[y*pitch+x]==0xa5,"actual frame padding preserved");
        check(zv.data[y*pitch+x]==0,"actual owned depth padding preserved");
    }
    sgp::UnlockBackgroundSurface(FRAME_BUFFER);
    check(ShutdownZBuffer(depth),"owned depth cleanup");
    check(sgp::ReleaseReservedVideoSurfaces(),"reserved frame cleanup");
    check(ShutdownVideoSurfaceManager(),"production surface manager shutdown");
}
void blankFrames()
{
    Buffers b;SGPRect clip{0,0,8,4};
    const std::vector<UINT16> color(b.color,b.color+32),depth(b.z,b.z+32);
    for(const auto dimensions:{std::array<UINT16,2>{0,0},std::array<UINT16,2>{0,2},std::array<UINT16,2>{5,0}}) {
        Sprite s(dimensions[0],dimensions[1],{}),alpha(dimensions[0],dimensions[1],{});
        check(Blt8BPPDataTo16BPPBufferTransShadowZNBClip(b.color,16,b.z,10,&s.value,1,1,0,&clip,palette.data(),FALSE),
              "blank clipped shadow frame succeeds before payload bind");
        check(Blt8BPPDataTo16BPPBufferTransShadowZNBClipAlpha(b.color,16,b.z,10,&s.value,&alpha.value,1,1,0,&clip,palette.data(),FALSE),
              "blank clipped alpha frame succeeds before alpha bind");
        check(std::equal(color.begin(),color.end(),b.color)&&std::equal(depth.begin(),depth.end(),b.z),
              "blank shadow/alpha frame preserves entire color and depth buffers");
    }
}
void rejectionAndGeometry()
{
    Buffers b;Sprite s(2,2,{2,1,1,0,2,1,1,0}),a(2,2,{2,255,255,0,2,255,255,0});
    const std::vector<UINT16> before(b.color,b.color+32),zbefore(b.z,b.z+32);
    auto atomic=[&]{check(std::equal(before.begin(),before.end(),b.color)&&std::equal(zbefore.begin(),zbefore.end(),b.z),"rejection is atomic for entire buffers");};
    s.data.back()=1;
    check(!Blt8BPPDataTo16BPPBufferTransShadowZ(b.color,16,b.z,10,&s.value,0,0,0,palette.data(),FALSE),"malformed later row rejects before first row");atomic();s.data.back()=0;
    check(!Blt8BPPDataTo16BPPBufferTransShadow(b.color,16,&s.value,7,0,0,palette.data(),FALSE),"out of actual width rejects");atomic();
    check(!Blt8BPPDataTo16BPPBufferTransShadow(b.color,18,&s.value,0,0,0,palette.data(),FALSE),"mismatched actual pitch rejects");atomic();
    check(!Blt8BPPDataTo16BPPBufferTransShadowZ(b.color,16,b.color,10,&s.value,0,0,0,palette.data(),FALSE),"color/depth alias rejects");atomic();
    check(!Blt8BPPDataTo16BPPBufferTransShadow(b.color,16,&s.value,0,0,0,b.color,FALSE),"palette destination alias rejects");atomic();
    auto* saved=s.value.pPixData;s.value.pPixData=b.color;
    check(!Blt8BPPDataTo16BPPBufferTransShadow(b.color,16,&s.value,0,0,0,palette.data(),FALSE),"encoded source destination alias rejects");atomic();s.value.pPixData=saved;
    a.object.uiDataLength=7;a.value.uiSizePixData=7;
    check(!Blt8BPPDataTo16BPPBufferTransShadowAlpha(b.color,16,&s.value,&a.value,0,0,0,palette.data(),FALSE),"actual alpha stream short rejects");atomic();
    check(!Blt8BPPDataTo16BPPBufferTransShadowAlpha(b.color,16,&s.value,nullptr,0,0,0,palette.data(),FALSE),"required alpha cannot silently become opaque");atomic();
    UINT16 foreign[32]{};
    check(!Blt8BPPDataTo16BPPBufferTransShadow(foreign,16,&s.value,0,0,0,palette.data(),FALSE),"unregistered pointer rejects");atomic();
    SGPRect clip{0,0,8,4};s.object.sOffsetX=-1;s.object.sOffsetY=-1;
    check(Blt8BPPDataTo16BPPBufferTransShadowClip(b.color,16,&s.value,0,0,0,&clip,palette.data(),FALSE),"negative offsets clipped into real allocation");
    check(b.color[0]==0xf800&&b.color[1]==0xffff&&b.color[8]==0xffff,"literal clipped source offset result");
}
}
int main()
{
    try {
        ConfigureRGB565ImageFormat();BuildShadeTable();palette.fill(0xf800);
        check(ShadeTable[0xffff]==0x7bcf,"real production shade table literal reference");
        exportsAndDepth();alphaAndClips();blankFrames();rejectionAndGeometry();realSurfaceAndZ();
        std::cout<<"PASS: "<<checks<<" portable TransShadow production-provider checks\n";return 0;
    }catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1;}
}
