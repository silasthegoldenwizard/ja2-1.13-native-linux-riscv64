/*
 * P710 native notice preparation: tests/native-core/native_video_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add reserved framebuffer/video-manager contract tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/native_video_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "video.h"
#include "Font.h"
#include "clip_state.h"
#include <climits>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace
{
unsigned checks=0;
void check(bool b, const char* message)
{ ++checks; if (!b) throw std::runtime_error(message); }
sgp::NativeVideoRegionState regionState{10,false};
sgp::NativeVideoRegionState readRegionState() noexcept { return regionState; }
constexpr std::size_t W=32, H=20, P=72;
using Bytes=std::vector<BYTE>;
Bytes image(UINT16 color)
{
    Bytes v(P*H,0xa5);
    for (std::size_t y=0; y<H; ++y) for (std::size_t x=0; x<W; ++x)
        std::memcpy(v.data()+y*P+x*2,&color,2);
    return v;
}
Bytes read(UINT32 id)
{
    UINT32 pitch=0; auto* p=LockVideoSurface(id,&pitch); check(p && pitch==P,"read padded surface");
    Bytes v(p,p+P*H); UnLockVideoSurface(id); return v;
}
void expect(UINT32 id, const Bytes& v) { check(read(id)==v,"whole pixels and row sentinels"); }
void fill(UINT32 id, UINT16 color)
{ blt_vs_fx fx{}; fx.ColorFill=color; check(BltVideoSurface(id,0,0,0,0,VS_BLT_COLORFILL,&fx),"unclipped seed fill"); }
void patch(Bytes& v, INT32 l, INT32 t, INT32 r, INT32 b, UINT16 color)
{
    for (INT32 y=t; y<b; ++y) for (INT32 x=l; x<r; ++x)
        std::memcpy(v.data()+static_cast<std::size_t>(y)*P+static_cast<std::size_t>(x)*2,&color,2);
}
void inactive()
{
    check(ghFrameBuffer==nullptr,"inactive original framebuffer identity");
    for (UINT32 id : {PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER,MOUSE_BUFFER})
    {
        HVSURFACE p=nullptr; UINT32 pitch=99;
        check(!GetVideoSurface(&p,id) && !p,"inactive alias lookup");
        check(!LockVideoSurface(id,&pitch) && pitch==0,"inactive alias lock");
        check(!DeleteVideoSurfaceFromIndex(id) && !ColorFillVideoSurfaceArea(id,0,0,1,1,1),"inactive alias operations");
    }
}
}
int main()
{
    try
    {
        check(sgp::ShutdownNativeVideo(),"initial idempotent shutdown");
        check(InitializeVideoSurfaceManager(),"surface initialization"); inactive();
        SGPRect clip{}; SetClippingRect(&clip);
        VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=32; d.usHeight=20; d.ubBitDepth=16;
        UINT32 ordinary=0; check(AddVideoSurface(&d,&ordinary),"preexisting ordinary surface");
        fill(ordinary,0x1234);
        HVSURFACE keep=nullptr; check(GetVideoSurface(&keep,ordinary),"ordinary pointer");
        const auto preserved=read(ordinary);
        check(!sgp::ConfigureNativeVideo(0,20,16,8,"bad") && !sgp::ConfigureNativeVideo(32,0,16,8,"bad"),"zero geometry");
        check(!sgp::ConfigureNativeVideo(32,20,8,8,"bad") && !sgp::ConfigureNativeVideo(32,20,16,0,"bad"),"depth and scale");
        check(!sgp::ConfigureNativeVideo(65536,20,16,1,"bad") && !sgp::ConfigureNativeVideo(32,20,16,UINT_MAX,"bad"),"dimension and scale overflow");
        char title[]="owned title"; check(sgp::ConfigureNativeVideo(32,20,16,8,title),"configuration"); title[0]='X';
        check(!InitializeVideoManager(&d,0,nullptr) && !InitializeVideoManager(nullptr,1,nullptr) && !InitializeVideoManager(nullptr,0,&d),"unsupported initialization arguments");
        inactive();
        GetClippingRect(&clip);
        check(clip.iLeft==0 && clip.iTop==0 && clip.iRight==0 && clip.iBottom==0,"rejected initialization preserves empty drawing clip");
        check(InitializeVideoManager(nullptr,0,nullptr),"real initialization");
        GetClippingRect(&clip);
        check(clip.iLeft==0 && clip.iTop==0 && clip.iRight==32 && clip.iBottom==20,"startup initializes full drawing clip");
        check(ColorFillVideoSurfaceArea(FRAME_BUFFER,31,19,32,20,0x5678),"startup clipped draw at bottom-right edge");
        const auto startup=read(FRAME_BUFFER); UINT16 startupPixel=0;
        std::memcpy(&startupPixel,startup.data()+19*P+31*2,2);
        check(startupPixel==0x5678,"startup drawing clip admits the final framebuffer pixel");
        check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"real X11 required");
        HVSURFACE realFrame=nullptr;check(GetVideoSurface(&realFrame,FRAME_BUFFER) && ghFrameBuffer==realFrame,"original framebuffer identity is actual native handle");
        check(!sgp::ConfigureNativeVideo(32,20,16,8,"active"),"active configuration refusal");
        UINT16 w=0,h=0; UINT8 depth=0; GetCurrentVideoSettings(&w,&h,&depth);
        check(w==32 && h==20 && depth==16,"video settings"); GetCurrentVideoSettings(nullptr,nullptr,nullptr);
        const UINT32 aliases[]={PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER};
        HVSURFACE pointers[3]{};
        UINT32 backing[3]{};
        for (std::size_t i=0; i<3; ++i)
        {
            check(GetVideoSurface(&pointers[i],aliases[i]),"active alias");
            backing[i]=sgp::ReservedVideoSurfaceBackingId(aliases[i]);
            check(backing[i] && backing[i]<PRIMARY_SURFACE,"ordinary backing capability");
            HVSURFACE p=nullptr; check(GetVideoSurface(&p,backing[i]) && p==pointers[i],"backing and alias identity");
            check(!DeleteVideoSurfaceFromIndex(aliases[i]) && !DeleteVideoSurfaceFromIndex(backing[i]) && !DeleteVideoSurface(pointers[i]),"all deletion forms protected");
            check(SetFontDestBuffer(aliases[i],0,0,32,20,FALSE),"original font destination accepts alias");
            for (std::size_t j=0; j<i; ++j) check(pointers[i]!=pointers[j] && backing[i]!=backing[j],"three distinct surfaces");
        }
        check(!ShutdownVideoSurfaceManager(),"external manager shutdown refused");
        check(sgp::RefreshNativeVideo(),"initial full refresh"); fill(FRAME_BUFFER,7); fill(BACKBUFFER,3); fill(PRIMARY_SURFACE,2);
        SGPRect tiny{0,0,1,1}; SetClippingRect(&tiny);
        check(InitializeVideoManager(nullptr,0,nullptr),"repeated active initialization");
        GetClippingRect(&clip);
        check(clip.iLeft==0 && clip.iTop==0 && clip.iRight==1 && clip.iBottom==1,"repeated initialization preserves caller drawing clip");
        InvalidateRegion(2,3,10,7);
        check(!CanBlitToFrameBuffer(),"pending framebuffer transfer is not BUFFER_READY");
        using Lock=PTR (*)(UINT32*);
        using Unlock=void (*)();
        const Lock locks[]={LockPrimarySurface,LockBackBuffer,LockFrameBuffer};
        const Unlock unlocks[]={UnlockPrimarySurface,UnlockBackBuffer,UnlockFrameBuffer};
        for (std::size_t i=0; i<3; ++i)
        {
            UINT32 pitch=0; auto* p=static_cast<BYTE*>(locks[i](&pitch)); check(p && pitch==P,"legacy exclusive lock");
            UINT32 nested=99; check(!LockVideoSurface(aliases[i],&nested) && nested==0 && !LockVideoSurfaceBuffer(pointers[i],&nested),"shared exclusive lock");
            check(!sgp::RefreshNativeVideo() && !sgp::ShutdownNativeVideo() && !ShutdownVideoSurfaceManager(),"locked refresh and shutdown refusal");
            RefreshScreen(nullptr); ShutdownVideoManager();
            check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"refusal keeps window alive");
            for (std::size_t j=0; j<3; ++j)
            {
                const auto expected=image(static_cast<UINT16>(j==0 ? 2 : j==1 ? 3 : 7));
                if (i==j) check(Bytes(p,p+P*H)==expected,"locked bytes unchanged");
                else expect(aliases[j],expected);
            }
            UnLockVideoSurfaceBuffer(pointers[i]);
            check(LockVideoSurface(aliases[i],&pitch)!=nullptr,"object unlock releases legacy lock");
            check(!locks[i](&nested),"alias blocks legacy lock"); unlocks[i]();
            check(LockVideoSurfaceBuffer(pointers[i],&pitch)!=nullptr,"object lock"); check(!LockVideoSurface(aliases[i],&nested),"object blocks alias lock"); UnLockVideoSurface(aliases[i]);
        }
        check(sgp::RefreshNativeVideo(),"pending dirty region survives every refusal");
        check(CanBlitToFrameBuffer(),"successful transfer restores BUFFER_READY");
        auto expected=image(3); patch(expected,2,3,10,7,7); expect(BACKBUFFER,expected); expect(PRIMARY_SURFACE,expected); expect(FRAME_BUFFER,image(7));
        // Invalid rectangles and a null untrusted array must not request a full copy.
        fill(BACKBUFFER,9); fill(FRAME_BUFFER,11);
        InvalidateRegion(5,5,2,2); InvalidateRegion(1,1,1,2); InvalidateRegion(1,2,2,2);
        InvalidateRegion(INT_MIN,INT_MIN,-1,-1); InvalidateRegion(INT_MAX,INT_MAX,INT_MIN,INT_MIN);
        InvalidateRegions(nullptr,UINT_MAX);
        check(sgp::RefreshNativeVideo(),"no-dirty direct backbuffer refresh"); expect(BACKBUFFER,image(9)); expect(PRIMARY_SURFACE,image(9));
        SGPRect regions[]={{INT_MIN,INT_MIN,2,2},{30,19,INT_MAX,INT_MAX},{3,4,3,5}};
        InvalidateRegions(regions,3); check(sgp::RefreshNativeVideo(),"clipped signed region array");
        expected=image(9); patch(expected,0,0,2,2,11); patch(expected,30,19,32,20,11); expect(BACKBUFFER,expected); expect(PRIMARY_SURFACE,expected);
        InvalidateRegion(INT_MIN,INT_MIN,INT_MAX,INT_MAX); check(sgp::RefreshNativeVideo(),"extreme full rectangle"); expect(PRIMARY_SURFACE,image(11));
        fill(FRAME_BUFFER,12); fill(BACKBUFFER,13);
        for (unsigned i=0; i<129; ++i) InvalidateRegion(0,0,1,1);
        check(sgp::RefreshNativeVideo(),"region overflow full fallback"); expect(BACKBUFFER,image(12)); expect(PRIMARY_SURFACE,image(12));
        fill(FRAME_BUFFER,14); SGPRect one{0,0,1,1}; InvalidateRegions(&one,UINT_MAX);
        SuspendVideoManager(); check(!sgp::RefreshNativeVideo(),"suspended pending work"); expect(BACKBUFFER,image(12)); expect(PRIMARY_SURFACE,image(12));
        check(RestoreVideoManager() && sgp::RefreshNativeVideo(),"resume without recreation"); expect(PRIMARY_SURFACE,image(14));
        for (auto invalidate : {InvalidateScreen,InvalidateFrameBuffer})
        {
            fill(FRAME_BUFFER,15); fill(BACKBUFFER,16); StartFrameBufferRender(); invalidate();
            check(!CanBlitToFrameBuffer(),"full invalidation remains dirty until transferred");
            check(sgp::RefreshNativeVideo(),"legacy full invalidation"); expect(BACKBUFFER,image(15)); expect(PRIMARY_SURFACE,image(15));
        }
        fill(FRAME_BUFFER,17); fill(BACKBUFFER,18); StartFrameBufferRender();
        InvalidateRegion(2,3,10,7); EndFrameBufferRender();
        check(!CanBlitToFrameBuffer(),"end-render marks BUFFER_DIRTY without full invalidation");
        check(sgp::RefreshNativeVideo(),"end-render transfers only requested dirty work");
        expected=image(18); patch(expected,2,3,10,7,17); expect(BACKBUFFER,expected); expect(PRIMARY_SURFACE,expected);
        check(CanBlitToFrameBuffer(),"successful end-render clears BUFFER_DIRTY");
        fill(FRAME_BUFFER,19); StartFrameBufferRender(); EndFrameBufferRender();
        check(!CanBlitToFrameBuffer(),"end-render without rectangles is still pending");
        check(sgp::RefreshNativeVideo(),"end-render without rectangles preserves direct backbuffer work");
        expect(BACKBUFFER,expected); expect(PRIMARY_SURFACE,expected);
        check(CanBlitToFrameBuffer(),"frame available"); UINT32 pitch=0; check(LockFrameBuffer(&pitch)!=nullptr,"frame lock"); check(!CanBlitToFrameBuffer(),"locked frame unavailable"); UnlockFrameBuffer();
        sgp::SetNativeVideoRegionSource(&readRegionState);
        fill(FRAME_BUFFER,21); fill(BACKBUFFER,22); fill(PRIMARY_SURFACE,23);
        InvalidateRegionEx(2,7,8,13,UINT_MAX); // split across viewport; flags are unused in the original consumer
        check(!CanBlitToFrameBuffer(),"extended regions mark real pending transfer");
        check(LockFrameBuffer(&pitch)!=nullptr,"extended-region frame lock");
        check(!sgp::RefreshNativeVideo(),"extended regions survive locked refresh");
        expect(BACKBUFFER,image(22)); expect(PRIMARY_SURFACE,image(23)); UnlockFrameBuffer();
        regionState.scrolling=true;
        check(!sgp::RefreshNativeVideo(),"unbound scroll request fails before copy");
        expect(BACKBUFFER,image(22)); expect(PRIMARY_SURFACE,image(23));
        regionState.scrolling=false;
        check(sgp::RefreshNativeVideo(),"extended retry after lock and scroll refusal");
        expected=image(22); patch(expected,2,7,8,13,21);
        expect(BACKBUFFER,expected); expect(PRIMARY_SURFACE,expected); expect(FRAME_BUFFER,image(21));
        check(CanBlitToFrameBuffer(),"successful extended transfer clears work");
        fill(FRAME_BUFFER,24); fill(BACKBUFFER,25);
        for (const auto& r : {SGPRect{5,5,2,2},SGPRect{1,1,1,2},SGPRect{INT_MIN,INT_MIN,-1,-1},SGPRect{INT_MAX,INT_MAX,INT_MIN,INT_MIN}})
            InvalidateRegionEx(r.iLeft,r.iTop,r.iRight,r.iBottom,0);
        check(CanBlitToFrameBuffer() && sgp::RefreshNativeVideo(),"invalid extended geometry is empty");
        expect(BACKBUFFER,image(25)); expect(PRIMARY_SURFACE,image(25));
        InvalidateRegionEx(INT_MIN,INT_MIN,INT_MAX,INT_MAX,0);
        check(sgp::RefreshNativeVideo(),"extreme extended geometry clipped safely"); expect(PRIMARY_SURFACE,image(24));
        fill(FRAME_BUFFER,26); fill(BACKBUFFER,27);
        for (unsigned i=0;i<128;++i) { InvalidateRegion(0,0,1,1); InvalidateRegionEx(0,0,1,1,0); }
        check(sgp::RefreshNativeVideo(),"independent regular and extended budgets");
        expected=image(27); patch(expected,0,0,1,1,26); expect(PRIMARY_SURFACE,expected);
        fill(FRAME_BUFFER,28); fill(BACKBUFFER,29);
        for (unsigned i=0;i<129;++i) InvalidateRegionEx(0,0,1,1,0);
        check(sgp::RefreshNativeVideo(),"extended overflow forces real full refresh"); expect(PRIMARY_SURFACE,image(28));
        fill(FRAME_BUFFER,30); fill(BACKBUFFER,31);
        // Each cross-viewport request occupies two original extended slots.
        for (unsigned i=0;i<64;++i) InvalidateRegionEx(0,9,1,11,0);
        check(sgp::RefreshNativeVideo(),"exact split capacity remains a partial transfer");
        expected=image(31); patch(expected,0,9,1,11,30); expect(PRIMARY_SURFACE,expected);
        fill(FRAME_BUFFER,32); fill(BACKBUFFER,33);
        for (unsigned i=0;i<65;++i) InvalidateRegionEx(0,9,1,11,0);
        check(sgp::RefreshNativeVideo(),"viewport split retains original overflow boundary"); expect(PRIMARY_SURFACE,image(32));
        // Original full-refresh request selects full GAME_SCREEN copy, disables
        // scrolling, survives a refused locked transfer and clears on commit.
        fill(FRAME_BUFFER,40);fill(BACKBUFFER,41);regionState.gameScreen=true;regionState.scrolling=true;
        gfNextRefreshFullScreen=TRUE;check(LockFrameBuffer(&pitch)!=nullptr,"hold original full-refresh frame");
        check(!sgp::RefreshNativeVideo() && gfNextRefreshFullScreen,"full request retained on locked refusal");UnlockFrameBuffer();
        check(sgp::RefreshNativeVideo() && !gfNextRefreshFullScreen,"game full request commits without scroll hooks");
        expect(BACKBUFFER,image(40));expect(PRIMARY_SURFACE,image(40));
        fill(FRAME_BUFFER,42);fill(BACKBUFFER,43);regionState.gameScreen=false;regionState.scrolling=false;
        gfNextRefreshFullScreen=TRUE;check(sgp::RefreshNativeVideo() && !gfNextRefreshFullScreen,"non-game request preserves original backbuffer presentation");
        expect(BACKBUFFER,image(43));expect(PRIMARY_SURFACE,image(43));
        sgp::SetNativeVideoRegionSource(nullptr);
        expect(ordinary,preserved);
        auto* borrowed=LockVideoSurfaceBuffer(keep,&pitch); check(borrowed!=nullptr,"ordinary borrow during video teardown");
        check(sgp::ShutdownNativeVideo() && sgp::ShutdownNativeVideo(),"idempotent teardown with ordinary lock"); ShutdownVideoManager(); inactive();
        check(Bytes(borrowed,borrowed+P*H)==preserved && !ShutdownVideoSurfaceManager(),"ordinary borrow survives video teardown"); UnLockVideoSurfaceBuffer(keep);
        check(!DeleteVideoSurface(pointers[0]) && !LockVideoSurfaceBuffer(pointers[0],&pitch),"stale pointer after teardown");
        expect(ordinary,preserved); HVSURFACE same=nullptr; check(GetVideoSurface(&same,ordinary) && same==keep,"ordinary identity retained");
        check(sgp::ConfigureNativeVideo(31,19,16,8,"reconfigured"),"inactive geometry reconfiguration");
        check(InitializeVideoManager(nullptr,0,nullptr),"reinitialization");
        GetClippingRect(&clip);
        check(clip.iLeft==0 && clip.iTop==0 && clip.iRight==31 && clip.iBottom==19,"reinitialization resets drawing clip to new geometry");
        check(ColorFillVideoSurfaceArea(FRAME_BUFFER,30,18,31,19,0x4321),"reinitialized clipped edge draw");
        auto* reinitialized=static_cast<BYTE*>(LockFrameBuffer(&pitch)); check(reinitialized!=nullptr,"reinitialized frame pixels");
        UINT16 reinitializedPixel=0; std::memcpy(&reinitializedPixel,reinitialized+18*pitch+30*2,2); UnlockFrameBuffer();
        check(reinitializedPixel==0x4321,"reinitialized clip admits new framebuffer edge");
        HVSURFACE newer=nullptr; check(GetVideoSurface(&newer,PRIMARY_SURFACE) && newer!=pointers[0],"no pointer resurrection");
        check(!LockVideoSurfaceBuffer(pointers[0],&pitch) && !DeleteVideoSurfaceFromIndex(backing[0]),"no stale capability resurrection");
        check(sgp::ShutdownNativeVideo(),"final video shutdown"); expect(ordinary,preserved); check(DeleteVideoSurfaceFromIndex(ordinary),"ordinary owner can delete");
        check(ShutdownVideoSurfaceManager() && ShutdownVideoSurfaceManager(),"final surface teardown");
        std::cout << "PASS: " << checks << " real X11 video/extended-region checks; dirty transfers, ownership, locks and whole padded buffers; unbound scrolling safely refused\n"; return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n'; for (UINT32 id : {PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER}) UnLockVideoSurface(id);
        (void)sgp::ShutdownNativeVideo(); ShutdownVideoSurfaceManager(); return 1;
    }
}
