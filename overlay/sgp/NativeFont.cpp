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

#include "native_font.h"
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
UINT32 print(BYTE* buffer, UINT32 pitch, INT32 font, INT32 x, INT32 y, const wchar_t* format, va_list args, bool mono, bool surface)
{
    if (!format || !IsFontLoaded(font) || FontDestBPP!=16) return failure;
    wchar_t text[512];
    const int count=std::vswprintf(text,512,format,args);
    if (count<0 || count>=512) return failure;
    auto* f=GetFontObject(font);
    for (int i=0; i<count; ++i) if (!encoding(f,static_cast<UINT16>(GetIndex(text[i])))) return failure;
    SGPRect clip=FontDestRegion;
    if (surface)
    {
        UINT16 w,h; UINT8 bpp;
        if (!GetVideoSurfaceDescription(FontDestBuffer,&w,&h,&bpp)) return failure;
        clip.iLeft=std::clamp(clip.iLeft,0,static_cast<INT32>(w));
        clip.iTop=std::clamp(clip.iTop,0,static_cast<INT32>(h));
        clip.iRight=std::clamp(clip.iRight,clip.iLeft,static_cast<INT32>(w));
        clip.iBottom=std::clamp(clip.iBottom,clip.iTop,static_cast<INT32>(h));
        buffer=LockVideoSurface(FontDestBuffer,&pitch);
    }
    if (!buffer || !destination(pitch,clip))
    { if (surface && buffer) UnLockVideoSurface(FontDestBuffer); return failure; }
    I dx=x, dy=y;
    for (int i=0; i<count; ++i)
    {
        const auto index=static_cast<UINT16>(GetIndex(text[i]));
        const auto& g=f->pETRLEObject[index];
        const I gx=dx+g.sOffsetX, gy=dy+g.sOffsetY;
        if (FontDestWrap && (gx<clip.iLeft || gy<clip.iTop || gx+g.usWidth>clip.iRight || gy+g.usHeight>clip.iBottom))
        { dx=x; dy+=static_cast<I>(g.usHeight)+g.sOffsetY; }
        glyph(buffer,pitch,f,index,dx,dy,clip,mono);
        dx+=static_cast<I>(g.usWidth)+g.sOffsetX;
    }
    if (surface) UnLockVideoSurface(FontDestBuffer);
    return 0;
}
}
BOOLEAN IsFontLoaded(INT32 f) { return initialized && f>=0 && f<MAX_FONTS && fonts[static_cast<std::size_t>(f)] ? TRUE : FALSE; }
HVOBJECT GetFontObject(INT32 f) { return IsFontLoaded(f) ? fonts[static_cast<std::size_t>(f)] : nullptr; }
BOOLEAN SetFont(INT32 f) { if (!IsFontLoaded(f)) return FALSE; FontDefault=f; return TRUE; }
void UnloadFont(UINT32 f)
{
    if (f>=MAX_FONTS || !fonts[f]) return;
    DeleteVideoObject(fonts[f]); fonts[f]=nullptr;
    if (FontDefault==static_cast<INT32>(f)) FontDefault=-1;
}
void ShutdownFontManager()
{
    for (UINT32 i=0; i<MAX_FONTS; ++i) UnloadFont(i);
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
    auto* f=GetFontObject(i);
    if (!f || !f->pETRLEObject || !f->usNumberOfObjects) return 0;
    const auto& g=f->pETRLEObject[0];
    return static_cast<UINT16>(std::clamp<INT32>(static_cast<INT32>(g.usHeight)+g.sOffsetY,0,65535));
}
INT16 StringPixLength(const CHAR16* s, INT32 i)
{
    auto* f=GetFontObject(i);
    if (!s || !f) return 0;
    UINT32 width=0;
    while (*s) { width+=GetWidth(f,GetIndex(*s++)); if (width>=32767) return 32767; }
    return static_cast<INT16>(width);
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
void SetFontForeground(UINT8 c) { if (IsFontLoaded(FontDefault)) foreground=indexed(c); }
void SetFontBackground(UINT8 c) { if (IsFontLoaded(FontDefault)) background=indexed(c); }
void SetFontShadow(UINT8 c)
{ if (IsFontLoaded(FontDefault)) { shadow=indexed(c); if (c && !shadow) shadow=1; } }
void SetFontColors(UINT16 c) { SetFontForeground(static_cast<UINT8>(c)); SetFontBackground(static_cast<UINT8>(c>>8)); }
void SetRGBFontForeground(UINT32 r, UINT32 g, UINT32 b) { foreground=rgb(r,g,b); }
void SetRGBFontBackground(UINT32 r, UINT32 g, UINT32 b) { background=rgb(r,g,b); }
void SetRGBFontShadow(UINT32 r, UINT32 g, UINT32 b) { shadow=rgb(r,g,b); }
void SetRGBFontForeground(UINT16 c) { foreground=c; }
void SetRGBFontShadow(UINT16 c) { shadow=c; }
void SaveFontSettings()
{
    saved={FontDefault,IsFontLoaded(FontDefault)?generations[static_cast<std::size_t>(FontDefault)]:0,
        FontDestBuffer,FontDestPitch,FontDestBPP,FontDestRegion,FontDestWrap,foreground,background,shadow,true};
}
void RestoreFontSettings()
{
    if (!saved.valid) return;
    FontDefault=IsFontLoaded(saved.font) && generations[static_cast<std::size_t>(saved.font)]==saved.generation ? saved.font : -1;
    FontDestBuffer=0; FontDestPitch=0; FontDestBPP=16; FontDestRegion={0,0,0,0}; FontDestWrap=FALSE;
    SetFontDestBuffer(saved.dest,saved.region.iLeft,saved.region.iTop,saved.region.iRight,saved.region.iBottom,saved.wrap);
    foreground=saved.fg; background=saved.bg; shadow=saved.sh;
}
UINT32 mprintf(INT32 x, INT32 y, const CHAR16* fmt, ...)
{
    va_list a; va_start(a,fmt); const auto r=print(nullptr,0,FontDefault,x,y,fmt,a,true,true); va_end(a); return r;
}
UINT32 gprintf(INT32 x, INT32 y, const STR16 fmt, ...)
{
    va_list a; va_start(a,fmt); const auto r=print(nullptr,0,FontDefault,x,y,fmt,a,false,true); va_end(a); return r;
}
UINT32 mprintf_buffer(UINT8* p, UINT32 pitch, UINT32 f, INT32 x, INT32 y, const STR16 fmt, ...)
{
    if (f>=MAX_FONTS) return failure;
    va_list a; va_start(a,fmt); const auto r=print(p,pitch,static_cast<INT32>(f),x,y,fmt,a,true,false); va_end(a); return r;
}
UINT32 gprintf_buffer(UINT8* p, UINT32 pitch, UINT32 f, INT32 x, INT32 y, const STR16 fmt, ...)
{
    if (f>=MAX_FONTS) return failure;
    va_list a; va_start(a,fmt); const auto r=print(p,pitch,static_cast<INT32>(f),x,y,fmt,a,false,false); va_end(a); return r;
}
