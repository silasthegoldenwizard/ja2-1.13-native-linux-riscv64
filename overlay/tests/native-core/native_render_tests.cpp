/*
 * P710 native notice preparation: tests/native-core/native_render_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add native software-surface and bitmap-font contract tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/native_render_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Font.h"
#include "vsurface.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <climits>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
std::size_t assertions=0;
void check(bool b, const char* what)
{ ++assertions; if (!b) throw std::runtime_error(what); }
constexpr UINT32 fail=0xffffffffu;
using Grid=std::vector<std::vector<int>>;
UINT32 surface(UINT32 w, UINT32 h)
{
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=w; d.usHeight=h; d.ubBitDepth=16;
    UINT32 id=0; check(AddVideoSurface(&d,&id) && id!=0,"create"); return id;
}
std::vector<BYTE> read(UINT32 id)
{
    UINT16 w,h; UINT8 bpp;
    check(GetVideoSurfaceDescription(id,&w,&h,&bpp) && bpp==16,"description");
    UINT32 pitch=0; auto* p=LockVideoSurface(id,&pitch); check(p!=nullptr,"read lock");
    std::vector<BYTE> result(p,p+static_cast<std::size_t>(pitch)*h);
    UnLockVideoSurface(id); return result;
}
void expect(UINT32 id, const Grid& grid)
{
    auto actual=read(id);
    const std::size_t w=grid[0].size(), h=grid.size(), pitch=((w*2+7)&~std::size_t(7))+8;
    std::vector<BYTE> expected(pitch*h,0xa5);
    for (std::size_t y=0; y<h; ++y) for (std::size_t x=0; x<w; ++x)
    { const auto c=static_cast<UINT16>(grid[y][x]); std::memcpy(expected.data()+y*pitch+x*2,&c,2); }
    check(actual==expected,"whole allocation including padding");
}
void place(Grid& dest, int x, int y, const Grid& source)
{
    for (std::size_t sy=0; sy<source.size(); ++sy) for (std::size_t sx=0; sx<source[sy].size(); ++sx)
    {
        const auto dx=static_cast<std::int64_t>(x)+static_cast<std::int64_t>(sx);
        const auto dy=static_cast<std::int64_t>(y)+static_cast<std::int64_t>(sy);
        if (source[sy][sx]>=0 && dx>=0 && dy>=0 && dy<static_cast<std::int64_t>(dest.size()) && dx<static_cast<std::int64_t>(dest[0].size()))
            dest[static_cast<std::size_t>(dy)][static_cast<std::size_t>(dx)]=source[sy][sx];
    }
}
void clear(UINT32 id, int w, int h, UINT16 c=0)
{ check(ColorFillVideoSurfaceArea(id,0,0,w,h,c),"clear"); }
void surfacesTest()
{
    check(InitializeVideoSurfaceManager(),"surface init"); check(InitializeVideoSurfaceManager(),"repeat init");
    SGPRect all{INT_MIN,INT_MIN,INT_MAX,INT_MAX}; SetClippingRect(&all);
    VSURFACE_DESC bad{}; UINT32 id=99;
    check(!AddStandardVideoSurface(nullptr,&id) && id==0,"null desc resets id");
    check(!AddStandardVideoSurface(&bad,nullptr),"null output");
    bad={VSURFACE_CREATE_DEFAULT,{},65536,1,16}; check(!CreateVideoSurface(&bad),"large width");
    bad.usWidth=1; bad.usHeight=65536; check(!CreateVideoSurface(&bad),"large height");
    bad.usHeight=1; bad.ubBitDepth=24; check(!CreateVideoSurface(&bad),"unsupported depth");
    bad.ubBitDepth=16; bad.fCreateFlags=VSURFACE_CREATE_FROMFILE; check(!CreateVideoSurface(&bad),"fromfile");
    bad.fCreateFlags=VSURFACE_CREATE_DEFAULT|VSURFACE_VIDEO_MEM_USAGE; check(!CreateVideoSurface(&bad),"unsupported video-memory creation flags");
    for (UINT32 r : {PRIMARY_SURFACE,BACKBUFFER,FRAME_BUFFER,MOUSE_BUFFER,0u})
    {
        HVSURFACE p=nullptr; UINT32 pitch=99;
        check(!GetVideoSurface(&p,r) && !p,"reserved lookup");
        check(!LockVideoSurface(r,&pitch) && pitch==0,"reserved lock");
        check(!DeleteVideoSurfaceFromIndex(r),"reserved delete");
        check(!ColorFillVideoSurfaceArea(r,0,0,1,1,1),"reserved fill");
    }
    id=surface(5,4); expect(id,Grid(4,std::vector<int>(5,0)));
    HVSURFACE p=nullptr; check(GetVideoSurface(&p,id),"get surface");
    UINT32 pitch=0; check(LockVideoSurfaceBuffer(p,&pitch) && pitch==24,"padded lock");
    UINT32 nested=99; check(!LockVideoSurface(id,&nested) && nested==0,"nested lock");
    check(!DeleteVideoSurface(p) && !DeleteVideoSurfaceFromIndex(id),"locked delete");
    check(!ShutdownVideoSurfaceManager(),"locked shutdown");
    check(!ColorFillVideoSurfaceArea(id,0,0,5,4,9),"locked mutation");
    UnLockVideoSurface(id); UnLockVideoSurfaceBuffer(p);
    check(!LockVideoSurfaceBuffer(p,nullptr),"null pitch");
    check(!DeleteVideoSurface(nullptr),"null delete");
    int foreign=0; check(!DeleteVideoSurface(reinterpret_cast<HVSURFACE>(&foreign)),"foreign pointer");
    SGPRect clip{1,1,4,3}; SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(id,INT_MIN,INT_MIN,INT_MAX,INT_MAX,7),"extreme fill");
    expect(id,{{0,0,0,0,0},{0,7,7,7,0},{0,7,7,7,0},{0,0,0,0,0}});
    auto before=read(id);
    check(!ColorFillVideoSurfaceArea(id,3,2,2,3,8),"reversed fill"); check(read(id)==before,"invalid fill atomic");
    SetClippingRect(&all);
    UINT32 tile=surface(3,2); clear(tile,3,2,3); check(ColorFillVideoSurfaceArea(tile,1,0,2,2,0),"key stripe");
    clear(id,5,4,9); check(BltVideoSurface(id,tile,0,1,1,0,nullptr),"opaque copy");
    expect(id,{{9,9,9,9,9},{9,3,0,3,9},{9,3,0,3,9},{9,9,9,9,9}});
    clear(id,5,4,9); check(SetVideoSurfaceTransparency(tile,FROMRGB(0,0,0)),"key");
    check(BltVideoSurface(id,tile,0,1,1,VS_BLT_USECOLORKEY,nullptr),"key copy");
    expect(id,{{9,9,9,9,9},{9,3,9,3,9},{9,3,9,3,9},{9,9,9,9,9}});
    for (auto xy : std::vector<std::pair<int,int>>{{-2,0},{4,0},{0,-1},{0,3},{INT_MIN,INT_MAX},{INT_MAX,INT_MIN}})
    {
        clear(id,5,4,9); Grid g(4,std::vector<int>(5,9)); place(g,xy.first,xy.second,{{3,0,3},{3,0,3}});
        check(BltVideoSurface(id,tile,0,xy.first,xy.second,0,nullptr),"edge copy"); expect(id,g);
    }
    clear(id,5,4,9); blt_vs_fx fx{}; fx.SrcRect={1,0,3,2};
    // Original HandleTopMessages initializes only SrcRect. Other effects
    // fields have meaning only when their corresponding flags select them.
    fx.DestRegion=0x6a5a;
    check(BltVideoSurface(id,tile,0,1,1,VS_BLT_SRCSUBRECT,&fx),"subrect ignores unselected destination-region effect");
    expect(id,{{9,9,9,9,9},{9,0,3,9,9},{9,0,3,9,9},{9,9,9,9,9}});
    fx.SrcRect={-1,-1,2,2}; clear(id,5,4,9);
    check(BltVideoSurface(id,tile,0,0,0,VS_BLT_SRCSUBRECT,&fx),"negative subrect");
    expect(id,{{9,9,9,9,9},{9,3,0,9,9},{9,3,0,9,9},{9,9,9,9,9}});
    clip={2,1,3,2}; SetClippingRect(&clip); clear(id,5,4,8);
    check(BltVideoSurface(id,tile,0,1,1,VS_BLT_CLIPPED,nullptr),"global blit clip");
    expect(id,{{9,9,9,9,9},{9,3,0,9,9},{9,3,0,9,9},{9,9,9,9,9}});
    SetClippingRect(&all); before=read(id);
    for (UINT32 flag : {VS_BLT_FAST,VS_BLT_USEDESTCOLORKEY,VS_BLT_SRCREGION,VS_BLT_DESTREGION,VS_BLT_MIRROR_Y,0x80000000u})
    { check(!BltVideoSurface(id,tile,0,0,0,flag,nullptr),"unsupported flag"); check(read(id)==before,"flag atomic"); }
    fx.SrcRect={2,0,1,1}; check(!BltVideoSurface(id,tile,0,0,0,VS_BLT_SRCSUBRECT,&fx),"bad subrect");
    check(!BltVideoSurface(id,tile,0,0,0,VS_BLT_SRCSUBRECT,nullptr),"missing fx");
    check(!BltVideoSurface(id,tile,1,0,0,0,nullptr),"region rejected"); check(read(id)==before,"invalid rect atomic");
    fx={}; fx.ColorFill=6; fx.FillRect={1,1,4,3}; fx.DestRegion=0x6a5a;
    check(BltVideoSurface(id,0,0,0,0,VS_BLT_COLORFILL,&fx),"whole fill blit"); expect(id,Grid(4,std::vector<int>(5,6)));
    fx.ColorFill=8; check(BltVideoSurface(id,0,0,0,0,VS_BLT_COLORFILLRECT,&fx),"rect fill blit");
    expect(id,{{6,6,6,6,6},{6,8,8,8,6},{6,8,8,8,6},{6,6,6,6,6}});
    before=read(id); check(!BltVideoSurface(id,0,0,0,0,VS_BLT_COLORFILL|VS_BLT_COLORFILLRECT,&fx),"conflicting fill"); check(read(id)==before,"fill flags atomic");
    clear(id,5,4); for (int x=0; x<5; ++x) check(ColorFillVideoSurfaceArea(id,x,0,x+1,1,static_cast<UINT16>(x+1)),"self seed");
    check(BltVideoSurface(id,id,0,1,0,0,nullptr),"self right");
    expect(id,{{1,1,2,3,4},{0,0,0,0,0},{0,0,0,0,0},{0,0,0,0,0}});
    check(SetVideoSurfaceTransparencyColor(p,FROMRGB(0,0,0)),"pointer transparency");
    check(BltVideoSurfaceToVideoSurface(p,p,0,-1,1,VS_BLT_USECOLORKEY,nullptr),"self keyed down left");
    expect(id,{{1,1,2,3,4},{1,2,3,4,0},{0,0,0,0,0},{0,0,0,0,0}});
    check(DeleteVideoSurface(p),"pointer delete"); check(!DeleteVideoSurface(p),"stale pointer");
    check(!DeleteVideoSurfaceFromIndex(id),"stale id"); UINT32 newer=surface(1,1); check(newer>id,"monotonic ids");
    check(!LockVideoSurfaceBuffer(p,&pitch),"stale pointer after allocation");
    check(ShutdownVideoSurfaceManager() && ShutdownVideoSurfaceManager(),"repeat shutdown");
    check(InitializeVideoSurfaceManager(),"restart"); auto next=surface(1,1); check(next>newer,"ids survive restart");
    check(DeleteVideoSurfaceFromIndex(next),"delete restart surface");
}
void fontsTest()
{
    UINT16 values[]={65,66,32}; FontTranslationTable table{3,values};
    check(!InitializeFontManager(8,&table) && !InitializeFontManager(16,nullptr),"font init rejects");
    check(InitializeFontManager(16,&table),"font init"); values[0]=90;
    check(GetIndex(L'A')==0 && GetIndex(L'B')==1 && GetIndex(L' ')==2 && GetIndex(L'Z')==0 && GetIndex(L'\U0001f600')==0,"copied translation and fallback");
    char file[]="font.sti", alt[]="font_alt.sti", malformed[]="bad_descriptor.sti", badRun[]="bad_run.sti";
    check(LoadFontFile(malformed)==-1 && LoadFontFile(badRun)==-1,"malformed loads");
    const INT32 f=LoadFontFile(file), other=LoadFontFile(alt); check(f==0 && other==1,"font slots");
    check(!IsFontLoaded(-1) && !IsFontLoaded(MAX_FONTS) && !GetFontObject(MAX_FONTS),"font bounds");
    check(!SetFont(-1) && !SetFont(MAX_FONTS) && SetFont(f),"selection bounds");
    check(GetWidth(GetFontObject(f),0)==4 && GetWidth(GetFontObject(f),1)==2 && GetWidth(GetFontObject(f),-1)==0,"width offsets");
    check(GetFontHeight(f)==3 && GetFontHeight(other)==4 && StringPixLength(L"AB ?",f)==12,"metrics and fallback");
    std::wstring longText(10000,L'A'); check(StringPixLength(longText.c_str(),f)==32767,"saturated metrics");
    for (int i=2; i<MAX_FONTS; ++i) check(LoadFontFile(file)==i,"all font slots");
    check(LoadFontFile(file)==-1,"font capacity");
    for (UINT32 i=2; i<MAX_FONTS; ++i) UnloadFont(i);
    UINT32 d=surface(10,9); check(SetFontDestBuffer(d,-5,-5,99,99,FALSE),"clamped dest");
    check(FontDestPitch==32 && FontDestBPP==16 && FontDestRegion.iRight==10,"dest description");
    check(!SetFontDestBuffer(BACKBUFFER,0,0,10,9,FALSE) && FontDestBuffer==d,"invalid dest atomic");
    check(!SetFontDestBuffer(d,10,0,11,9,FALSE) && !SetFontDestBuffer(d,2,2,1,3,FALSE),"invalid dest region");
    wchar_t a[]=L"A", ab[]=L"AB", space[]=L" ";
    SetFontForeground(2); SetFontShadow(1); SetFontBackground(3);
    clear(d,10,9,9); check(mprintf(1,1,L"A")==0,"mono");
    Grid g(9,std::vector<int>(10,9)); place(g,2,1,{{0x001f,0x07e0,0x001f},{0x07e0,0xf800,0x07e0},{0x07e0,0x001f,0x07e0}}); expect(d,g);
    SetRGBFontForeground(static_cast<UINT16>(0)); SetRGBFontShadow(static_cast<UINT16>(0)); SetFontBackground(0);
    clear(d,10,9,9); check(mprintf(1,1,L"A")==0,"zero foreground");
    g=Grid(9,std::vector<int>(10,9)); place(g,2,1,{{-1,0,-1},{0,-1,0},{0,-1,0}}); expect(d,g);
    clear(d,10,9,9); check(gprintf(1,1,a)==0,"colored");
    g=Grid(9,std::vector<int>(10,9)); place(g,2,1,{{-1,0x07e0,-1},{0x07e0,0xf800,0x07e0},{0x07e0,0,0x07e0}}); expect(d,g);
    SetRGBFontForeground(255,255,255); SetRGBFontBackground(0,0,255); SetRGBFontShadow(0,255,0);
    clear(d,10,9,9); check(mprintf(1,1,L" ")==0,"space background");
    g=Grid(9,std::vector<int>(10,9)); place(g,1,1,{{31,31},{31,31},{31,31}}); expect(d,g);
    SetFontBackground(0); auto before=read(d); check(gprintf(1,1,space)==0 && read(d)==before,"colored transparent space");
    const Grid colored={{-1,0x07e0,-1},{0x07e0,0xf800,0x07e0},{0x07e0,0,0x07e0}};
    for (auto xy : std::vector<std::pair<int,int>>{{-2,0},{8,0},{0,-2},{0,8},{INT_MIN,INT_MAX},{INT_MAX,INT_MIN}})
    {
        clear(d,10,9,9); check(gprintf(xy.first,xy.second,a)==0,"font edge");
        g=Grid(9,std::vector<int>(10,9));
        if (xy.first!=INT_MAX && xy.first!=INT_MIN) place(g,xy.first+1,xy.second,colored);
        expect(d,g);
    }
    FontDestRegion={INT_MIN,INT_MIN,INT_MAX,INT_MAX}; clear(d,10,9,9); check(gprintf(8,8,a)==0,"mutable region physical clamp");
    g=Grid(9,std::vector<int>(10,9)); expect(d,g);
    check(SetFontDestBuffer(d,0,0,6,9,TRUE),"wrap dest"); clear(d,10,9,9);
    check(gprintf(2,0,ab)==0,"wrap"); g=Grid(9,std::vector<int>(10,9));
    place(g,3,0,colored); place(g,2,5,{{31,31},{31,-1},{31,0xf800}}); expect(d,g);
    check(SetFontDestBuffer(d,0,0,10,9,FALSE),"full dest");
    std::vector<BYTE> buffer(32*9,0xa5);
    check(gprintf_buffer(buffer.data(),32,static_cast<UINT32>(other),0,0,a)==0,"explicit colored font");
    std::vector<BYTE> expected(32*9,0xa5);
    auto seed=[&](int x,int y,UINT16 c) { std::memcpy(expected.data()+static_cast<std::size_t>(y)*32+static_cast<std::size_t>(x)*2,&c,2); };
    seed(0,1,31); seed(1,1,31); seed(0,2,31); seed(0,3,31); seed(1,3,0xf800);
    check(buffer==expected,"explicit colored grid and padding");
    SetFontColors(2); SetFontShadow(1); buffer.assign(32*9,0xa5); expected.assign(32*9,0xa5);
    seed(0,1,0x07e0); seed(1,1,0x07e0); seed(0,2,0x07e0); seed(0,3,0x07e0); seed(1,3,0xf800);
    check(mprintf_buffer(buffer.data(),32,static_cast<UINT32>(other),0,0,a)==0 && buffer==expected,"explicit mono font");
    auto untouched=buffer;
    check(mprintf_buffer(buffer.data(),1,0,0,0,a)==fail && buffer==untouched,"bad pitch");
    check(gprintf_buffer(nullptr,32,0,0,0,a)==fail,"null buffer");
    check(gprintf_buffer(buffer.data(),32,MAX_FONTS,0,0,a)==fail,"explicit font bounds");
    before=read(d); wchar_t hugeFormat[]=L"%0600d";
    check(mprintf(0,0,L"%0600d",1)==fail && read(d)==before,"surface format bound");
    check(gprintf_buffer(buffer.data(),32,0,0,0,hugeFormat,1)==fail && buffer==untouched,"buffer format bound");
    auto* object=GetFontObject(f); auto descriptor=object->pETRLEObject[0];
    object->pETRLEObject[0].uiDataOffset=UINT_MAX;
    check(mprintf(0,0,L"BA")==fail && read(d)==before,"descriptor string atomic");
    object->pETRLEObject[0]=descriptor;
    auto* data=static_cast<BYTE*>(object->pPixData); const BYTE control=data[descriptor.uiDataOffset];
    data[descriptor.uiDataOffset]=0x84; check(gprintf(0,0,a)==fail && read(d)==before,"run malformed"); data[descriptor.uiDataOffset]=control;
    object->pETRLEObject[0].uiDataLength=1; check(gprintf(0,0,a)==fail && read(d)==before,"truncated run"); object->pETRLEObject[0]=descriptor;
    SetRGBFontForeground(static_cast<UINT16>(0xffff)); SetRGBFontShadow(static_cast<UINT16>(0x07e0)); SetFontBackground(0);
    SaveFontSettings(); check(SetFont(other),"change font"); SetRGBFontForeground(static_cast<UINT16>(1));
    check(SetFontDestBuffer(d,1,1,2,2,TRUE),"change settings"); RestoreFontSettings();
    check(FontDefault==f && FontDestBuffer==d && !FontDestWrap && FontDestRegion.iRight==10,"settings restored");
    clear(d,10,9,9); check(mprintf(1,1,L"A")==0,"restored colors");
    g=Grid(9,std::vector<int>(10,9)); place(g,2,1,{{-1,0xffff,-1},{0xffff,0x07e0,0xffff},{0xffff,-1,0xffff}}); expect(d,g);
    SaveFontSettings(); UnloadFont(static_cast<UINT32>(f)); check(!IsFontLoaded(f),"unload");
    check(LoadFontFile(file)==f,"reuse slot"); check(DeleteVideoSurfaceFromIndex(d),"destroy font dest"); RestoreFontSettings();
    check(FontDefault==-1 && FontDestBuffer==0 && mprintf(0,0,L"A")==fail,"stale saved font and dest");
    UnloadFont(MAX_FONTS); ShutdownFontManager(); ShutdownFontManager();
    values[0]=65; check(InitializeFontManager(16,&table),"font restart"); check(LoadFontFile(file)==0,"font reload"); ShutdownFontManager();
}
void cleanup()
{
    ShutdownFontManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc, char** argv)
{
    try
    {
        check(argc==2,"fixture root argument");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(dir->init(),"real VFS directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("render",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release()); check(getVFS()->addLocation(location,mounted),"real VFS mount");
        check(InitializeVideoObjectManager(),"object init"); surfacesTest(); fontsTest();
        check(ShutdownVideoSurfaceManager(),"final shutdown"); cleanup();
        std::cout << "PASS: " << assertions << " actual rendering assertions\n"; return 0;
    }
    catch (const std::exception& e)
    { std::cerr << "FAIL after " << assertions << " assertions: " << e.what() << '\n'; cleanup(); return 1; }
}
