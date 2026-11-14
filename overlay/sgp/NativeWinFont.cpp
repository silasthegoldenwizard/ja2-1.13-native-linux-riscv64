/*
 * P710 native snapshot update: sgp/NativeWinFont.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native implementation of the original WinFont boundary. Original Windows
// implementation stays in WinFont.cpp. Uses the approved project-local FreeType
// core and actual FileMan/VFS; no Windows handle/type emulation.
#include "native_winfont.h"
#include "NativeScalableFont.h"
#include "native_format.h"
#include "native_mousesystem.h"
#include "FileMan.h"
#include "Font Control.h"
#include "GameSettings.h"
#include "local.h"
#include <language.hpp>
#include <vfs/Tools/vfs_property_container.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <fstream>
#include <limits>
#include <map>
#include <vector>

INT32 WinFontMap[MAX_WINFONTMAP]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
INT32 TOOLTIP_IFONT=-1, TOOLTIP_IFONT_BOLD=-1;
namespace {
constexpr const char* defaultRegular="/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
constexpr const char* defaultBold="/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
constexpr std::size_t maxBytes=16u*1024u*1024u;
struct Entry {
    NativeWinFontSubstitutionInfo info;
    COLORVAL back=0;
    std::array<INT32,128> widths{};
};
std::map<INT32,Entry> fonts;
std::array<INT32,WIN_LASTFONT> mainFonts=[] { std::array<INT32,WIN_LASTFONT> a{}; a.fill(-1); return a; }();
bool mainReady=false, tooltipReady=false;
struct FontDefault { const char* key; INT32 height,weight; COLORVAL color; };
// Exact twenty entries/default heights/weights/colors from WinFont.cpp.
const FontDefault defaults[]={
    {"LargeFont1",-12,700,FROMRGB(98, 98, 98)},
    {"SmallFont1",-11,700,FROMRGB(98, 98, 98)},
    {"TinyFont1",-12,400,FROMRGB(98, 98, 98)},
    {"12PointFont1",-14,400,0},
    {"CompFont",-12,400,FROMRGB(0, 255, 0)},
    {"SmallCompFont",-10,400,FROMRGB(98, 98, 98)},
    {"10PointRoman",-12,400,FROMRGB(0, 255 , 0)},
    {"12PointRoman",-14,400,FROMRGB(0, 255 , 0)},
    {"14PointSansSerif",-16,400,FROMRGB(0, 255 , 0)},
    {"10PointArial",-11,400,0},
    {"14PointArial",-16,400,FROMRGB(0, 255 , 0)},
    {"12PointArial",-13,400,FROMRGB(222, 222, 222)},
    {"BlockyFont",-11,700,FROMRGB(217, 217, 217)},
    {"BlockyFont2",-12,400,FROMRGB(217, 217, 217)},
    {"10PointArialBold",-11,700,0},
    {"12PointArialFixedFont",-12,400,0},
    {"16PointArial",-20,400,FROMRGB(222, 222, 222)},
    {"BlockFontNarrow",-11,700,FROMRGB(128, 0 , 0)},
    {"14PointHumanist",-15,700,FROMRGB(255, 0, 0)},
    {"HugeFont",-19,700,FROMRGB(0, 255, 0)},
};
static_assert(sizeof(defaults)/sizeof(defaults[0])==WIN_LASTFONT,"font table");
INT16 metric(INT32 n) { return static_cast<INT16>(std::clamp<INT32>(n,0,std::numeric_limits<INT16>::max())); }
bool live(INT32 id) noexcept {
    sgp::NativeScalableFontMetrics m;
    return fonts.find(id)!=fonts.end() && sgp::GetNativeScalableFontMetrics(id,&m);
}
bool readBytes(const std::string& file,std::vector<BYTE>& bytes) {
    if (file.empty()) return false;
    if (file.front()=='/') {
        std::ifstream in(file,std::ios::binary|std::ios::ate);
        if (!in) return false;
        const auto n=in.tellg();
        if (n<=0 || n>static_cast<std::streamoff>(maxBytes)) return false;
        bytes.resize(static_cast<std::size_t>(n)); in.seekg(0);
        return bool(in.read(reinterpret_cast<char*>(bytes.data()),n));
    }
    const auto handle=FileOpen(const_cast<char*>(file.c_str()),FILE_ACCESS_READ|FILE_OPEN_EXISTING);
    if (!handle) return false;
    struct Close { HWFILE h; ~Close(){FileClose(h);} } close{handle};
    const UINT32 size=FileGetSize(handle);
    if (!size || size>maxBytes) return false;
    bytes.resize(size); UINT32 count=0;
    return FileRead(handle,bytes.data(),size,&count) && count==size;
}
void loadProperties(vfs::PropertyContainer& props) {
    // Like the original: absent INI uses defaults. A present file is read by
    // the original VFS PropertyContainer, including mounted profile selection.
    props.initFromIniFile(GAME_INI_FILE);
}
std::string configuredFile(vfs::PropertyContainer& props,INT32 weight) {
    return props.getStringProperty(L"Native Fonts",weight>=700?L"BoldFile":L"RegularFile",
                                  weight>=700?defaultBold:defaultRegular).utf8();
}
void clearMappings() {
    sgp::ClearNativeFontReplacements();
    std::fill(std::begin(WinFontMap),std::end(WinFontMap),-1);
}
bool mapFont(INT32 bitmap,INT32 ordinal) {
    if (bitmap<0 || bitmap>=MAX_WINFONTMAP || ordinal<0 || ordinal>=WIN_LASTFONT) return false;
    const auto id=mainFonts[ordinal];
    if (!sgp::SetNativeFontReplacement(bitmap,id,
            sgp::NativeFontReplacementPolicy{WinFontStringPixLength,GetWinFontHeight})) return false;
    WinFontMap[bitmap]=id; return true;
}
}
INT32 CreateWinFont(const NativeWinFontDescriptor& descriptor) {
    if (fonts.size()>=MAX_WINFONTMAP || descriptor.height==0 || descriptor.height==std::numeric_limits<INT32>::min()) return -1;
    const INT32 pixels=descriptor.height<0?-descriptor.height:descriptor.height;
    if (pixels>512) return -1;
    INT32 id=-1;
    try {
        const auto file=descriptor.fileName.empty()?(descriptor.weight>=700?defaultBold:defaultRegular):descriptor.fileName.c_str();
        std::vector<BYTE> bytes;
        if (!readBytes(file,bytes)) return -1;
        id=sgp::CreateNativeScalableFont(bytes.data(),bytes.size(),static_cast<UINT32>(pixels));
        if (id<0) return -1;
        Entry entry;
        entry.info={descriptor.faceName,file,descriptor.height,descriptor.weight,!descriptor.fileName.empty()};
        // Preserve the original Chinese approximate-width policy explicitly.
        // DejaVu may yield .notdef; this does not promise Chinese coverage.
        wchar_t c[2]={L'啊',0};
        if (!sgp::MeasureNativeScalableText(id,c,&entry.widths[0])) { sgp::DeleteNativeScalableFont(id); return -1; }
        for (INT32 i=1;i<128;++i) {
            c[0]=static_cast<wchar_t>(i);
            // Core refuses controls. They have no printable width here.
            if (i>=32 && i!=127 && !sgp::MeasureNativeScalableText(id,c,&entry.widths[i])) { sgp::DeleteNativeScalableFont(id); return -1; }
        }
        fonts.emplace(id,std::move(entry)); return id;
    } catch (...) { if (id>=0) sgp::DeleteNativeScalableFont(id); return -1; }
}
void DeleteWinFont(INT32 id) {
    const auto it=fonts.find(id); if (it==fonts.end()) return;
    if (id==TOOLTIP_IFONT || id==TOOLTIP_IFONT_BOLD) sgp::ClearNativeScalableFastHelp();
    sgp::DeleteNativeScalableFont(id); fonts.erase(it);
    for (INT32 i=0;i<MAX_WINFONTMAP;++i) if (WinFontMap[i]==id) {
        WinFontMap[i]=-1; sgp::SetNativeFontReplacement(i,-1);
    }
    for (auto& owned:mainFonts) if (owned==id) {owned=-1;mainReady=false;}
    if (TOOLTIP_IFONT==id) {TOOLTIP_IFONT=-1;tooltipReady=false;}
    if (TOOLTIP_IFONT_BOLD==id) {TOOLTIP_IFONT_BOLD=-1;tooltipReady=false;}
}
void ShutdownWinFonts() {
    mainReady=false; clearMappings();
    const auto owned=mainFonts; mainFonts.fill(-1);
    for (const auto id:owned) DeleteWinFont(id);
}
void ShutdownTooltipFonts() {
    tooltipReady=false; sgp::ClearNativeScalableFastHelp();
    const INT32 regular=TOOLTIP_IFONT,bold=TOOLTIP_IFONT_BOLD;
    TOOLTIP_IFONT=TOOLTIP_IFONT_BOLD=-1; DeleteWinFont(regular); DeleteWinFont(bold);
}
void InitWinFonts() {
    ShutdownWinFonts();
    try {
        vfs::PropertyContainer props; loadProperties(props);
        const auto adjust=props.getIntProperty(L"Ja2 Settings",L"WIN_FONT_ADJUST",0);
        for (INT32 i=0;i<WIN_LASTFONT;++i) {
            const auto h=props.getIntProperty(defaults[i].key,"Height",defaults[i].height);
            const auto weight=props.getIntProperty(defaults[i].key,"Weight",defaults[i].weight);
            // Check before addition: malformed INI cannot overflow native ints.
            if (h < -512 || h>512 || adjust < -512 || adjust>512 || h+adjust < -512 || h+adjust>512 || weight<std::numeric_limits<INT32>::min() || weight>std::numeric_limits<INT32>::max()) {ShutdownWinFonts();return;}
            NativeWinFontDescriptor d;
            d.faceName=props.getStringProperty(defaults[i].key,L"Name",L"ja2font3").utf8();
            d.height=static_cast<INT32>(h+adjust); d.weight=static_cast<INT32>(weight); d.fileName=configuredFile(props,d.weight);
            const auto id=CreateWinFont(d);
            if (id<0) {ShutdownWinFonts();return;}
            mainFonts[i]=id; COLORVAL color=defaults[i].color; SetWinFontForeColor(id,&color);
        }
        // Actual Font Control IDs, with the original mapping and aliases.
        const std::pair<INT32,INT32> mappings[]={
            {FONT12ARIAL,WIN_12POINTARIAL},
            {LARGEFONT1,WIN_LARGEFONT1},
            {SMALLFONT1,WIN_SMALLFONT1},
            {TINYFONT1,WIN_TINYFONT1},
            {FONT12POINT1,WIN_12POINTFONT1},
            {COMPFONT,WIN_COMPFONT},
            {SMALLCOMPFONT,WIN_SMALLCOMPFONT},
            {FONT10ROMAN,WIN_10POINTROMAN},
            {FONT12ROMAN,WIN_12POINTROMAN},
            {FONT14SANSERIF,WIN_14POINTSANSSERIF},
            {MILITARYFONT1,WIN_BLOCKYFONT},
            {FONT10ARIAL,WIN_10POINTARIAL},
            {FONT14ARIAL,WIN_14POINTARIAL},
            {FONT10ARIALBOLD,WIN_10POINTARIALBOLD},
            {BLOCKFONT,WIN_BLOCKYFONT},
            {BLOCKFONT2,WIN_BLOCKYFONT2},
            {FONT12ARIALFIXEDWIDTH,WIN_12POINTARIALFIXEDFONT},
            {FONT16ARIAL,WIN_16POINTARIAL},
            {BLOCKFONTNARROW,WIN_BLOCKFONTNARROW},
            {FONT14HUMANIST,WIN_14POINTHUMANIST},
        };
        for (const auto& mapping:mappings) if (!mapFont(mapping.first,mapping.second)) {ShutdownWinFonts();return;}
        mainReady=true;
    } catch (...) {ShutdownWinFonts();}
}
void InitTooltipFonts() {
    ShutdownTooltipFonts();
    // Original compound assignment truncates -11 * factor toward zero.
    const float scaled=-11.0f*fTooltipScaleFactor;
    if (!std::isfinite(scaled) || scaled>-1 || scaled < -512) return;
    try {
        vfs::PropertyContainer props; loadProperties(props);
        NativeWinFontDescriptor d; d.faceName="Arial"; d.height=static_cast<INT32>(scaled); d.weight=400;
        d.fileName=configuredFile(props,d.weight); TOOLTIP_IFONT=CreateWinFont(d);
        d.weight=700; d.fileName=configuredFile(props,d.weight); TOOLTIP_IFONT_BOLD=CreateWinFont(d);
        if (!live(TOOLTIP_IFONT) || !live(TOOLTIP_IFONT_BOLD)) {ShutdownTooltipFonts();return;}
        COLORVAL regular=FROMRGB(201,197,143),bold=FROMRGB(223,176,1);
        SetWinFontForeColor(TOOLTIP_IFONT,&regular); SetWinFontForeColor(TOOLTIP_IFONT_BOLD,&bold);
        if (!sgp::ConfigureNativeScalableFastHelp(TOOLTIP_IFONT,TOOLTIP_IFONT_BOLD,fTooltipScaleFactor)) {ShutdownTooltipFonts();return;}
        tooltipReady=true;
    } catch (...) {ShutdownTooltipFonts();}
}
bool NativeWinFontsReady() noexcept {
    if (!mainReady) return false;
    for (auto id:mainFonts) if (!live(id)) return false;
    return true;
}
bool NativeTooltipFontsReady() noexcept {return tooltipReady && live(TOOLTIP_IFONT) && live(TOOLTIP_IFONT_BOLD);}
INT32 GetNativeWinFontScalableHandle(INT32 id) noexcept {return live(id)?id:-1;}
void SetWinFontForeColor(INT32 id,COLORVAL* color) {if (color && live(id)) sgp::SetNativeScalableFontColor(id,*color);}
void SetWinFontBackColor(INT32 id,COLORVAL* color) {if (color && live(id)) fonts.find(id)->second.back=*color;}
bool GetNativeWinFontBackColor(INT32 id,COLORVAL* color) noexcept {
    if (!color || !live(id)) return false; *color=fonts.find(id)->second.back; return true;
}
bool GetNativeWinFontSubstitutionInfo(INT32 id,NativeWinFontSubstitutionInfo* output) {
    if (!output || !live(id)) return false; *output=fonts.find(id)->second.info; return true;
}
bool PrintNativeWinFontText(UINT32 dest,INT32 id,INT32 x,INT32 y,const CHAR16* text,const SGPRect* clip) {
    sgp::NativeScalableFontMetrics m; COLORVAL color=0;
    if (!live(id) || !text || !sgp::GetNativeScalableFontMetrics(id,&m) || !sgp::GetNativeScalableFontColor(id,&color)) return false;
    if (static_cast<std::int64_t>(y)-m.internalLeading>=0) y-=m.internalLeading;
    return sgp::DrawNativeScalableText(dest,id,x,y,text,color,clip);
}
void PrintWinFont(UINT32 dest,INT32 id,INT32 x,INT32 y,STR16 format,...) {
    if (!live(id) || !format) return;
    wchar_t text[512]; va_list args; va_start(args,format);
    const int count=ja2::native::VFormatWide(text,512,format,args); va_end(args);
    if (count>=0) PrintNativeWinFontText(dest,id,x,y,text);
}
INT16 WinFontStringPixLength(const CHAR16* text,INT32 id) {
    if (!text || !live(id)) return 0;
    if (g_lang==i18n::Lang::zh) {
        std::int64_t width=0; std::size_t count=0; const auto& widths=fonts.find(id)->second.widths;
        for (;*text;++text) {
            if (++count>4096 || *text<0 || static_cast<std::uint32_t>(*text)>0x10ffff || (*text>=0xd800 && *text<=0xdfff)) return 0;
            if (*text!=L'|') width+=widths[*text>=128?0:static_cast<std::size_t>(*text)];
        }
        return metric(static_cast<INT32>(std::min<std::int64_t>(width,std::numeric_limits<INT16>::max())));
    }
    INT32 width=0; return sgp::MeasureNativeScalableText(id,text,&width)?metric(width):0;
}
INT16 GetWinFontHeight(INT32 id) {
    sgp::NativeScalableFontMetrics m; if (!live(id) || !sgp::GetNativeScalableFontMetrics(id,&m)) return 0;
    if (g_lang==i18n::Lang::zh) {
        const auto mapped=[id](INT32 bitmap){return bitmap>=0 && bitmap<MAX_WINFONTMAP && WinFontMap[bitmap]==id;};
        if (mapped(TINYFONT1) || mapped(SMALLFONT1) || mapped(FONT14ARIAL)) m.ascent+=2;
    }
    return metric(m.ascent);
}
