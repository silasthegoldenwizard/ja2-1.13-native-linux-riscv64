/*
 * P710 native snapshot update: tests/native-core/native_overlay_scroll_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Links the real Render Dirty transaction and its actual background/surface
// services. Only engine configuration globals and an overlay callback fixture
// are supplied; no replacement dirty/overlay game logic is defined here.
#include "Render Dirty.h"
#include "native_background.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

UINT16 SCREEN_WIDTH=6,SCREEN_HEIGHT=6;
INT16 gsVIEWPORT_START_X=0,gsVIEWPORT_END_X=6;
INT16 gsVIEWPORT_START_Y=0,gsVIEWPORT_END_Y=6;
INT16 gsVIEWPORT_WINDOW_START_Y=0,gsVIEWPORT_WINDOW_END_Y=6;
extern BACKGROUND_SAVE gBackSaves[1500];
extern VIDEO_OVERLAY gVideoOverlays[100];
extern UINT32 guiNumBackSaves,guiNumVideoOverlays;
extern BOOLEAN gfViewportDirty;
namespace {
unsigned checks=0,calls=0;
bool throwFromCallback=false,changeDuringCallback=false;
void check(bool okay,const char* reason) { ++checks;if (!okay) throw std::runtime_error(reason); }
UINT16 read(const BYTE* bytes,std::size_t offset) { UINT16 value;std::memcpy(&value,bytes+offset,2);return value; }
void write(BYTE* bytes,std::size_t offset,UINT16 value) { std::memcpy(bytes+offset,&value,2); }
void overlayCallback(VIDEO_OVERLAY* overlay) {
    ++calls;
    check(overlay->uiDestBuff==BACKBUFFER,"actual alternate destination reaches callback");
    if (throwFromCallback) throw std::runtime_error("fixture overlay failure");
    if (changeDuringCallback) { DeleteVideoOverlaysArea();return; }
    UINT32 pitch=0;auto* pixels=sgp::LockBackgroundSurface(overlay->uiDestBuff,&pitch);
    check(pixels,"actual callback surface lock");write(pixels,pitch+2,0x7777);
    sgp::UnlockBackgroundSurface(overlay->uiDestBuff);
}
UINT32 makeSurface() {
    VSURFACE_DESC desc{};desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    desc.usWidth=6;desc.usHeight=6;desc.ubBitDepth=16;
    UINT32 id=0;check(AddVideoSurface(&desc,&id),"real native surface creation");return id;
}
void seed() {
    gBackSaves[0]={};gVideoOverlays[0]={};guiNumBackSaves=guiNumVideoOverlays=1;
    auto& b=gBackSaves[0];b.fAllocated=TRUE;b.fFreeMemory=TRUE;
    b.uiFlags=BGND_FLAG_PERMANENT|BGND_FLAG_SAVERECT|BGND_FLAG_SAVE_Z;
    b.fZBuffer=TRUE;b.sLeft=b.sTop=1;b.sRight=b.sBottom=3;b.sWidth=b.sHeight=2;
    b.pSaveArea=sgp::AllocateBackgroundBuffer(2,2);b.pZSaveArea=sgp::AllocateBackgroundBuffer(2,2);
    check(b.pSaveArea&&b.pZSaveArea,"actual owned background color/Z saves");
    auto& o=gVideoOverlays[0];o.fAllocated=TRUE;o.fActivelySaving=TRUE;o.uiBackground=0;
    o.pBackground=&b;o.BltCallback=overlayCallback;o.uiDestBuff=FRAME_BUFFER;
    o.pSaveArea=sgp::AllocateBackgroundBuffer(2,2);check(o.pSaveArea,"actual owned overlay save");
    for (unsigned i=0;i<4;++i) { b.pSaveArea[i]=INT16(200+i);b.pZSaveArea[i]=INT16(300+i);o.pSaveArea[i]=INT16(100+i); }
    gDirtyClipRect={0,0,6,6};gfViewportDirty=FALSE;
    UINT32 pitch=0;auto* pixels=sgp::LockBackgroundSurface(BACKBUFFER,&pitch);check(pixels,"seed actual BACK");
    for(unsigned y=0;y<6;++y)for(unsigned x=0;x<6;++x)write(pixels,y*pitch+x*2,UINT16(y*10+x));
    sgp::UnlockBackgroundSurface(BACKBUFFER);
}
struct Saved {
    std::array<BACKGROUND_SAVE,1500> backgrounds;
    std::array<VIDEO_OVERLAY,100> overlays;
    std::array<INT16,4> color,z,overlay;
    SGPRect clip;BOOLEAN dirty;
    UINT32 backgroundCount=guiNumBackSaves,overlayCount=guiNumVideoOverlays;
    Saved() :clip(gDirtyClipRect),dirty(gfViewportDirty) {
        std::memcpy(backgrounds.data(),gBackSaves,sizeof(gBackSaves));
        std::memcpy(overlays.data(),gVideoOverlays,sizeof(gVideoOverlays));
        std::memcpy(color.data(),gBackSaves[0].pSaveArea,8);
        std::memcpy(z.data(),gBackSaves[0].pZSaveArea,8);
        std::memcpy(overlay.data(),gVideoOverlays[0].pSaveArea,8);
    }
    void verify() const {
        check(guiNumBackSaves==backgroundCount&&guiNumVideoOverlays==overlayCount,"actual counts restored");
        check(std::memcmp(backgrounds.data(),gBackSaves,sizeof(gBackSaves))==0,"all background structs restored byte exactly");
        check(std::memcmp(overlays.data(),gVideoOverlays,sizeof(gVideoOverlays))==0,"all overlay structs restored byte exactly");
        check(std::memcmp(color.data(),gBackSaves[0].pSaveArea,8)==0,"original color bytes restored");
        check(std::memcmp(z.data(),gBackSaves[0].pZSaveArea,8)==0,"original Z-save bytes restored");
        check(std::memcmp(overlay.data(),gVideoOverlays[0].pSaveArea,8)==0,"original overlay bytes restored");
        check(std::memcmp(&clip,&gDirtyClipRect,sizeof(clip))==0&&dirty==gfViewportDirty,"dirty clip/viewport flag restored");
    }
};
void run() {
    seed();Saved original;
    const auto refuseAlias=[&](INT16*& slot,INT16* alias,const char* reason) {
        auto* owned=slot;slot=alias;
        Saved before;
        check(!BeginNativeOverlayScroll(),reason);
        before.verify(); // Refusal preserves every array/count and actual save byte.
        slot=owned;
        check(BeginNativeOverlayScroll(),"alias refusal left no active transaction");
        EndNativeOverlayScroll(false);original.verify();
    };
    refuseAlias(gVideoOverlays[0].pSaveArea,gBackSaves[0].pSaveArea,
        "background-to-overlay duplicate private ownership refused");
    refuseAlias(gBackSaves[0].pZSaveArea,gBackSaves[0].pSaveArea,
        "background color-to-Z duplicate private ownership refused");
    gVideoOverlays[1]=gVideoOverlays[0];guiNumVideoOverlays=2;
    {
        Saved before;
        check(!BeginNativeOverlayScroll(),"overlay-to-overlay duplicate private ownership refused");
        before.verify();
    }
    gVideoOverlays[1]={};guiNumVideoOverlays=1;
    check(BeginNativeOverlayScroll(),"overlay alias refusal left no active transaction");
    EndNativeOverlayScroll(false);original.verify();
    check(BeginNativeOverlayScroll(),"snapshot actual arrays and owned saves");
    check(!BeginNativeOverlayScroll(),"nested transaction refused without replacing snapshot");
    check(FinishNativeOverlayScroll(1,0),"real shifted restore/save/callback sequence");
    check(calls==1,"real callback executed exactly once");
    const INT16 expected[]{11,100,21,102};
    check(std::memcmp(gVideoOverlays[0].pSaveArea,expected,sizeof(expected))==0,"literal save after shifted restore before callback");
    UINT32 pitch=0;auto* back=sgp::LockBackgroundSurface(BACKBUFFER,&pitch);check(back,"observe actual BACK");
    check(read(back,pitch+2)==0x7777&&read(back,pitch+4)==100&&read(back,2*pitch+6)==103,"literal callback plus shifted restore pixels");
    sgp::UnlockBackgroundSurface(BACKBUFFER);
    gDirtyClipRect={1,1,4,4};gfViewportDirty=TRUE;
    EndNativeOverlayScroll(false);original.verify();

    check(BeginNativeOverlayScroll(),"begin real lifecycle rollback");
    auto* old=gVideoOverlays[0].pSaveArea;
    DeleteVideoOverlaysArea();
    sgp::BackgroundView view;check(sgp::OwnedBackgroundBufferView(old,&view),"original freed overlay allocation remains alive until End");
    SaveVideoOverlaysArea(BACKBUFFER);
    auto* created=gVideoOverlays[0].pSaveArea;check(created&&created!=old,"real save allocates a distinct tracked replacement");
    check(FreeBackgroundRect(0),"real background lifecycle defers color/Z frees");
    check(sgp::OwnedBackgroundBufferView(original.color.data(),&view)==false,"foreign pointer remains unowned");
    check(sgp::OwnedBackgroundBufferView(original.backgrounds[0].pSaveArea,&view),"original background memory retained");
    EndNativeOverlayScroll(false);original.verify();
    check(!sgp::OwnedBackgroundBufferView(created,&view),"rollback frees actual new allocation");

    check(BeginNativeOverlayScroll(),"begin exception rollback");throwFromCallback=true;
    check(!FinishNativeOverlayScroll(1,0),"actual throwing callback reports failure");throwFromCallback=false;
    EndNativeOverlayScroll(false);original.verify();

    check(BeginNativeOverlayScroll(),"begin callback lifecycle failure");changeDuringCallback=true;
    check(!FinishNativeOverlayScroll(1,0),"callback invalidated saves cannot claim successful finish");changeDuringCallback=false;
    EndNativeOverlayScroll(false);original.verify();

    check(BeginNativeOverlayScroll(),"begin locked BACK failure");
    back=sgp::LockBackgroundSurface(BACKBUFFER,&pitch);check(back,"hold actual external BACK lock");
    check(!FinishNativeOverlayScroll(1,0),"real restore lock failure propagates");
    sgp::UnlockBackgroundSurface(BACKBUFFER);EndNativeOverlayScroll(false);original.verify();

    check(BeginNativeOverlayScroll(),"begin successful replacement commit");
    old=gVideoOverlays[0].pSaveArea;DeleteVideoOverlaysArea();
    check(FinishNativeOverlayScroll(1,0),"real replacement save/callback finish");
    created=gVideoOverlays[0].pSaveArea;EndNativeOverlayScroll(true);
    check(!sgp::OwnedBackgroundBufferView(old,&view)&&sgp::OwnedBackgroundBufferView(created,&view),"commit frees deferred original and retains actual replacement");
    check(gVideoOverlays[0].uiDestBuff==FRAME_BUFFER,"alternate destination restored after successful callback");

    auto* actual=gVideoOverlays[0].pSaveArea;std::array<INT16,4> foreign{};
    gVideoOverlays[0].pSaveArea=foreign.data();
    check(!BeginNativeOverlayScroll(),"unknown buffer extent refuses before activation");gVideoOverlays[0].pSaveArea=actual;
    auto* zBuffer=InitZBuffer(pitch,6);check(zBuffer,"actual Z allocation for ownership refusal");
    gVideoOverlays[0].pSaveArea=reinterpret_cast<INT16*>(zBuffer);
    check(!BeginNativeOverlayScroll(),"known Z storage is refused as an owned overlay save");
    gVideoOverlays[0].pSaveArea=actual;check(ShutdownZBuffer(zBuffer),"actual Z cleanup");
    check(BeginNativeOverlayScroll(),"failed begin leaves no active transaction");EndNativeOverlayScroll(false);
    DeleteVideoOverlaysArea();check(FreeBackgroundRect(0),"real background cleanup");
    std::memset(gVideoOverlays,0,sizeof(gVideoOverlays));guiNumVideoOverlays=0;
}
}
int main() {
    try {
        check(InitializeVideoSurfaceManager(),"actual manager initialization");
        const UINT32 ids[]{makeSurface(),makeSurface(),makeSurface(),makeSurface()};
        check(sgp::BindReservedVideoSurfaces(ids),"actual reserved surfaces bound");run();
        check(sgp::ReleaseReservedVideoSurfaces(),"actual reserved surface release");
        check(ShutdownVideoSurfaceManager(),"actual manager cleanup");
        std::cout<<"PASS: "<<checks<<" real dirty/overlay scroll transaction checks\n";return 0;
    } catch(const std::exception& error) {
        EndNativeOverlayScroll(false);
        std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';return 1;
    }
}
