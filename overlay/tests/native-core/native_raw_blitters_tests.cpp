/*
 * P710 native snapshot update: tests/native-core/native_raw_blitters_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeRawBlitters.h"
#include "vobject_blitters.h"
#include "native_background.h"
#include <array>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
unsigned checks=0;
void check(bool value,const char* reason) { ++checks;if (!value) throw std::runtime_error(reason); }
UINT32 surface(UINT32 width,UINT32 height,UINT8 depth=16) {
    VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=width;d.usHeight=height;d.ubBitDepth=depth;
    UINT32 id=0;check(AddVideoSurface(&d,&id),"actual surface allocation");return id;
}
void run() {
    check(InitializeVideoSurfaceManager(),"manager starts");
    UINT32 red=7,green=8,blue=9;
    check(!GetPrimaryRGBDistributionMasks(&red,&green,&blue)&&red==7&&green==8&&blue==9,"inactive primary leaves masks unchanged");
    const UINT32 ids[4]{surface(8,4),surface(8,4),surface(8,4),surface(2,2)};
    check(sgp::BindReservedVideoSurfaces(ids),"actual primary/frame binding");
    check(GetPrimaryRGBDistributionMasks(&red,&green,&blue)&&red==0xf800&&green==0x07e0&&blue==0x001f,"actual native primary is RGB565");
    check(!GetPrimaryRGBDistributionMasks(nullptr,&green,&blue),"missing mask output rejected");
    UINT32 pitch=0;check(LockVideoSurface(FRAME_BUFFER,&pitch),"query actual pitch");UnLockVideoSurface(FRAME_BUFFER);
    auto* z=InitZBuffer(pitch,4);check(z,"owned Z created before frame lock");
    auto* bytes=sgp::LockBackgroundSurface(FRAME_BUFFER,&pitch);check(bytes,"actual frame lock");
    auto* dest=reinterpret_cast<UINT16*>(bytes);sgp::BackgroundView view;check(sgp::BackgroundBufferView(bytes,&view),"actual frame extent");
    const auto reset=[&] {std::memset(bytes,0x5a,view.bytes);};
    const auto pixel=[&](unsigned x,unsigned y=0) {return dest[y*(pitch/2)+x];};
    SGPRect clip{0,0,8,4};SetClippingRect(&clip);BuildShadeTable();BuildIntensityTable();
    std::array<UINT16,8> source{1,2,3,0xabcd,4,5,6,0xabcd};
    reset();check(Blt16BPPTo16BPP(dest,pitch,source.data(),8,1,1,0,0,3,2),"raw padded copy");
    check(pixel(1,1)==1&&pixel(3,1)==3&&pixel(1,2)==4&&pixel(3,2)==6&&pixel(4,1)==0x5a5a,"literal rows and pitch padding");
    reset();check(Blt16BPPTo16BPPTrans(dest,pitch,source.data(),8,0,0,0,0,3,1,2),"raw transparency");
    check(pixel(0)==1&&pixel(1)==0x5a5a&&pixel(2)==3,"exact key leaves destination");
    reset();check(Blt16BPPTo16BPPTransShadow(dest,pitch,source.data(),8,0,0,0,0,3,1,2),"raw shadow");
    check(pixel(0)==ShadeTable[0x5a5a]&&pixel(1)==0x5a5a&&pixel(2)==ShadeTable[0x5a5a],"shadow is based on destination");
    reset();check(Blt16BPPTo16BPPMirror(dest,pitch,source.data(),8,-1,0,123,123,3,1),"mirror global left clip and ignored source coordinates");
    check(pixel(0)==2&&pixel(1)==1&&pixel(2)==0x5a5a,"literal clipped reverse order");
    std::array<UINT8,8> small{1,2,3,99,4,5,6,99},target{};
    check(Blt8BPPTo8BPP(target.data(),4,small.data(),4,1,0,0,0,2,2),"raw indexed copy");
    check(target==std::array<UINT8,8>{0,1,2,0,0,4,5,0},"literal indexed pitch copy");
    std::array<UINT32,3> rgba{0x00ffffff,0xff0000ff,0xff00ff00};
    reset();check(Blt32BPPTo16BPPTrans(dest,pitch,rgba.data(),12,0,0,0,0,3,1),"actual original alpha cache arithmetic");
    check(pixel(0)==0x5a5a&&pixel(1)==0xf800&&pixel(2)==0x07e0,"literal zero alpha and low-byte red");
    reset();check(Blt32BPPTo16BPPTransShadow(dest,pitch,rgba.data(),12,0,0,0,0,1,1)&&pixel(0)==0x5a5a,"alpha shadow zero is unchanged");
    check(blendWithAlpha(0xffff,0,0)==0&&blendWithAlpha(0xffff,0,255)==0xffff&&blendWithAlpha(0xffff,0,128)==0x8410,"x87 0,128,255 channel results");
    check(blendWithAlpha(0xffff,0,65535)==0,"x87 signed FILD alpha minus one rounds each channel to zero");
    reset();SGPRect area{-1,-1,2,1};
    check(Blt16BPPBufferShadowRect(dest,pitch,&area)&&area.iLeft==0&&area.iTop==0&&area.iRight==2&&area.iBottom==1,"shadow mutates clipped inclusive rectangle");
    check(pixel(2,1)==ShadeTable[0x5a5a]&&pixel(3,1)==0x5a5a,"inclusive shadow endpoint");
    reset();area={1,1,2,2};check(Blt16BPPBufferShadowRectAlternateTable(dest,pitch,&area)&&pixel(1,1)==IntensityTable[0x5a5a],"alternate genuine table");
    reset();area={1,1,4,2};UINT8 pattern[8][8]{};pattern[0][0]=pattern[1][1]=1;
    check(Blt16BPPBufferPixelateRectWithColor(dest,pitch,&area,pattern,0x1234),"pixel pattern draws");
    check(pixel(1,1)==0x1234&&pixel(2,1)==0x5a5a&&pixel(1,2)==0x1234&&pixel(2,2)==0x1234,"clipped origin and original first-column row-reset quirk");
    check(Blt16BPPBufferPixelateRect(dest,pitch,&area,pattern)&&pixel(1,1)==0,"black pattern wrapper");
    reset();check(Blt16BPPBufferHatchRectWithColor(dest,pitch,&area,0x4444)&&pixel(1,1)==0x4444&&pixel(2,1)==0x5a5a&&pixel(2,2)==0x4444,"literal hatch");
    check(Blt16BPPBufferHatchRect(dest,pitch,&area)&&pixel(1,1)==0,"black hatch wrapper");
    reset();check(Blt16BPPBufferLooseHatchRectWithColor(dest,pitch,&area,0x2222)&&pixel(1,1)==0x2222&&pixel(2,1)==0x5a5a&&pixel(1,2)==0x2222,"loose hatch retains first-column quirk");
    check(Blt16BPPBufferLooseHatchRect(dest,pitch,&area)&&pixel(1,1)==0,"black loose wrapper");
    reset();check(FillRect16BPP(dest,pitch,1,1,2,2,0x7777)&&pixel(2,2)==0x7777&&pixel(3,2)==0x5a5a,"fill has inclusive endpoints");
    std::memset(z,0,view.bytes);check(BlitZRect(z,pitch,-1,-1,2,2,321),"depth rect clips half-open");
    check(z[0]==321&&z[1]==321&&z[2]==0&&z[pitch/2]==321&&z[pitch]==0,"depth endpoint stays untouched");
    check(!BlitZRect(z,pitch,2,2,2,3,321),"zero-width depth rectangle refused");
    const auto saved=pixel(0);check(!Blt16BPPTo16BPP(dest,pitch,dest,pitch,1,0,0,0,2,1)&&pixel(0)==saved,"overlap refused before writes");
    check(!Blt16BPPTo16BPP(dest,pitch,source.data(),8,7,0,0,0,3,1),"actual destination width excludes padding");
    check(!Blt16BPPTo16BPP(dest,pitch,source.data(),8,0,4,0,0,1,1),"actual destination height");
    check(Blt16BPPTo16BPP(dest,pitch,source.data(),8,0,0,0,0,0,0),"empty raw copy terminates safely");
    // Full/subrect indexed conversion reads the genuine locked source palette.
    const UINT32 indexedId=surface(4,2,8);HVSURFACE indexed=nullptr;check(GetVideoSurface(&indexed,indexedId),"indexed handle");
    std::array<SGPPaletteEntry,256> colors{};colors[1].peRed=255;colors[2].peGreen=255;colors[3].peBlue=255;
    check(SetVideoSurfacePalette(indexed,colors.data()),"genuine copied indexed palette");UINT32 ip=0;auto* indices=LockVideoSurface(indexedId,&ip);check(indices,"indexed lock");
    indices[0]=1;indices[1]=2;indices[2]=3;indices[3]=0;indices[ip]=3;indices[ip+1]=2;indices[ip+2]=1;
    reset();check(Blt8BPPDataTo16BPPBuffer(dest,pitch,indexed,indices,1,1),"full native indexed conversion");
    check(pixel(1,1)==0xf800&&pixel(2,1)==0x07e0&&pixel(3,1)==0x001f&&pixel(1,2)==0x001f,"literal indexed palette rows");
    reset();area={1,0,3,2};check(Blt8BPPDataSubTo16BPPBuffer(dest,pitch,indexed,indices,ip,0,0,&area),"indexed subrectangle");
    check(pixel(0)==0x07e0&&pixel(1)==0x001f&&pixel(0,1)==0x07e0&&pixel(1,1)==0xf800,"subrectangle uses source position");
    check(!Blt8BPPDataSubTo16BPPBuffer(dest,pitch,indexed,indices,ip+1,0,0,&area),"wrong indexed pitch rejected");
    UnLockVideoSurface(indexedId);check(DeleteVideoSurfaceFromIndex(indexedId),"indexed release");
    sgp::UnlockBackgroundSurface(FRAME_BUFFER);check(ShutdownZBuffer(z),"Z release");
    check(sgp::ReleaseReservedVideoSurfaces()&&ShutdownVideoSurfaceManager(),"manager releases all real storage");
}
}
int main() {try {run();std::cout<<"native raw blitters: "<<checks<<" checks passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
