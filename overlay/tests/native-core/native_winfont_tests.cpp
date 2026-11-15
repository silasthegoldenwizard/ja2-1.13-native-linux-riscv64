/*
 * P710 native snapshot update: tests/native-core/native_winfont_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "WinFont.h"
#include "WordWrap.h"
#include "NativeScalableFont.h"
#include "Font Control.h"
#include "native_mousesystem.h"
#include "FileManBackend.h"
#include <language.hpp>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <unistd.h>

// Actual original Font Control defines the font IDs/objects and initialization.
// These original process settings are fixture inputs, not replacement services.
int iUseWinFonts=0;
float fTooltipScaleFactor=1.5f;
UINT16 SCREEN_WIDTH=128,SCREEN_HEIGHT=64;
i18n::Lang g_lang=i18n::Lang::en;
extern bool isTooltipScalingEnabled();
extern INT16 GetWidthOfString(const STR16);
extern INT16 GetNumberOfLinesInHeight(const STR16);
extern void DisplayHelpTokenizedString(const STR16,INT16,INT16);

namespace {
unsigned checks=0;
std::filesystem::path scratch;
bool scratchOwned=false;
void check(bool result,const char* message) {++checks;if (!result) throw std::runtime_error(message);}
std::vector<BYTE> pixels(UINT32 id) {
    UINT16 w,h;UINT8 bpp; check(GetVideoSurfaceDescription(id,&w,&h,&bpp),"surface description");
    UINT32 pitch;auto* p=LockVideoSurface(id,&pitch);check(p!=nullptr,"actual surface lock");
    std::vector<BYTE> result(p,p+std::size_t(pitch)*h);UnLockVideoSurface(id);return result;
}
void clear(UINT32 id) {
    // A clipped fill succeeds as a no-op while the global clip is empty.
    // Independent alpha comparisons need an actual full-surface reset.
    blt_vs_fx fx{};fx.ColorFill=0x2945;
    check(BltVideoSurface(id,0,0,0,0,VS_BLT_COLORFILL,&fx),"unclipped full surface clear");
    const auto bytes=pixels(id);UINT16 first=0,last=0;
    std::memcpy(&first,bytes.data(),2);
    const auto rowPitch=bytes.size()/64;
    std::memcpy(&last,bytes.data()+63*rowPitch+127*2,2);
    check(first==0x2945 && last==0x2945,"fixture clears visible corners before alpha comparisons");
}
void cleanup() {
    ShutdownFonts();sgp::ShutdownNativeScalableFonts();ShutdownFontManager();
    ShutdownVideoSurfaceManager();ShutdownVideoObjectManager();
    fileman::CloseAll();vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();vfs::ObjectAllocator::clear();
    if (scratchOwned) { std::filesystem::remove_all(scratch);scratchOwned=false; }
}
void writeIni(bool bad=false) {
    std::ofstream out(scratch/"Ja2.ini",std::ios::binary|std::ios::trunc);
    out << "[Ja2 Settings]\nWIN_FONT_ADJUST=1\n[LargeFont1]\nName=Requested custom family\nHeight=-19\nWeight=700\n[Native Fonts]\nRegularFile="
        << (bad?"Fonts/missing.ttf":"Fonts/public-regular.ttf") << "\nBoldFile=Fonts/public-bold.ttf\n";
    check(bool(out),"authored INI written");
}
void prepare(const char* authoredRoot,const char* regular,const char* bold) {
    scratch=std::filesystem::temp_directory_path()/("p710-native-winfont-"+std::to_string(::getpid()));
    check(!std::filesystem::exists(scratch),"isolated scratch path unused");
    check(std::filesystem::create_directory(scratch),"isolated scratch directory");scratchOwned=true;
    check(std::filesystem::create_directory(scratch/"Fonts"),"isolated authored-font directory");
    const char* names[]={"LARGEFONT1","SMALLFONT1","TINYFONT1","FONT12POINT1","CLOCKFONT","COMPFONT","SMALLCOMPFONT", "FONT10ROMAN","FONT12ROMAN","FONT14SANSERIF","FONT10ARIAL","FONT14ARIAL","FONT10ARIALBOLD","FONT12ARIAL","BLOCKFONT","BLOCKFONT2","FONT12ARIALFIXEDWIDTH","FONT16ARIAL","BLOCKFONTNARROW","FONT14HUMANIST","HUGEFONT"};
    for (auto name:names) std::filesystem::copy_file(std::filesystem::path(authoredRoot)/"font.sti",scratch/"Fonts"/(std::string(name)+".sti"));
    // Actual public files; no proprietary game files or simulated font services.
    std::filesystem::copy_file(regular,scratch/"Fonts/public-regular.ttf");
    std::filesystem::copy_file(bold,scratch/"Fonts/public-bold.ttf");writeIni();
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(scratch.u8string()));
    check(directory->init(),"actual VFS directory");
    auto profile=std::make_unique<vfs::CVirtualProfile>("winfont",vfs::Path(scratch.u8string()),false);
    profile->addLocation(directory.get());auto* location=directory.release();auto* mounted=profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location,mounted),"actual VFS mount");
}
void suite() {
    check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"real render managers");
    UINT16 symbols[]={65,66,32};FontTranslationTable table{3,symbols};
    check(InitializeFontManager(16,&table),"real bitmap font manager");
    UINT32 ids[4];VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=128;d.usHeight=64;d.ubBitDepth=16;
    for (auto& id:ids) check(AddVideoSurface(&d,&id),"actual primary surface allocation");
    check(sgp::BindReservedVideoSurfaces(ids),"actual framebuffer aliases");
    // Even with bitmap mode, original InitializeFonts must initialize tooltips.
    check(InitializeFonts() && gfFontsInit,"original FontControl initialization");
    check(!NativeWinFontsReady() && NativeTooltipFontsReady(),"tooltip-only readiness in bitmap mode");
    check(sgp::ConfigureNativeFastHelp(FONT10ARIAL,FONT10ARIALBOLD,FRAME_BUFFER),"actual GameLoop post-FontControl bitmap helper binding");
    check(WinFontMap[LARGEFONT1]==-1 && IsFontLoaded(LARGEFONT1),"actual bitmap IDs retained without replacement");
    check(isTooltipScalingEnabled(),"actual scale predicate enabled above one");
    sgp::NativeScalableFontMetrics m;
    check(sgp::GetNativeScalableFontMetrics(TOOLTIP_IFONT,&m),"real tooltip metrics");
    NativeWinFontSubstitutionInfo info;
    check(GetNativeWinFontSubstitutionInfo(TOOLTIP_IFONT,&info) && info.requestedFace=="Arial" && info.requestedHeight==-16 && info.requestedWeight==400 && info.resolvedFile=="Fonts/public-regular.ttf","float scale truncation, requested family and actual VFS file reported");
    check(GetNativeWinFontSubstitutionInfo(TOOLTIP_IFONT_BOLD,&info) && info.requestedWeight==700 && info.resolvedFile=="Fonts/public-bold.ttf","actual bold VFS file selected");
    COLORVAL color=0;
    check(sgp::GetNativeScalableFontColor(TOOLTIP_IFONT,&color) && color==FROMRGB(201,197,143),"original regular tooltip color");
    check(sgp::GetNativeScalableFontColor(TOOLTIP_IFONT_BOLD,&color) && color==FROMRGB(223,176,1),"original bold tooltip color");
    const auto tooltip=TOOLTIP_IFONT;ShutdownFonts();
    check(!NativeTooltipFontsReady() && GetNativeWinFontScalableHandle(tooltip)==-1 && !IsFontLoaded(LARGEFONT1),"original shutdown removes real fonts and stale tooltip IDs");
    iUseWinFonts=1;check(InitializeFonts() && NativeWinFontsReady() && NativeTooltipFontsReady(),"original FontControl scalable initialization");
    check(sgp::ConfigureNativeFastHelp(FONT10ARIAL,FONT10ARIALBOLD,FRAME_BUFFER),"actual GameLoop helper rebind after original FontControl reinitialization");
    const auto large=WinFontMap[LARGEFONT1], tiny=WinFontMap[TINYFONT1];
    check(large>tooltip && GetNativeWinFontScalableHandle(large)==large,"mapped actual bitmap ID to unique core handle");
    check(GetNativeWinFontSubstitutionInfo(large,&info) && info.requestedFace=="Requested custom family" && info.requestedHeight==-18 && info.requestedWeight==700 && info.resolvedFile=="Fonts/public-bold.ttf","real INI Name Height Weight and WIN_FONT_ADJUST");
    check(sgp::GetNativeScalableFontMetrics(large,&m) && GetWinFontHeight(large)==m.ascent,"ascent-based approximate original GDI metric");
    INT32 width=0;check(sgp::MeasureNativeScalableText(large,L"AV 100%",&width) && WinFontStringPixLength(L"AV 100%",large)==width,"WinFont width through actual core");
    check(StringPixLength(L"AV 100%",LARGEFONT1)==width,"original metric caller honors actual mapped bitmap ID");
    const auto normalTiny=GetWinFontHeight(tiny);g_lang=i18n::Lang::zh;
    check(GetWinFontHeight(tiny)==normalTiny+2,"original Chinese tactical height adjustment");
    const auto ascii=WinFontStringPixLength(L"A",large),chinese=WinFontStringPixLength(L"啊",large);
    check(WinFontStringPixLength(L"A|啊",large)==ascii+chinese && WinFontStringPixLength(L"|",large)==0,"original Chinese pipe skip and approximate cached width policy");
    check(WinFontStringPixLength(L"\x80",large)==chinese,"bounded Chinese non-ASCII width at former 128 boundary");
    // The actual original Font API, WF wrappers and positioning must invoke
    // the live bridge policy, including language changes made after init.
    const INT32 tactical[]={TINYFONT1,SMALLFONT1,FONT14ARIAL};
    for (const auto bitmap:tactical) {
        const auto handle=WinFontMap[bitmap];
        check(GetFontHeight(bitmap)==GetWinFontHeight(handle) && WFGetFontHeight(bitmap)==GetWinFontHeight(handle),"actual mapped Font and WF heights honor dynamic Chinese tactical policy");
    }
    wchar_t policyText[]=L"A|啊B";
    check(StringPixLength(policyText,LARGEFONT1)==WinFontStringPixLength(policyText,large) &&
          WFStringPixLength(policyText,LARGEFONT1)==WinFontStringPixLength(policyText,large) &&
          StringPixLength(L"|",LARGEFONT1)==0,"actual mapped Font and WF width honor Chinese pipe skip and cached width");
    wchar_t prefixFormat[]=L"%s";
    check(StringPixLengthArg(LARGEFONT1,3,prefixFormat,policyText)==WinFontStringPixLength(L"A|啊",large),"actual formatted prefix uses Chinese bridge measurement after prefix truncation");
    INT16 positionedX=0,positionedY=0;
    FindFontCenterCoordinates(0,0,128,64,policyText,TINYFONT1,&positionedX,&positionedY);
    check(positionedX==(128-WinFontStringPixLength(policyText,tiny)+1)/2 && positionedY==(64-GetWinFontHeight(tiny))/2,"actual positioning uses Chinese bridge width and height");
    g_lang=i18n::Lang::en;
    for (const auto bitmap:tactical) {
        const auto handle=WinFontMap[bitmap];
        check(GetFontHeight(bitmap)==GetWinFontHeight(handle) && WFGetFontHeight(bitmap)==GetWinFontHeight(handle),"mapped Font/WF height changes back to English without reinitializing");
    }
    check(StringPixLength(policyText,LARGEFONT1)==WinFontStringPixLength(policyText,large) &&
          StringPixLength(L"|",LARGEFONT1)>0,"mapped width changes back to English without reinitializing");
    check(StringPixLengthArg(LARGEFONT1,1,prefixFormat,L"AB")==WinFontStringPixLength(L"A",large),"actual formatted prefix measures displayed scalable A rather than bitmap advance");
    COLORVAL back=FROMRGB(13,27,49);SetWinFontBackColor(large,&back);COLORVAL read=0;
    check(GetNativeWinFontBackColor(large,&read) && read==back,"transparent background state retained");
    COLORVAL foreground=FROMRGB(239,111,53);SetWinFontForeColor(large,&foreground);
    clear(ids[0]);clear(ids[1]);clear(ids[2]);
    wchar_t format[]=L"%s %d%%";PrintWinFont(ids[0],large,2,7,format,L"HP",100);
    check(PrintNativeWinFontText(ids[1],large,2,7,L"HP 100%"),"already formatted percent text literal");
    check(pixels(ids[0])==pixels(ids[1]),"bounded formatter and literal path match actual full surface bytes");
    check(sgp::GetNativeScalableFontMetrics(large,&m),"bridge oracle metrics");
    const INT32 adjustedY=7-m.internalLeading>=0?7-m.internalLeading:7;
    check(sgp::DrawNativeScalableText(ids[2],large,2,adjustedY,L"HP 100%",foreground) && pixels(ids[0])==pixels(ids[2]),"bridge foreground, transparent pixels and original conditional leading correction match direct core");
    // Real mapped public surface and locked-buffer entry points must match
    // the direct bridge, including both sides of the leading threshold.
    check(m.internalLeading>0,"real public DejaVu fixture exercises nonzero internal leading");
    check(SetFont(LARGEFONT1),"select actual mapped FontControl bitmap ID");
    const SGPRect publicClip{1,1,127,63};
    const INT32 ys[]={0,m.internalLeading-1,m.internalLeading,m.internalLeading+7};
    for (const auto y:ys) {
        clear(ids[0]);clear(ids[1]);clear(ids[2]);clear(ids[3]);
        check(SetFontDestBuffer(ids[0],publicClip.iLeft,publicClip.iTop,publicClip.iRight,publicClip.iBottom,FALSE),"mapped mprintf destination");
        check(mprintf(2,y,L"%s %d%%",L"HP",100)==0,"actual mapped mprintf formatter");
        check(PrintNativeWinFontText(ids[1],large,2,y,L"HP 100%",&publicClip) && pixels(ids[0])==pixels(ids[1]),"mapped mprintf pixels equal direct WinFont leading/color/clip contract");
        check(SetFontDestBuffer(ids[2],publicClip.iLeft,publicClip.iTop,publicClip.iRight,publicClip.iBottom,FALSE),"mapped gprintf destination");
        check(gprintf(2,y,format,L"HP",100)==0 && pixels(ids[2])==pixels(ids[1]),"actual mapped gprintf equals direct bridge and formats literal percent once");
        check(SetFontDestBuffer(ids[3],publicClip.iLeft,publicClip.iTop,publicClip.iRight,publicClip.iBottom,FALSE),"mapped raw destination state");
        UINT32 pitch=0;auto* raw=LockVideoSurface(ids[3],&pitch);check(raw!=nullptr,"actual bounded raw surface lock");
        const auto result=mprintf_buffer(raw,pitch,FONT10ARIAL,2,y,format,L"HP",100);
        UnLockVideoSurface(ids[3]);
        check(result==0 && pixels(ids[3])==pixels(ids[1]),"actual mapped locked-buffer output uses selected font and matches direct WinFont leading/clip contract");
    }
    wchar_t reduced[]=L"AAAAAAAAAAAA";
    const UINT32 reducedWidth=2*static_cast<UINT32>(StringPixLength(L"A",LARGEFONT1))+static_cast<UINT32>(StringPixLength(L"...",LARGEFONT1));
    check(ReduceStringLength(reduced,reducedWidth,LARGEFONT1) && std::wstring(reduced)==L"AA...","real original WordWrap reduction keeps admitted prefix and ellipsis");
    wchar_t fitting[]=L"AA";
    check(ReduceStringLength(fitting,static_cast<UINT32>(StringPixLength(fitting,LARGEFONT1)),LARGEFONT1) && std::wstring(fitting)==L"AA","fitting original text stays unchanged");
    struct Short { wchar_t text[3];wchar_t guard; } shortText{{L'A',L'B',0},0x4e34};
    const auto shortBefore=shortText;
    check(!ReduceStringLength(shortText.text,0,LARGEFONT1) && std::memcmp(&shortText,&shortBefore,sizeof(shortText))==0,"reduction refuses impossible ellipsis without growing a short writable span");
    std::wstring overlong(1024,L'A');const auto longBefore=overlong;
    check(!ReduceStringLength(overlong.data(),0,LARGEFONT1) && overlong==longBefore,"reduction input bound refuses before any writes");
    const auto rendered=pixels(ids[0]);
    wchar_t bad[]=L"%0600d";PrintWinFont(ids[0],large,0,0,bad,1);
    check(pixels(ids[0])==rendered,"oversize format causes no pixel changes");
    wchar_t malformed[]={0xd800,0};check(!PrintNativeWinFontText(ids[0],large,0,0,malformed) && pixels(ids[0])==rendered,"invalid Unicode causes no pixel changes");
    check(!PrintNativeWinFontText(ids[0],-17,0,0,L"A") && WinFontStringPixLength(L"A",-17)==0 && GetWinFontHeight(-17)==0,"invalid ID refusal");
    // Shared real tooltip text/token services are bound by InitTooltipFonts.
    wchar_t text[]=L"A|B";check(GetWidthOfString(text)>0 && GetNumberOfLinesInHeight(text)==1,"real tokenized tooltip width and line services");
    check(SetFontDestBuffer(FRAME_BUFFER,0,0,128,64,FALSE),"caller selects actual frame destination before shared tooltip drawing");
    clear(FRAME_BUFFER);const auto beforeTooltip=pixels(FRAME_BUFFER);
    DisplayHelpTokenizedString(text,2,2);
    check(pixels(FRAME_BUFFER)!=beforeTooltip,"shared tooltip draw reaches actual frame surface");
    const auto oldTooltip=TOOLTIP_IFONT;ShutdownTooltipFonts();
    check(!NativeTooltipFontsReady() && NativeWinFontsReady() && GetNativeWinFontScalableHandle(oldTooltip)==-1 && GetWinFontHeight(large)>0,"tooltip shutdown preserves main fonts and refuses stale tooltip");
    InitTooltipFonts();check(NativeTooltipFontsReady() && TOOLTIP_IFONT>oldTooltip,"tooltip reinit assigns a fresh core identity");
    NativeWinFontDescriptor missing;missing.fileName="Fonts/does-not-exist.ttf";
    check(CreateWinFont(missing)==-1,"explicit missing VFS font fails truthfully");
    missing.fileName="/definitely-missing-p710-font.ttf";
    check(CreateWinFont(missing)==-1,"explicit missing physical font fails truthfully");
    ShutdownFonts();
    NativeWinFontDescriptor physical;physical.faceName="ja2font3";
    const auto physicalID=CreateWinFont(physical);
    check(physicalID>0 && GetNativeWinFontSubstitutionInfo(physicalID,&info) && info.resolvedFile=="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" && !info.explicitFile,"physical deterministic default substitution is explicit in report query");
    DeleteWinFont(physicalID);DeleteWinFont(physicalID);
    check(GetNativeWinFontScalableHandle(physicalID)==-1 && !PrintNativeWinFontText(ids[0],physicalID,0,0,L"A"),"deleted standalone font ID remains stale");
    fTooltipScaleFactor=0;
    check(!InitializeFonts() && !gfFontsInit && !NativeTooltipFontsReady() && !NativeWinFontsReady() && !IsFontLoaded(LARGEFONT1),"invalid scale fails original FontControl with partial-init cleanup");
    fTooltipScaleFactor=1.5f;writeIni(true);
    check(!InitializeFonts() && !gfFontsInit && !NativeTooltipFontsReady() && !NativeWinFontsReady() && !IsFontLoaded(LARGEFONT1),"actual missing configured file fails original FontControl with complete cleanup");
}
}
int main(int argc,char** argv) {
    try {check(argc==4,"authored-STI root and two existing public TTF arguments");prepare(argv[1],argv[2],argv[3]);suite();cleanup();std::cout<<"PASS: "<<checks<<" actual FontControl/WinFont bridge assertions\n";return 0;}
    catch(const std::exception& e) {std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';cleanup();return 1;}
}
