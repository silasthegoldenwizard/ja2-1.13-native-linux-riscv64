/*
 * P710 native snapshot update: tests/native-core/native_scalable_font_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeScalableFont.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

// Deterministic failure of application-owned C++ allocations only. FreeType's
// separately built allocator is not intercepted; no FreeType OOM claim follows.
namespace { std::ptrdiff_t allocationCountdown=-1; }
void* operator new(std::size_t size) {
    if (allocationCountdown==0) throw std::bad_alloc();
    if (allocationCountdown>0) --allocationCountdown;
    if (void* p=std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }

namespace {
unsigned checks=0;
void check(bool value,const char* what) { ++checks; if (!value) throw std::runtime_error(what); }
std::vector<BYTE> file(const char* name) {
    std::ifstream input(name,std::ios::binary);
    check(bool(input),"public font fixture readable");
    return std::vector<BYTE>(std::istreambuf_iterator<char>(input),{});
}
struct Oracle {
    FT_Library library=nullptr; FT_Face face=nullptr;
    explicit Oracle(const std::vector<BYTE>& bytes,UINT32 size) {
        check(!FT_Init_FreeType(&library),"oracle library");
        check(!FT_New_Memory_Face(library,bytes.data(),static_cast<FT_Long>(bytes.size()),0,&face),"oracle face");
        check(!FT_Select_Charmap(face,FT_ENCODING_UNICODE),"oracle Unicode");
        check(!FT_Set_Pixel_Sizes(face,0,size),"oracle size");
    }
    ~Oracle() { if (face) FT_Done_Face(face); if (library) FT_Done_FreeType(library); }
    int ascent() const { return int(std::ceil(face->size->metrics.ascender/64.0)); }
    int descent() const { return int(std::ceil(-face->size->metrics.descender/64.0)); }
    int height() const { return std::max(int(std::ceil(face->size->metrics.height/64.0)),ascent()+descent()); }
    int width(const wchar_t* text,bool kerning=true) {
        FT_Pos pen=0; FT_UInt last=0;
        for (const wchar_t* at=text;*at;++at) {
            const FT_UInt index=FT_Get_Char_Index(face,static_cast<FT_ULong>(*at));
            if (kerning && last && FT_HAS_KERNING(face)) { FT_Vector k{}; check(!FT_Get_Kerning(face,last,index,FT_KERNING_DEFAULT,&k),"oracle kerning"); pen+=k.x; }
            check(!FT_Load_Glyph(face,index,FT_LOAD_DEFAULT|FT_LOAD_TARGET_NORMAL),"oracle load");
            pen+=face->glyph->advance.x; last=index;
        }
        return int(std::ceil(pen/64.0));
    }
    void paint(std::vector<BYTE>& out,UINT32 width,UINT32 height,UINT32 pitch,INT32 x,INT32 y,
               const wchar_t* text,COLORVAL color,const SGPRect* clip=nullptr) {
        FT_Pos pen=0; FT_UInt last=0;
        for (const wchar_t* at=text;*at;++at) {
            const FT_UInt index=FT_Get_Char_Index(face,static_cast<FT_ULong>(*at));
            if (last && FT_HAS_KERNING(face)) { FT_Vector k{}; check(!FT_Get_Kerning(face,last,index,FT_KERNING_DEFAULT,&k),"oracle render kerning"); pen+=k.x; }
            check(!FT_Load_Glyph(face,index,FT_LOAD_DEFAULT|FT_LOAD_TARGET_NORMAL),"oracle render load");
            check(!FT_Render_Glyph(face->glyph,FT_RENDER_MODE_NORMAL),"oracle render mask");
            const auto& bitmap=face->glyph->bitmap;
            // Public DejaVu fixtures exercise FreeType-generated GRAY/down-flow.
            // No synthetic glyph masks are passed to the production renderer.
            if (bitmap.width && bitmap.rows) check(bitmap.pixel_mode==FT_PIXEL_MODE_GRAY && bitmap.pitch>0 && bitmap.num_grays==256,"oracle fixture bitmap format");
            const std::int64_t left=std::int64_t(x)+std::int64_t(std::floor(pen/64.0))+face->glyph->bitmap_left;
            const std::int64_t top=std::int64_t(y)+ascent()-face->glyph->bitmap_top;
            for (UINT32 row=0;row<bitmap.rows;++row) for (UINT32 col=0;col<bitmap.width;++col) {
                const std::int64_t px=left+col,py=top+row;
                if (px<0 || py<0 || px>=width || py>=height || (clip && (px<clip->iLeft || px>=clip->iRight || py<clip->iTop || py>=clip->iBottom))) continue;
                const unsigned a=bitmap.buffer[std::size_t(row)*bitmap.pitch+col];
                if (!a) continue;
                const std::size_t offset=std::size_t(py)*pitch+std::size_t(px)*2;
                UINT16 old; std::memcpy(&old,out.data()+offset,2);
                // Independent floating weighted average over the actual 565
                // channel ranges, with nearest rounding. No production helper.
                const double opacity=a/255.0;
                const auto mix=[opacity](unsigned src,unsigned dst) { return unsigned(std::lround(src*opacity+dst*(1.0-opacity))); };
                const unsigned r=mix((color&255)/8,old/2048);
                const unsigned g=mix(((color>>8)&255)/4,(old/32)%64);
                const unsigned b=mix(((color>>16)&255)/8,old%32);
                const UINT16 expected=UINT16(r*2048+g*32+b);
                std::memcpy(out.data()+offset,&expected,2);
            }
            pen+=face->glyph->advance.x; last=index;
        }
    }
};
std::vector<BYTE> seed(UINT32 pitch,UINT32 height) {
    std::vector<BYTE> out(std::size_t(pitch)*height);
    for (std::size_t i=0;i<out.size();++i) out[i]=BYTE((i*73+19)%256);
    return out;
}
void compare(INT32 font,Oracle& oracle,const wchar_t* text,INT32 x,INT32 y,const SGPRect* clip=nullptr,COLORVAL color=FROMRGB(219,93,37)) {
    constexpr UINT32 width=79,height=53,pitch=172;
    auto actual=seed(pitch,height),expected=actual;
    oracle.paint(expected,width,height,pitch,x,y,text,color,clip);
    check(sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),width,height,pitch,font,x,y,text,color,clip),"bounded draw success");
    check(actual==expected,"entire padded destination equals independent FT/blend oracle");
    INT32 measured=-123;
    check(sgp::MeasureNativeScalableText(font,text,&measured) && measured==oracle.width(text),"measure equals direct FT kerning oracle");
}
void suite(const char* regularPath,const char* boldPath) {
    auto regular=file(regularPath),bold=file(boldPath);
    Oracle oracle(regular,23),boldOracle(bold,23);
    FT_Int major,minor,patch; FT_Library_Version(oracle.library,&major,&minor,&patch);
    check(major==2 && minor==15 && patch==0,"approved pinned FreeType 2.15.0 linked");
    const INT32 font=sgp::CreateNativeScalableFont(regular.data(),regular.size(),23);
    const INT32 boldFont=sgp::CreateNativeScalableFont(bold.data(),bold.size(),23);
    check(font>0 && boldFont>font,"unique positive handle identities");
    sgp::NativeScalableFontMetrics metrics;
    check(sgp::GetNativeScalableFontMetrics(font,&metrics),"metrics queried");
    check(metrics.ascent==oracle.ascent() && metrics.descent==oracle.descent() && metrics.height==oracle.height() && metrics.internalLeading==std::max(0,oracle.height()-23),"actual face metrics and documented leading");
    COLORVAL color=0;
    check(sgp::GetNativeScalableFontColor(font,&color) && color==FROMRGB(255,255,255),"default font color");
    check(sgp::SetNativeScalableFontColor(font,FROMRGB(11,205,78)) && sgp::GetNativeScalableFontColor(font,&color) && color==FROMRGB(11,205,78),"per-handle real color state");
    check(oracle.width(L"AV")<oracle.width(L"AV",false),"chosen AV pair genuinely exercises kerning");
    compare(font,oracle,L"AV To !",2,3);
    check(!FT_Load_Char(oracle.face,'A',FT_LOAD_RENDER|FT_LOAD_TARGET_NORMAL),"real antialias fixture loaded");
    bool partial=false,opaque=false;
    const auto& antialias=oracle.face->glyph->bitmap;
    for (UINT32 row=0;row<antialias.rows;++row) for (UINT32 col=0;col<antialias.width;++col) {
        const unsigned a=antialias.buffer[std::size_t(row)*antialias.pitch+col];
        partial=partial || (a>0 && a<255); opaque=opaque || a==255;
    }
    check(partial && opaque,"actual mask covers partial and opaque blend cases");
    compare(boldFont,boldOracle,L"AV To !",2,3);
    compare(font,oracle,L"\u041f\u0440\u0438\u0432\u0435\u0442",-2,0);
    check(FT_Get_Char_Index(oracle.face,0x1f600)!=0,"public fixture includes chosen non-BMP glyph");
    compare(font,oracle,L"\U0001f600",20,2);
    check(FT_Get_Char_Index(oracle.face,0x10ffff)==0,"chosen valid missing codepoint uses .notdef");
    compare(font,oracle,L"A\U0010ffffV",3,3);
    // Preserve the original WinFont branch's literal forwarding domain:
    // palette-coded byte values and C0/C1 are ordinary cmap/glyph0 input here.
    // The oracle performs no control parsing, line motion or tab expansion.
    const wchar_t coded[]={wchar_t(180),wchar_t(2),L'A',wchar_t(181),L'B',0};
    compare(font,oracle,coded,1,2);
    compare(font,oracle,L"A\002V",1,2);
    compare(font,oracle,L"A\205V",1,2);
    compare(font,oracle,L"A\nV",1,2);
    compare(font,oracle,L"A\tV",1,2);
    compare(font,oracle,L"A\r\177\237V",1,2);
    std::wstring literalControls;
    for (unsigned cp=1;cp<32;++cp) literalControls.push_back(static_cast<wchar_t>(cp));
    for (unsigned cp=127;cp<=159;++cp) literalControls.push_back(static_cast<wchar_t>(cp));
    compare(font,oracle,literalControls.c_str(),1,2);
    const wchar_t nulTerminated[]={L'A',0,L'V',0};
    compare(font,oracle,nulTerminated,1,2);
    check(oracle.width(nulTerminated)==oracle.width(L"A"),"embedded NUL remains string terminator");
    compare(font,oracle,L" ",1,1);
    compare(font,oracle,L"",1,1);
    const SGPRect clips[]={{5,6,48,32},{-100,-200,15,19},{20,20,20,25},{-50,-50,-10,-10},{300,200,400,250}};
    for (const auto& clip:clips) compare(font,oracle,L"gjAV",1,-2,&clip);
    const INT32 positions[][2]={{-7,5},{72,5},{5,-19},{5,45},{-50,-50},{std::numeric_limits<INT32>::min(),std::numeric_limits<INT32>::max()},{std::numeric_limits<INT32>::max(),std::numeric_limits<INT32>::min()}};
    for (const auto& p:positions) compare(font,oracle,L"gjAV",p[0],p[1]);
    compare(font,oracle,L"AV",2,3,nullptr,FROMRGB(0,0,0));
    compare(font,oracle,L"AV",2,3,nullptr,FROMRGB(255,255,255));
    // Caller input ownership: mutation and release cannot alter a created face.
    auto borrowed=regular; const INT32 copied=sgp::CreateNativeScalableFont(borrowed.data(),borrowed.size(),23);
    check(copied>0,"copy-lifetime font"); std::fill(borrowed.begin(),borrowed.end(),0); std::vector<BYTE>().swap(borrowed);
    compare(copied,oracle,L"Lifetime AV",0,0); sgp::DeleteNativeScalableFont(copied);
    const auto baseline=seed(172,53); auto actual=baseline;
    const auto reject=[&](const wchar_t* text) {
        check(!sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),79,53,172,font,0,0,text,0),"invalid text refused");
        check(actual==baseline,"invalid text leaves complete buffer unchanged");
        INT32 width=872; check(!sgp::MeasureNativeScalableText(font,text,&width) && width==872,"invalid measurement preserves output");
    };
    reject(nullptr);
    const wchar_t surrogate[]={L'A',wchar_t(0xd800),0}; reject(surrogate);
    const wchar_t tooHigh[]={L'A',wchar_t(0x110000),0}; reject(tooHigh);
    const std::wstring overlong(4097,L'A'); reject(overlong.c_str());
    const std::wstring maximum(4096,L'A'); INT32 maximumWidth=0;
    check(sgp::MeasureNativeScalableText(font,maximum.c_str(),&maximumWidth) && maximumWidth>0,"4096 scalar boundary accepted");
    const SGPRect reversed{10,0,9,20};
    check(!sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),79,53,172,font,0,0,L"A",0,&reversed),"reversed clip refused");
    check(!sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size()-1,79,53,172,font,0,0,L"A",0),"actual allocation bounds enforced");
    check(!sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),90,53,172,font,0,0,L"A",0),"narrow pitch refused");
    check(!sgp::DrawNativeScalableTextBuffer(nullptr,actual.size(),79,53,172,font,0,0,L"A",0),"null output refused");
    check(actual==baseline,"invalid layout metadata writes nothing");
    const BYTE garbage[]={0,1,2,3,4,5};
    check(sgp::CreateNativeScalableFont(garbage,sizeof garbage,23)==-1,"malformed font refused");
    check(sgp::CreateNativeScalableFont(regular.data(),64,23)==-1,"truncated real font directory refused");
    check(sgp::CreateNativeScalableFont(nullptr,regular.size(),23)==-1,"null font input refused");
    check(sgp::CreateNativeScalableFont(regular.data(),regular.size(),0)==-1 && sgp::CreateNativeScalableFont(regular.data(),regular.size(),513)==-1,"pixel bounds refused");
    check(sgp::CreateNativeScalableFont(regular.data(),16u*1024u*1024u+1,23)==-1,"font byte budget checked before reading input");
    // Every preparation-allocation refusal must be atomic. Stop after the first
    // successful attempt; this covers actual vector allocation sites in drawing.
    unsigned allocationFailures=0;
    for (int budget=0;budget<128;++budget) {
        actual=baseline; allocationCountdown=budget;
        const bool ok=sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),79,53,172,font,0,0,L"AVTo",FROMRGB(100,200,50));
        allocationCountdown=-1;
        if (ok) break;
        ++allocationFailures; check(actual==baseline,"application OOM leaves bytes unchanged");
    }
    check(allocationFailures>=5 && allocationFailures<128,"real mask/layout allocation failures exercised through eventual success");
    const INT32 large=sgp::CreateNativeScalableFont(regular.data(),regular.size(),512);
    check(large>0,"maximum allowed pixel size accepted");
    const std::wstring excessiveMasks(300,L'A'); actual=baseline;
    check(!sgp::DrawNativeScalableTextBuffer(actual.data(),actual.size(),79,53,172,large,0,0,excessiveMasks.c_str(),0) && actual==baseline,"aggregate glyph-mask budget refusal is atomic");
    sgp::DeleteNativeScalableFont(large);
    check(InitializeVideoSurfaceManager(),"real surface manager initialized");
    VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT; desc.usWidth=79; desc.usHeight=53; desc.ubBitDepth=16;
    UINT32 surface=0; check(AddStandardVideoSurface(&desc,&surface),"real manager surface allocated");
    UINT32 pitch=0; BYTE* data=LockVideoSurface(surface,&pitch); sgp::NativeSurfaceView view;
    check(data && sgp::LockedNativeSurfaceView(data,&view),"actual locked allocation view available");
    actual=seed(pitch,53); std::memcpy(data,actual.data(),actual.size());
    check(!sgp::DrawNativeScalableText(surface,font,0,0,L"A",0),"foreign existing lock refused");
    check(sgp::LockedNativeSurfaceView(data,&view) && !ShutdownVideoSurfaceManager(),"failed draw preserves caller-owned lock");
    check(std::memcmp(data,actual.data(),actual.size())==0,"failed locked draw writes nothing");
    UnLockVideoSurface(surface);
    oracle.paint(actual,79,53,pitch,1,2,L"AV",FROMRGB(137,53,219));
    check(sgp::DrawNativeScalableText(surface,font,1,2,L"AV",FROMRGB(137,53,219)),"draw through real manager lock");
    data=LockVideoSurface(surface,&pitch); check(data!=nullptr,"successful draw released owned lock");
    check(std::memcmp(data,actual.data(),actual.size())==0,"real surface entire allocation matches independent oracle"); UnLockVideoSurface(surface);
    check(!sgp::DrawNativeScalableText(0,font,0,0,L"A",0),"invalid surface refused");
    check(!sgp::DrawNativeScalableText(surface,font,0,0,surrogate,0),"invalid layout rejected before acquiring surface");
    check(!sgp::DrawNativeScalableText(surface,font,0,0,L"A",0,&reversed),"invalid clip refused after owned manager acquisition");
    data=LockVideoSurface(surface,&pitch); check(data!=nullptr,"invalid draw leaves surface unlockable"); UnLockVideoSurface(surface);
    check(DeleteVideoSurfaceFromIndex(surface),"real surface deleted"); check(ShutdownVideoSurfaceManager(),"surface manager cleaned up");
    sgp::DeleteNativeScalableFont(font); sgp::DeleteNativeScalableFont(font);
    INT32 width=923; metrics.ascent=123;
    check(!sgp::MeasureNativeScalableText(font,L"A",&width) && width==923,"deleted handle measurement refused without output change");
    check(!sgp::GetNativeScalableFontMetrics(font,&metrics) && metrics.ascent==123,"deleted metrics unchanged");
    check(!sgp::GetNativeScalableFontColor(font,&color) && !sgp::SetNativeScalableFontColor(font,0),"stale color handle refused");
    const INT32 reused=sgp::CreateNativeScalableFont(regular.data(),regular.size(),23); check(reused>boldFont && reused!=font,"slot reuse never revives deleted handle");
    sgp::ShutdownNativeScalableFonts();
    check(!sgp::MeasureNativeScalableText(reused,L"A",&width),"shutdown invalidates all handles");
    std::vector<INT32> handles;
    for (unsigned i=0;i<32;++i) { const INT32 id=sgp::CreateNativeScalableFont(regular.data(),regular.size(),8); check(id>reused,"post-shutdown monotonic IDs and accepted capacity"); handles.push_back(id); }
    check(sgp::CreateNativeScalableFont(regular.data(),regular.size(),8)==-1,"33rd live face refused");
    sgp::DeleteNativeScalableFont(handles[10]);
    const INT32 last=sgp::CreateNativeScalableFont(regular.data(),regular.size(),8); check(last>handles.back(),"capacity recovery issues fresh handle");
    sgp::ShutdownNativeScalableFonts();
}
}
int main(int argc,char** argv) {
    try {
        suite(argc>1 ? argv[1] : "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",argc>2 ? argv[2] : "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf");
        std::cout<<"native scalable font: "<<checks<<" checks passed\n"; return 0;
    } catch (const std::exception& error) { allocationCountdown=-1; sgp::ShutdownNativeScalableFonts(); std::cerr<<"native scalable font failure after "<<checks<<" checks: "<<error.what()<<'\n'; return 1; }
}
