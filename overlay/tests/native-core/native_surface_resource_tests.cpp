/*
 * P710 native snapshot update: tests/native-core/native_surface_resource_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native port verification, 2026-11-12. Existing authored resources only.
#include "native_surface.h"
#include "himage.h"
#include "shading.h"
#include "clip_state.h"
#include "FileManIO.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_allocator.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
unsigned checks=0;
std::unordered_map<UINT32,std::string> surfaceContexts;
void check(bool okay, const std::string& label)
{ ++checks; if (!okay) throw std::runtime_error(label); }
void mount(const std::filesystem::path& root, const char* name)
{
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(),"read-only fixture directory");
    auto profile=std::make_unique<vfs::CVirtualProfile>(name,vfs::Path(root.u8string()),false);
    profile->addLocation(directory.get()); directory.release();
    auto* entry=profile.get(); getVFS()->getProfileStack()->pushProfile(entry); profile.release();
    for (auto it=entry->begin(); !it.end(); it.next())
        check(getVFS()->addLocation(it.value(),entry),"actual VFS mount");
}
void shutdown()
{
    ShutdownVideoSurfaceManager(); fileman::CloseAll();
    vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
UINT32 file(const std::string& name, UINT32 choice=0)
{
    VSURFACE_DESC description{};
    description.fCreateFlags=VSURFACE_CREATE_FROMFILE|VSURFACE_SYSTEM_MEM_USAGE|choice;
    // File dimensions/bpp must override these deliberately invalid defaults.
    description.usWidth=123456; description.usHeight=0; description.ubBitDepth=1;
    check(name.size()<sizeof(description.ImageFile),"fixture name fits");
    std::memcpy(description.ImageFile,name.c_str(),name.size()+1);
    UINT32 id=123; check(AddVideoSurface(&description,&id),"load actual resource "+name);
    surfaceContexts[id]=name;
    return id;
}
UINT32 blank(UINT32 width, UINT32 height)
{
    VSURFACE_DESC description{}; description.fCreateFlags=VSURFACE_CREATE_DEFAULT;
    description.usWidth=width; description.usHeight=height; description.ubBitDepth=16;
    UINT32 id=0; check(AddVideoSurface(&description,&id),"blank surface");
    surfaceContexts[id]="blank("+std::to_string(width)+"x"+std::to_string(height)+")"; return id;
}
struct Capture { UINT16 width,height; UINT32 pitch; std::vector<BYTE> bytes; };
Capture capture(UINT32 id)
{
    Capture result{}; UINT8 depth=0;
    check(GetVideoSurfaceDescription(id,&result.width,&result.height,&depth) && depth==16,"RGB565 description");
    auto* bytes=LockVideoSurface(id,&result.pitch); check(bytes!=nullptr,"capture locks");
    result.bytes.assign(bytes,bytes+std::size_t(result.pitch)*result.height); UnLockVideoSurface(id);
    return result;
}
UINT16 pixel(const Capture& capture, UINT32 x, UINT32 y)
{
    UINT16 result; std::memcpy(&result,capture.bytes.data()+std::size_t(y)*capture.pitch+x*2,2); return result;
}
void expect(UINT32 id, UINT16 width, UINT16 height, const std::vector<UINT16>& pixels)
{
    auto got=capture(id); check(got.width==width && got.height==height,"resource dimensions");
    check(pixels.size()==std::size_t(width)*height,"oracle dimensions");
    for (UINT32 y=0; y<height; ++y)
    {
        for (UINT32 x=0; x<width; ++x)
        {
            const UINT16 actual=pixel(got,x,y), expected=pixels[std::size_t(y)*width+x];
            ++checks;
            if (actual!=expected)
            {
                std::ostringstream error;
                error << "literal RGB565 pixel: " << surfaceContexts[id] << " surface=" << id
                      << " at (" << x << ',' << y << ") expected=0x" << std::hex
                      << std::setfill('0') << std::setw(4) << expected << " actual=0x"
                      << std::setw(4) << actual;
                throw std::runtime_error(error.str());
            }
        }
        for (UINT32 offset=width*2; offset<got.pitch; ++offset)
            check(got.bytes[std::size_t(y)*got.pitch+offset]==0xa5,"padded row guard unchanged");
    }
}
void indexedResources()
{
    for (const auto* name : {"indexed8.sti","indexed8_zlib.sti","padded.pcx"})
    {
        const auto id=file(name,VSURFACE_NATIVE_INDEXED8);
        HVSURFACE surface=nullptr; check(GetVideoSurface(&surface,id),"indexed resource handle");
        UINT16 w=0,h=0; UINT8 depth=0;
        check(GetVideoSurfaceDescription(id,&w,&h,&depth) && w==3 && h==2 && depth==8,"explicit indexed resource layout");
        SGPPaletteEntry entries[256]{};
        check(GetVSurfacePaletteEntries(surface,entries) && entries[1].peRed==255 && entries[2].peGreen==255,"real retained palette after image deletion");
        UINT32 pitch=0; auto* pixels=LockVideoSurface(id,&pitch); check(pixels,"indexed resource lock");
        const std::array<BYTE,6> sti{{0,1,2,3,4,5}}, pcx{{1,2,3,193,0,0}};
        const auto& expected=std::string(name)=="padded.pcx" ? pcx : sti;
        for (unsigned y=0;y<2;++y)
        {
            for (unsigned x=0;x<3;++x) check(pixels[y*pitch+x]==expected[y*3+x],"literal retained resource index");
            for (UINT32 x=3;x<pitch;++x) check(pixels[y*pitch+x]==0xa5,"indexed resource padding");
        }
        UnLockVideoSurface(id);
        const auto dest=blank(3,2);
        std::array<UINT16,256> shades{}; shades[0]=0x1111; shades[1]=0x2222; shades[2]=0x3333;
        shades[3]=0x4444; shades[4]=0x5555; shades[5]=0x6666; shades[193]=0x7777;
        check(SetVideoSurfacePalette16(surface,shades.data()),"real resource recolor");
        check(BltVideoSurface(dest,id,0,0,0,0,nullptr),"real retained index conversion");
        if (std::string(name)=="padded.pcx") expect(dest,3,2,{0x2222,0x3333,0x4444,0x7777,0x1111,0x1111});
        else expect(dest,3,2,{0x1111,0x2222,0x3333,0x4444,0x5555,0x6666});
        check(DeleteVideoSurfaceFromIndex(id)&&DeleteVideoSurfaceFromIndex(dest),"indexed resource cleanup");
    }
    const auto etrle=file("surface_indexed_key.png",VSURFACE_NATIVE_INDEXED8);
    UINT16 ew=0,eh=0; UINT8 ed=0;
    check(GetVideoSurfaceDescription(etrle,&ew,&eh,&ed)&&ew==2&&eh==1&&ed==8,"indexed ETRLE resource description");
    UINT32 ep=0; auto* ei=LockVideoSurface(etrle,&ep); check(ei&&ei[0]==0&&ei[1]==1,"ETRLE skips preserve index zero and literal index one");
    UnLockVideoSurface(etrle);
    const auto edest=blank(2,1);
    check(ColorFillVideoSurfaceArea(edest,0,0,2,1,0x1234),"indexed ETRLE sentinel");
    check(BltVideoSurface(edest,etrle,0,0,0,VS_BLT_USECOLORKEY,nullptr),"indexed ETRLE colorkey before conversion");
    expect(edest,2,1,{0x1234,0x07e0});
    check(DeleteVideoSurfaceFromIndex(etrle)&&DeleteVideoSurfaceFromIndex(edest),"indexed ETRLE cleanup");
    VSURFACE_DESC reject{}; reject.fCreateFlags=VSURFACE_CREATE_FROMFILE|VSURFACE_NATIVE_INDEXED8;
    std::strcpy(reject.ImageFile,"rgb24.tga"); check(!CreateVideoSurface(&reject),"indexed opt-in refuses true-color source");
    std::strcpy(reject.ImageFile,"bad_missing_eol.sti"); check(!CreateVideoSurface(&reject),"indexed opt-in refuses malformed source");
}
void loadedResources()
{
    for (UINT32 hint : {VSURFACE_DEFAULT_MEM_USAGE,VSURFACE_SYSTEM_MEM_USAGE})
    {
        VSURFACE_DESC description{}; description.fCreateFlags=VSURFACE_CREATE_DEFAULT|hint;
        description.usWidth=2; description.usHeight=2; description.ubBitDepth=16;
        auto* surface=CreateVideoSurface(&description);
        check(surface!=nullptr,"ordinary software-memory hint");
        check(DeleteVideoSurface(surface),"delete direct hinted surface");
    }
    for (const auto* name : {"chooser.sti","only.sti"})
    {
        const auto id=file(name,VSURFACE_CREATE_FROMPNG_FALLBACK);
        expect(id,2,1,{0xf800,0x07e0}); check(DeleteVideoSurfaceFromIndex(id),"delete PNG replacement");
    }
    for (const auto* name : {"fallback.sti","indexed8.sti","indexed8_zlib.sti"})
    {
        const auto id=file(name,VSURFACE_CREATE_FROMPNG_FALLBACK);
        expect(id,3,2,{0,0xf800,0x07e0,0x001f,0xffff,0x0001});
        check(DeleteVideoSurfaceFromIndex(id),"delete indexed surface");
    }
    const auto pcx=file("padded.pcx"); expect(pcx,3,2,{0xf800,0x07e0,0x001f,0,0,0});
    check(DeleteVideoSurfaceFromIndex(pcx),"delete PCX");
    for (const auto* name : {"rgb16.tga","rgb24.tga","top.tga","right.tga","top_right.tga"})
    {
        const auto id=file(name); expect(id,3,2,{0xf800,0x07e0,0xffe0,0x001f,0xffff,0});
        check(DeleteVideoSurfaceFromIndex(id),"delete TGA");
    }
    for (const auto* name : {"rgb16.sti","rgb16_zlib.sti","rgb24.sti","rgb24_zlib.sti","rgb32.sti","rgb32_zlib.sti"})
    {
        const auto id=file(name); expect(id,3,2,{0,0xf800,0x07e0,0x001f,0xffff,0xffe0});
        check(DeleteVideoSurfaceFromIndex(id),"delete true-color STCI");
    }
    const auto indexed=file("indexed.png");
    expect(indexed,4,2,{0,0xf800,0xf800,0,0x07e0,0,0x001f,0x001f});
    check(DeleteVideoSurfaceFromIndex(indexed),"delete indexed PNG after image owner destroyed");
    // The PNG encoder treats index zero as transparent even when that palette
    // entry is red. ETRLE skip runs must retain the surface's zero key.
    const auto indexedKey=file("surface_indexed_key.png");
    expect(indexedKey,2,1,{0,0x07e0});
    const auto keyedDest=blank(2,1);
    check(ColorFillVideoSurfaceArea(keyedDest,0,0,2,1,0x1234),"keyed destination sentinel");
    check(BltVideoSurface(keyedDest,indexedKey,0,0,0,VS_BLT_USECOLORKEY,nullptr),
          "transparent ETRLE run respects surface color key");
    expect(keyedDest,2,1,{0x1234,0x07e0});
    check(DeleteVideoSurfaceFromIndex(indexedKey) && DeleteVideoSurfaceFromIndex(keyedDest),
          "delete ETRLE keyed surfaces");
    const auto rgb=file("rgb.png"); expect(rgb,3,1,{0xf800,0x07e0,0x001f});
    check(DeleteVideoSurfaceFromIndex(rgb),"delete RGB PNG");
    const auto rgba=file("rgba.png"); expect(rgba,2,1,{0x0042,0x5bda});
    check(DeleteVideoSurfaceFromIndex(rgba),"delete RGBA PNG alpha-composited over black");
    // Source-backed alpha oracle: original cache truncates 78/255 times
    // [12,34,56] to [3,10,17], then RGB565 produces 0x0042. Alpha zero must
    // skip even a brightly colored source; full alpha retains its RGB565 value.
    const auto alphas=file("surface_rgba.png"); expect(alphas,3,1,{0,0x0042,0x5bda});
    check(DeleteVideoSurfaceFromIndex(alphas),"delete zero/partial/full-alpha fixture");
    const auto jpcRgba=file("single_rgba.jpc.7z"); expect(jpcRgba,2,1,{0x0042,0x5bda});
    check(DeleteVideoSurfaceFromIndex(jpcRgba),"JPC RGBA uses the same original alpha mapping");
}
void stretch()
{
    const auto source=file("rgb16.sti"); const auto dest=blank(7,5);
    SGPRect src{0,0,3,2}, dst{-1,-1,5,3};
    // Placement rectangles own the translation; original DDBlt ignores x/y.
    check(BltStretchVideoSurface(dest,source,123,-45,0,&src,&dst),"stretch clipped to physical surface");
    expect(dest,7,5,{0,0xf800,0xf800,0x07e0,0x07e0,0,0,
                    0x001f,0xffff,0xffff,0xffe0,0xffe0,0,0,
                    0x001f,0xffff,0xffff,0xffe0,0xffe0,0,0,
                    0,0,0,0,0,0,0, 0,0,0,0,0,0,0});
    const auto before=capture(dest);
    UINT32 pitch=0; auto* locked=LockVideoSurface(source,&pitch); check(locked!=nullptr,"lock source");
    check(!BltStretchVideoSurface(dest,source,0,0,0,&src,&dst),"locked source rejects stretch");
    UnLockVideoSurface(source); check(capture(dest).bytes==before.bytes,"locked source has no partial writes");
    locked=LockVideoSurface(dest,&pitch); check(locked!=nullptr,"lock destination");
    check(!BltStretchVideoSurface(dest,source,0,0,0,&src,&dst),"locked destination rejects stretch");
    UnLockVideoSurface(dest); check(capture(dest).bytes==before.bytes,"locked destination has no partial writes");
    for (UINT32 flags : {VS_BLT_COLORFILL,VS_BLT_FAST,VS_BLT_MIRROR_Y,0x80000000u})
        check(!BltStretchVideoSurface(dest,source,0,0,flags,&src,&dst),"unsupported stretch flag rejects");
    SGPRect reverse{2,0,1,2};
    check(!BltStretchVideoSurface(dest,source,0,0,0,&reverse,&dst),"reversed source rejects");
    check(!BltStretchVideoSurface(dest,source,0,0,0,nullptr,&dst),"null rectangle rejects");
    check(capture(dest).bytes==before.bytes,"invalid stretch operations have no partial writes");
    // Global clipping must retain original scale origin and never mutate it.
    SGPRect clip{2,1,4,3}; SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(dest,0,0,7,5,0x1234),"fill clip sentinel");
    const auto clippedBefore=capture(dest); dst={0,0,6,4};
    check(BltStretchVideoSurface(dest,source,0,0,VS_BLT_CLIPPED,&src,&dst),"stretch global clip");
    auto got=capture(dest);
    for (unsigned y=0; y<5; ++y) for (unsigned x=0; x<7; ++x)
    {
        UINT16 expected=pixel(clippedBefore,x,y);
        if (x>=2 && x<4 && y>=1 && y<3) expected=y==1 ? 0xf800 : 0xffff;
        check(pixel(got,x,y)==expected,"global clip literal mapping");
    }
    check(std::memcmp(&ClippingRect,&clip,sizeof(clip))==0,"stretch retains global clip");
    clip={0,0,7,5}; SetClippingRect(&clip);
    // Out-of-source sample columns remain untouched; destination geometry stays fixed.
    check(ColorFillVideoSurfaceArea(dest,0,0,7,5,0x1234),"reset destination");
    src={-1,0,3,2}; dst={0,0,4,2};
    check(BltStretchVideoSurface(dest,source,0,0,0,&src,&dst),"out-of-source stretch clips samples");
    got=capture(dest);
    check(pixel(got,0,0)==0x1234 && pixel(got,1,0)==0 && pixel(got,2,0)==0xf800 &&
          pixel(got,3,0)==0x07e0,"source clipping retains original mapping");
    check(SetVideoSurfaceTransparency(source,FROMRGB(0,0,0)),"set stretch color key");
    src={0,0,3,2}; dst={0,0,6,4};
    check(BltStretchVideoSurface(dest,source,0,0,VS_BLT_USECOLORKEY,&src,&dst),"stretch source key");
    got=capture(dest); check(pixel(got,0,0)==0x1234 && pixel(got,1,0)==0,"zero pixels preserve destination");
    // A self-overlap must read the original pixels, including zero source samples.
    const auto self=file("rgb16.sti"); src={0,0,2,2}; dst={1,0,3,2};
    check(BltStretchVideoSurface(self,self,0,0,0,&src,&dst),"self-alias stretch");
    expect(self,3,2,{0,0,0xf800,0x001f,0x001f,0xffff});
    // INT32-wide rectangles must not overflow the interpolation multiply.
    src={std::numeric_limits<INT32>::min(),0,std::numeric_limits<INT32>::max(),1};
    dst=src;
    check(BltStretchVideoSurface(self,self,0,0,0,&src,&dst),"full-width integer rectangle");
    expect(self,3,2,{0,0,0xf800,0x001f,0x001f,0xffff});
    check(DeleteVideoSurfaceFromIndex(self) && DeleteVideoSurfaceFromIndex(source) &&
          DeleteVideoSurfaceFromIndex(dest),"delete stretch surfaces");
}
void shadow()
{
    BuildIntensityTable();
    check(IntensityTable[0xffff]==0xce59 && IntensityTable[0xf800]==0xc800 &&
          IntensityTable[0x07e0]==0x0640 && IntensityTable[0x001f]==0x0019,
          "original BuildIntensityTable literal RGB565 mapping");
    const auto id=file("rgb16.sti"); SGPRect clip{0,0,3,2}; SetClippingRect(&clip);
    check(ShadowVideoSurfaceRectUsingLowPercentTable(id,0,0,2,1),"original inclusive shadow endpoints");
    expect(id,3,2,{0,0xc800,0x0640,0x0019,0xce59,0xce40});
    auto before=capture(id); UINT32 pitch=0;
    check(LockVideoSurface(id,&pitch)!=nullptr,"lock shadow target");
    check(!ShadowVideoSurfaceRectUsingLowPercentTable(id,0,0,2,1),"shadow locked target fails");
    UnLockVideoSurface(id); check(capture(id).bytes==before.bytes,"failed shadow no partial writes");
    check(!ShadowVideoSurfaceRectUsingLowPercentTable(id,0,0,0,1),"original zero-width span rejected");
    check(!ShadowVideoSurfaceRectUsingLowPercentTable(id,0,1,2,0),"reversed shadow rejected");
    const auto clipped=file("rgb16.sti"); clip={1,0,2,2}; SetClippingRect(&clip);
    check(ShadowVideoSurfaceRectUsingLowPercentTable(clipped,-10,-10,100,100),"shadow surface/global clipping");
    expect(clipped,3,2,{0,0xc800,0x07e0,0x001f,0xce59,0xffe0});
    check(std::memcmp(&ClippingRect,&clip,sizeof(clip))==0,"shadow retains global clip");
    check(DeleteVideoSurfaceFromIndex(id) && DeleteVideoSurfaceFromIndex(clipped),"delete shadow surfaces");
}
void failures()
{
    const auto retained=file("fallback.sti"); const auto before=capture(retained);
    const auto marker=blank(2,2);
    for (const auto* name : {"broken.sti","missing.sti","invalid.png","bad_short.pcx","bad_short.tga",
                             "bad_crc.png","bad_truncated.png","bad_short_header.sti","bad_flags_none.sti",
                             "sprite.sti","frames.jpc.7z"})
        for (unsigned repeat=0; repeat<3; ++repeat)
        {
            VSURFACE_DESC description{};
            description.fCreateFlags=VSURFACE_CREATE_FROMFILE|VSURFACE_SYSTEM_MEM_USAGE|VSURFACE_CREATE_FROMPNG_FALLBACK;
            std::strcpy(description.ImageFile,name);
            UINT32 id=999;
            check(!AddVideoSurface(&description,&id) && id==0,"malformed/unsupported load rolls back");
            check(capture(retained).bytes==before.bytes,"load failure preserves existing pixels and guards");
        }
    VSURFACE_DESC invalid{}; invalid.fCreateFlags=VSURFACE_CREATE_FROMFILE;
    std::memset(invalid.ImageFile,'x',sizeof(invalid.ImageFile));
    check(!CreateVideoSurface(&invalid),"unterminated filename rejects");
    std::strcpy(invalid.ImageFile,"fallback.sti");
    for (UINT32 extra : {VSURFACE_VIDEO_MEM_USAGE,VSURFACE_RESERVED_SURFACE,VSURFACE_CREATE_DEFAULT,
                        UINT32(VSURFACE_CREATE_FROMPNG|VSURFACE_CREATE_FROMPNG_FALLBACK),0x80000000u})
    {
        invalid.fCreateFlags=VSURFACE_CREATE_FROMFILE|extra;
        check(!CreateVideoSurface(&invalid),"invalid/unsupported creation flags reject");
    }
    const auto next=blank(2,2); check(next==marker+1,"failed loads consume no surface handles");
    HVSURFACE stale=nullptr; check(GetVideoSurface(&stale,retained),"borrow resource handle");
    check(DeleteVideoSurfaceFromIndex(retained),"delete retained resource");
    UINT32 pitch=99; check(!LockVideoSurfaceBuffer(stale,&pitch) && pitch==0,"resource pointer tombstone remains invalid");
    check(DeleteVideoSurfaceFromIndex(marker) && DeleteVideoSurfaceFromIndex(next),"cleanup failure tests");
}
}
int main(int argc, char** argv)
{
    try
    {
        if (argc!=4) throw std::runtime_error("expected resource, PNG/JPC and STCI generated fixture directories");
        const std::array<const char*,3> manifests{{"resource_manifest.json","png_manifest.json","image_manifest.json"}};
        for (int i=1; i<=3; ++i)
        {
            const auto root=std::filesystem::canonical(argv[i]);
            check(std::filesystem::is_regular_file(root/manifests[std::size_t(i-1)]),"existing authored fixture input");
            mount(root, i==1 ? "surfaces-resources" : (i==2 ? "surfaces-png" : "surfaces-stci"));
        }
        check(InitializeVideoSurfaceManager(),"surface manager init");
        // The production clip starts empty. Establish the test viewport before
        // using the original clipped fill API to prepare sentinel destinations.
        SGPRect initialClip{0,0,640,480}; SetClippingRect(&initialClip);
        loadedResources(); indexedResources(); stretch(); shadow(); failures();
        check(ShutdownVideoSurfaceManager(),"surface manager shutdown"); shutdown();
        std::cout << "PASS: " << checks << " resource-backed surface/stretch/shadow checks\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n'; shutdown(); return 1;
    }
}
