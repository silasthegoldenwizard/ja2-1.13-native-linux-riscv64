/*
 * P710 native snapshot update: tests/native-core/native_scroll_refresh_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercises the production RefreshNativeVideo transaction with diagnostic
// callbacks and real surfaces/Z. These callbacks are not the world renderer.
#include "native_video.h"
#include "clip_state.h"
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>

namespace {
thread_local bool refuseAllocations=false;
thread_local unsigned allocationAttempts=0;
}
void* operator new(std::size_t size)
{
    if (refuseAllocations) { ++allocationAttempts;throw std::bad_alloc(); }
    if (void* data=std::malloc(size ? size : 1)) return data;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* data) noexcept { std::free(data); }
void operator delete[](void* data) noexcept { std::free(data); }
void operator delete(void* data,std::size_t) noexcept { std::free(data); }
void operator delete[](void* data,std::size_t) noexcept { std::free(data); }

namespace {
using Bytes=std::vector<BYTE>;
unsigned checks=0,begins=0,rollbacks=0,commits=0,presents=0,strips=0,finishes=0;
bool overlayActive=false,beginFailure=false;
enum class Failure { None, Render, Throw, Finish };
Failure failure=Failure::None;
UINT16* zBuffer=nullptr;
sgp::BackgroundView zView;
sgp::NativeVideoRegionState state{5,false};
void check(bool value,const char* reason)
{ ++checks;if (!value) throw std::runtime_error(reason); }
void write(BYTE* bytes,std::size_t offset,UINT16 value)
{ std::memcpy(bytes+offset,&value,2); }
UINT16 read(const BYTE* bytes,std::size_t offset)
{ UINT16 value;std::memcpy(&value,bytes+offset,2);return value; }
sgp::NativeVideoRegionState region() noexcept { return state; }
Bytes surfaceBytes(UINT32 id)
{
    UINT32 pitch=0;auto* bytes=LockVideoSurface(id,&pitch);
    check(bytes && pitch>12,"actual padded six-wide surface lock");
    Bytes result(bytes,bytes+std::size_t(pitch)*6);UnLockVideoSurface(id);return result;
}
void seed(UINT32 id,UINT16 base,BYTE padding)
{
    UINT32 pitch=0;auto* bytes=LockVideoSurface(id,&pitch);check(bytes,"seed actual surface");
    std::memset(bytes,padding,std::size_t(pitch)*6);
    for (unsigned y=0;y<6;++y) for (unsigned x=0;x<6;++x)
        write(bytes,y*pitch+x*2,UINT16(base+y*10+x));
    UnLockVideoSurface(id);
}
bool begin()
{
    ++begins;
    if (beginFailure || overlayActive) return false;
    overlayActive=true;return true;
}
bool render(const SGPRect& rect)
{
    ++strips;
    if (!overlayActive || rect.iLeft!=0 || rect.iTop!=1 || rect.iRight!=1 || rect.iBottom!=5)
        return false;
    UINT32 pitch=0;auto* bytes=LockVideoSurface(FRAME_BUFFER,&pitch);
    if (!bytes) return false;
    for (unsigned y=1;y<5;++y) write(bytes,y*pitch,UINT16(0xc000+y));
    UnLockVideoSurface(FRAME_BUFFER);
    std::memset(zView.data,0x7b,zView.bytes);
    if (failure==Failure::Render || failure==Failure::Throw) {
        // All snapshot preparation has completed. Rollback must not allocate.
        refuseAllocations=true;
        if (failure==Failure::Throw) throw 7;
        return false;
    }
    return true;
}
bool finish(INT32 dx,INT32 dy)
{
    ++finishes;
    if (!overlayActive || dx!=1 || dy!=0) return false;
    UINT32 pitch=0;auto* bytes=LockVideoSurface(BACKBUFFER,&pitch);
    if (!bytes) return false;
    write(bytes,2*pitch+4,0xbeef);UnLockVideoSurface(BACKBUFFER);
    write(zView.data,0,0x1234);
    if (failure==Failure::Finish) { refuseAllocations=true;return false; }
    return true;
}
void end(bool commit) noexcept
{
    if (!overlayActive) return;
    if (commit) ++commits;else ++rollbacks;
    overlayActive=false;
}
void committedFrame() noexcept { ++presents;state.scrolling=false; }
unsigned scrollFrame=0;
bool renderBottomStrip(const SGPRect& rect)
{
    if (!overlayActive || rect.iLeft!=0 || rect.iTop!=4 || rect.iRight!=6 || rect.iBottom!=5)
        return false;
    UINT32 pitch=0;auto* bytes=LockVideoSurface(FRAME_BUFFER,&pitch);
    if (!bytes) return false;
    for (unsigned x=0;x<6;++x) write(bytes,4*pitch+x*2,UINT16(0xc100+scrollFrame*16+x));
    UnLockVideoSurface(FRAME_BUFFER);return true;
}
bool finishVerticalScroll(INT32 dx,INT32 dy)
{ return overlayActive && dx==0 && dy==-1; }
void consecutiveScrollFrames(UINT32 pitch)
{
    // The tactical loop calls EndFrameBufferRender every frame. Its frame
    // interior stays stale while only the exposed strip is rendered afresh.
    // A full frame transfer here overwrites the accumulated BACKBUFFER view.
    seed(PRIMARY_SURFACE,200,0xd1);seed(BACKBUFFER,0,0xb2);seed(FRAME_BUFFER,100,0xf3);
    sgp::SetNativeVideoScrollHooks({&renderBottomStrip,&begin,&finishVerticalScroll,&end,&committedFrame});
    const std::array<std::array<UINT16,36>,3> expected{{
        {{100,1,2,3,4,5, 20,21,22,23,24,25, 30,31,32,33,34,35, 40,41,42,43,44,45,
          0xc110,0xc111,0xc112,0xc113,0xc114,0xc115, 50,51,52,53,154,155}},
        {{100,1,2,3,4,5, 30,31,32,33,34,35, 40,41,42,43,44,45,
          0xc110,0xc111,0xc112,0xc113,0xc114,0xc115,
          0xc120,0xc121,0xc122,0xc123,0xc124,0xc125, 50,51,52,53,154,155}},
        {{100,1,2,3,4,5, 40,41,42,43,44,45,
          0xc110,0xc111,0xc112,0xc113,0xc114,0xc115,
          0xc120,0xc121,0xc122,0xc123,0xc124,0xc125,
          0xc130,0xc131,0xc132,0xc133,0xc134,0xc135, 50,51,52,53,154,155}}
    }};
    for (scrollFrame=1;scrollFrame<=3;++scrollFrame) {
        state={5,true,{{0,1,6,5},0x02,1,1},zBuffer};
        UINT32 actualPitch=0;auto* frame=LockVideoSurface(FRAME_BUFFER,&actualPitch);
        check(frame && actualPitch==pitch,"stale tactical frame available");
        // A changing label-like mark must never leak from the stale world
        // frame into the scrolling backbuffer through a full transfer.
        write(frame,2*pitch+4,UINT16(0xe000+scrollFrame));UnLockVideoSurface(FRAME_BUFFER);
        StartFrameBufferRender();
        InvalidateRegion(0,0,1,1); // ordinary dirty regions still transfer
        InvalidateRegionEx(0,1,6,5,0); // world dirty work is skipped during scroll
        InvalidateRegionEx(4,4,6,6,0); // split interface portion still transfers
        EndFrameBufferRender();
        check(!CanBlitToFrameBuffer(),"end-render preserves BUFFER_DIRTY availability contract");
        if (scrollFrame==1) {
            SuspendVideoManager();
            check(!sgp::RefreshNativeVideo() && !CanBlitToFrameBuffer(),"refused refresh retains pending render");
            check(RestoreVideoManager(),"resume pending tactical frame");
            check(!CanBlitToFrameBuffer(),"resumed frame remains dirty until successful presentation");
        }
        check(sgp::RefreshNativeVideo(),"consecutive tactical-style scroll frame presents");
        check(CanBlitToFrameBuffer(),"successful tactical frame clears BUFFER_DIRTY");
        for (const auto id:{PRIMARY_SURFACE,BACKBUFFER}) {
            const auto result=surfaceBytes(id);
            for (unsigned y=0;y<6;++y) {
                for (unsigned x=0;x<6;++x)
                    check(read(result.data(),y*pitch+x*2)==expected[scrollFrame-1][y*6+x],
                        "three-frame literal world/strip/interface result excludes stale labels");
                for (unsigned x=12;x<pitch;++x)
                    check(result[y*pitch+x]==(id==PRIMARY_SURFACE?0xd1:0xb2),"consecutive scroll preserves padding");
            }
        }
    }
    seed(FRAME_BUFFER,500,0xf3);InvalidateScreen();EndFrameBufferRender();
    check(sgp::RefreshNativeVideo(),"explicit full invalidation still transfers after EndFrameBufferRender");
    const auto back=surfaceBytes(BACKBUFFER);
    for (unsigned y=0;y<6;++y) for (unsigned x=0;x<6;++x)
        check(read(back.data(),y*pitch+x*2)==500+y*10+x,"explicit full copy retains every visible pixel");
}
void run()
{
    check(sgp::ConfigureNativeVideo(6,6,16,8,"scroll failure fixture"),"configure real video");
    check(InitializeVideoManager(nullptr,0,nullptr),"initialize real video");
    check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"real X11 presentation");
    check(sgp::RefreshNativeVideo(),"initial full present");
    UINT32 pitch=0;check(LockVideoSurface(FRAME_BUFFER,&pitch),"read frame pitch");
    UnLockVideoSurface(FRAME_BUFFER);
    zBuffer=InitZBuffer(pitch,6);check(zBuffer,"actual owned Z");
    check(sgp::OwnedZBufferView(zBuffer,&zView),"actual Z metadata");
    seed(PRIMARY_SURFACE,200,0xd1);seed(BACKBUFFER,0,0xb2);seed(FRAME_BUFFER,100,0xf3);
    std::memset(zView.data,0xa4,zView.bytes);
    const std::array<Bytes,3> before{surfaceBytes(PRIMARY_SURFACE),surfaceBytes(BACKBUFFER),surfaceBytes(FRAME_BUFFER)};
    const Bytes beforeZ(zView.data,zView.data+zView.bytes);
    auto unchanged=[&]() {
        check(surfaceBytes(PRIMARY_SURFACE)==before[0],"rollback restores entire primary including padding");
        check(surfaceBytes(BACKBUFFER)==before[1],"rollback restores entire back including padding");
        check(surfaceBytes(FRAME_BUFFER)==before[2],"rollback restores entire frame including padding");
        check(std::memcmp(zView.data,beforeZ.data(),zView.bytes)==0,"rollback restores entire Z including padding");
    };
    state={5,true,{{0,1,6,5},0x08,1,1},zBuffer};
    sgp::SetNativeVideoRegionSource(&region);
    sgp::SetNativeVideoScrollHooks({&render,&begin,&finish,&end,&committedFrame});
    InvalidateRegion(5,0,6,1); // regular dirty work is transferred before scroll
    InvalidateRegionEx(2,2,4,4,0); // world portion must be skipped while scrolling
    auto* foreignZ=sgp::AllocateBackgroundBuffer(6,6);check(foreignZ,"other owned buffer kind");
    state.zBuffer=reinterpret_cast<UINT16*>(foreignZ);
    check(!sgp::RefreshNativeVideo() && begins==0,"wrong Z ownership refuses before hooks");
    unchanged();check(sgp::FreeBackgroundBuffer(foreignZ),"other allocation cleanup");state.zBuffer=zBuffer;
    beginFailure=true;
    check(!sgp::RefreshNativeVideo() && !overlayActive,"snapshot preparation failure refuses before mutation");
    unchanged();beginFailure=false;
    for (const auto mode:{Failure::Render,Failure::Throw,Failure::Finish}) {
        failure=mode;allocationAttempts=0;
        const bool ok=sgp::RefreshNativeVideo();
        refuseAllocations=false;
        check(!ok && allocationAttempts==0,"failure rollback performs no allocation");
        check(!overlayActive && state.scrolling && commits==0 && presents==0,"failure retains scroll and aborts overlay transaction");
        unchanged();
        check(!CanBlitToFrameBuffer(),"dirty work retained for a clean retry");
    }
    check(rollbacks==3,"each failed mutation ended its transaction");
    failure=Failure::None;
    check(sgp::RefreshNativeVideo(),"successful retry through real production refresh");
    check(commits==1 && presents==1 && !state.scrolling && !overlayActive,"only successful present commits");
    check(CanBlitToFrameBuffer(),"successful retry clears pending dirty work");
    const std::array<UINT16,36> expected{
        0,1,2,3,4,105,
        0xc001,10,11,12,13,14,
        0xc002,20,0xbeef,22,23,24,
        0xc003,30,31,32,33,34,
        0xc004,40,41,42,43,44,
        50,51,52,53,54,55};
    for (const auto id:{PRIMARY_SURFACE,BACKBUFFER}) {
        auto result=surfaceBytes(id);
        for (unsigned y=0;y<6;++y) {
            for (unsigned x=0;x<6;++x)
                check(read(result.data(),y*pitch+x*2)==expected[y*6+x],"literal refresh result proves dirty/shift/strip/finish order");
            for (unsigned x=12;x<pitch;++x)
                check(result[y*pitch+x]==(id==PRIMARY_SURFACE?0xd1:0xb2),"successful visible-only copies preserve row padding");
        }
    }
    check(read(zView.data,0)==0x1234 && zView.data[zView.bytes-1]==0x7b,"successful callback Z changes retained");
    check(sgp::RefreshNativeVideo() && presents==2,"non-scrolling successful frame also signals commit");
    consecutiveScrollFrames(pitch);
    sgp::SetNativeVideoRegionSource(nullptr);sgp::SetNativeVideoScrollHooks({});
    check(ShutdownZBuffer(zBuffer),"Z cleanup");zBuffer=nullptr;
    check(sgp::ShutdownNativeVideo() && ShutdownVideoSurfaceManager(),"real manager teardown");
}
}
int main()
{
    try {
        run();std::cout<<"PASS: "<<checks<<" production scroll-refresh checks; allocation refusal, rollback, literal ordering and real X11 present; diagnostic hooks only\n";return 0;
    } catch (const std::exception& error) {
        refuseAllocations=false;std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';
    } catch (...) {
        refuseAllocations=false;std::cerr<<"FAIL: unexpected exception\n";
    }
    sgp::SetNativeVideoRegionSource(nullptr);sgp::SetNativeVideoScrollHooks({});
    for (const auto id:{PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER}) UnLockVideoSurface(id);
    if (zBuffer) ShutdownZBuffer(zBuffer);
    (void)sgp::ShutdownNativeVideo();ShutdownVideoSurfaceManager();return 1;
}
