/*
 * P710 native snapshot update: sgp/NativeTooltipText.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation of the actual mousesystem.cpp tooltip token/run policy.
// FreeType and the existing bitmap renderer own glyph measurement/rasterization.
#include "NativeTooltipText.h"
#include "NativeScalableFont.h"
#include "native_video.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>
namespace {
struct Binding {
    INT32 regular=-1,bold=-1,scalableRegular=-1,scalableBold=-1;
    std::uint64_t regularGeneration=0,boldGeneration=0;
    UINT32 destination=0,backing=0;double scale=1;
} binding;
struct FontScope {
    sgp::NativeFontState state=sgp::CaptureNativeFontState();
    ~FontScope() { sgp::RestoreNativeFontState(state); }
};
std::uint64_t fontGeneration(INT32 font) {
    FontScope scope;if (!SetFont(font)) return 0;
    return sgp::CaptureNativeFontState().generation;
}
UINT32 backingId(UINT32 id) {
    const auto backing=sgp::ReservedVideoSurfaceBackingId(id);return backing?backing:id;
}
bool validBinding() {
    UINT16 w=0,h=0;UINT8 depth=0;
    return binding.backing && backingId(binding.destination)==binding.backing &&
        fontGeneration(binding.regular)==binding.regularGeneration && binding.regularGeneration &&
        fontGeneration(binding.bold)==binding.boldGeneration && binding.boldGeneration &&
        GetVideoSurfaceDescription(binding.destination,&w,&h,&depth) && depth==16;
}
INT16 narrow(INT32 value) { return static_cast<INT16>(std::clamp<INT32>(value,0,32767)); }
UINT32 screenHeight() {
    UINT16 w=0,h=0;UINT8 b=0;
    if (!GetVideoSurfaceDescription(binding.destination,&w,&h,&b)) GetCurrentVideoSettings(&w,&h,&b);
    return h;
}
INT32 height() {
    if (!validBinding()) return 0;
    if (binding.scale>1) {
        sgp::NativeScalableFontMetrics m;
        return sgp::GetNativeScalableFontMetrics(binding.scalableRegular,&m)?m.ascent:0;
    }
    return GetFontHeight(binding.regular);
}
bool measure(const std::wstring& text,bool bold,INT32& width) {
    if (binding.scale>1)
        return sgp::MeasureNativeScalableText(bold?binding.scalableBold:binding.scalableRegular,text.c_str(),&width);
    const auto font=bold?binding.bold:binding.regular;
    if (!IsFontLoaded(font)) return false;
    width=StringPixLength(text.c_str(),font);return true;
}
}
namespace sgp {
bool ConfigureNativeTooltipText(INT32 normal,INT32 bold,UINT32 destination) {
    UINT16 w=0,h=0;UINT8 depth=0;
    if (!IsFontLoaded(normal)||!IsFontLoaded(bold)||!GetVideoSurfaceDescription(destination,&w,&h,&depth)||depth!=16) return false;
    binding.regular=normal;binding.bold=bold;binding.destination=destination;
    binding.regularGeneration=fontGeneration(normal);binding.boldGeneration=fontGeneration(bold);
    binding.backing=backingId(destination);return true;
}
void ClearNativeTooltipText() noexcept { binding={}; }
bool ConfigureNativeScalableTooltipText(INT32 normal,INT32 bold,double scale) {
    NativeScalableFontMetrics a,b;
    if (!std::isfinite(scale)||scale<=0 || scale>512.0/11 ||
        !GetNativeScalableFontMetrics(normal,&a)||!GetNativeScalableFontMetrics(bold,&b)) return false;
    binding.scalableRegular=normal;binding.scalableBold=bold;binding.scale=scale;return true;
}
void ClearNativeScalableTooltipText() noexcept { binding.scalableRegular=binding.scalableBold=-1;binding.scale=1; }
bool NativeTooltipScalingEnabled() noexcept { return binding.scale>1; }
double NativeTooltipScale() noexcept { return binding.scale; }
bool LayoutNativeTooltipText(const wchar_t* input,UINT32 screen,NativeTooltipLayout& output) {
    if (!input || screen<=10) return false;
    const INT32 fh=height();if (fh<=0) return false;
    std::size_t length=0;while (length<4096 && input[length]) ++length;
    if (length==4096) return false;
    try {
        NativeTooltipLayout result;result.lineHeight=fh;
        std::wstring run;INT32 x=0,line=0;bool bold=false;
        const auto flush=[&]() {
            if (run.empty()) return true;
            INT32 width=0;if (!measure(run,bold,width)||width<0||x>std::numeric_limits<INT32>::max()-width) return false;
            result.runs.push_back({run,x,line*(fh+1),bold});x+=width;
            result.width=std::max(result.width,x);run.clear();bold=false;return true;
        };
        for (std::size_t i=0;i<length;++i) {
            if (input[i]==L'\n') {
                if (!flush()) return false;
                // Original overflow draws the ellipsis at the beginning of the
                // current line. It does not invent another line or wrap words.
                if (std::int64_t(line+2)*(fh+1)>std::int64_t(screen)-10) {
                    INT32 width=0;if (!measure(L"...",false,width)) return false;
                    result.runs.push_back({L"...",0,line*(fh+1),false});result.width=std::max(result.width,width);break;
                }
                x=0;++line;if (i+1<length) result.lines=line+1;
            } else if (input[i]==L'|') bold=true;
            else {
                run.push_back(input[i]);
                if (i+1==length || input[i+1]==L'\n' || (bold&&input[i+1]!=L'|') || (!bold&&input[i+1]==L'|'))
                    if (!flush()) return false;
            }
        }
        if (!flush()) return false;
        output=std::move(result);return true;
    } catch (...) { return false; }
}
bool DrawNativeTooltipLayout(const NativeTooltipLayout& layout,BYTE* bytes,std::size_t size,
    UINT32 width,UINT32 height,UINT32 pitch,INT32 x,INT32 y) {
    if (!validBinding() || !bytes || !width || !height || width>INT32_MAX || height>INT32_MAX || std::uint64_t(width)*2>pitch || std::uint64_t(pitch)*height>size) return false;
    FontScope scope;
    auto state=scope.state;state.region={0,0,static_cast<INT32>(width),static_cast<INT32>(height)};
    state.bpp=16;state.pitch=pitch;state.wrap=FALSE;RestoreNativeFontState(state);
    for (const auto& run:layout.runs) {
        const auto dx=std::int64_t(x)+run.x,dy=std::int64_t(y)+run.y;
        if (dx<INT32_MIN||dx>INT32_MAX||dy<INT32_MIN||dy>INT32_MAX) return false;
        INT32 core=-1;
        if (binding.scale>1) core=run.bold?binding.scalableBold:binding.scalableRegular;
        else {
            const auto bitmap=run.bold?binding.bold:binding.regular;
            if (!SetFont(bitmap)) return false;
            SetFontBackground(0);SetFontShadow(141);SetFontForeground(run.bold?146:130);
            core=GetNativeFontReplacement(bitmap);
            if (core<0) {
                if (mprintf_buffer(bytes,pitch,static_cast<UINT32>(bitmap),static_cast<INT32>(dx),static_cast<INT32>(dy),L"%ls",run.text.c_str())==UINT32_MAX) return false;
                continue;
            }
        }
        COLORVAL color=0;NativeScalableFontMetrics m;
        if (!GetNativeScalableFontColor(core,&color)||!GetNativeScalableFontMetrics(core,&m)) return false;
        // Preserve PrintWinFont's original conditional internal-leading shift.
        const auto top=dy>=m.internalLeading?dy-m.internalLeading:dy;
        if (!DrawNativeScalableTextBuffer(bytes,size,width,height,pitch,core,static_cast<INT32>(dx),static_cast<INT32>(top),run.text.c_str(),color)) return false;
    }
    return true;
}
}
bool isTooltipScalingEnabled() { return sgp::NativeTooltipScalingEnabled(); }
UINT16 GetScaledFontHeight() { return static_cast<UINT16>(std::clamp<INT32>(height(),0,65535)); }
INT16 GetNumberOfLinesInHeight(const STR16 text) {
    if (!text) return 0;
    const INT32 fh=height();if (!fh) return 0;
    std::size_t n=0;while (n<4096&&text[n]) ++n;if (n==4096) return 0;
    INT32 lines=1;const auto screen=screenHeight();
    for (std::size_t i=0;i<n && std::int64_t(lines)*(fh+1)<std::int64_t(screen)-10;++i)
        if (text[i]==L'\n'&&i+1<n) ++lines;
    return narrow(lines);
}
INT16 GetWidthOfString(const STR16 text) {
    sgp::NativeTooltipLayout layout;return sgp::LayoutNativeTooltipText(text,screenHeight(),layout)?narrow(layout.width):0;
}
void DisplayHelpTokenizedString(const STR16 text,INT16 x,INT16 y) {
    UINT32 pitch=0;auto* pixels=LockVideoSurface(FontDestBuffer,&pitch);if (!pixels) return;
    sgp::NativeSurfaceView view;const bool valid=sgp::LockedNativeSurfaceView(pixels,&view);UnLockVideoSurface(FontDestBuffer);
    if (!valid) return;
    try {
        sgp::NativeTooltipLayout layout;if (!sgp::LayoutNativeTooltipText(text,view.height,layout)) return;
        pixels=LockVideoSurface(FontDestBuffer,&pitch);if (!pixels) return;
        std::vector<BYTE> snapshot;
        try { snapshot.assign(pixels,pixels+view.bytes); } catch (...) { UnLockVideoSurface(FontDestBuffer);return; }
        UnLockVideoSurface(FontDestBuffer);
        if (!sgp::DrawNativeTooltipLayout(layout,snapshot.data(),snapshot.size(),view.width,view.height,view.pitch,x,y)) return;
        pixels=LockVideoSurface(FontDestBuffer,&pitch);if (!pixels) return;
        sgp::NativeSurfaceView current;
        if (sgp::LockedNativeSurfaceView(pixels,&current)&&current.data==view.data&&current.bytes==view.bytes&&current.pitch==view.pitch)
            std::memcpy(pixels,snapshot.data(),snapshot.size());
        UnLockVideoSurface(FontDestBuffer);
    } catch (...) { return; }
}
