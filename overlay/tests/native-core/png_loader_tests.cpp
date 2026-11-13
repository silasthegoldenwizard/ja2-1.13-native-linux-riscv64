/*
 * P710 native notice preparation: tests/native-core/png_loader_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add PNG/JPC pixel, descriptor and malformed-resource tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include "PngLoader.h"
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
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace
{
unsigned checks = 0;
void check(bool passed, const std::string& label)
{
    ++checks; if (!passed) throw std::runtime_error(label);
}
struct Image
{
    image_type value{};
    explicit Image(const std::string& name)
    { check(name.size() < sizeof(value.ImageFile), "fixture filename bound"); std::memcpy(value.ImageFile, name.c_str(), name.size() + 1); }
    ~Image() { ReleaseImageData(&value, IMAGE_ALLDATA); }
    bool load(UINT16 mask = IMAGE_ALLDATA)
    { return std::string(value.ImageFile).find(".7z") == std::string::npos ? LoadPNGFileToImage(&value, mask) : LoadJPCFileToImage(&value, mask); }
};
const std::array<UINT8, 13> indexed{{0x81,2,1,1,0x81,0,1,2,0x81,2,3,3,0}};
const std::array<UINT8, 8> rgba{{12,34,56,78,90,123,210,255}};
const std::array<UINT16, 3> rgb{{0xf800,0x07e0,0x001f}};
void verifyPalette(const image_type& image)
{
    check(image.pPalette && (image.fFlags & IMAGE_PALETTE), "real indexed palette owned");
    check(image.pPalette[0].peRed == 0 && image.pPalette[1].peRed == 255 && image.pPalette[2].peGreen == 255 && image.pPalette[3].peBlue == 255, "literal RGB palette entries");
    for (unsigned i=4;i<256;++i) check(!image.pPalette[i].peRed && !image.pPalette[i].peGreen && !image.pPalette[i].peBlue && !image.pPalette[i].peFlags, "zero filled fixture palette");
}
void full()
{
    for (const char* name : {"indexed.png", "single_indexed.jpc.7z", "trailing.png", "micrometer.png"})
    {
        Image image(name); check(image.load(), name);
        const auto& v = image.value;
        check(v.usWidth == 4 && v.usHeight == 2 && v.ubBitDepth == 8 && v.usNumberOfObjects == 1, "indexed dimensions and descriptor");
        check(v.fFlags == (IMAGE_BITMAPDATA|IMAGE_TRLECOMPRESSED|IMAGE_PALETTE), "indexed exact flags");
        check(v.uiSizePixData == indexed.size() && std::memcmp(v.pImageData,indexed.data(),indexed.size())==0,"independent literal ETRLE bytes and row terminators");
        check(v.pETRLEObject[0].sOffsetX == -2 && v.pETRLEObject[0].sOffsetY == 3 && v.pETRLEObject[0].uiDataLength == indexed.size(),"PNG signed offsets");
        verifyPalette(v);
    }
    for (const char* name : {"rgb.png", "single_rgb.jpc.7z"})
    {
        Image image(name); check(image.load(), name); const auto& v=image.value;
        check(v.ubBitDepth==16 && v.usWidth==3 && v.usHeight==1 && v.usNumberOfObjects==1,"RGB native16 descriptor");
        check(v.fFlags==IMAGE_BITMAPDATA && v.uiSizePixData==6 && std::memcmp(v.pImageData,rgb.data(),6)==0,"literal RGB565 conversion and ownership");
    }
    for (const char* name : {"rgba.png", "single_rgba.jpc.7z"})
    {
        Image image(name); check(image.load(), name); const auto& v=image.value;
        check(v.ubBitDepth==32 && v.uiSizePixData==8 && std::memcmp(v.pImageData,rgba.data(),8)==0,"RGBA original literal byte representation");
        check(v.usNumberOfObjects==1 && v.pETRLEObject[0].usWidth==2 && v.pETRLEObject[0].usHeight==1,"RGBA descriptor");
        if(std::string(name).find("7z")!=std::string::npos) check(v.pETRLEObject[0].sOffsetX==0 && v.pETRLEObject[0].sOffsetY==6 && v.uiAppDataSize==16,"single member XML plus original missing-axis override semantics");
    }
    Image one("one.png"); check(one.load(),"one pixel PNG");
    const UINT8 blank[]{0x81,0}; check(one.value.uiSizePixData==2 && std::memcmp(one.value.pImageData,blank,2)==0,"one transparent pixel uses bounded encoder");
    Image longRun("long.png"); check(longRun.load(),"127 run boundary PNG");
    std::vector<UINT8> expected{127}; expected.insert(expected.end(),127,1); expected.insert(expected.end(),{1,1,0x82,0});
    check(longRun.value.uiSizePixData==expected.size() && std::memcmp(longRun.value.pImageData,expected.data(),expected.size())==0,"127 and128 opaque runs split literally");
}
void frames()
{
    Image image("frames.jpc.7z"); check(image.load(),"numeric JPC frames with XML"); const auto& v=image.value;
    check(v.ubBitDepth==8 && v.usWidth==640 && v.usHeight==480 && v.usNumberOfObjects==3,"original multi-frame JPC canvas and count");
    check(v.uiSizePixData==19 && v.fFlags==(IMAGE_ALLDATA|IMAGE_TRLECOMPRESSED),"multi-frame flags and encoded byte count");
    check(std::memcmp(v.pImageData,indexed.data(),indexed.size())==0,"numeric0 first");
    const UINT8 ordered[]{1,2,0,1,3,0}; check(std::memcmp(v.pPixData8+13,ordered,6)==0,"numeric2 sorts before10 independent of archive order");
    const auto* obj=v.pETRLEObject;
    check(obj[0].sOffsetX==-7 && obj[0].sOffsetY==8 && obj[0].uiDataOffset==0 && obj[0].uiDataLength==13,"XML0 offsets");
    check(obj[1].sOffsetX==1 && obj[1].sOffsetY==-1 && obj[1].uiDataOffset==13 && obj[1].uiDataLength==3,"PNG2 retained offsets");
    check(obj[2].sOffsetX==5 && obj[2].sOffsetY==0 && obj[2].uiDataOffset==16,"XML2 override uses compact numeric order");
    const UINT8 app0[]{9,2,0x34,0x12,0,0,0,1,2,0x23,0,0,0,0,0,0};
    check(sizeof(AuxObjectData)==16 && v.uiAppDataSize==48 && std::memcmp(v.pAppData,app0,16)==0,"literal AuxObjectData bytes including unused bytes");
    for(unsigned i=16;i<48;++i) check(v.pAppData[i]==0,"unmentioned AuxObjectData defaults");
    verifyPalette(v);
}
void masks()
{
    for(const char* name:{"indexed.png","rgb.png","rgba.png","frames.jpc.7z","single_rgba.jpc.7z"})
    for(unsigned bits=0;bits<8;++bits)
    {
        const UINT16 mask=static_cast<UINT16>(((bits&1)?IMAGE_PALETTE:0)|((bits&2)?IMAGE_BITMAPDATA:0)|((bits&4)?IMAGE_APPDATA:0));
        const bool indexedType=std::string(name)=="indexed.png" || std::string(name)=="frames.jpc.7z";
        const bool app=std::string(name).find("7z")!=std::string::npos;
        Image fresh(name); check(fresh.load(mask),"requested sections accepted"); const auto& v=fresh.value;
        check(bool(v.pPalette)==bool(indexedType && (mask&IMAGE_PALETTE)),"no unrequested palette allocation");
        check(bool(v.pImageData)==bool(mask&IMAGE_BITMAPDATA) && bool(v.pETRLEObject)==bool(mask&IMAGE_BITMAPDATA),"no unrequested pixel/object allocations");
        check(bool(v.pAppData)==bool(app && (mask&IMAGE_APPDATA)),"no unrequested app allocation");
        check((v.fFlags&IMAGE_ALLDATA)==(mask & static_cast<UINT16>((indexedType?IMAGE_PALETTE:0)|IMAGE_BITMAPDATA|(app?IMAGE_APPDATA:0))),"exact requested flags");
        Image incremental(name); check(incremental.load(),"load all before independent subset reload");
        auto* palette=incremental.value.pPalette; auto* pixels=incremental.value.pImageData; auto* apps=incremental.value.pAppData;
        check(incremental.load(mask),"incremental reload succeeds");
        if(!(mask&IMAGE_PALETTE)) check(incremental.value.pPalette==palette,"unrequested palette survives");
        if(!(mask&IMAGE_BITMAPDATA)) check(incremental.value.pImageData==pixels,"unrequested pixels survive");
        if(!(mask&IMAGE_APPDATA)) check(incremental.value.pAppData==apps,"unrequested app survives");
        check(ReleaseImageData(&incremental.value,IMAGE_ALLDATA) && ReleaseImageData(&incremental.value,IMAGE_ALLDATA),"repeated independent section release");
    }
}
void malformed(const fs::path& root)
{
    for(const auto& entry:fs::directory_iterator(root))
    {
        const auto name=entry.path().filename().string(); if(name.rfind("bad_",0)!=0) continue;
        for(UINT16 mask:{UINT16(0),UINT16(IMAGE_PALETTE),UINT16(IMAGE_BITMAPDATA),UINT16(IMAGE_APPDATA),UINT16(IMAGE_ALLDATA)})
        {
            Image fresh(name); check(!fresh.load(mask),"reject malformed "+name);
            check(!fresh.value.fFlags && !fresh.value.pImageData && !fresh.value.pPalette && !fresh.value.pAppData && !fresh.value.pETRLEObject,"malformed fresh leaves empty ownership");
            Image existing("indexed.png"); check(existing.load(),"existing before malformed transaction");
            std::memcpy(existing.value.ImageFile,name.c_str(),name.size()+1);
            auto before=existing.value;
            check(!existing.load(mask),"malformed reload rejected");
            check(std::memcmp(&before,&existing.value,sizeof(before))==0,"malformed reload leaves every HIMAGE byte unchanged");
            check(std::memcmp(existing.value.pImageData,indexed.data(),indexed.size())==0,"prior pixels still valid");
        }
    }
    Image invalid("indexed.png"); check(!invalid.load(0x8000),"unknown requested flag rejection");
    check(!LoadPNGFileToImage(nullptr,IMAGE_ALLDATA) && !LoadJPCFileToImage(nullptr,IMAGE_ALLDATA),"null HIMAGE rejection");
    std::memset(invalid.value.ImageFile,'x',sizeof(invalid.value.ImageFile));
    check(!LoadPNGFileToImage(&invalid.value,IMAGE_ALLDATA),"unterminated filename rejection");
}
void shutdown()
{ fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear(); }
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=2) throw std::runtime_error("expected PNG fixture directory");
        const auto root=fs::canonical(argv[1]);
        auto tree=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(tree->init(),"initialize actual fixture directory tree");
        auto profile=std::make_unique<vfs::CVirtualProfile>("png",vfs::Path(root.u8string()),false);
        profile->addLocation(tree.get()); tree.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(mounted); profile.release();
        for(auto it=mounted->begin();!it.end();it.next()) check(getVFS()->addLocation(it.value(),mounted),"mount actual VFS location");
        ConfigureRGB565ImageFormat(); full(); frames(); masks(); malformed(root); shutdown();
        std::cout<<"PASS: "<<checks<<" PNG/JPC production loader checks\n"; return 0;
    }
    catch(const std::exception& error) { std::cerr<<"FAIL after "<<checks<<": "<<error.what()<<'\n'; shutdown(); return 1; }
}
