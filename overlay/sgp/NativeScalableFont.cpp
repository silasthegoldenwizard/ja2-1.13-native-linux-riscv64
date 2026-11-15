/*
 * P710 native snapshot update: sgp/NativeScalableFont.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeScalableFont.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>

namespace sgp { namespace {
using I=std::int64_t;
constexpr std::size_t maxBytes=16u*1024u*1024u, maxText=4096;
struct Font {
    std::vector<BYTE> bytes;
    FT_Library library=nullptr;
    FT_Face face=nullptr;
    INT32 id=0;
    COLORVAL color=FROMRGB(255,255,255);
    NativeScalableFontMetrics metrics;
    ~Font() { if (face) FT_Done_Face(face); if (library) FT_Done_FreeType(library); }
};
std::array<std::unique_ptr<Font>,32> fonts;
std::uint64_t serial=0;
Font* find(INT32 id) noexcept {
    if (id<=0) return nullptr;
    for (const auto& f:fonts) if (f && f->id==id) return f.get();
    return nullptr;
}
I floor64(I n) { return n>=0 ? n/64 : -((-n+63)/64); }
I ceil64(I n) { return n>=0 ? (n+63)/64 : -((-n)/64); }
struct Glyph { I x=0,y=0; UINT32 width=0,height=0; std::vector<BYTE> mask; };
struct Layout { std::vector<Glyph> glyphs; INT32 width=0; };
bool scalar(const wchar_t*& text, std::uint32_t& value) {
    const auto first=static_cast<std::uint32_t>(*text++);
    if constexpr (sizeof(wchar_t)==2) {
        if (first>=0xd800 && first<=0xdbff) {
            const auto next=static_cast<std::uint32_t>(*text);
            if (next<0xdc00 || next>0xdfff) return false;
            ++text; value=0x10000+((first-0xd800)<<10)+(next-0xdc00); return true;
        }
    }
    if (first>0x10ffff || (first>=0xd800 && first<=0xdfff)) return false;
    value=first; return true;
}
// Masks are copied immediately: FT_Load_Glyph replaces the face's glyph slot.
bool mask(const FT_Bitmap& bitmap, Glyph& out, std::size_t& total) {
    if (!bitmap.width || !bitmap.rows) return true;
    if (!bitmap.buffer || bitmap.width>4096 || bitmap.rows>4096) return false;
    if (bitmap.pixel_mode!=FT_PIXEL_MODE_GRAY && bitmap.pixel_mode!=FT_PIXEL_MODE_MONO) return false;
    const I pitch=bitmap.pitch;
    const I stride=pitch<0 ? -pitch : pitch;
    const I minimum=bitmap.pixel_mode==FT_PIXEL_MODE_MONO ? (I(bitmap.width)+7)/8 : bitmap.width;
    if (stride<minimum || stride*bitmap.rows>I(maxBytes)) return false;
    if (bitmap.pixel_mode==FT_PIXEL_MODE_GRAY && (bitmap.num_grays<2 || bitmap.num_grays>256)) return false;
    const std::size_t count=std::size_t(bitmap.width)*bitmap.rows;
    if (count>maxBytes-total) return false;
    out.mask.resize(count); out.width=bitmap.width; out.height=bitmap.rows; total+=count;
    const BYTE* top=bitmap.buffer;
    // Match pinned FreeType's FT_Bitmap_Convert: buffer is allocation base;
    // a negative pitch stores the top row at its highest row address.
    if (pitch<0) top+=static_cast<std::ptrdiff_t>(-pitch*(bitmap.rows-1));
    for (UINT32 y=0;y<out.height;++y) {
        const auto* row=top+static_cast<std::ptrdiff_t>(I(y)*pitch);
        for (UINT32 x=0;x<out.width;++x) {
            unsigned alpha;
            if (bitmap.pixel_mode==FT_PIXEL_MODE_MONO) alpha=(row[x/8]&(0x80u>>(x%8))) ? 255 : 0;
            else {
                const unsigned sample=row[x], maximum=bitmap.num_grays-1;
                if (sample>maximum) return false;
                alpha=(sample*255u+maximum/2)/maximum;
            }
            out.mask[std::size_t(y)*out.width+x]=static_cast<BYTE>(alpha);
        }
    }
    return true;
}
bool layout(Font* font,const wchar_t* text,Layout& out,bool render) {
    if (!font || !text) return false;
    I pen=0; FT_UInt previous=0; std::size_t count=0,total=0;
    while (*text) {
        if (count++==maxText) return false;
        std::uint32_t cp=0;
        // Original WinFont callers forward coded strings literally. All valid
        // scalars therefore use this face's cmap (or glyph 0), without adding
        // newline, tab, palette-control or other text-layout interpretation.
        if (!scalar(text,cp)) return false;
        const FT_UInt index=FT_Get_Char_Index(font->face,cp);
        if (previous && FT_HAS_KERNING(font->face)) {
            FT_Vector kern{};
            if (FT_Get_Kerning(font->face,previous,index,FT_KERNING_DEFAULT,&kern)) return false;
            // Cap all library-provided contributions before signed arithmetic.
            if (kern.x<-I(maxBytes) || kern.x>I(maxBytes) || kern.y) return false;
            pen+=kern.x;
        }
        if (pen<-I(maxBytes)*64 || pen>I(maxBytes)*64) return false;
        if (FT_Load_Glyph(font->face,index,FT_LOAD_DEFAULT|FT_LOAD_TARGET_NORMAL)) return false;
        const auto slot=font->face->glyph;
        if (slot->advance.x<0 || slot->advance.x>I(maxBytes) || slot->advance.y) return false;
        if (render) {
            if (slot->format!=FT_GLYPH_FORMAT_BITMAP && FT_Render_Glyph(slot,FT_RENDER_MODE_NORMAL)) return false;
            Glyph glyph;
            glyph.x=floor64(pen)+slot->bitmap_left;
            glyph.y=I(font->metrics.ascent)-slot->bitmap_top;
            if (!mask(slot->bitmap,glyph,total)) return false;
            out.glyphs.push_back(std::move(glyph));
        }
        pen+=slot->advance.x; previous=index;
    }
    const I width=ceil64(pen);
    if (width<0 || width>std::numeric_limits<INT32>::max()) return false;
    out.width=static_cast<INT32>(width); return true;
}
bool destination(BYTE* data,std::size_t bytes,UINT32 width,UINT32 height,UINT32 pitch,
                 const SGPRect* requested,SGPRect& clip) {
    if (!data || !width || !height || width>UINT32(std::numeric_limits<INT32>::max()) ||
        height>UINT32(std::numeric_limits<INT32>::max()) || I(width)*2>pitch ||
        std::uint64_t(pitch)*height>bytes || bytes>std::size_t(std::numeric_limits<std::ptrdiff_t>::max())) return false;
    clip={0,0,static_cast<INT32>(width),static_cast<INT32>(height)};
    if (requested) {
        if (requested->iRight<requested->iLeft || requested->iBottom<requested->iTop) return false;
        clip.iLeft=std::clamp(requested->iLeft,0,clip.iRight);
        clip.iTop=std::clamp(requested->iTop,0,clip.iBottom);
        clip.iRight=std::clamp(requested->iRight,clip.iLeft,clip.iRight);
        clip.iBottom=std::clamp(requested->iBottom,clip.iTop,clip.iBottom);
    }
    return true;
}
UINT16 blend(UINT16 old,COLORVAL color,unsigned a) {
    const unsigned r=(color&255u)>>3, g=((color>>8)&255u)>>2, b=((color>>16)&255u)>>3;
    const auto mix=[a](unsigned foreground,unsigned background) { return (foreground*a+background*(255-a)+127)/255; };
    return static_cast<UINT16>((mix(r,old>>11)<<11)|(mix(g,(old>>5)&63u)<<5)|mix(b,old&31u));
}
void write(BYTE* data,UINT32 pitch,INT32 x,INT32 y,const Layout& prepared,COLORVAL color,const SGPRect& clip) noexcept {
    for (const auto& glyph:prepared.glyphs) {
        const I left=I(x)+glyph.x,top=I(y)+glyph.y;
        const I x0=std::max<I>(left,clip.iLeft),y0=std::max<I>(top,clip.iTop);
        const I x1=std::min<I>(left+glyph.width,clip.iRight),y1=std::min<I>(top+glyph.height,clip.iBottom);
        for (I py=y0;py<y1;++py) for (I px=x0;px<x1;++px) {
            const unsigned alpha=glyph.mask[std::size_t(py-top)*glyph.width+std::size_t(px-left)];
            if (!alpha) continue;
            BYTE* pixel=data+std::size_t(py)*pitch+std::size_t(px)*2;
            UINT16 old; std::memcpy(&old,pixel,2);
            const UINT16 value=blend(old,color,alpha); std::memcpy(pixel,&value,2);
        }
    }
}
struct Unlock { UINT32 id; ~Unlock() { UnLockVideoSurface(id); } };
} // namespace

INT32 CreateNativeScalableFont(const BYTE* input,std::size_t bytes,UINT32 pixels) {
    if (!input || !bytes || bytes>maxBytes || !pixels || pixels>512 ||
        serial>=std::uint64_t(std::numeric_limits<INT32>::max())) return -1;
    auto slot=std::find_if(fonts.begin(),fonts.end(),[](const auto& f) { return !f; });
    if (slot==fonts.end()) return -1;
    try {
        auto font=std::make_unique<Font>(); font->bytes.assign(input,input+bytes);
        if (FT_Init_FreeType(&font->library) ||
            FT_New_Memory_Face(font->library,font->bytes.data(),static_cast<FT_Long>(bytes),0,&font->face) ||
            !FT_IS_SCALABLE(font->face) || FT_Select_Charmap(font->face,FT_ENCODING_UNICODE) ||
            FT_Set_Pixel_Sizes(font->face,0,pixels)) return -1;
        const auto& m=font->face->size->metrics;
        const I ascent=ceil64(m.ascender),descent=ceil64(-I(m.descender));
        const I height=std::max(ceil64(m.height),ascent+descent);
        if (ascent<0 || descent<0 || height<1 || height>65535) return -1;
        font->metrics={static_cast<INT32>(ascent),static_cast<INT32>(descent),static_cast<INT32>(height),
                       static_cast<INT32>(std::max<I>(0,height-pixels))};
        font->id=static_cast<INT32>(++serial);
        const INT32 id=font->id; *slot=std::move(font); return id;
    } catch (...) { return -1; }
}
void DeleteNativeScalableFont(INT32 id) noexcept { for (auto& font:fonts) if (font && font->id==id) { font.reset(); return; } }
void ShutdownNativeScalableFonts() noexcept { for (auto& font:fonts) font.reset(); }
bool SetNativeScalableFontColor(INT32 id,COLORVAL color) { auto* f=find(id); if (!f) return false; f->color=color; return true; }
bool GetNativeScalableFontColor(INT32 id,COLORVAL* color) { auto* f=find(id); if (!f || !color) return false; *color=f->color; return true; }
bool GetNativeScalableFontMetrics(INT32 id,NativeScalableFontMetrics* out) { auto* f=find(id); if (!f || !out) return false; *out=f->metrics; return true; }
bool MeasureNativeScalableText(INT32 id,const wchar_t* text,INT32* width) {
    if (!width) return false;
    try { Layout prepared; if (!layout(find(id),text,prepared,false)) return false; *width=prepared.width; return true; }
    catch (...) { return false; }
}
bool DrawNativeScalableTextBuffer(BYTE* data,std::size_t bytes,UINT32 width,UINT32 height,UINT32 pitch,
                                  INT32 id,INT32 x,INT32 y,const wchar_t* text,COLORVAL color,const SGPRect* requested) {
    try {
        SGPRect clip; Layout prepared;
        if (!destination(data,bytes,width,height,pitch,requested,clip) || !layout(find(id),text,prepared,true)) return false;
        write(data,pitch,x,y,prepared,color,clip); return true;
    } catch (...) { return false; }
}
bool DrawNativeScalableText(UINT32 dest,INT32 id,INT32 x,INT32 y,const wchar_t* text,COLORVAL color,const SGPRect* requested) {
    try {
        Layout prepared;
        if (!layout(find(id),text,prepared,true)) return false;
        UINT32 pitch=0; BYTE* data=LockVideoSurface(dest,&pitch);
        if (!data) return false;
        Unlock unlock{dest}; NativeSurfaceView view; SGPRect clip;
        if (!LockedNativeSurfaceView(data,&view) || pitch!=view.pitch ||
            !destination(data,view.bytes,view.width,view.height,pitch,requested,clip)) return false;
        write(data,pitch,x,y,prepared,color,clip); return true;
    } catch (...) { return false; }
}
} // namespace sgp
