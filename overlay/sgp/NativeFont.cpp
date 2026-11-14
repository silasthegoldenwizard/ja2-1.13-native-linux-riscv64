/*
 * P710 native notice preparation: sgp/NativeFont.cpp
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: implement native bitmap-font API subset.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/NativeFont.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_font.h"
#include "NativeScalableFont.h"
#include "native_format.h"
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <limits>
#include <vector>

INT32 FontDefault=-1;
UINT32 FontDestBuffer=0, FontDestPitch=0, FontDestBPP=16;
SGPRect FontDestRegion{0,0,0,0};
BOOLEAN FontDestWrap=FALSE;
namespace
{
using I=std::int64_t;
constexpr UINT32 failure=std::numeric_limits<UINT32>::max();
std::array<HVOBJECT,MAX_FONTS> fonts{};
std::array<std::uint64_t,MAX_FONTS> generations{};
std::uint64_t serial=0;
std::array<INT32,MAX_FONTS> replacements=[] {
    std::array<INT32,MAX_FONTS> r{}; r.fill(-1); return r;
}();
std::array<sgp::NativeFontReplacementPolicy,MAX_FONTS> replacementPolicies{};
INT32 replacement(INT32 font)
{
    if (!IsFontLoaded(font)) return -1;
    const INT32 handle=replacements[static_cast<std::size_t>(font)];
    sgp::NativeScalableFontMetrics metrics;
    return handle>0 && sgp::GetNativeScalableFontMetrics(handle,&metrics) ? handle : -1;
}
COLORVAL selectedColor(INT32 handle)
{
    COLORVAL color=0;
    sgp::GetNativeScalableFontColor(handle,&color);
    return color;
}
void setSelectedColor(COLORVAL color)
{
    const auto handle=replacement(FontDefault);
    if (handle>0) sgp::SetNativeScalableFontColor(handle,color);
}
// Scalable raw output accepts only an actual live RGB565 surface lock. A
// pointer/pitch pair cannot establish allocation bounds for an arbitrary buffer.
UINT32 scalableLiteral(BYTE* buffer, UINT32 pitch, INT32 handle, INT32 x,
                       INT32 y, const wchar_t* text, bool surface)
{
    if (FontDestBPP!=16) return failure;
    const COLORVAL color=selectedColor(handle);
    sgp::NativeScalableFontMetrics metrics;
    if (!sgp::GetNativeScalableFontMetrics(handle,&metrics)) return failure;
    // Original PrintWinFont preserves the requested top for small y values;
    // otherwise subtracts internal leading before handing text to GDI.
    if (y>=metrics.internalLeading) y-=metrics.internalLeading;
    if (surface)
        return sgp::DrawNativeScalableText(FontDestBuffer,handle,x,y,text,color,&FontDestRegion) ? 0 : failure;
    sgp::NativeSurfaceView view;
    if (!sgp::LockedNativeSurfaceView(buffer,&view) || pitch!=view.pitch) return failure;
    return sgp::DrawNativeScalableTextBuffer(view.data,view.bytes,view.width,view.height,
        view.pitch,handle,x,y,text,color,&FontDestRegion) ? 0 : failure;
}
std::vector<UINT16> translation;
bool initialized=false;
UINT16 foreground=0xffff, background=0, shadow=0;
struct Settings
{
    INT32 font=-1;
    std::uint64_t generation=0;
    UINT32 dest=0, pitch=0, bpp=16;
    SGPRect region{0,0,0,0};
    BOOLEAN wrap=FALSE;
    UINT16 fg=0, bg=0, sh=0;
    INT32 scalable=-1; COLORVAL scalableColor=0;
    bool valid=false;
} saved;
UINT16 rgb(UINT32 r, UINT32 g, UINT32 b)
{ return static_cast<UINT16>(((r&248u)<<8)|((g&252u)<<3)|((b&248u)>>3)); }
UINT16 indexed(UINT8 index)
{
    auto* f=GetFontObject(FontDefault);
    if (!f || !f->pPaletteEntry) return 0;
    auto c=f->pPaletteEntry[index]; return rgb(c.peRed,c.peGreen,c.peBlue);
}
// Parse control bytes structurally, including zero-valued literal pixels.
// Validation is performed for the entire formatted string before any writes.
bool encoding(HVOBJECT f, UINT16 index)
{
    if (!f || f->ubBitDepth!=8 || !f->pETRLEObject || !f->pPixData || !f->p16BPPPalette || index>=f->usNumberOfObjects) return false;
    const auto& g=f->pETRLEObject[index];
    if (!g.usWidth || !g.usHeight || g.uiDataOffset>f->uiSizePixData || g.uiDataLength>f->uiSizePixData-g.uiDataOffset) return false;
    auto* data=static_cast<const BYTE*>(f->pPixData)+g.uiDataOffset;
    std::size_t pos=0;
    for (UINT32 y=0; y<g.usHeight; ++y)
    {
        UINT32 x=0;
        for (;;)
        {
            if (pos>=g.uiDataLength) return false;
            const UINT8 control=data[pos++];
            if (!control) { if (x!=g.usWidth) return false; break; }
            const UINT32 n=control&127u;
            if (n>g.usWidth-x) return false;
            x+=n;
            if (!(control&128u)) { if (n>g.uiDataLength-pos) return false; pos+=n; }
        }
    }
    return true;
}
void glyph(BYTE* buffer, UINT32 pitch, HVOBJECT f, UINT16 index, I x, I y, const SGPRect& clip, bool mono)
{
    const auto& g=f->pETRLEObject[index];
    x+=g.sOffsetX; y+=g.sOffsetY;
    auto* data=static_cast<const BYTE*>(f->pPixData)+g.uiDataOffset;
    std::size_t pos=0;
    for (UINT32 row=0; row<g.usHeight; ++row)
    {
        I column=0;
        for (;;)
        {
            const UINT8 control=data[pos++];
            if (!control) break;
            const UINT32 n=control&127u;
            const bool transparent=(control&128u)!=0;
            for (UINT32 k=0; k<n; ++k)
            {
                UINT16 color=background;
                bool write=mono && background!=0;
                if (!transparent)
                {
                    const UINT8 p=data[pos+k];
                    if (!mono) { color=f->p16BPPPalette[p]; write=true; }
                    else if (p==1) { color=shadow; write=shadow!=0; }
                    else if (p!=0) { color=foreground; write=true; }
                }
                const I px=x+column+k, py=y+row;
                if (write && px>=clip.iLeft && px<clip.iRight && py>=clip.iTop && py<clip.iBottom)
                    std::memcpy(buffer+static_cast<std::size_t>(py)*pitch+static_cast<std::size_t>(px)*2,&color,2);
            }
            if (!transparent) pos+=n;
            column+=n;
        }
    }
}
bool destination(UINT32 pitch, const SGPRect& c)
{
    if (!pitch || c.iLeft<0 || c.iTop<0 || c.iRight<c.iLeft || c.iBottom<c.iTop || static_cast<std::uint64_t>(c.iRight)*2>pitch) return false;
    const auto end=c.iBottom ? static_cast<std::uint64_t>(c.iBottom-1)*pitch+static_cast<std::uint64_t>(c.iRight)*2 : 0;
    return end<=static_cast<std::uint64_t>(std::numeric_limits<std::ptrdiff_t>::max());
}
UINT32 literal(BYTE* buffer, UINT32 pitch, INT32 font, INT32 x, INT32 y, const wchar_t* text, bool mono, bool surface, const UINT16* colors=nullptr, bool allowScalable=true)
{
    if (!text || !IsFontLoaded(font) || FontDestBPP!=16) return failure;
    const auto handle=allowScalable ? replacement(font) : -1;
    if (handle>0) return scalableLiteral(buffer,pitch,handle,x,y,text,surface);
    const std::size_t count=std::wcslen(text);
    auto* f=GetFontObject(font);
    for (std::size_t i=0; i<count; ++i) if (!encoding(f,static_cast<UINT16>(GetIndex(text[i])))) return failure;
    SGPRect clip=FontDestRegion;
    if (surface)
    {
        UINT16 w,h; UINT8 bpp;
        if (!GetVideoSurfaceDescription(FontDestBuffer,&w,&h,&bpp) || bpp!=16) return failure;
        clip.iLeft=std::clamp(clip.iLeft,0,static_cast<INT32>(w));
        clip.iTop=std::clamp(clip.iTop,0,static_cast<INT32>(h));
        clip.iRight=std::clamp(clip.iRight,clip.iLeft,static_cast<INT32>(w));
        clip.iBottom=std::clamp(clip.iBottom,clip.iTop,static_cast<INT32>(h));
        buffer=LockVideoSurface(FontDestBuffer,&pitch);
    }
    if (!buffer || !destination(pitch,clip))
    { if (surface && buffer) UnLockVideoSurface(FontDestBuffer); return failure; }
    I dx=x, dy=y;
    for (std::size_t i=0; i<count; ++i)
    {
        const auto index=static_cast<UINT16>(GetIndex(text[i]));
        const auto& g=f->pETRLEObject[index];
        const I gx=dx+g.sOffsetX, gy=dy+g.sOffsetY;
        if (FontDestWrap && (gx<clip.iLeft || gy<clip.iTop || gx+g.usWidth>clip.iRight || gy+g.usHeight>clip.iBottom))
        { dx=x; dy+=static_cast<I>(g.usHeight)+g.sOffsetY; }
        if (colors) foreground=colors[i];
        glyph(buffer,pitch,f,index,dx,dy,clip,mono);
        dx+=static_cast<I>(g.usWidth)+g.sOffsetX;
    }
    if (surface) UnLockVideoSurface(FontDestBuffer);
    return 0;
}
INT16 metric(const CHAR16* text, std::size_t count, HVOBJECT font)
{
    UINT32 width=0;
    for (std::size_t i=0; i<count; ++i)
    {
        width+=GetWidth(font,GetIndex(text[i]));
        if (width>=32767) return 32767;
    }
    return static_cast<INT16>(width);
}
void coordinates(INT16 left, INT16 top, INT16 width, INT16 height,
                 const CHAR16* text, INT32 font, INT16* x, INT16* y, bool center)
{
    if (!text || !IsFontLoaded(font) || !x || !y) return;
    // Original Font.cpp:892-914: the +1 belongs only to horizontal centering.
    // Signed division deliberately truncates toward zero, including overflow
    // text/negative rectangles. Widen before arithmetic, clamp at the ABI edge.
    const I spare=static_cast<I>(width)-StringPixLength(text,font);
    const I newX=static_cast<I>(left)+(center ? (spare+1)/2 : spare);
    const I newY=static_cast<I>(top)+(static_cast<I>(height)-GetFontHeight(font))/2;
    const auto narrow=[](I n) { return static_cast<INT16>(std::clamp<I>(n,-32768,32767)); };
    const INT16 resultX=narrow(newX), resultY=narrow(newY);
    *x=resultX; *y=resultY;
}
void formattedCoordinates(INT16 left, INT16 top, INT16 width, INT16 height,
                          INT32 font, INT16* x, INT16* y,
                          const CHAR16* format, va_list args, bool center)
{
    if (!format || !IsFontLoaded(font) || !x || !y) return;
    CHAR16 text[512];
    const int count=ja2::native::VFormatWide(text,512,format,args);
    if (count<0 || count>=512) return;
    coordinates(left,top,width,height,text,font,x,y,center);
}
UINT32 print(BYTE* buffer, UINT32 pitch, INT32 font, INT32 x, INT32 y, const wchar_t* format, va_list args, bool mono, bool surface, bool allowScalable=true)
{
    if (!format) return failure;
    wchar_t text[512];
    const int count=ja2::native::VFormatWide(text,512,format,args);
    if (count<0 || count>=512) return failure;
    return literal(buffer,pitch,font,x,y,text,mono,surface,nullptr,allowScalable);
}

}
BOOLEAN IsFontLoaded(INT32 f) { return initialized && f>=0 && f<MAX_FONTS && fonts[static_cast<std::size_t>(f)] ? TRUE : FALSE; }
HVOBJECT GetFontObject(INT32 f) { return IsFontLoaded(f) ? fonts[static_cast<std::size_t>(f)] : nullptr; }
BOOLEAN SetFont(INT32 f) { if (!IsFontLoaded(f)) return FALSE; FontDefault=f; return TRUE; }
void UnloadFont(UINT32 f)
{
    if (f>=MAX_FONTS || !fonts[f]) return;
    replacements[f]=-1; replacementPolicies[f]={};
    DeleteVideoObject(fonts[f]); fonts[f]=nullptr;
    if (FontDefault==static_cast<INT32>(f)) FontDefault=-1;
}
void ShutdownFontManager()
{
    for (UINT32 i=0; i<MAX_FONTS; ++i) UnloadFont(i);
    sgp::ClearNativeFontReplacements();
    translation.clear(); initialized=false; saved={};
    FontDefault=-1; FontDestBuffer=0; FontDestPitch=0; FontDestBPP=16;
    FontDestRegion={0,0,0,0}; FontDestWrap=FALSE;
    foreground=0xffff; background=0; shadow=0;
}
BOOLEAN InitializeFontManager(UINT16 depth, FontTranslationTable* t)
{
    if (depth!=16 || !t || !t->usNumberOfSymbols || t->usNumberOfSymbols>32768 || !t->DynamicArrayOf16BitValues) return FALSE;
    std::vector<UINT16> copy;
    try { copy.assign(t->DynamicArrayOf16BitValues,t->DynamicArrayOf16BitValues+t->usNumberOfSymbols); }
    catch (...) { return FALSE; }
    ShutdownFontManager(); translation.swap(copy); initialized=true;
    ConfigureRGB565ImageFormat(); return TRUE;
}
INT32 LoadFontFile(const STR8 name)
{
    if (!initialized || !name || !*name || std::strlen(name)>=SGPFILENAME_LEN || serial==std::numeric_limits<std::uint64_t>::max()) return -1;
    std::size_t slot=0;
    while (slot<fonts.size() && fonts[slot]) ++slot;
    if (slot==fonts.size()) return -1;
    VOBJECT_DESC d{}; d.fCreateFlags=VOBJECT_CREATE_FROMFILE;
    std::strcpy(d.ImageFile,name);
    auto* f=CreateVideoObject(&d);
    if (!f) return -1;
    bool valid=f->usNumberOfObjects>0 && f->pPaletteEntry;
    for (UINT16 i=0; valid && i<f->usNumberOfObjects; ++i) valid=encoding(f,i);
    if (!valid) { DeleteVideoObject(f); return -1; }
    fonts[slot]=f; generations[slot]=++serial;
    if (FontDefault<0) FontDefault=static_cast<INT32>(slot);
    return static_cast<INT32>(slot);
}
INT16 GetIndex(CHAR16 c)
{
    for (std::size_t i=0; i<translation.size(); ++i) if (static_cast<std::uint32_t>(c)==translation[i]) return static_cast<INT16>(i);
    return 0;
}
UINT32 GetWidth(HVOBJECT f, INT16 i)
{
    if (!f || !f->pETRLEObject || i<0 || i>=f->usNumberOfObjects) return 0;
    const auto& g=f->pETRLEObject[i];
    return static_cast<UINT32>(std::max<INT32>(0,static_cast<INT32>(g.usWidth)+g.sOffsetX));
}
UINT16 GetFontHeight(INT32 i)
{
    const auto handle=replacement(i);
    if (handle>0 && replacementPolicies[static_cast<std::size_t>(i)].height)
        return static_cast<UINT16>(std::max<INT16>(0,replacementPolicies[static_cast<std::size_t>(i)].height(handle)));
    sgp::NativeScalableFontMetrics metrics;
    if (handle>0 && sgp::GetNativeScalableFontMetrics(handle,&metrics))
        // Original WinFonts[].Height stores tmAscent, not total line height.
        return static_cast<UINT16>(std::clamp<INT32>(metrics.ascent,0,65535));
    auto* f=GetFontObject(i);
    if (!f || !f->pETRLEObject || !f->usNumberOfObjects) return 0;
    const auto& g=f->pETRLEObject[0];
    return static_cast<UINT16>(std::clamp<INT32>(static_cast<INT32>(g.usHeight)+g.sOffsetY,0,65535));
}
INT16 StringPixLength(const CHAR16* s, INT32 i)
{
    auto* f=GetFontObject(i);
    if (!s || !f) return 0;
    const auto handle=replacement(i);
    if (handle>0) {
        const auto policy=replacementPolicies[static_cast<std::size_t>(i)].measure;
        if (policy) return policy(s,handle);
        INT32 width=0;
        return sgp::MeasureNativeScalableText(handle,s,&width) ?
            static_cast<INT16>(std::clamp<INT32>(width,0,32767)) : 0;
    }
    return metric(s,std::wcslen(s),f);
}
INT16 StringNPixLength(STR16 s, UINT32 count, INT32 i)
{
    auto* f=GetFontObject(i);
    if (!s || !f) return 0;
    std::size_t length=0;
    while (length<count && s[length]) ++length;
    return metric(s,length,f);
}
INT16 SubstringPixLength(STR16 s, UINT32 start, UINT32 end, INT32 i)
{
    auto* f=GetFontObject(i);
    if (!s || !f || start>end || end>std::wcslen(s)) return 0;
    // The original loop is iStart <= i <= iEnd, including a requested NUL.
    return metric(s+start,static_cast<std::size_t>(end)-start+1,f);
}
INT16 StringPixLengthArg(INT32 font, UINT32 count, STR16 format, ...)
{
    if (!format || !IsFontLoaded(font)) return 0;
    CHAR16 text[512];
    va_list args; va_start(args,format);
    const int length=ja2::native::VFormatWide(text,512,format,args);
    va_end(args);
    if (length<0 || length>=512) return 0;
    // Original wrapper truncates the formatted temporary then dispatches full
    // StringPixLength, including its active Win/scalable metric policy.
    text[std::min<std::size_t>(count,static_cast<std::size_t>(length))]=0;
    return StringPixLength(text,font);
}
void FindFontRightCoordinates(INT16 l, INT16 t, INT16 w, INT16 h, const STR16 text, INT32 font, INT16* x, INT16* y)
{ coordinates(l,t,w,h,text,font,x,y,false); }
void FindFontCenterCoordinates(INT16 l, INT16 t, INT16 w, INT16 h, const STR16 text, INT32 font, INT16* x, INT16* y)
{ coordinates(l,t,w,h,text,font,x,y,true); }
void VarFindFontRightCoordinates(INT16 l, INT16 t, INT16 w, INT16 h, INT32 font, INT16* x, INT16* y, const CHAR16* format, ...)
{
    va_list args; va_start(args,format);
    formattedCoordinates(l,t,w,h,font,x,y,format,args,false);
    va_end(args);
}
void VarFindFontCenterCoordinates(INT16 l, INT16 t, INT16 w, INT16 h, INT32 font, INT16* x, INT16* y, const CHAR16* format, ...)
{
    va_list args; va_start(args,format);
    formattedCoordinates(l,t,w,h,font,x,y,format,args,true);
    va_end(args);
}
BOOLEAN SetFontDestBuffer(UINT32 id, INT32 l, INT32 t, INT32 r, INT32 b, BOOLEAN wrap)
{
    UINT16 w,h; UINT8 bpp;
    if (l>=r || t>=b || !GetVideoSurfaceDescription(id,&w,&h,&bpp)) return FALSE;
    SGPRect c{std::max<INT32>(0,l),std::max<INT32>(0,t),std::min<INT32>(w,r),std::min<INT32>(h,b)};
    if (c.iLeft>=c.iRight || c.iTop>=c.iBottom) return FALSE;
    UINT32 pitch;
    if (!LockVideoSurface(id,&pitch)) return FALSE;
    UnLockVideoSurface(id);
    FontDestBuffer=id; FontDestPitch=pitch; FontDestBPP=16; FontDestRegion=c; FontDestWrap=wrap;
    return TRUE;
}
void SetFontForeground(UINT8 c) {
    if (IsFontLoaded(FontDefault)) {
        foreground=indexed(c);
        const auto entry=GetFontObject(FontDefault)->pPaletteEntry[c];
        setSelectedColor(FROMRGB(entry.peRed,entry.peGreen,entry.peBlue));
    }
}
void SetFontBackground(UINT8 c) { if (IsFontLoaded(FontDefault)) background=indexed(c); }
void SetFontShadow(UINT8 c)
{ if (IsFontLoaded(FontDefault)) { shadow=indexed(c); if (c && !shadow) shadow=1; } }
void SetFontColors(UINT16 c) { SetFontForeground(static_cast<UINT8>(c)); SetFontBackground(static_cast<UINT8>(c>>8)); }
void SetRGBFontForeground(UINT32 r, UINT32 g, UINT32 b) { foreground=rgb(r,g,b); setSelectedColor(FROMRGB(r,g,b)); }
void SetRGBFontBackground(UINT32 r, UINT32 g, UINT32 b) { background=rgb(r,g,b); }
void SetRGBFontShadow(UINT32 r, UINT32 g, UINT32 b) { shadow=rgb(r,g,b); }
void SetRGBFontForeground(UINT16 c) { foreground=c; setSelectedColor(GetRGBColor(c)); }
void SetRGBFontShadow(UINT16 c) { shadow=c; }
void SaveFontSettings()
{
    saved={FontDefault,IsFontLoaded(FontDefault)?generations[static_cast<std::size_t>(FontDefault)]:0,
        FontDestBuffer,FontDestPitch,FontDestBPP,FontDestRegion,FontDestWrap,foreground,background,shadow,
        replacement(FontDefault),selectedColor(replacement(FontDefault)),true};
}
void RestoreFontSettings()
{
    if (!saved.valid) return;
    FontDefault=IsFontLoaded(saved.font) && generations[static_cast<std::size_t>(saved.font)]==saved.generation ? saved.font : -1;
    FontDestBuffer=0; FontDestPitch=0; FontDestBPP=16; FontDestRegion={0,0,0,0}; FontDestWrap=FALSE;
    SetFontDestBuffer(saved.dest,saved.region.iLeft,saved.region.iTop,saved.region.iRight,saved.region.iBottom,saved.wrap);
    foreground=saved.fg; background=saved.bg; shadow=saved.sh;
    if (saved.scalable>0 && replacement(FontDefault)==saved.scalable)
        sgp::SetNativeScalableFontColor(saved.scalable,saved.scalableColor);
}
UINT32 mprintf(INT32 x, INT32 y, const CHAR16* fmt, ...)
{
    va_list a; va_start(a,fmt); const auto r=print(nullptr,0,FontDefault,x,y,fmt,a,true,true); va_end(a); return r;
}
static UINT32 codedPrint(UINT8* buffer, UINT32 pitch, INT32 x, INT32 y,
    const STR16 fmt, va_list args, bool surface)
{
    if (!fmt || !IsFontLoaded(FontDefault)) return failure;
    wchar_t formatted[512], text[512]; UINT16 colors[512];
    const int count=ja2::native::VFormatWide(formatted,512,fmt,args);
    if (count<0) return failure;
    // Original Win-font branch precedes palette control parsing. The safe native
    // adapter formats exactly once, then passes 180/181 as literal Unicode.
    const auto handle=replacement(FontDefault);
    if (handle>0) return scalableLiteral(buffer,pitch,handle,x,y,formatted,surface);
    const UINT16 original=foreground; UINT16 current=original;
    std::size_t written=0;
    // Original Font.cpp color controls:180 selects the following palette
    // index;181 restores the entering foreground. Safely accept terminal
    // reset and reject a dangling color selector before any state/pixel write.
    for (int i=0;i<count;++i) {
        if (formatted[i]==180) {
            if (++i==count) return failure;
            current=indexed(static_cast<UINT8>(formatted[i]));
        } else if (formatted[i]==181) current=original;
        else { text[written]=formatted[i]; colors[written++]=current; }
    }
    text[written]=0;
    const auto result=literal(buffer,pitch,FontDefault,x,y,text,true,surface,colors);
    if (result==0) foreground=current;
    return result;
}
UINT32 mprintf_coded(INT32 x, INT32 y, const STR16 fmt, ...)
{
    va_list args; va_start(args,fmt);
    const auto result=codedPrint(nullptr,0,x,y,fmt,args,true); va_end(args); return result;
}
UINT32 mprintf_buffer_coded(UINT8* buffer, UINT32 pitch, UINT32 fontType, INT32 x, INT32 y, const STR16 fmt, ...)
{
    // The pinned coded-buffer entry uses FontDefault; FontType is unused there.
    (void)fontType;
    va_list args; va_start(args,fmt);
    const auto result=codedPrint(buffer,pitch,x,y,fmt,args,false); va_end(args); return result;
}
UINT32 gprintf(INT32 x, INT32 y, const STR16 fmt, ...)
{
    va_list a; va_start(a,fmt); const auto r=print(nullptr,0,FontDefault,x,y,fmt,a,false,true); va_end(a); return r;
}
UINT32 mprintf_buffer(UINT8* p, UINT32 pitch, UINT32 f, INT32 x, INT32 y, const STR16 fmt, ...)
{
    const auto mapped=replacement(FontDefault);
    if (mapped<0 && f>=MAX_FONTS) return failure;
    const INT32 selected=mapped>0 ? FontDefault : static_cast<INT32>(f);
    va_list a; va_start(a,fmt); const auto r=print(p,pitch,selected,x,y,fmt,a,true,false); va_end(a); return r;
}
UINT32 gprintf_buffer(UINT8* p, UINT32 pitch, UINT32 f, INT32 x, INT32 y, const STR16 fmt, ...)
{
    if (f>=MAX_FONTS) return failure;
    va_list a; va_start(a,fmt); const auto r=print(p,pitch,static_cast<INT32>(f),x,y,fmt,a,false,false,false); va_end(a); return r;
}

namespace sgp {
bool SetNativeFontReplacement(INT32 bitmapFont, INT32 scalableHandle, NativeFontReplacementPolicy policy) {
    if (!IsFontLoaded(bitmapFont)) return false;
    if (scalableHandle!=-1) {
        NativeScalableFontMetrics metrics;
        if (scalableHandle<=0 || !GetNativeScalableFontMetrics(scalableHandle,&metrics)) return false;
    }
    replacements[static_cast<std::size_t>(bitmapFont)]=scalableHandle;
    replacementPolicies[static_cast<std::size_t>(bitmapFont)]=scalableHandle==-1 ? NativeFontReplacementPolicy{} : policy;
    return true;
}
void ClearNativeFontReplacements() noexcept { replacements.fill(-1); replacementPolicies.fill(NativeFontReplacementPolicy{}); }
INT32 GetNativeFontReplacement(INT32 bitmapFont) { return replacement(bitmapFont); }
NativeFontState CaptureNativeFontState() {
    return {FontDefault,IsFontLoaded(FontDefault)?generations[static_cast<std::size_t>(FontDefault)]:0,
        FontDestBuffer,FontDestPitch,FontDestBPP,FontDestRegion,FontDestWrap,foreground,background,shadow,
        replacement(FontDefault),selectedColor(replacement(FontDefault))};
}
void RestoreNativeFontState(const NativeFontState& s) {
    FontDefault=IsFontLoaded(s.font) && generations[static_cast<std::size_t>(s.font)]==s.generation ? s.font : -1;
    FontDestBuffer=s.dest; FontDestPitch=s.pitch; FontDestBPP=s.bpp;
    FontDestRegion=s.region; FontDestWrap=s.wrap;
    foreground=s.foreground; background=s.background; shadow=s.shadow;
    if (s.scalable>0 && replacement(FontDefault)==s.scalable)
        SetNativeScalableFontColor(s.scalable,s.scalableColor);
}
UINT32 DrawNativeFontText(INT32 x, INT32 y, const CHAR16* text, bool multicolor) {
    return literal(nullptr,0,FontDefault,x,y,text,!multicolor,true);
}
}
