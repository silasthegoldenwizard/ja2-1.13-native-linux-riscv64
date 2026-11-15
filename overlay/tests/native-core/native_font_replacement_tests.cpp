/*
 * P710 native snapshot update: tests/native-core/native_font_replacement_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Font.h"
#include "NativeScalableFont.h"
#include "vsurface.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
unsigned checks=0;
void check(bool value,const char* message) {
    ++checks; if (!value) throw std::runtime_error(message);
}
std::vector<BYTE> pixels(UINT32 id) {
    UINT16 w,h; UINT8 bpp;
    check(GetVideoSurfaceDescription(id,&w,&h,&bpp),"surface description");
    UINT32 pitch=0; auto* p=LockVideoSurface(id,&pitch);
    check(p!=nullptr,"surface read lock");
    std::vector<BYTE> result(p,p+static_cast<std::size_t>(pitch)*h);
    UnLockVideoSurface(id); return result;
}
void clear(UINT32 id) {
    check(ColorFillVideoSurfaceArea(id,0,0,160,64,0x2104),"fill destination including behind transparent glyphs");
}
COLORVAL color(INT32 id) {
    COLORVAL c=0; check(sgp::GetNativeScalableFontColor(id,&c),"read actual scalable color"); return c;
}
void cleanup() {
    ShutdownFontManager(); sgp::ShutdownNativeScalableFonts();
    ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
void run(const std::filesystem::path& publicFont) {
    std::ifstream input(publicFont,std::ios::binary);
    std::vector<BYTE> bytes((std::istreambuf_iterator<char>(input)),{});
    check(!bytes.empty(),"public scalable font bytes");
    const INT32 face=sgp::CreateNativeScalableFont(bytes.data(),bytes.size(),19);
    const INT32 secondFace=sgp::CreateNativeScalableFont(bytes.data(),bytes.size(),27);
    check(face>0 && secondFace>0 && face!=secondFace,"two actual FreeType font handles");
    UINT16 symbols[]={65,66,32}; FontTranslationTable table{3,symbols};
    check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"real rendering managers");
    check(InitializeFontManager(16,&table),"bitmap manager");
    char filename[]="font.sti", alternate[]="font_alt.sti";
    INT32 bitmap=LoadFontFile(filename), other=LoadFontFile(alternate);
    check(bitmap>=0 && other>=0 && SetFont(bitmap),"authored STCI bitmap fonts");
    wchar_t text[]=L"AB", percentFormat[]=L"%s%%", literalPercent[]=L"AB%";
    check(StringPixLength(text,bitmap)==6 && GetFontHeight(bitmap)==3,"replacement default is off");
    check(!sgp::SetNativeFontReplacement(-1,face) && !sgp::SetNativeFontReplacement(MAX_FONTS,face) &&
          !sgp::SetNativeFontReplacement(bitmap,0) && !sgp::SetNativeFontReplacement(bitmap,INT32_MAX),"mapping rejects absent bitmap and invalid core handles");
    check(StringPixLength(text,bitmap)==6,"failed selection preserves bitmap mode");
    check(sgp::SetNativeFontReplacement(bitmap,face) && sgp::GetNativeFontReplacement(bitmap)==face,
          "explicit replacement opt-in and real validated lookup");
    INT32 expectedWidth=0; sgp::NativeScalableFontMetrics metrics;
    check(sgp::MeasureNativeScalableText(face,text,&expectedWidth) &&
          sgp::GetNativeScalableFontMetrics(face,&metrics),"actual core metric oracle");
    check(StringPixLength(text,bitmap)==expectedWidth && GetFontHeight(bitmap)==metrics.ascent,"mapped full width and height use real core");
    wchar_t prefixFormat[]=L"%s";INT32 prefixWidth=0;
    check(sgp::MeasureNativeScalableText(face,L"A",&prefixWidth) && prefixWidth!=4,
          "actual formatted prefix oracle differs from authored bitmap advance");
    check(StringPixLengthArg(bitmap,1,prefixFormat,L"AB")==prefixWidth &&
          StringPixLengthArg(bitmap,99,prefixFormat,L"AB")==expectedWidth &&
          StringPixLengthArg(bitmap,0,prefixFormat,L"AB")==0,
          "formatted prefix truncates then uses active scalable full-string metric policy");
    check(StringNPixLength(text,1,bitmap)==4 && SubstringPixLength(text,0,1,bitmap)==6 &&
          SubstringPixLength(text,1,2,bitmap)==6,"NPix/substrings retain original bitmap policy including NUL fallback");
    check(StringPixLength(text,other)==4 && GetFontHeight(other)==4,"unmapped explicit bitmap unaffected");
    INT16 x=0,y=0; FindFontCenterCoordinates(0,0,100,50,text,bitmap,&x,&y);
    check(x==(100-expectedWidth+1)/2 && y==(50-metrics.ascent)/2,"positioning consumes scalable metrics");

    VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    desc.usWidth=160; desc.usHeight=64; desc.ubBitDepth=16;
    UINT32 dest=0,oracle=0;
    check(AddVideoSurface(&desc,&dest) && AddVideoSurface(&desc,&oracle),"two actual RGB565 surfaces");
    check(SetFontDestBuffer(dest,1,2,150,60,FALSE),"destination and clipping");
    SetRGBFontForeground(231,57,109);
    const COLORVAL initial=FROMRGB(231,57,109);
    check(color(face)==initial,"RGB setter retains full 24-bit core color");
    const SGPRect clip=FontDestRegion;
    const INT32 adjustedTop=4>=metrics.internalLeading ? 4-metrics.internalLeading : 4;
    const auto compare=[&](auto draw,const wchar_t* expected) {
        clear(dest); clear(oracle);
        check(draw()==0,"mapped public drawing success");
        check(sgp::DrawNativeScalableText(oracle,face,3,adjustedTop,expected,color(face),&clip),"direct real core draw oracle");
        check(pixels(dest)==pixels(oracle),"public path equals real core glyphs, color, clipping and transparent background");
    };
    compare([&] { return mprintf(3,4,L"%s%%",L"AB"); },literalPercent);
    clear(dest);clear(oracle);
    check(mprintf(3,0,L"AB")==0 &&
          sgp::DrawNativeScalableText(oracle,face,3,0,text,color(face),&clip) &&
          pixels(dest)==pixels(oracle),"small y retains requested top under conditional leading policy");
    compare([&] { return gprintf(3,4,percentFormat,L"AB"); },literalPercent);
    compare([&] { return sgp::DrawNativeFontText(3,4,literalPercent,false); },literalPercent);
    wchar_t controls[]={180,2,L'A',181,L'B',180,0};
    // A trailing 180 is a valid literal glyph in the original Win-font branch.
    compare([&] { return mprintf_coded(3,4,controls); },controls);
    compare([&] { return mprintf_coded(3,4,percentFormat,L"AB"); },literalPercent);
    check(color(face)==initial,"mapped coded output never applies bitmap palette controls");
    SetFontForeground(2);
    const auto palette=GetFontObject(bitmap)->pPaletteEntry[2];
    check(color(face)==FROMRGB(palette.peRed,palette.peGreen,palette.peBlue),"indexed setter uses real selected bitmap palette in core");
    SetRGBFontForeground(UINT16{0x1234});
    check(color(face)==GetRGBColor(0x1234),"packed RGB565 setter uses original RGB expansion");
    SetFontBackground(2); SetRGBFontBackground(255,0,255); SetFontShadow(2);
    compare([&] { return mprintf(3,4,L"AB"); },text);

    SaveFontSettings(); const auto captured=sgp::CaptureNativeFontState();
    check(captured.scalable==face && captured.scalableColor==color(face),"capture includes live core color and identity");
    check(sgp::SetNativeScalableFontColor(face,FROMRGB(7,8,9)),"external legacy bridge color mutation");
    RestoreFontSettings(); check(color(face)==captured.scalableColor,"legacy save restores core color");
    SetRGBFontForeground(99,88,77); sgp::RestoreNativeFontState(captured);
    check(color(face)==captured.scalableColor,"composition restore includes core color");
    check(sgp::SetNativeFontReplacement(bitmap,secondFace),"replace mapping with different live handle");
    check(sgp::SetNativeScalableFontColor(secondFace,FROMRGB(12,34,56)),"second handle distinct color");
    RestoreFontSettings(); sgp::RestoreNativeFontState(captured);
    check(color(secondFace)==FROMRGB(12,34,56),"saved state cannot restore stale color into replacement handle");
    check(sgp::SetNativeFontReplacement(bitmap,face),"restore test mapping");

    // Raw mapped mono paths select current FontDefault even when FontType is
    // invalid/different; the pointer must identify a real currently locked surface.
    const auto rawCompare=[&](bool coded) {
        clear(dest); clear(oracle);
        UINT32 pitch=0; auto* p=LockVideoSurface(dest,&pitch);
        check(p!=nullptr,"real raw surface lock");
        check((coded ? mprintf_buffer_coded(p,pitch,MAX_FONTS,3,4,controls) :
                       mprintf_buffer(p,pitch,MAX_FONTS,3,4,percentFormat,L"AB"))==0,"mapped raw current-font rendering");
        UnLockVideoSurface(dest);
        check(sgp::DrawNativeScalableText(oracle,face,3,adjustedTop,coded ? controls : literalPercent,color(face),&clip),"raw core oracle");
        check(pixels(dest)==pixels(oracle),"locked raw pixels equal real core");
        return p;
    };
    BYTE* expired=rawCompare(false); rawCompare(true);
    auto before=pixels(dest);
    check(mprintf_buffer(expired,FontDestPitch,bitmap,3,4,percentFormat,L"AB")==UINT32_MAX && pixels(dest)==before,"unlocked native raw pointer refused without write");
    std::vector<BYTE> foreign(160*64*2+32,0xa5); const auto foreignBefore=foreign;
    check(mprintf_buffer(foreign.data()+16,320,bitmap,3,4,percentFormat,L"AB")==UINT32_MAX &&
          mprintf_buffer_coded(foreign.data()+16,320,bitmap,3,4,controls)==UINT32_MAX && foreign==foreignBefore,"foreign raw buffers truthfully refused without bounds fabrication");
    UINT32 pitch=0; auto* p=LockVideoSurface(dest,&pitch);
    check(p && mprintf_buffer(p,pitch+2,bitmap,3,4,percentFormat,L"AB")==UINT32_MAX,"wrong pitch refused on real lock");
    UnLockVideoSurface(dest); check(pixels(dest)==before,"pitch refusal leaves all bytes alone");

    // gprintf_buffer has no original Win branch: compare its actual bitmap
    // output with mapping off and on, using identical state and buffer contents.
    check(sgp::SetNativeFontReplacement(bitmap,-1),"clear explicit mapping");
    std::vector<BYTE> bitmapRaw(320*64,0),mappedRaw=bitmapRaw;
    check(gprintf_buffer(bitmapRaw.data(),320,bitmap,3,4,text)==0,"bitmap gprintf raw baseline");
    check(sgp::SetNativeFontReplacement(bitmap,face) && gprintf_buffer(mappedRaw.data(),320,bitmap,3,4,text)==0 && mappedRaw==bitmapRaw,"gprintf_buffer retains bitmap path under mapping");

    SaveFontSettings(); const auto oldGeneration=sgp::CaptureNativeFontState();
    UnloadFont(static_cast<UINT32>(bitmap)); bitmap=LoadFontFile(filename);
    check(bitmap==oldGeneration.font && SetFont(bitmap),"bitmap slot reused with fresh generation");
    check(StringPixLength(text,bitmap)==6 && GetFontHeight(bitmap)==3,"unload clears replacement before slot reuse");
    check(sgp::SetNativeFontReplacement(bitmap,face),"new generation independently opts in");
    check(sgp::SetNativeScalableFontColor(face,FROMRGB(1,22,33)),"new generation color");
    RestoreFontSettings(); sgp::RestoreNativeFontState(oldGeneration);
    check(FontDefault==-1 && color(face)==FROMRGB(1,22,33),"stale saved bitmap generation cannot restore selection or core color");
    check(SetFont(bitmap),"reselect live bitmap");
    sgp::DeleteNativeScalableFont(face);
    check(StringPixLength(text,bitmap)==6 && GetFontHeight(bitmap)==3 && sgp::GetNativeFontReplacement(bitmap)==-1,
          "deleted scalable handle safely falls back to live bitmap and lookup");
    check(!sgp::SetNativeFontReplacement(bitmap,face),"dead core cannot be registered again");
    check(sgp::SetNativeFontReplacement(bitmap,secondFace),"second live mapping");
    sgp::ClearNativeFontReplacements();
    check(StringPixLength(text,bitmap)==6,"global explicit clear returns bitmap metrics");
    check(sgp::SetNativeFontReplacement(bitmap,secondFace),"mapping before shutdown");
    ShutdownFontManager(); check(InitializeFontManager(16,&table),"font manager reinitialize");
    bitmap=LoadFontFile(filename);
    check(bitmap>=0 && StringPixLength(text,bitmap)==6,"shutdown never resurrects a mapping");
    check(DeleteVideoSurfaceFromIndex(dest) && DeleteVideoSurfaceFromIndex(oracle),"surface cleanup");
}
}
int main(int argc,char** argv) {
    try {
        check(argc==3,"authored render fixture directory and public TTF path arguments");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(directory->init(),"real VFS directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("font-replacement",vfs::Path(root.u8string()),false);
        profile->addLocation(directory.get()); auto* location=directory.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"real VFS mount");
        run(std::filesystem::canonical(argv[2])); cleanup();
        std::cout << "PASS: " << checks << " native bitmap/scalable replacement assertions\n"; return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n'; cleanup(); return 1;
    }
}
