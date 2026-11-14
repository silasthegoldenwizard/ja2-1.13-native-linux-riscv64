/*
 * P710 native snapshot update: tests/native-core/native_background_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Literal production background-buffer/lock/Z integration tests. No game stubs.
#include "native_background.h"
#include "clip_state.h"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool good,const char* reason)
{ ++checks;if (!good) throw std::runtime_error(reason); }
UINT16 read(const BYTE* data,std::size_t offset)
{ UINT16 value;std::memcpy(&value,data+offset,2);return value; }
void write(BYTE* data,std::size_t offset,UINT16 value)
{ std::memcpy(data+offset,&value,2); }
UINT32 surface(UINT32 w,UINT32 h,UINT8 depth=16)
{
    VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=w;d.usHeight=h;d.ubBitDepth=depth;
    UINT32 id=0;check(AddVideoSurface(&d,&id),"real surface creation");return id;
}
void rawCopies()
{
    std::array<BYTE,32> source{},dest{};source.fill(0xa5);dest.fill(0x5a);
    sgp::BackgroundView s{source.data(),source.size(),3,2,16},d{dest.data(),dest.size(),3,2,16};
    const UINT16 input[6]{1,2,3,11,12,13};
    for(unsigned y=0;y<2;++y) for(unsigned x=0;x<3;++x) write(source.data(),y*16+x*2,input[y*3+x]);
    check(sgp::CopyBackgroundRect(d,s,0,0,0,0,3,2),"actual padded RGB565 copy");
    for(unsigned y=0;y<2;++y)
    {
        for(unsigned x=0;x<3;++x) check(read(dest.data(),y*16+x*2)==input[y*3+x],"literal visible copied sample");
        for(unsigned x=6;x<16;++x) check(dest[y*16+x]==0x5a,"padded destination guard");
    }
    dest.fill(0x5a);
    check(sgp::CopyBackgroundRect(d,s,-1,0,0,0,3,2),"negative destination clips correlated source");
    check(read(dest.data(),0)==2&&read(dest.data(),2)==3&&read(dest.data(),4)==0x5a5a,"literal clipped first row");
    check(read(dest.data(),16)==12&&read(dest.data(),18)==13,"literal clipped second row");
    auto before=dest;
    auto bad=d;bad.pitch=5;check(!sgp::CopyBackgroundRect(bad,s,0,0,0,0,3,2),"odd/short destination pitch rejects");
    bad=d;bad.bytes=21;check(!sgp::CopyBackgroundRect(bad,s,0,0,0,0,3,2),"last-row extent rejects before writes");
    bad=s;bad.height=std::numeric_limits<UINT32>::max();check(!sgp::CopyBackgroundRect(d,bad,0,0,0,0,3,2),"overflowing source extent rejects");
    check(dest==before,"invalid metadata leaves entire destination allocation untouched");
    check(sgp::CopyBackgroundRect(d,s,100,100,0,0,3,2)&&dest==before,"fully clipped valid rectangle is no-op");
    source.fill(0xa5);for(unsigned x=0;x<3;++x) write(source.data(),x*2,UINT16(x+1));
    check(sgp::CopyBackgroundRect(s,s,1,0,0,0,2,1),"overlapping row uses actual snapshot");
    check(read(source.data(),0)==1&&read(source.data(),2)==1&&read(source.data(),4)==2,"literal self-overlap samples");
}
void saveRestore()
{
    const auto id=surface(5,3),indexed=surface(5,3,8);
    UINT32 pitch=99;check(!sgp::LockBackgroundSurface(indexed,&pitch)&&pitch==0,"indexed surface refused as RGB565");
    auto* image=sgp::LockBackgroundSurface(id,&pitch);check(image,"actual padded native lock");
    sgp::BackgroundView view;check(sgp::BackgroundBufferView(image,&view)&&view.width==5&&view.height==3&&view.pitch==pitch,"lock metadata reflects real dimensions");
    for(unsigned y=0;y<3;++y) for(unsigned x=0;x<5;++x) write(image,y*pitch+x*2,UINT16(y*10+x));
    auto* color=sgp::AllocateBackgroundBuffer(3,2);check(color,"owned color snapshot");
    sgp::BackgroundView owned;
    check(sgp::OwnedBackgroundBufferView(color,&owned)&&owned.bytes==12,"owned save-only allocation identity");
    check(!sgp::OwnedZBufferView(color,&owned)&&!owned.data,"save allocation is not owned Z");
    check(!sgp::OwnedBackgroundBufferView(image,&owned)&&!owned.data,"locked surface is not freeable save storage");
    check(sgp::CopyBackgroundBuffers(reinterpret_cast<UINT16*>(color),6,reinterpret_cast<UINT16*>(image),pitch,0,0,1,1,3,2),"save actual background rectangle");
    const UINT16 saved[6]{11,12,13,21,22,23};for(unsigned i=0;i<6;++i)check(read(reinterpret_cast<BYTE*>(color),i*2)==saved[i],"literal saved background");
    for(unsigned y=1;y<3;++y)for(unsigned x=1;x<4;++x)write(image,y*pitch+x*2,0xffff);
    check(sgp::CopyBackgroundBuffers(reinterpret_cast<UINT16*>(image),pitch,reinterpret_cast<UINT16*>(color),6,1,1,0,0,3,2),"restore owned snapshot to real padded surface");
    for(unsigned y=0;y<3;++y)
    {
        for(unsigned x=0;x<5;++x)check(read(image,y*pitch+x*2)==y*10+x,"literal restored surface");
        for(unsigned x=10;x<pitch;++x)check(image[y*pitch+x]==0xa5,"surface padding survives save/restore");
    }
    const std::vector<BYTE> before(image,image+pitch*3);
    check(!sgp::CopyBackgroundBuffers(reinterpret_cast<UINT16*>(image),pitch+2,reinterpret_cast<UINT16*>(color),6,0,0,0,0,3,2),"mismatched actual lock pitch refuses");
    std::array<UINT16,6> foreign{};
    check(!sgp::CopyBackgroundBuffers(reinterpret_cast<UINT16*>(image),pitch,foreign.data(),6,0,0,0,0,3,2),"unknown raw pointer is not a surface ID");
    check(std::memcmp(image,before.data(),before.size())==0,"failed adapter copies atomic");
    UINT32 secondPitch=99;check(!sgp::LockBackgroundSurface(id,&secondPitch)&&secondPitch==0,"duplicate helper lock rejected");
    sgp::UnlockBackgroundSurface(id);check(!sgp::BackgroundBufferView(image,&view),"unlock retires byte view");
    // The helper cannot release somebody else's existing manager lock.
    image=LockVideoSurface(id,&pitch);check(image,"external real manager lock");
    check(!sgp::LockBackgroundSurface(id,&secondPitch),"helper refuses externally held lock");
    sgp::UnlockBackgroundSurface(id);
    check(!DeleteVideoSurfaceFromIndex(id),"failed helper acquisition did not unlock outer owner");
    UnLockVideoSurface(id);
    check(sgp::FreeBackgroundBuffer(color)&&!sgp::FreeBackgroundBuffer(color),"owned snapshot lifetime/double-free refusal");
    check(!sgp::BackgroundBufferView(color,&view),"freed snapshot metadata gone");
    check(!sgp::AllocateBackgroundBuffer(0,2)&&!sgp::AllocateBackgroundBuffer(UINT32_MAX,2),"invalid raw allocation geometry rejects");
    check(DeleteVideoSurfaceFromIndex(id)&&DeleteVideoSurfaceFromIndex(indexed),"surface cleanup");
}
void zBuffers()
{
    check(!InitZBuffer(10,3),"Z allocation requires real reserved frame");
    const UINT32 ids[4]{surface(5,3),surface(5,3),surface(5,3),surface(2,2)};
    check(sgp::BindReservedVideoSurfaces(ids),"real frame aliases bound without a window");
    UINT32 pitch=0;auto* frame=LockVideoSurface(FRAME_BUFFER,&pitch);check(frame,"read actual frame pitch");
    std::memset(frame,0x6b,pitch*3);
    const std::vector<BYTE> frameBefore(frame,frame+pitch*3);
    SGPRect narrow{1,1,3,2};SetClippingRect(&narrow);
    check(!InitZBuffer(pitch+2,3)&&!InitZBuffer(pitch,2),"locked frame still rejects inconsistent pitch/height");
    check(ClippingRect.iLeft==1&&ClippingRect.iTop==1&&ClippingRect.iRight==3&&ClippingRect.iBottom==2,
          "rejected Z initialization preserves clipping");
    // Original MainGameScreenInit locks FRAME_BUFFER, initializes Z from its
    // actual pitch, then unlocks. Z allocation must not acquire/release that lock.
    auto* z=InitZBuffer(pitch,3);check(z,"original prelocked-frame startup allocates real Z storage");
    UINT32 nestedPitch=99;
    check(!LockVideoSurface(FRAME_BUFFER,&nestedPitch)&&nestedPitch==0,"Z initialization preserves exclusive outer lock");
    HVSURFACE frameHandle=nullptr;check(GetVideoSurface(&frameHandle,FRAME_BUFFER),"actual frame handle");
    UINT32 metadataPitch=99;
    check(sgp::GetNativeSurfacePitch(frameHandle,&metadataPitch)&&metadataPitch==pitch,"read-only pitch metadata available while locked");
    check(!sgp::GetNativeSurfacePitch(frameHandle,nullptr),"pitch metadata rejects null output");
    metadataPitch=99;
    check(!sgp::GetNativeSurfacePitch(nullptr,&metadataPitch)&&metadataPitch==99,"failed pitch query preserves output");
    check(!LockVideoSurfaceBuffer(frameHandle,&nestedPitch)&&nestedPitch==0,"outer lock also excludes handle API");
    check(!sgp::ReleaseReservedVideoSurfaces(),"Z initialization cannot release caller-owned frame lock");
    check(std::memcmp(frame,frameBefore.data(),frameBefore.size())==0,"Z initialization leaves frame pixels and padding untouched");
    check(ClippingRect.iLeft==0&&ClippingRect.iTop==0&&ClippingRect.iRight==5&&ClippingRect.iBottom==3,
          "successful Z initialization retains full-frame clipping contract");
    sgp::BackgroundView zv;check(sgp::BackgroundBufferView(z,&zv)&&zv.width==5&&zv.height==3&&zv.pitch==pitch,"Z has truthful width/padded pitch/extent");
    check(zv.bytes==frameBefore.size(),"Z allocation spans every real framebuffer row including padding");
    sgp::BackgroundView owned;
    check(sgp::OwnedZBufferView(z,&owned)&&owned.bytes==zv.bytes,"Z-only allocation identity");
    check(!sgp::OwnedBackgroundBufferView(z,&owned)&&!owned.data,"owned Z cannot be freed as overlay save storage");
    for(std::size_t i=0;i<zv.bytes;++i)check(zv.data[i]==0,"real Z storage initially zero");
    UnLockVideoSurface(FRAME_BUFFER);
    check(sgp::GetNativeSurfacePitch(frameHandle,&metadataPitch)&&metadataPitch==pitch,"unlocked frame has the same real pitch");
    auto* unlockedZ=InitZBuffer(pitch,3);check(unlockedZ&&unlockedZ!=z,"unlocked frame initialization remains supported");
    check(ShutdownZBuffer(unlockedZ),"independent unlocked Z allocation cleanup");
    frame=LockVideoSurfaceBuffer(frameHandle,&nestedPitch);check(frame&&nestedPitch==pitch,"caller reacquires frame through handle API");
    auto* handleZ=InitZBuffer(pitch,3);check(handleZ,"handle-owned frame lock also permits Z initialization");
    check(ShutdownZBuffer(handleZ),"Z shutdown is independent of outer frame lock");
    check(!LockVideoSurface(FRAME_BUFFER,&nestedPitch)&&nestedPitch==0,"Z shutdown leaves handle-owned outer lock intact");
    check(std::memcmp(frame,frameBefore.data(),frameBefore.size())==0,"repeated Z initialization preserves frame bytes");
    UnLockVideoSurfaceBuffer(frameHandle);
    for(unsigned y=0;y<3;++y)for(unsigned x=0;x<5;++x)write(zv.data,y*pitch+x*2,UINT16(100+y*10+x));
    auto* snapshot=sgp::AllocateBackgroundBuffer(2,2);check(snapshot,"owned Z rectangle snapshot");
    check(sgp::CopyBackgroundBuffers(reinterpret_cast<UINT16*>(snapshot),4,z,pitch,0,0,2,1,2,2),"save-Z actual writes");
    const UINT16 expected[4]{112,113,122,123};for(unsigned i=0;i<4;++i)check(read(reinterpret_cast<BYTE*>(snapshot),i*2)==expected[i],"literal saved Z values");
    for(unsigned y=1;y<3;++y)for(unsigned x=2;x<4;++x)write(zv.data,y*pitch+x*2,0);
    check(sgp::CopyBackgroundBuffers(z,pitch,reinterpret_cast<UINT16*>(snapshot),4,2,1,0,0,2,2),"restore-Z actual writes");
    for(unsigned y=1;y<3;++y)for(unsigned x=2;x<4;++x)check(read(zv.data,y*pitch+x*2)==100+y*10+x,"literal restored Z values");
    check(!ShutdownZBuffer(reinterpret_cast<UINT16*>(snapshot)),"Z shutdown refuses other allocation owners");
    check(sgp::FreeBackgroundBuffer(snapshot),"snapshot cleanup");
    check(ShutdownZBuffer(z)&&!ShutdownZBuffer(z),"owned Z shutdown/double-free refusal");
    check(!sgp::BackgroundBufferView(z,&zv),"Z metadata retired after shutdown");
    check(sgp::ReleaseReservedVideoSurfaces(),"real reserved surface cleanup");
    metadataPitch=99;
    check(!sgp::GetNativeSurfacePitch(frameHandle,&metadataPitch)&&metadataPitch==99,"deleted frame handle has no pitch metadata");
    check(!InitZBuffer(pitch,3),"unbound frame cannot initialize Z after cleanup");
}
}
int main()
{
    try {
        check(InitializeVideoSurfaceManager(),"surface manager init");rawCopies();saveRestore();zBuffers();
        check(ShutdownVideoSurfaceManager(),"surface manager clean shutdown");
        std::cout<<"PASS: "<<checks<<" production background/save-Z/lock checks\n";return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1; }
}
