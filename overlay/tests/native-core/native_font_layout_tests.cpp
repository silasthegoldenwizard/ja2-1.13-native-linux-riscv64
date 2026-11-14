/*
 * P710 native snapshot update: tests/native-core/native_font_layout_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Font.h"
#include "vsurface.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
unsigned checks=0;
void check(bool result, const char* message)
{ ++checks; if (!result) throw std::runtime_error(message); }
bool same(const sgp::NativeFontState& a, const sgp::NativeFontState& b)
{
    return a.font==b.font && a.generation==b.generation && a.dest==b.dest &&
        a.pitch==b.pitch && a.bpp==b.bpp && a.wrap==b.wrap &&
        a.region.iLeft==b.region.iLeft && a.region.iTop==b.region.iTop &&
        a.region.iRight==b.region.iRight && a.region.iBottom==b.region.iBottom &&
        a.foreground==b.foreground && a.background==b.background && a.shadow==b.shadow;
}
std::vector<BYTE> pixels(UINT32 id)
{
    UINT16 w,h; UINT8 bpp;
    check(GetVideoSurfaceDescription(id,&w,&h,&bpp),"read description");
    UINT32 pitch=0; auto* data=LockVideoSurface(id,&pitch);
    check(data!=nullptr,"read lock");
    std::vector<BYTE> result(data,data+static_cast<std::size_t>(h)*pitch);
    UnLockVideoSurface(id); return result;
}
void cleanup()
{
    ShutdownFontManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
void layout()
{
    UINT16 symbols[]={65,66,32}; FontTranslationTable table{3,symbols};
    check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"render managers");
    check(InitializeFontManager(16,&table),"font init");
    char regularFile[]="font.sti", otherFile[]="font_alt.sti";
    const INT32 regular=LoadFontFile(regularFile), other=LoadFontFile(otherFile);
    check(regular>=0 && other>=0 && regular!=other,"authored fixture fonts loaded");
    check(GetFontHeight(regular)==3 && GetFontHeight(other)==4,"fixture heights");
    wchar_t a[]=L"A", ab[]=L"AB", empty[]=L"", unknown[]=L"?";
    check(StringPixLength(a,regular)==4 && StringPixLength(a,other)==2,"fixture advance includes glyph offset");
    check(SetFont(other),"select different current font");
    struct Row { INT16 l,t,w,h,cx,rx,y; };
    // Expected coordinates are literal examples of original Font.cpp's integer
    // geometry, not computed by the implementation under test.
    const Row rows[]={
        {10,20,10,9,13,16,23}, {10,20,9,8,13,15,22},
        {0,0,3,2,0,-1,0}, {0,0,2,1,0,-2,-1},
        {0,0,1,0,-1,-3,-1}, {0,0,0,-1,-1,-4,-2},
        {-10,-20,-3,-4,-13,-17,-23}, {7,-7,-4,-5,4,-1,-11},
        {-32768,-32768,-32768,-32768,-32768,-32768,-32768},
        {32767,32767,32767,32767,32767,32767,32767}
    };
    const auto selected=sgp::CaptureNativeFontState();
    for (const auto& row:rows)
    {
        INT16 x=123,y=456;
        FindFontCenterCoordinates(row.l,row.t,row.w,row.h,a,regular,&x,&y);
        check(x==row.cx && y==row.y,"center odd/even/negative/saturated geometry");
        FindFontRightCoordinates(row.l,row.t,row.w,row.h,a,regular,&x,&y);
        check(x==row.rx && y==row.y,"right odd/even/negative/saturated geometry");
    }
    check(same(selected,sgp::CaptureNativeFontState()),"explicit-font positioning preserves selection");
    INT16 x=0,y=0;
    FindFontCenterCoordinates(0,0,10,10,a,other,&x,&y);
    check(x==4 && y==3,"requested second font dimensions");
    FindFontCenterCoordinates(10,20,9,8,empty,regular,&x,&y);
    check(x==15 && y==22,"empty text keeps font height and center bias");
    FindFontRightCoordinates(10,20,9,8,unknown,regular,&x,&y);
    check(x==15 && y==22,"unknown character uses translated fallback");
    VarFindFontCenterCoordinates(10,20,15,9,regular,&x,&y,L"%s%ls%d%%",L"A",L"B",7);
    check(x==11 && y==23,"legacy wide percent-s/ls, integer and percent formatting");
    VarFindFontRightCoordinates(10,20,15,9,regular,&x,&y,L"%s%ls%d%%",L"A",L"B",7);
    check(x==11 && y==23,"right formatted coordinates");
    VarFindFontCenterCoordinates(0,0,10,9,regular,&x,&y,L"%S","AB");
    check(x==2 && y==3,"legacy narrow percent-S formatting");
    std::wstring bound(511,L'A');
    VarFindFontRightCoordinates(0,0,0,3,regular,&x,&y,L"%s",bound.c_str());
    check(x==-2044 && y==0,"511-character format boundary accepted");
    const auto invalid=[&](auto call) {
        x=123; y=-456; call(); check(x==123 && y==-456,"invalid input leaves both outputs unchanged");
    };
    invalid([&] { FindFontCenterCoordinates(1,2,3,4,nullptr,regular,&x,&y); });
    invalid([&] { FindFontRightCoordinates(1,2,3,4,a,-1,&x,&y); });
    invalid([&] { FindFontCenterCoordinates(1,2,3,4,a,MAX_FONTS,&x,&y); });
    invalid([&] { FindFontCenterCoordinates(1,2,3,4,a,regular,nullptr,&y); });
    invalid([&] { FindFontRightCoordinates(1,2,3,4,a,regular,&x,nullptr); });
    invalid([&] { VarFindFontCenterCoordinates(1,2,3,4,regular,&x,&y,nullptr); });
    invalid([&] { VarFindFontRightCoordinates(1,2,3,4,regular,&x,&y,L"%"); });
    invalid([&] { VarFindFontCenterCoordinates(1,2,3,4,regular,&x,&y,L"%0600d",1); });
    invalid([&] { VarFindFontRightCoordinates(1,2,3,4,-1,&x,&y,L"A"); });
    invalid([&] { VarFindFontCenterCoordinates(1,2,3,4,regular,nullptr,&y,L"A"); });
    invalid([&] { VarFindFontRightCoordinates(1,2,3,4,regular,&x,nullptr,L"A"); });
    bound.push_back(L'A');
    invalid([&] { VarFindFontRightCoordinates(0,0,0,3,regular,&x,&y,L"%s",bound.c_str()); });
    wchar_t format[]=L"%s%d";
    check(StringPixLengthArg(regular,1,format,L"AB",2)==4,"formatted metric counts requested prefix");
    check(StringPixLengthArg(regular,99,format,L"AB",2)==10,"formatted metric caps count at text length");
    check(StringPixLengthArg(regular,0,format,L"AB",2)==0,"zero prefix");
    wchar_t tooLong[]=L"%0600d";
    check(StringPixLengthArg(regular,1,tooLong,1)==0 && StringPixLengthArg(regular,1,nullptr)==0,"formatted metric failures");
    check(StringNPixLength(ab,1,regular)==4 && StringNPixLength(ab,99,regular)==6,"bounded metric honors count and terminator");
    check(StringNPixLength(nullptr,1,regular)==0 && StringNPixLength(ab,1,-1)==0,"bounded metric invalid inputs");
    check(SubstringPixLength(ab,0,0,regular)==4 && SubstringPixLength(ab,1,1,regular)==2 &&
          SubstringPixLength(ab,0,1,regular)==6,"inclusive substring endpoints");
    check(SubstringPixLength(ab,1,2,regular)==6,"legacy substring NUL endpoint counts fallback glyph");
    check(SubstringPixLength(ab,2,1,regular)==0 && SubstringPixLength(ab,0,3,regular)==0 &&
          SubstringPixLength(nullptr,0,0,regular)==0 && SubstringPixLength(ab,0,1,-1)==0,"substring invalid ranges/font/text");
    std::wstring huge(10000,L'A');
    check(StringPixLength(huge.c_str(),regular)==32767 && StringNPixLength(huge.data(),10000,regular)==32767 &&
          SubstringPixLength(huge.data(),0,9999,regular)==32767,"all metric forms saturate consistently");
    FindFontCenterCoordinates(0,0,0,0,huge.data(),regular,&x,&y);
    check(x==-16383 && y==-1,"positioning reuses saturated width");

    VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    desc.usWidth=12; desc.usHeight=10; desc.ubBitDepth=16;
    UINT32 dest=0; check(AddVideoSurface(&desc,&dest),"real font destination");
    check(SetFont(regular) && SetFontDestBuffer(dest,0,0,12,10,FALSE),"regular drawing state");
    SetRGBFontForeground(static_cast<UINT16>(0x1234)); SetRGBFontShadow(static_cast<UINT16>(0)); SetFontBackground(0);
    SaveFontSettings(); const auto saved=sgp::CaptureNativeFontState();
    check(SetFont(other) && SetFontDestBuffer(dest,1,1,11,9,TRUE),"distinct active state");
    SetRGBFontForeground(static_cast<UINT16>(0xabcd));
    const auto active=sgp::CaptureNativeFontState(); const auto before=pixels(dest);
    FindFontCenterCoordinates(0,0,12,10,a,regular,&x,&y);
    VarFindFontRightCoordinates(0,0,12,10,regular,&x,&y,L"%s",L"AB");
    check(same(active,sgp::CaptureNativeFontState()) && pixels(dest)==before,"positioning preserves complete active state and pixels");
    RestoreFontSettings();
    check(same(saved,sgp::CaptureNativeFontState()),"positioning preserves legacy saved slot");
    FindFontCenterCoordinates(0,0,12,10,a,regular,&x,&y);
    check(x==4 && y==3 && mprintf(x,y,L"A")==0,"computed coordinates feed real glyph renderer");
    auto expected=before;
    const UINT32 pitch=FontDestPitch;
    // Literal fixture A has foreground at (1,0),(0,1),(2,1),(0,2),(2,2),
    // plus its +1 x descriptor offset; shadow/transparent pixels stay zero.
    for (const auto& p:std::vector<std::pair<int,int>>{{6,3},{5,4},{7,4},{5,5},{7,5}})
    {
        const UINT16 color=0x1234;
        std::memcpy(expected.data()+static_cast<std::size_t>(p.second)*pitch+p.first*2,&color,2);
    }
    check(pixels(dest)==expected,"actual positioned glyph grid, preserved colors, allocation padding");
    blt_vs_fx fill{}; fill.ColorFill=0;
    check(BltVideoSurface(dest,0,0,0,0,VS_BLT_COLORFILL,&fill),"clear coded-string destination");
    auto codedExpected=pixels(dest); SetRGBFontForeground(UINT16{0xf800});
    wchar_t coded[]={180,2,L'A',181,L'B',181,0};
    check(mprintf_coded(0,0,coded)==0,"real coded palette selection and terminal reset");
    for (const auto& point:std::vector<std::pair<int,int>>{{2,0},{1,1},{3,1},{1,2},{3,2},{4,1},{5,1},{4,2},{4,3}}) {
        const UINT16 color=point.first>=4 ? 0xf800 : 0x07e0;
        std::memcpy(codedExpected.data()+static_cast<std::size_t>(point.second)*pitch+point.first*2,&color,2);
    }
    check(pixels(dest)==codedExpected && sgp::CaptureNativeFontState().foreground==0xf800,"literal coded glyph colors, reset foreground and padding");
    const auto codedState=sgp::CaptureNativeFontState(); wchar_t dangling[]={L'A',180,0};
    check(mprintf_coded(0,0,dangling)==UINT32_MAX && pixels(dest)==codedExpected && same(codedState,sgp::CaptureNativeFontState()),"dangling code fails before state or pixels");
    std::vector<BYTE> codedRaw(16+codedExpected.size()+9,0xa5);
    std::fill(codedRaw.begin()+16,codedRaw.end()-9,0);
    auto rawExpected=codedRaw;
    for (const auto& point:std::vector<std::pair<int,int>>{{2,0},{1,1},{3,1},{1,2},{3,2},{4,1},{5,1},{4,2},{4,3}}) {
        const UINT16 color=point.first>=4 ? 0xf800 : 0x07e0;
        std::memcpy(rawExpected.data()+16+static_cast<std::size_t>(point.second)*pitch+point.first*2,&color,2);
    }
    check(mprintf_buffer_coded(codedRaw.data()+16,pitch,MAX_FONTS,0,0,coded)==0 && codedRaw==rawExpected &&
          same(codedState,sgp::CaptureNativeFontState()),
          "original coded-buffer API uses selected font, literal colors, reset, guards and padding");
    check(mprintf_buffer_coded(codedRaw.data()+16,pitch,0,0,0,dangling)==UINT32_MAX && codedRaw==rawExpected &&
          same(codedState,sgp::CaptureNativeFontState()),"coded-buffer dangling control is atomic");
    check(mprintf_buffer_coded(nullptr,pitch,0,0,0,coded)==UINT32_MAX &&
          mprintf_buffer_coded(codedRaw.data()+16,1,0,0,0,coded)==UINT32_MAX && codedRaw==rawExpected &&
          same(codedState,sgp::CaptureNativeFontState()),"coded-buffer invalid destination/pitch preserves state and pixels");
    auto* fontObject=GetFontObject(regular); const auto originalLength=fontObject->pETRLEObject[1].uiDataLength;
    fontObject->pETRLEObject[1].uiDataLength=1;
    check(mprintf_coded(0,0,coded)==UINT32_MAX && pixels(dest)==codedExpected && same(codedState,sgp::CaptureNativeFontState()),"coded output prevalidates every glyph before writes");
    check(mprintf_buffer_coded(codedRaw.data()+16,pitch,0,0,0,coded)==UINT32_MAX && codedRaw==rawExpected &&
          same(codedState,sgp::CaptureNativeFontState()),"coded-buffer prevalidates malformed final glyph before writes");
    fontObject->pETRLEObject[1].uiDataLength=originalLength;
    UnloadFont(static_cast<UINT32>(regular));
    invalid([&] { FindFontRightCoordinates(0,0,1,1,a,regular,&x,&y); });
    check(DeleteVideoSurfaceFromIndex(dest),"destination cleanup");
}
}
int main(int argc, char** argv)
{
    try {
        check(argc==2,"render fixture root argument");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(directory->init(),"real VFS directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("font-layout",vfs::Path(root.u8string()),false);
        profile->addLocation(directory.get()); auto* location=directory.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"real VFS mount");
        layout(); cleanup();
        std::cout << "PASS: " << checks << " native font layout assertions\n"; return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n'; cleanup(); return 1;
    }
}
