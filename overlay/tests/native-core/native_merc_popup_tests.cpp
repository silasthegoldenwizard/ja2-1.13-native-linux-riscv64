/*
 * P710 native snapshot update: tests/native-core/native_merc_popup_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted virtual inventory and lifetime checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production modal popup preparation and rendering with authored resources.
#include "MercTextBox.h"
#include "Font Control.h"
#include "Font.h"
#include "native_surface.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

extern MercPopUpBox* GetPopUpBoxIndex(INT32);
extern BOOLEAN ShutDownPopUpBoxes();
extern UINT16 SCREEN_WIDTH,SCREEN_HEIGHT;

namespace {
unsigned checks=0;
void check(bool value,const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
UINT32 surface(UINT16 width,UINT16 height) {
    VSURFACE_DESC description{};
    description.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    description.usWidth=width;
    description.usHeight=height;
    description.ubBitDepth=16;
    UINT32 id=0;
    check(AddVideoSurface(&description,&id),"create real RGB565 surface");
    return id;
}
std::vector<UINT16> pixels(UINT32 id,UINT16& width,UINT16& height) {
    UINT8 depth=0;
    check(GetVideoSurfaceDescription(id,&width,&height,&depth) && depth==16,"RGB565 capture metadata");
    UINT32 pitch=0;
    auto* data=LockVideoSurface(id,&pitch);
    check(data!=nullptr,"capture actual surface pixels");
    std::vector<UINT16> result(std::size_t(width)*height);
    for (UINT16 y=0;y<height;++y) {
        std::memcpy(result.data()+std::size_t(y)*width,data+std::size_t(y)*pitch,width*2);
        for (UINT32 x=width*2;x<pitch;++x) check(data[std::size_t(y)*pitch+x]==0xA5,"surface row padding remains intact");
    }
    UnLockVideoSurface(id);
    return result;
}
void seed(UINT32 destination,UINT16 color) {
    blt_vs_fx fill{};
    fill.ColorFill=color;
    check(BltVideoSurface(destination,0,0,0,0,VS_BLT_COLORFILL,&fill),"seed destination sentinel");
}
void renderPopup(bool transparent) {
    if (transparent) SetPrepareMercPopupFlags(MERC_POPUP_PREPARE_FLAGS_TRANS_BACK);
    UINT16 width=0,height=0;
    CHAR16 question[]=L"AA BB";
    const INT32 id=PrepareMercPopupBox(-1,DIALOG_MERC_POPUP_BACKGROUND,DIALOG_MERC_POPUP_BORDER,
        question,180,10,0,30,&width,&height,TRUE);
    check(id>=0 && width>40 && height>20,"actual modal popup compositor creates the box");
    auto* box=GetPopUpBoxIndex(id);
    check(box && box->sWidth==width && box->sHeight==height,"production popup owns composed dimensions");
    UINT16 backgroundWidth=0,backgroundHeight=0;
    UINT8 depth=0;
    check(GetVideoSurfaceDescription(box->uiMercTextPopUpBackground,&backgroundWidth,&backgroundHeight,&depth) &&
          depth==8 && backgroundWidth==350 && backgroundHeight==207,
          "popup loader retains indexed PCX pixels for the original background converter");
    UINT16 sw=0,sh=0;
    const auto source=pixels(box->uiSourceBufferIndex,sw,sh);
    const UINT16 background=transparent ? 0xFFE0 : 0x07E0;
    check(source[std::size_t(height-20)*width+width/2]==background,
          "actual compositor supplies the authored background or transparency key");
    check(source[0]==0xF800,"actual border renderer writes the authored red frame");
    bool textVisible=false;
    for (unsigned y=10;y<18 && y<height-10;++y)
        for (unsigned x=20;x<90 && x<width-10;++x)
            textVisible|=source[std::size_t(y)*width+x]!=background;
    check(textVisible,"actual wrapped font renderer writes prompt glyphs inside the box");

    const auto destination=surface(width+14,height+18);
    seed(destination,0x1234);
    check(RenderMercPopUpBoxFromIndex(id,7,9,destination),"production popup transfer accepts FAST copy");
    UINT16 dw=0,dh=0;
    auto actual=pixels(destination,dw,dh);
    for (UINT16 y=0;y<dh;++y) for (UINT16 x=0;x<dw;++x) {
        UINT16 expected=0x1234;
        if (x>=7 && x<width+7 && y>=9 && y<height+9) {
            const auto color=source[std::size_t(y-9)*width+x-7];
            if (!transparent || color!=0xFFE0) expected=color;
        }
        check(actual[std::size_t(y)*dw+x]==expected,"entire composed popup transfers with exact position and color-key behavior");
    }
    UINT32 pitch=0;
    check(LockVideoSurface(box->uiSourceBufferIndex,&pitch)!=nullptr,"lock source to exercise transfer failure");
    check(!RenderMercPopUpBoxFromIndex(id,7,9,destination),"popup reports a rejected source transfer");
    UnLockVideoSurface(box->uiSourceBufferIndex);
    UINT16 aw=0,ah=0;
    check(pixels(destination,aw,ah)==actual,"failed popup transfer preserves destination pixels");
    check(DeleteVideoSurfaceFromIndex(destination),"delete popup destination");
    check(RemoveMercPopupBoxFromIndex(id),"release actual popup resources");
}
void fastAdapter() {
    const auto source=surface(2,1),destination=surface(4,2);
    seed(source,0x4567);
    seed(destination,0x1234);
    HVSURFACE src=nullptr,dst=nullptr;
    check(GetVideoSurface(&src,source) && GetVideoSurface(&dst,destination),"actual adapter handles");
    check(BltVSurfaceUsingDD(dst,src,VS_BLT_FAST,1,1,nullptr),"legacy handle adapter accepts FAST copy");
    UINT16 width=0,height=0;
    check(pixels(destination,width,height)==std::vector<UINT16>({0x1234,0x1234,0x1234,0x1234,0x1234,0x4567,0x4567,0x1234}),
          "FAST adapter preserves literal pixel placement");
    const auto before=pixels(destination,width,height);
    check(!BltVideoSurface(destination,source,0,0,0,VS_BLT_FAST|VS_BLT_USEDESTCOLORKEY,nullptr),
          "FAST does not admit unsupported destination-key semantics");
    check(pixels(destination,width,height)==before,"unsupported flags still reject before writes");
    check(DeleteVideoSurfaceFromIndex(source) && DeleteVideoSurfaceFromIndex(destination),"adapter cleanup");
}
void cleanup() {
    ShutDownPopUpBoxes();
    ShutdownFontManager();
    ShutdownVideoSurfaceManager();
    ShutdownVideoObjectManager();
    fileman::CloseAll();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"authored popup fixture root argument");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(directory->init(),"read-only authored popup directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("merc-popup",vfs::Path(root.u8string()),false);
        profile->addLocation(directory.get());
        auto* location=directory.release();
        auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"mount actual VFS fixture");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"actual rendering managers");
        UINT16 symbols[]={65,66,32};
        FontTranslationTable table{3,symbols};
        check(InitializeFontManager(16,&table),"actual bitmap font manager");
        CHAR8 filename[]="font.sti";
        gp12PointArial=LoadFontFile(filename);
        check(gp12PointArial>=0,"load authored prompt glyphs through production font loader");
        SCREEN_WIDTH=640;
        SCREEN_HEIGHT=480;
        SGPRect clip{0,0,640,480};
        SetClippingRect(&clip);
        renderPopup(false);
        renderPopup(true);
        fastAdapter();
        cleanup();
        std::cout<<"PASS: "<<checks<<" actual modal popup rendering assertions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<"FAIL: "<<error.what()<<'\n';
        cleanup();
        return 1;
    }
}
