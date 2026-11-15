/*
 * P710 native snapshot update: tests/native-core/native_indexed_surface_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored literal indexed surface oracles. No game assets or service stubs.
#include "native_surface.h"
#include "clip_state.h"
#include <array>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool okay,const char* message)
{ ++checks; if (!okay) throw std::runtime_error(message); }
UINT32 make(UINT32 width,UINT32 height,UINT8 depth)
{
    VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    desc.usWidth=width; desc.usHeight=height; desc.ubBitDepth=depth;
    UINT32 id=0; check(AddVideoSurface(&desc,&id),"create real indexed/RGB565 surface"); return id;
}
HVSURFACE handle(UINT32 id)
{ HVSURFACE s=nullptr; check(GetVideoSurface(&s,id),"opaque live handle"); return s; }
std::vector<BYTE> capture(UINT32 id)
{
    UINT16 w,h; UINT8 depth; check(GetVideoSurfaceDescription(id,&w,&h,&depth),"capture metadata");
    UINT32 pitch; auto* data=LockVideoSurface(id,&pitch); check(data,"capture locks");
    std::vector<BYTE> result(data,data+std::size_t(pitch)*h); UnLockVideoSurface(id); return result;
}
void expect(UINT32 id,UINT8 depth,UINT16 w,UINT16 h,const std::vector<UINT16>& values)
{
    UINT16 aw,ah; UINT8 ad; check(GetVideoSurfaceDescription(id,&aw,&ah,&ad)&&aw==w&&ah==h&&ad==depth,"literal layout metadata");
    UINT32 pitch; auto* p=LockVideoSurface(id,&pitch); check(p,"expect locks");
    check(pitch>w*(depth/8),"actual padded pitch");
    for (UINT16 y=0;y<h;++y)
    {
        for (UINT16 x=0;x<w;++x)
        {
            UINT16 value=p[std::size_t(y)*pitch+x];
            if (depth==16) std::memcpy(&value,p+std::size_t(y)*pitch+x*2,2);
            check(value==values[std::size_t(y)*w+x],"literal index/palette pixel");
        }
        for (UINT32 i=w*(depth/8);i<pitch;++i) check(p[std::size_t(y)*pitch+i]==0xa5,"padding never written");
    }
    UnLockVideoSurface(id);
}
void paletteAndCopies()
{
    const UINT32 a=make(3,2,8),b=make(4,3,8),rgb=make(4,3,16);
    auto* s=handle(a); UINT32 pitch; BYTE* p=LockVideoSurface(a,&pitch);
    check(p,"indexed byte lock"); const BYTE input[6]{0,1,2,3,4,255};
    for (unsigned y=0;y<2;++y) std::memcpy(p+y*pitch,input+y*3,3);
    check(GetVideoSurfaceWidth(s)==0&&GetVideoSurfaceHeight(s)==0,"metadata getters reject active byte lock");
    std::array<SGPPaletteEntry,256> entries{},readback{};
    entries[1]={255,0,0,7}; entries[2]={0,255,0,9}; entries[3]={0,0,255,11};entries[4]={255,255,255,13};
    check(!SetVideoSurfacePalette(s,entries.data())&&!GetVSurfacePaletteEntries(s,readback.data()),"palette replacement/readback reject locks");
    UnLockVideoSurface(a); check(GetVideoSurfaceWidth(s)==3&&GetVideoSurfaceHeight(s)==2,"unlocked metadata");
    check(!GetVSurfacePaletteEntries(s,readback.data()),"new indexed surface has no invented palette");
    check(!BltVideoSurface(rgb,a,0,0,0,0,nullptr),"indexed RGB copy requires real palette");
    check(SetVideoSurfacePalette(s,entries.data()),"set actual RGB palette");
    entries[1]={0,0,255,0};
    check(GetVSurfacePaletteEntries(s,readback.data())&&readback[1].peRed==255&&readback[1].peFlags==7,"palette owns deep copy including flags");
    readback[2].peGreen=0; check(GetVSurfacePaletteEntries(s,readback.data())&&readback[2].peGreen==255,"readback cannot mutate owner");
    std::array<UINT16,256> colors{}; check(GetVideoSurfacePalette16(s,colors.data()),"get converted palette");
    check(colors[0]==0&&colors[1]==0xf800&&colors[2]==0x07e0&&colors[3]==0x001f&&colors[4]==0xffff,"literal primary palette conversion");
    check(SetVideoSurfaceTransparency(a,0)&&!SetVideoSurfaceTransparency(a,256),"indexed transparency uses bounded index");
    SGPRect all{0,0,10,10}; SetClippingRect(&all);
    check(ColorFillVideoSurfaceArea(b,0,0,4,3,9),"index fill");
    check(!ColorFillVideoSurfaceArea(b,0,0,4,3,256),"out-of-index fill fails");
    check(BltVideoSurface(b,a,0,1,1,VS_BLT_USECOLORKEY,nullptr),"actual8to8 copy with key");
    expect(b,8,4,3,{9,9,9,9,9,9,1,2,9,3,4,255});
    check(BltVideoSurface(rgb,a,0,1,1,0,nullptr),"actual8to16 copy");
    expect(rgb,16,4,3,{0,0,0,0,0,0,0xf800,0x07e0,0,0x001f,0xffff,0});
    auto before=capture(b); check(!BltVideoSurface(b,rgb,0,0,0,0,nullptr)&&capture(b)==before,"16to8 refuses before writes");
    blt_vs_fx overflowFill{}; overflowFill.ColorFill=0x10000; overflowFill.FillRect={0,0,4,3};
    check(!BltVideoSurface(b,0,0,0,0,VS_BLT_COLORFILL,&overflowFill),"indexed whole fill rejects high-word COLORVAL before narrowing");
    check(capture(b)==before,"high-word whole fill is atomic");
    check(!BltVideoSurface(b,0,0,0,0,VS_BLT_COLORFILLRECT,&overflowFill),"indexed rectangle fill rejects high-word COLORVAL before narrowing");
    check(capture(b)==before,"high-word rectangle fill is atomic");
    // Index self-overlap takes a snapshot rather than smearing source samples.
    blt_vs_fx fx{}; fx.SrcRect={0,0,2,2}; check(BltVideoSurface(a,a,0,1,0,VS_BLT_SRCSUBRECT,&fx),"indexed self-overlap");
    expect(a,8,3,2,{0,0,1,3,3,4});
    // Clip shifts placement while preserving original source coordinates.
    SGPRect clip{1,0,3,1}; SetClippingRect(&clip);
    check(BltVideoSurface(b,a,0,0,0,VS_BLT_CLIPPED,nullptr),"indexed destination clip");
    expect(b,8,4,3,{9,0,1,9,9,9,1,2,9,3,4,255});
    SGPRect src{0,0,3,2},dst{0,0,3,2}; SetClippingRect(&all);
    check(BltStretchVideoSurface(rgb,a,0,0,0,&src,&dst),"indexed stretch converts");
    expect(rgb,16,4,3,{0,0,0xf800,0,0x001f,0x001f,0xffff,0x07e0,0,0x001f,0xffff,0});
    check(!ShadowVideoSurfaceRect(a,0,0,2,1),"RGB shadow refuses indexed storage");
    blt_vs_fx rgbFill{}; rgbFill.ColorFill=0x11234;
    check(BltVideoSurface(rgb,0,0,0,0,VS_BLT_COLORFILL,&rgbFill),"RGB565 fill retains original packed-word narrowing");
    expect(rgb,16,4,3,{0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234,0x1234});
    check(DeleteVideoSurfaceFromIndex(a),"delete palette owner");
    check(!GetVSurfacePaletteEntries(s,readback.data())&&!GetVideoSurfacePalette16(s,colors.data())&&!SetVideoSurfacePalette16(s,colors.data())&&GetVideoSurfaceWidth(s)==0,"stale metadata and palette capabilities reject");
    check(!GetVSurfacePaletteEntries(reinterpret_cast<HVSURFACE>(std::uintptr_t(1)),readback.data()),"foreign handle rejects safely");
    check(DeleteVideoSurfaceFromIndex(b)&&DeleteVideoSurfaceFromIndex(rgb),"copy cleanup");
}
void legacySurfaceAdapter()
{
    const UINT32 source=make(3,2,16),destination=make(4,3,16);
    auto* src=handle(source); auto* dst=handle(destination);
    UINT32 pitch; auto* data=LockVideoSurface(source,&pitch); check(data,"adapter source fill lock");
    const UINT16 literal[6]{0,0x1111,0x2222,0x3333,0,0x4444};
    for (unsigned y=0;y<2;++y) std::memcpy(data+y*pitch,literal+y*3,6);
    UnLockVideoSurface(source);
    SGPRect all{0,0,10,10}; SetClippingRect(&all);
    check(ColorFillVideoSurfaceArea(destination,0,0,4,3,0xaaaa),"adapter destination sentinel");
    check(BltVSurfaceUsingDD(dst,src,VS_BLT_USECOLORKEY,1,1,nullptr),"adapter real keyed copy");
    expect(destination,16,4,3,{0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0xaaaa,0x1111,0x2222,0xaaaa,0x3333,0xaaaa,0x4444});
    SGPRect sub{1,0,3,2}; check(BltVSurfaceUsingDD(dst,src,0,0,0,&sub),"adapter actual source rectangle copy");
    expect(destination,16,4,3,{0x1111,0x2222,0xaaaa,0xaaaa,0,0x4444,0x1111,0x2222,0xaaaa,0x3333,0xaaaa,0x4444});
    const auto before=capture(destination);
    check(!BltVSurfaceUsingDD(dst,src,VS_BLT_USEDESTCOLORKEY,0,0,nullptr),"adapter refuses unsupported destination key");
    SGPRect bad{3,0,1,2}; check(!BltVSurfaceUsingDD(dst,src,0,0,0,&bad),"adapter refuses reversed source rectangle");
    data=LockVideoSurface(source,&pitch); check(data,"adapter source lock");
    check(!BltVSurfaceUsingDD(dst,src,0,0,0,nullptr),"adapter rejects source lock");UnLockVideoSurface(source);
    data=LockVideoSurface(destination,&pitch); check(data,"adapter destination lock");
    check(!BltVSurfaceUsingDD(dst,src,0,0,0,nullptr),"adapter rejects destination lock");UnLockVideoSurface(destination);
    check(capture(destination)==before,"adapter failed copies preserve full destination");
    check(DeleteVideoSurfaceFromIndex(source),"adapter source deletion");
    check(!BltVSurfaceUsingDD(dst,src,0,0,0,nullptr),"adapter rejects stale capability");
    check(DeleteVideoSurfaceFromIndex(destination),"adapter cleanup");
}
void baseHalfSize()
{
    const UINT32 source=make(7,5,8),destination=make(5,4,16);auto* s=handle(source);
    std::array<SGPPaletteEntry,256> entries{};check(SetVideoSurfacePalette(s,entries.data()),"base map palette");
    std::array<UINT16,256> palette{};for (unsigned i=0;i<256;++i) palette[i]=static_cast<UINT16>(0x2000+i);
    check(SetVideoSurfacePalette16(s,palette.data()),"base map colors");
    UINT32 sp,dp;auto* in=LockVideoSurface(source,&sp);auto* out=LockVideoSurface(destination,&dp);
    check(in&&out,"base map source/destination locks");
    for(unsigned y=0;y<5;++y) for(unsigned x=0;x<7;++x) in[y*sp+x]=static_cast<BYTE>(10*y+x);
    // Original full Half has no global clip. A deliberately empty clip must
    // not remove these source-aligned samples or change their placement.
    SGPRect empty{0,0,0,0};SetClippingRect(&empty);
    check(Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in,sp,1,1),"base map full Half samples even source positions");
    const std::vector<BYTE> before(out,out+dp*4);
    check(!Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in,sp,-1,0),"base map negative destination rejects");
    check(!Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in,sp,3,0),"base map destination width rejects before padding");
    check(!Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,3),"base map destination height rejects");
    check(!Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in+1,sp,0,0),"base map wrong source pointer rejects");
    check(std::memcmp(out,before.data(),before.size())==0,"invalid base map Half copies preserve allocation");
    UnLockVideoSurface(source);UnLockVideoSurface(destination);
    expect(destination,16,5,4,{0,0,0,0,0,0,0x2000,0x2002,0x2004,0,0,0x2014,0x2016,0x2018,0,0,0,0,0,0});
    check(!Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0),"base map unlocked source rejects");
    check(DeleteVideoSurfaceFromIndex(source)&&DeleteVideoSurfaceFromIndex(destination),"base map cleanup");
    const UINT32 narrow=make(1,3,8),target=make(2,2,16);auto* ns=handle(narrow);
    check(SetVideoSurfacePalette(ns,entries.data()),"sub-two base palette");
    in=LockVideoSurface(narrow,&sp);out=LockVideoSurface(target,&dp);check(in&&out,"sub-two base locks");
    check(Blt8BPPDataTo16BPPBufferHalf(reinterpret_cast<UINT16*>(out),dp,ns,in,sp,0,0),"sub-two base map is safe empty operation");
    UnLockVideoSurface(narrow);UnLockVideoSurface(target);expect(target,16,2,2,{0,0,0,0});
    check(DeleteVideoSurfaceFromIndex(narrow)&&DeleteVideoSurfaceFromIndex(target),"sub-two cleanup");
}
void halfSize()
{
    const UINT32 a=make(7,5,8),b=make(5,4,16); auto* s=handle(a);
    std::array<SGPPaletteEntry,256> palette{}; check(SetVideoSurfacePalette(s,palette.data()),"attach indexed palette");
    std::array<UINT16,256> colors{},saved{};
    for (unsigned i=0;i<256;++i) colors[i]=static_cast<UINT16>(0x1000+i);
    check(SetVideoSurfacePalette16(s,colors.data())&&GetVideoSurfacePalette16(s,saved.data()),"select deep copied RGB565 palette");
    colors.fill(0xeeee);
    UINT32 sp,dp; auto* in=LockVideoSurface(a,&sp);auto* out=LockVideoSurface(b,&dp);
    check(in&&out,"lock actual half-size source/destination");
    for (unsigned y=0;y<5;++y) for(unsigned x=0;x<7;++x) in[y*sp+x]=static_cast<BYTE>(10*y+x);
    SGPRect r{1,1,6,5};
    check(Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,1,1,&r),"odd source span samples relative top-left");
    // A valid cell recolor while the indexed byte lock remains active.
    saved[11]=0xabcd; check(SetVideoSurfacePalette16(s,saved.data()),"shade select during source byte lock");
    SGPRect one{1,1,3,3};
    check(Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0,&one),"recolor same immutable index");
    const std::vector<BYTE> before(out,out+dp*4);
    for (SGPRect malformed : {SGPRect{-1,0,3,2},SGPRect{0,0,8,2},SGPRect{0,0,2,6},SGPRect{3,1,2,3}})
        check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0,&malformed),"reject malformed source bounds");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in+1,sp,0,0,&r),"reject foreign lock pointer");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp+1,0,0,&r),"reject foreign source pitch");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,-1,0,&r),"reject negative destination");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,4,0,&r),"reject destination pixel-width overflow into padding");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,3,&r),"reject destination height overflow");
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp+2,s,in,sp,0,0,&r),"reject managed destination pitch mismatch");
    SGPRect tiny{0,0,1,1};check(Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0,&tiny),"sub-two source span is safe empty copy");
    check(std::memcmp(out,before.data(),before.size())==0,"invalid/empty half copies are atomic");
    UnLockVideoSurface(b);
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0,&r),"managed destination requires active lock");
    UnLockVideoSurface(a);
    check(!Blt8BPPDataTo16BPPBufferHalfRect(reinterpret_cast<UINT16*>(out),dp,s,in,sp,0,0,&r),"source requires active lock");
    expect(b,16,5,4,{0xabcd,0,0,0,0,0,0x100b,0x100d,0,0,0,0x101f,0x1021,0,0,0,0,0,0,0});
    expect(a,8,7,5,{0,1,2,3,4,5,6,10,11,12,13,14,15,16,20,21,22,23,24,25,26,30,31,32,33,34,35,36,40,41,42,43,44,45,46});
    check(DeleteVideoSurfaceFromIndex(a)&&DeleteVideoSurfaceFromIndex(b),"half-size cleanup");
}
}
int main()
{
    try
    {
        check(InitializeVideoSurfaceManager(),"manager initialization");
        paletteAndCopies(); halfSize(); legacySurfaceAdapter(); baseHalfSize(); check(ShutdownVideoSurfaceManager(),"manager shutdown");
        std::cout<<"PASS: "<<checks<<" indexed/palette/half-size literal checks\n";return 0;
    }
    catch(const std::exception& e)
    { std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n';return 1; }
}
