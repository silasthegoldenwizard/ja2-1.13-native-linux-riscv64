/*
 * P710 native notice preparation: tests/native-core/native_render_viewer.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add authored surface/font acceptance scene viewer.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/native_render_viewer.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Font.h"
#include "vsurface.h"
#include "clip_state.h"
#include "SDLPresentation.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
void require(bool b) { if (!b) throw std::runtime_error("native rendering viewer operation failed"); }
UINT32 create(UINT32 w, UINT32 h)
{
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=w; d.usHeight=h; d.ubBitDepth=16;
    UINT32 id=0; require(AddVideoSurface(&d,&id)); return id;
}
void cleanup()
{
    ShutdownFontManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear(); SDL_Quit();
}
}
int main(int argc, char** argv)
{
    sgp::SDLPresentation window;
    UINT32 locked=0;
    try
    {
        if (argc!=3) throw std::runtime_error("expected fixture root and output BMP path");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        require(dir->init());
        auto profile=std::make_unique<vfs::CVirtualProfile>("render",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release()); require(getVFS()->addLocation(location,mounted));
        require(InitializeVideoObjectManager()); require(InitializeVideoSurfaceManager());
        UINT16 values[]={65,66,32}; FontTranslationTable table{3,values}; require(InitializeFontManager(16,&table));
        char file[]="font.sti"; const INT32 font=LoadFontFile(file); require(font>=0 && SetFont(font));
        const UINT32 frame=create(32,20), tile=create(8,4);
        SGPRect clip{0,0,32,20}; SetClippingRect(&clip);
        require(ColorFillVideoSurfaceArea(frame,0,0,32,20,0x2104));
        require(ColorFillVideoSurfaceArea(frame,1,1,31,19,0x4208));
        require(ColorFillVideoSurfaceArea(tile,0,0,8,4,0x001f));
        require(ColorFillVideoSurfaceArea(tile,1,1,7,3,0xf800));
        require(BltVideoSurface(frame,tile,0,2,3,0,nullptr));
        require(SetFontDestBuffer(frame,0,0,32,20,FALSE));
        SetRGBFontForeground(static_cast<UINT16>(0xffff)); SetRGBFontShadow(static_cast<UINT16>(0x07e0)); SetFontBackground(0);
        require(mprintf(12,3,L"AB")==0); wchar_t ba[]=L"BA"; require(gprintf(20,10,ba)==0);
        require(window.Open("JA2 native surface and bitmap font",32,20,8));
        UINT32 pitch=0; auto* pixels=LockVideoSurface(frame,&pitch); require(pixels!=nullptr); locked=frame;
        const auto bytes=static_cast<std::size_t>(pitch)*20;
        require(window.Present565(pixels,bytes,pitch)); require(window.SaveWindowSurface(argv[2]));
        std::ofstream raw(std::filesystem::path(argv[2]).replace_extension("rgb565"),std::ios::binary);
        raw.write(reinterpret_cast<const char*>(pixels),static_cast<std::streamsize>(bytes)); require(static_cast<bool>(raw));
        raw.close(); require(static_cast<bool>(raw));
        UnLockVideoSurface(frame); locked=0;
        const auto end=SDL_GetTicks()+1000;
        while (SDL_GetTicks()<end && !window.QuitRequested()) SDL_Delay(10);
        std::cout << "PASS: owned surface -> bitmap font -> SDL3 " << window.VideoDriver()
                  << "; 32x20, pitch " << pitch << ", scale 8; raw and actual window BMP exported\n";
        window.Close(); UnloadFont(static_cast<UINT32>(font));
        require(DeleteVideoSurfaceFromIndex(tile)); require(DeleteVideoSurfaceFromIndex(frame));
        cleanup(); return 0;
    }
    catch (const std::exception& e)
    {
        if (locked) UnLockVideoSurface(locked);
        window.Close(); cleanup(); std::cerr << "FAIL: " << e.what() << '\n'; return 1;
    }
}
