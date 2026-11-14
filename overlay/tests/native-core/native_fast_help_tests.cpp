/*
 * P710 native snapshot update: tests/native-core/native_fast_help_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored bitmap-font fixtures; no window, device, game files or sleeps.
#include "native_mousesystem.h"
#include "native_font.h"
#include "NativeScalableFont.h"
#include "NativeTooltipText.h"
#include "native_buttons.h"
#include "clip_state.h"
#include "shading.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0,callbacks=0;
std::uint64_t tick=1000;
std::uint64_t clockHook() { return tick; }
void check(bool ok,const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
void done() { ++callbacks; }
struct Region : MOUSE_REGION { Region():MOUSE_REGION{} {} ~Region() { MSYS_RemoveRegion(this); } };
struct Pixels { UINT32 pitch; std::vector<BYTE> bytes; };
Pixels read(UINT32 id) {
    UINT16 w=0,h=0; UINT8 bpp=0;
    check(GetVideoSurfaceDescription(id,&w,&h,&bpp) && bpp==16,"RGB565 destination");
    UINT32 pitch=0; auto* p=LockVideoSurface(id,&pitch); check(p!=nullptr,"read lock");
    Pixels result{pitch,{p,p+static_cast<std::size_t>(pitch)*h}}; UnLockVideoSurface(id); return result;
}
UINT16 pixel(const Pixels& p,unsigned x,unsigned y) {
    UINT16 result=0; std::memcpy(&result,p.bytes.data()+static_cast<std::size_t>(y)*p.pitch+x*2,2); return result;
}
void hook(UINT16 type,int x=4,int y=22,bool held=false) {
    MSYS_SGP_Mouse_Handler_Hook(type,static_cast<UINT16>(x),static_cast<UINT16>(y),held?TRUE:FALSE,FALSE);
}
void define(Region& r) { MSYS_DefineRegion(&r,2,20,8,26,31,MSYS_NO_CURSOR,nullptr,nullptr); }
void setText(Region& r,const wchar_t* text) { SetRegionFastHelpText(&r,const_cast<wchar_t*>(text)); }
bool visible(const Region& r) { return (r.uiFlags&MSYS_FASTHELP)!=0; }
bool same(const sgp::NativeFontState& a,const sgp::NativeFontState& b) {
    return a.font==b.font && a.generation==b.generation && a.dest==b.dest && a.pitch==b.pitch && a.bpp==b.bpp &&
        a.foreground==b.foreground && a.background==b.background && a.shadow==b.shadow && a.wrap==b.wrap &&
        a.region.iLeft==b.region.iLeft && a.region.iTop==b.region.iTop && a.region.iRight==b.region.iRight && a.region.iBottom==b.region.iBottom &&
        a.scalable==b.scalable && a.scalableColor==b.scalableColor;
}
void run(UINT32 target,INT32 regular,INT32 bold) {
    BuildShadeTable(); check(ShadeTable[0xffff]==0x7bcf && ShadeTable[0x7bcf]==0x39c7,"literal source shade tables");
    for (INT32 f:{regular,bold}) {
        auto* font=GetFontObject(f); font->p16BPPPalette[130]=0xffff; font->p16BPPPalette[146]=0x07e0; font->p16BPPPalette[141]=0xf800;
        // Mono foreground/shadow indices resolve the original RGB palette,
        // whereas colored glyphs resolve p16BPPPalette. Author both views.
        auto& beige=font->pPaletteEntry[130]; beige.peRed=255; beige.peGreen=255; beige.peBlue=255;
        auto& highlight=font->pPaletteEntry[146]; highlight.peRed=0; highlight.peGreen=255; highlight.peBlue=0;
        auto& shadow=font->pPaletteEntry[141]; shadow.peRed=255; shadow.peGreen=0; shadow.peBlue=0;
    }
    SGPRect clip{0,0,64,48}; SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(target,0,0,64,48,0xffff),"white authored background");
    const auto base=read(target);
    check(MSYS_Init()==1,"region manager init");
    sgp::SetNativeFastHelpClockForTesting(clockHook); SetFastHelpDelay(100);
    check(!sgp::ConfigureNativeFastHelp(-1,bold,target) && sgp::ConfigureNativeFastHelp(regular,bold,target),"real font/destination binding");
    Region r; define(r); setText(r,L"A|B\nA"); SetRegionHelpEndCallback(&r,done);
    hook(MOUSE_POS); check(r.FastHelpTimer==100,"hover starts original delay even without movement callback");
    tick=1050; RenderFastHelp(); check(r.FastHelpTimer==50 && !visible(r) && read(target).bytes==base.bytes,"countdown does not paint");
    hook(LEFT_BUTTON_DOWN,4,22,true); tick=1150; RenderFastHelp(); check(r.FastHelpTimer==50,"held button pauses countdown");
    hook(LEFT_BUTTON_UP); tick=1200; RenderFastHelp(); check(r.FastHelpTimer==0 && !visible(r),"expiry retains original next-frame display");
    check(SetFont(bold) && SetFontDestBuffer(target,1,2,63,47,TRUE),"distinct caller font state");
    SetRGBFontForeground(static_cast<UINT16>(0x1234)); SetRGBFontBackground(1,2,3); SetRGBFontShadow(static_cast<UINT16>(0x4567));
    const auto fontState=sgp::CaptureNativeFontState();
    SaveFontSettings();
    RenderFastHelp(); check(visible(r) && r.FastHelpRect==0 && (r.uiFlags&MSYS_GOT_BACKGROUND),"real tooltip owns background");
    check(same(fontState,sgp::CaptureNativeFontState()),"font selection/colors/destination/wrap preserved");
    auto shown=read(target);
    auto noSelection=fontState; noSelection.font=-1; noSelection.generation=0;
    noSelection.background=0x2468; noSelection.shadow=0x3579; noSelection.foreground=0x1357;
    sgp::RestoreNativeFontState(noSelection);
    RenderFastHelp();
    check(read(target).bytes==shown.bytes && same(noSelection,sgp::CaptureNativeFontState()),
        "help colors do not depend on caller font selection or prior nonzero colors");
    check(SetFont(regular),"change caller selection after help"); SetRGBFontForeground(static_cast<UINT16>(0x5678));
    RestoreFontSettings(); check(same(fontState,sgp::CaptureNativeFontState()),"help rendering preserves caller legacy saved-state slot");
    std::vector<std::vector<UINT16>> expected(48,std::vector<UINT16>(64,0xffff));
    for (unsigned y=4;y<20;++y) for (unsigned x=12;x<28;++x) {
        const unsigned dx=x-12,dy=y-4;
        if ((dx==1||dx==15||dy==1||dy==15) && dx>=1 && dy>=1) expected[y][x]=0x41c1;
        if ((dx==0||dx==14||dy==0||dy==14) && dx<=14 && dy<=14) expected[y][x]=0xe62b;
        if (dx>=2&&dx<=13&&dy>=2&&dy<=13) expected[y][x]=0x39c7;
    }
    const auto a=[&](unsigned x,unsigned y) {
        expected[y][x+1]=0xffff; expected[y+1][x]=0xffff; expected[y+1][x+1]=0xf800;
        expected[y+1][x+2]=0xffff; expected[y+2][x]=0xffff; expected[y+2][x+2]=0xffff;
    };
    a(18,9); a(18,13); // authored A offset (+1,0), four-pixel advance
    expected[10][21]=expected[10][22]=0x07e0; expected[11][21]=0x07e0; expected[12][21]=0x07e0; expected[12][22]=0xf800;
    for (unsigned y=0;y<48;++y) {
        for (unsigned x=0;x<64;++x) check(pixel(shown,x,y)==expected[y][x],"literal complete tooltip/background pixel grid");
        for (unsigned x=128;x<shown.pitch;++x) check(shown.bytes[static_cast<std::size_t>(y)*shown.pitch+x]==0xa5,"padding untouched");
    }
    RenderFastHelp(); check(read(target).bytes==shown.bytes,"repeated rendering does not compound shadow");
    ExecuteMouseHelpEndCallBack(&r); check(callbacks==0,"original explicitly disabled callback remains disabled");
    hook(LEFT_BUTTON_DOWN,4,22,true); check(!visible(r) && r.FastHelpTimer==100 && read(target).bytes==base.bytes,"press hides and restores immediately");
    hook(LEFT_BUTTON_UP); tick+=100; RenderFastHelp(); RenderFastHelp(); check(visible(r),"help returns after fresh delay");
    hook(MOUSE_POS,40,40); check(!visible(r) && read(target).bytes==base.bytes,"leaving restores old region background");
    hook(MOUSE_POS); tick+=100; RenderFastHelp(); RenderFastHelp();
    DisableMouseFastHelp(); check(!visible(r) && read(target).bytes==base.bytes,"global disable removes pixels immediately");
    tick+=10000; RenderFastHelp(); check(!visible(r),"global disable gates elapsed time");
    EnableMouseFastHelp(); tick+=100; RenderFastHelp(); RenderFastHelp(); check(visible(r),"global enable restarts hover delay");
    gfRenderHilights=FALSE; RenderFastHelp(); check(!visible(r) && read(target).bytes==base.bytes,"original highlight gate suppresses real pixels"); gfRenderHilights=TRUE;
    RenderFastHelp(); check(visible(r),"highlight gate resumes");
    MSYS_DisableRegion(&r); check(!visible(r) && read(target).bytes==base.bytes,"disabled region removes tooltip");
    RefreshMouseRegions(); tick+=100; RenderFastHelp(); RenderFastHelp(); check(!visible(r),"disabled unallowed region does not render");
    MSYS_AllowDisabledRegionFastHelp(&r,TRUE); RefreshMouseRegions(); tick+=100; RenderFastHelp(); RenderFastHelp(); check(visible(r),"allowed disabled region actually renders");
    setText(r,L"B"); check(!visible(r) && read(target).bytes==base.bytes && r.FastHelpTimer==100,"replacement restores and resets");
    tick+=100; RenderFastHelp(); RenderFastHelp(); check(visible(r),"replacement is rendered");
    setText(r,L""); check(!visible(r) && read(target).bytes==base.bytes,"cleared text removes owned pixels");
    setText(r,L"A"); tick+=100; RenderFastHelp(); RenderFastHelp(); check(visible(r),"show before deferred restoration");
    UINT32 pitch=0; check(LockVideoSurface(target,&pitch)!=nullptr,"caller locks destination"); DisableMouseFastHelp();
    check(!visible(r) && !sgp::RestoreNativeFastHelpBackground(),"locked background restoration deferred honestly");
    UnLockVideoSurface(target); RenderFastHelp(); check(read(target).bytes==base.bytes,"next unlocked render restores pending pixels");
    EnableMouseFastHelp(); SetFastHelpDelay(0); hook(MOUSE_POS,40,40); hook(MOUSE_POS); RenderFastHelp(); check(visible(r),"zero delay paints first frame");
    check(ColorFillVideoSurfaceArea(target,12,8,13,9,0x1234),"underlying scene changes one pixel");
    check(sgp::RestoreNativeFastHelpBackground(),"before-scene restoration"); auto changed=read(target);
    check(pixel(changed,12,8)==0x1234 && pixel(changed,13,8)==0xffff,"restoration preserves changed scene pixels");
    check(ColorFillVideoSurfaceArea(target,0,0,64,48,0xffff),"reset authored scene"); RenderFastHelp();
    MSYS_RemoveRegion(&r); check(read(target).bytes==base.bytes && !r.FastHelpText && !r.uiFlags,"removal releases text and tooltip");
    define(r); setText(r,L"A"); hook(MOUSE_POS); RenderFastHelp(); check(visible(r),"address reused as fresh region");
    sgp::CancelNativeMouseInput(); check(!visible(r) && !MSYS_CurrRegion && read(target).bytes==base.bytes,"input reset clears tooltip lifetime");
    hook(MOUSE_POS); RenderFastHelp();
    UnloadFont(static_cast<UINT32>(bold)); check(LoadFontFile(const_cast<char*>("font_alt.sti"))==bold,"font slot reused");
    RenderFastHelp(); check(!visible(r) && read(target).bytes==base.bytes,"configured font generation refuses reused slot");
    check(sgp::ConfigureNativeFastHelp(regular,bold,target),"rebind real replacement font"); RenderFastHelp();
    const auto saved=GetFontObject(regular)->pETRLEObject[0]; GetFontObject(regular)->pETRLEObject[0].uiDataOffset=0xffffffffu;
    RenderFastHelp(); check(!visible(r) && read(target).bytes==base.bytes,"malformed glyph staging fails without pixel writes");
    GetFontObject(regular)->pETRLEObject[0]=saved;
    SetFastHelpDelay(50); hook(MOUSE_POS,40,40); hook(MOUSE_POS); tick-=10; RenderFastHelp(); check(r.FastHelpTimer==50,"backwards test clock cannot expire timer");
    tick+=std::uint64_t(1)<<40; RenderFastHelp(); check(r.FastHelpTimer==0,"large monotonic interval saturates without narrowing wrap"); RenderFastHelp();
    MSYS_Shutdown(); check(read(target).bytes==base.bytes && !r.FastHelpText && !r.uiFlags,"shutdown restores pixels and releases owned text");
    sgp::SetNativeFastHelpClockForTesting(nullptr);
}
INT32 publicFace(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    const std::vector<BYTE> bytes((std::istreambuf_iterator<char>(input)),{});
    check(!bytes.empty(),"read public TTF fixture");
    const INT32 id=sgp::CreateNativeScalableFont(bytes.data(),bytes.size(),22);
    check(id>0,"actual 22px FreeType tooltip face"); return id;
}
void scaled(UINT32 target,INT32 regular,INT32 bold,const std::filesystem::path& regularPath,
            const std::filesystem::path& boldPath) {
    const INT32 normal=publicFace(regularPath),heavy=publicFace(boldPath);
    check(sgp::SetNativeScalableFontColor(normal,FROMRGB(201,197,143)) &&
          sgp::SetNativeScalableFontColor(heavy,FROMRGB(223,176,1)),"original tooltip initialization colors");
    sgp::NativeScalableFontMetrics normalMetrics,heavyMetrics;
    check(sgp::GetNativeScalableFontMetrics(normal,&normalMetrics) &&
          sgp::GetNativeScalableFontMetrics(heavy,&heavyMetrics),"actual tooltip metrics");
    const auto width=[&](const wchar_t* text,bool isBold=false) {
        INT32 result=0;check(sgp::MeasureNativeScalableText(isBold?heavy:normal,text,&result),"real run width oracle");return result;
    };
    const INT32 a=width(L"A"),b=width(L"B",true),av=width(L"AV"),dots=width(L"...");
    check(av<width(L"A")+width(L"V"),"public face provides meaningful AV kerning fixture");
    const INT32 fh=normalMetrics.ascent;
    check(fh>0 && 2*(fh+1)>38,"22px fixture exercises original 48px overflow threshold");
    SGPRect clip{0,0,64,48};SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(target,0,0,64,48,0xffff),"reset scaled scene");
    const auto base=read(target);
    check(MSYS_Init()==1,"scaled real region manager");
    SetFastHelpDelay(0);sgp::SetNativeFastHelpClockForTesting(clockHook);
    check(sgp::ConfigureNativeFastHelp(regular,bold,target),"rebind actual reused bitmap generation");
    check(sgp::ConfigureNativeScalableFastHelp(normal,heavy,2.0) && isTooltipScalingEnabled() &&
          GetScaledFontHeight()==fh,"scale two selects actual ascent");
    check(SetFont(bold) && SetFontDestBuffer(target,1,2,63,47,TRUE),"scaled distinct caller state");
    SetRGBFontForeground(UINT16{0x1234});SetRGBFontBackground(1,2,3);SetRGBFontShadow(UINT16{0x4567});
    const auto state=sgp::CaptureNativeFontState();SaveFontSettings();
    struct Run {const wchar_t* text;INT32 x;bool bold;};
    const auto drawOracle=[&](std::vector<BYTE>& output,UINT32 pitch,UINT32 w,UINT32 h,
                              INT32 x,INT32 y,const std::vector<Run>& runs) {
        for (const auto& run:runs) {
            const auto& metrics=run.bold?heavyMetrics:normalMetrics;
            const INT32 top=y>=metrics.internalLeading?y-metrics.internalLeading:y;
            check(sgp::DrawNativeScalableTextBuffer(output.data(),output.size(),w,h,pitch,
                run.bold?heavy:normal,x+run.x,top,run.text,
                run.bold?FROMRGB(223,176,1):FROMRGB(201,197,143)),"direct real core run rasterization oracle");
        }
    };
    const auto helper=[&](const wchar_t* text,INT32 expectedWidth,INT16 lines,const std::vector<Run>& runs) {
        check(GetWidthOfString(const_cast<wchar_t*>(text))==expectedWidth &&
              GetNumberOfLinesInHeight(const_cast<wchar_t*>(text))==lines,"public shared width/line helpers follow original token policy");
        check(ColorFillVideoSurfaceArea(target,0,0,64,48,0xffff),"reset helper destination");
        auto expected=read(target);drawOracle(expected.bytes,expected.pitch,64,48,3,4,runs);
        DisplayHelpTokenizedString(const_cast<wchar_t*>(text),3,4);
        check(read(target).bytes==expected.bytes,"public DisplayHelpTokenizedString equals source-bound runs and real FreeType pixels including padding");
        check(same(state,sgp::CaptureNativeFontState()),"public helpers preserve complete caller font state");
    };
    // Explicit source-bound runs, not a second tokenization implementation.
    helper(L"A|B",a+b,1,{{L"A",0,false},{L"B",a,true}});
    helper(L"AV",av,1,{{L"AV",0,false}});
    helper(L"A||B",a+b,1,{{L"A",0,false},{L"B",a,true}});
    helper(L"||AB",width(L"A",true)+width(L"B"),1,
           {{L"A",0,true},{L"B",width(L"A",true),false}});
    // Original overflow overlays an ellipsis on the current line; its line
    // counting helper independently counts a nonterminal newline before cap.
    helper(L"A\n",std::max(a,dots),1,{{L"A",0,false},{L"...",0,false}});
    helper(L"A\nB",std::max(a,dots),2,{{L"A",0,false},{L"...",0,false}});
    std::wstring tooLong(4096,L'A');const auto beforeLong=read(target);
    check(GetWidthOfString(tooLong.data())==0 && GetNumberOfLinesInHeight(tooLong.data())==0,"public bounded helpers reject 4096 text");
    DisplayHelpTokenizedString(tooLong.data(),3,4);
    check(read(target).bytes==beforeLong.bytes && same(state,sgp::CaptureNativeFontState()),"bounded display rejection is atomic");

    check(ColorFillVideoSurfaceArea(target,0,0,64,48,0xffff),"reset hover background");
    Region r;define(r);setText(r,L"A|B");hook(MOUSE_POS);RenderFastHelp();
    check(visible(r) && (r.uiFlags&MSYS_GOT_BACKGROUND),"real scaled region hover owns background");
    const auto shown=read(target);
    const INT32 w=std::min<INT32>(64,a+b+20),h=std::min<INT32>(48,fh+1+16);
    INT32 left=22;if (left+w>=64) left=std::max<INT32>(0,64-w-4);
    INT32 top=std::max<INT32>(0,20-h);if(top+h>=48)top=std::max<INT32>(0,48-h-10);
    std::vector<BYTE> box(static_cast<std::size_t>(w)*h*2);
    const auto put=[&](INT32 x,INT32 y,UINT16 c) {std::memcpy(box.data()+(static_cast<std::size_t>(y)*w+x)*2,&c,2);};
    for (INT32 y=0;y<h;++y)for(INT32 x=0;x<w;++x) {
        UINT16 c=0xffff;
        if ((x==1||x==w-1||y==1||y==h-1)&&x>=1&&y>=1)c=0x41c1;
        if ((x==0||x==w-2||y==0||y==h-2)&&x<=w-2&&y<=h-2)c=0xe62b;
        if(x>=2&&x<=w-3&&y>=2&&y<=h-3)c=0x39c7;
        put(x,y,c);
    }
    drawOracle(box,static_cast<UINT32>(w)*2,static_cast<UINT32>(w),static_cast<UINT32>(h),10,8,
               {{L"A",0,false},{L"B",a,true}});
    auto expected=base.bytes;
    for(INT32 y=0;y<h;++y)std::memcpy(expected.data()+static_cast<std::size_t>(y+top)*base.pitch+left*2,
        box.data()+static_cast<std::size_t>(y)*w*2,static_cast<std::size_t>(w)*2);
    check(shown.bytes==expected,"actual scale-two hover border/shade/core-glyph result and untouched allocation padding");
    check(same(state,sgp::CaptureNativeFontState()),"scaled hover preserves font state");
    RenderFastHelp();check(read(target).bytes==shown.bytes,"scaled cache redraw never compounds shading");
    hook(MOUSE_POS,40,40);check(!visible(r)&&read(target).bytes==base.bytes,"scaled hide restores exact owned background");
    hook(MOUSE_POS);RenderFastHelp();
    UINT32 pitch=0;check(LockVideoSurface(target,&pitch)!=nullptr,"caller lock before scaled hide");
    DisableMouseFastHelp();check(!visible(r)&&!sgp::RestoreNativeFastHelpBackground(),"locked scaled restoration is deferred honestly");
    UnLockVideoSurface(target);RenderFastHelp();check(read(target).bytes==base.bytes,"unlocked render restores deferred scaled background");
    EnableMouseFastHelp();RenderFastHelp();check(visible(r),"scaled hover returns after restoration");
    sgp::DeleteNativeScalableFont(heavy);RenderFastHelp();
    check(!visible(r)&&read(target).bytes==base.bytes,"deleted core handle restores background and refuses stale scaled rendering");
    check(same(state,sgp::CaptureNativeFontState()),"failed scaled render preserves caller state");
    sgp::ClearNativeScalableFastHelp();
    check(!isTooltipScalingEnabled()&&GetScaledFontHeight()==GetFontHeight(regular),"clearing scale restores original bitmap helper height");
    setText(r,L"A");RenderFastHelp();check(visible(r),"clear-scale resumes actual bitmap region rendering");
    DisableMouseFastHelp();check(read(target).bytes==base.bytes,"clear-scale bitmap hide restores background");
    check(SetFont(regular),"mutate caller after help");RestoreFontSettings();
    check(same(state,sgp::CaptureNativeFontState()),"all shared and scaled paths preserve caller legacy saved slot");
    check(GetScaledFontHeight()>0 && GetWidthOfString(const_cast<wchar_t*>(L"A"))>0,"configured public bitmap helper binding is live");
    UnloadFont(static_cast<UINT32>(regular));
    check(LoadFontFile(const_cast<char*>("font.sti"))==regular,"reuse actual public-helper bitmap slot");
    const auto stalePixels=read(target);
    check(GetScaledFontHeight()==0 && GetWidthOfString(const_cast<wchar_t*>(L"A"))==0 &&
          GetNumberOfLinesInHeight(const_cast<wchar_t*>(L"A"))==0,"public helpers reject stale generation after bitmap slot reuse");
    DisplayHelpTokenizedString(const_cast<wchar_t*>(L"A"),3,4);
    check(read(target).bytes==stalePixels.bytes,"stale shared helper display leaves complete surface unchanged");
    check(sgp::ConfigureNativeFastHelp(regular,bold,target) && GetScaledFontHeight()==GetFontHeight(regular) &&
          GetWidthOfString(const_cast<wchar_t*>(L"A"))>0,"explicit valid rebind admits the new bitmap generation");
    sgp::DeleteNativeScalableFont(normal);MSYS_Shutdown();
    const auto stoppedPixels=read(target);
    check(GetScaledFontHeight()==0 && GetWidthOfString(const_cast<wchar_t*>(L"A"))==0 &&
          GetNumberOfLinesInHeight(const_cast<wchar_t*>(L"A"))==0,"mouse shutdown invalidates shared helper bindings");
    DisplayHelpTokenizedString(const_cast<wchar_t*>(L"A"),3,4);
    check(read(target).bytes==stoppedPixels.bytes,"shutdown shared helper display is an atomic refusal");
    sgp::SetNativeFastHelpClockForTesting(nullptr);
}
void cleanup() {
    MSYS_Shutdown(); sgp::ClearNativeScalableFastHelp(); sgp::ShutdownNativeScalableFonts();
    ShutdownFontManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc,char** argv) {
    try {
        check(argc==4,"render fixture directory and public regular/bold TTF arguments"); const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string())); check(dir->init(),"authored fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("help",vfs::Path(root.u8string()),false); profile->addLocation(dir.get());
        auto* location=dir.release(); auto* mounted=profile.get(); getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"real VFS mount");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"real manager initialization");
        UINT16 values[]={65,66,32}; FontTranslationTable translation{3,values}; check(InitializeFontManager(16,&translation),"real bitmap-font manager");
        const auto normal=LoadFontFile(const_cast<char*>("font.sti")), bold=LoadFontFile(const_cast<char*>("font_alt.sti")); check(normal>=0&&bold>=0,"authored ETRLE fonts");
        VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT; desc.usWidth=64; desc.usHeight=48; desc.ubBitDepth=16;
        UINT32 target=0; check(AddVideoSurface(&desc,&target),"real padded RGB565 surface"); run(target,normal,bold);
        scaled(target,normal,bold,std::filesystem::canonical(argv[2]),std::filesystem::canonical(argv[3]));cleanup();
        std::cout<<"PASS: "<<checks<<" actual native fast-help assertions\n"; return 0;
    } catch (const std::exception& e) { std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; cleanup(); return 1; }
}
