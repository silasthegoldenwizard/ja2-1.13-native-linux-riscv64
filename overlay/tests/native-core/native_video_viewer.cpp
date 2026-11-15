/*
 * P710 native notice preparation: tests/native-core/native_video_viewer.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add authored framebuffer and dirty-region acceptance scene viewer.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/native_video_viewer.cpp
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
#include "FileManBackend.h"
#include <SDL3/SDL.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
namespace
{
void require(bool b) { if (!b) throw std::runtime_error("native video viewer operation failed"); }
UINT32 createTile()
{
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=8; d.usHeight=4; d.ubBitDepth=16;
    UINT32 id=0; require(AddVideoSurface(&d,&id)); return id;
}
void capture(const std::string& stem)
{
    require(sgp::SaveNativeVideoWindow((stem+".bmp").c_str()));
    UINT32 pitch=0; auto* pixels=LockVideoSurface(PRIMARY_SURFACE,&pitch); require(pixels!=nullptr);
    bool ok=false;
    try
    {
        std::ofstream raw(stem+".rgb565",std::ios::binary);
        raw.write(reinterpret_cast<const char*>(pixels),static_cast<std::streamsize>(pitch)*20);
        raw.close(); ok=static_cast<bool>(raw) && pitch==72;
    }
    catch (...) { UnlockPrimarySurface(); throw; }
    UnlockPrimarySurface(); require(ok);
}
void linger()
{
    const auto end=SDL_GetTicks()+600;
    while (SDL_GetTicks()<end && !sgp::NativeVideoQuitRequested()) SDL_Delay(10);
}
void cleanup()
{
    ShutdownFontManager(); ShutdownVideoManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc, char** argv)
{
    try
    {
        if (argc!=3) throw std::runtime_error("expected fixture-root and output base path");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        require(dir->init());
        auto profile=std::make_unique<vfs::CVirtualProfile>("render",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release()); require(getVFS()->addLocation(location,mounted));
        require(InitializeVideoObjectManager());
        require(sgp::ConfigureNativeVideo(32,20,16,8,"JA2 native reserved framebuffer"));
        require(InitializeVideoManager(nullptr,0,nullptr));
        require(std::strcmp(sgp::NativeVideoDriver(),"x11")==0);
        UINT16 values[]={65,66,32}; FontTranslationTable table{3,values}; require(InitializeFontManager(16,&table));
        char file[]="font.sti"; const INT32 font=LoadFontFile(file); require(font>=0 && SetFont(font));
        const UINT32 tile=createTile();
        SGPRect clip{0,0,32,20}; SetClippingRect(&clip);
        require(ColorFillVideoSurfaceArea(FRAME_BUFFER,0,0,32,20,0x2104));
        require(ColorFillVideoSurfaceArea(FRAME_BUFFER,1,1,31,19,0x4208));
        require(ColorFillVideoSurfaceArea(tile,0,0,8,4,0x001f));
        require(ColorFillVideoSurfaceArea(tile,1,1,7,3,0xf800));
        require(BltVideoSurface(FRAME_BUFFER,tile,0,2,3,0,nullptr));
        require(SetFontDestBuffer(FRAME_BUFFER,0,0,32,20,FALSE));
        SetRGBFontForeground(static_cast<UINT16>(0xffff)); SetRGBFontShadow(static_cast<UINT16>(0x07e0)); SetFontBackground(0);
        require(mprintf(12,3,L"AB")==0); wchar_t ba[]=L"BA"; require(gprintf(20,10,ba)==0);
        InvalidateScreen(); require(sgp::RefreshNativeVideo());
        const std::string base=argv[2]; capture(base+"-first"); linger();
        require(ColorFillVideoSurfaceArea(FRAME_BUFFER,2,3,10,7,0xffe0));
        InvalidateRegion(2,3,10,7); require(sgp::RefreshNativeVideo());
        capture(base+"-second"); linger();
        UINT16 width=0,height=0; UINT8 depth=0; GetCurrentVideoSettings(&width,&height,&depth);
        std::cout << "driver=" << sgp::NativeVideoDriver() << "; geometry=" << width << 'x' << height
                  << "; depth=" << static_cast<unsigned>(depth) << "; pitch=72; scale=8; window=256x160\n";
        UnloadFont(static_cast<UINT32>(font)); require(DeleteVideoSurfaceFromIndex(tile));
        require(sgp::ShutdownNativeVideo()); cleanup();
        std::cout << "PASS: two reserved-framebuffer frames exported; exit=0\n"; return 0;
    }
    catch (const std::exception& e)
    {
        cleanup(); std::cerr << "FAIL: " << e.what() << "; exit=1\n"; return 1;
    }
}
