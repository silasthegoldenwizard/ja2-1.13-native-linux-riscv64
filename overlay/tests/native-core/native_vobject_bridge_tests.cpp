/*
 * P710 native snapshot update: tests/native-core/native_vobject_bridge_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeVideoObjectBridge.h"
#include "clip_state.h"
#include "shading.h"
#include <array>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
unsigned checks=0;
void check(bool condition,const char* message)
{ ++checks;if(!condition)throw std::runtime_error(message); }
struct Pixel { unsigned x,y;UINT16 color; };
struct Canvas {
    UINT32 id=0,pitch=0;
    static constexpr unsigned width=7,height=4;
    Canvas() {
        VSURFACE_DESC description{};description.fCreateFlags=VSURFACE_CREATE_DEFAULT;
        description.usWidth=width;description.usHeight=height;description.ubBitDepth=16;
        check(AddVideoSurface(&description,&id),"real native surface creation");
        BYTE* data=LockVideoSurface(id,&pitch);check(data,"initial destination lock");UnLockVideoSurface(id);
    }
    ~Canvas() {DeleteVideoSurfaceFromIndex(id);}
    std::vector<BYTE> reset(UINT16 color=0x2222) {
        UINT32 actual=0;BYTE* data=LockVideoSurface(id,&actual);check(data&&actual==pitch,"reset lock/pitch");
        std::memset(data,0xa5,pitch*height);
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x)
            std::memcpy(data+y*pitch+x*2,&color,2);
        std::vector<BYTE> result(data,data+pitch*height);UnLockVideoSurface(id);return result;
    }
    void expect(std::vector<BYTE> expected,std::initializer_list<Pixel> changes={}) {
        for(const auto& pixel:changes)std::memcpy(expected.data()+pixel.y*pitch+pixel.x*2,&pixel.color,2);
        UINT32 actual=0;BYTE* data=LockVideoSurface(id,&actual);check(data&&actual==pitch,"readback proves bridge releases its lock");
        check(!std::memcmp(data,expected.data(),expected.size()),"independent literal pixels and all row padding match");
        UnLockVideoSurface(id);
    }
};
struct ManagedObject {
    UINT32 id=0;HVOBJECT object=nullptr;
    explicit ManagedObject(image_type& image) {
        VOBJECT_DESC description{};description.fCreateFlags=VOBJECT_CREATE_FROMHIMAGE;description.hImage=&image;
        check(AddStandardVideoObject(&description,&id)&&GetVideoObject(&object,id),"actual deep-copying resource manager");
    }
    ~ManagedObject() {DeleteVideoObjectFromIndex(id);}
};
void test() {
    check(InitializeVideoSurfaceManager()&&InitializeVideoObjectManager(),"actual managers initialize");
    {
        Canvas c;
        SGPRect full{0,0,7,4};SetClippingRect(&full);
        std::array<SGPPaletteEntry,256> palette{};
        palette[0]={0,0,255,0};palette[1]={255,0,0,0};palette[2]={0,255,0,0};
        palette[3]={255,255,0,0};palette[4]={255,255,255,0};palette[254]={255,0,255,0};
        std::array<UINT8,16> bytes{4,0,1,2,254,0x81,1,3,0,4,4,1,2,254,0x82,0};
        ETRLEObject frame{0,UINT32(bytes.size()),0,0,2,6};
        image_type image{};image.ubBitDepth=8;image.fFlags=IMAGE_BITMAPDATA|IMAGE_TRLECOMPRESSED|IMAGE_PALETTE;
        image.pPixData8=bytes.data();image.uiSizePixData=bytes.size();image.pETRLEObject=&frame;image.usNumberOfObjects=1;image.pPalette=palette.data();
        ManagedObject sprite(image);
        check(sprite.object->pPixData!=bytes.data()&&sprite.object->pETRLEObject!=&frame,"source allocations are production owned");
        // Deliberately mutate caller-owned input after creation. Draws must use
        // the production manager's original copied pixels and palette.
        bytes.fill(0xff);palette.fill({});
        auto before=c.reset();
        SGPRectangle region{1,1,1,1};
        check(BltVideoObject(c.id,sprite.object,0,region,VO_BLT_SRCTRANSPARENCY,nullptr),"real rectangle overload");
        c.expect(before,{{1,1,0x001f},{2,1,0xf800},{3,1,0x07e0},{4,1,0xf81f},{6,1,0xffe0},
                         {1,2,0xffff},{2,2,0xf800},{3,2,0x07e0},{4,2,0xf81f}});
        before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,2,1),"override crops, does not scale");
        c.expect(before,{{0,0,0x001f},{1,0,0xf800}});
        before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,65535,65535),"oversize override clamps to source");
        c.expect(before,{{0,0,0x001f},{1,0,0xf800},{2,0,0x07e0},{3,0,0xf81f},{5,0,0xffe0},
                         {0,1,0xffff},{1,1,0xf800},{2,1,0x07e0},{3,1,0xf81f}});
        // Preserve source's peculiar global-clipped branch: the override crop
        // is ignored and a complete sprite is drawn through the global clip.
        SGPRect strip{1,0,7,4};SetClippingRect(&strip);before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,2,1),"original override global-clipped branch");
        c.expect(before,{{1,0,0xf800},{2,0,0x07e0},{3,0,0xf81f},{5,0,0xffe0},
                         {1,1,0xf800},{2,1,0x07e0},{3,1,0xf81f}});
        SetClippingRect(&full);sprite.object->pETRLEObject[0].sOffsetX=1;before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,3,1),"offset crop uses original position-based subArea");
        c.expect(before,{{1,0,0x001f},{2,0,0xf800}});
        check(sprite.object->pETRLEObject[0].usWidth==6&&sprite.object->pETRLEObject[0].usHeight==2,"override never mutates source metadata");
        sprite.object->pETRLEObject[0].sOffsetX=0;
        ShadeTable[0x2222]=0x1111;
        before=c.reset();
        check(BltVideoObjectOutlineShadow(c.id,sprite.object,0,0,0),"unclipped direct outline shadow");
        c.expect(before,{{0,0,0x1111},{1,0,0x1111},{2,0,0x1111},{5,0,0x1111},
                         {0,1,0x1111},{1,1,0x1111},{2,1,0x1111}});
        SetClippingRect(&strip);before=c.reset();
        check(BltVideoObjectOutlineShadowFromIndex(c.id,sprite.id,0,0,0),"clipped indexed outline shadow");
        c.expect(before,{{1,0,0x1111},{2,0,0x1111},{3,0,0x1111},{5,0,0x1111},
                         {1,1,0x1111},{2,1,0x1111},{3,1,0x1111}});
        // Global-unclipped but actual surface-clipped must still exclude254.
        SGPRect wider{0,0,100,100};SetClippingRect(&wider);before=c.reset();
        check(BltVideoObjectOutlineShadow(c.id,sprite.object,0,3,3),"actual surface bounds preserve unclipped marker semantics");
        c.expect(before,{{3,3,0x1111},{4,3,0x1111},{5,3,0x1111}});
        SetClippingRect(&full);before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SHADOW,nullptr,4,1),"shadow override production dispatch");
        c.expect(before,{{0,0,0x1111},{1,0,0x1111},{2,0,0x1111},{3,0,0x1111}});
        before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SHADOW|VO_BLT_SRCTRANSPARENCY,nullptr,2,1),"transparency takes precedence over shadow");
        c.expect(before,{{0,0,0x001f},{1,0,0xf800}});
        // Late-row damage must reject before the otherwise valid first row.
        before=c.reset();auto* owned=static_cast<UINT8*>(sprite.object->pPixData);owned[15]=7;
        check(!BltVideoObjectOutlineShadow(c.id,sprite.object,0,0,0),"malformed later row rejected before writes");c.expect(before);
        check(!BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,1,1),"crop preflight validates complete encoding");c.expect(before);
        check(!BltVideoObjectOutlineShadow(c.id,sprite.object,0,100,100),"fully clipped malformed encoding still rejected");c.expect(before);owned[15]=0;
        check(!BltVideoObjectFromIndexWithOverride(c.id,sprite.id,1,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,1,1),"bad subimage index");c.expect(before);
        check(!BltVideoObjectFromIndexWithOverride(c.id,0xfffffff0,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,1,1),"bad manager index releases acquired lock");c.expect(before);
        check(!BltVideoObjectOutlineShadowFromIndex(c.id,0xfffffff0,0,0,0),"bad outline manager index releases lock");c.expect(before);
        check(!BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,0,nullptr,1,1),"unsupported8-bit no-effect flags rejected honestly");c.expect(before);
        check(!BltVideoObjectOutlineShadow(c.id,nullptr,0,0,0),"null direct source");c.expect(before);
        UINT32 pitch=0;BYTE* locked=LockVideoSurface(c.id,&pitch);check(locked,"caller destination lock");
        check(!BltVideoObjectOutlineShadow(c.id,sprite.object,0,0,0)&&
              !BltVideoObjectFromIndexWithOverride(c.id,sprite.id,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr,1,1),"exclusive caller lock rejected");
        check(!LockVideoSurface(c.id,&pitch),"failed bridge cannot unlock caller lock");
        UnLockVideoSurface(c.id);c.expect(before);
        SGPRect inverted{4,0,1,2};SetClippingRect(&inverted);
        check(!BltVideoObjectOutlineShadow(c.id,sprite.object,0,0,0),"invalid clipping rejected");c.expect(before);
        SetClippingRect(&full);
        before=c.reset();
        const UINT32 originalLength=sprite.object->pETRLEObject[0].uiDataLength;
        sprite.object->pETRLEObject[0].uiDataLength=1;
        check(!BltVideoObjectOutlineShadowFromIndex(c.id,sprite.id,0,0,0),"truncated encoded metadata fails atomically");c.expect(before);
        sprite.object->pETRLEObject[0].uiDataLength=originalLength;
        void* savedPixels=sprite.object->pPixData;
        locked=LockVideoSurface(c.id,&pitch);check(locked,"obtain real alias storage");UnLockVideoSurface(c.id);
        sprite.object->pPixData=locked;
        check(!BltVideoObjectOutlineShadow(c.id,sprite.object,0,0,0),"encoded source/destination alias rejected before decoding");c.expect(before);
        sprite.object->pPixData=savedPixels;
        check(BltVideoObjectOutlineShadow(c.id,sprite.object,0,INT32_MAX,INT32_MIN),"extreme fully clipped positions are safe");c.expect(before);
        check(!BltVideoObjectOutlineShadow(0xffffffe0u,sprite.object,0,0,0),"unknown destination rejected");
        VSURFACE_DESC indexedDescription{};indexedDescription.fCreateFlags=VSURFACE_CREATE_DEFAULT;
        indexedDescription.usWidth=7;indexedDescription.usHeight=4;indexedDescription.ubBitDepth=8;
        UINT32 indexed=0;check(AddVideoSurface(&indexedDescription,&indexed),"actual indexed destination");
        check(!BltVideoObjectOutlineShadow(indexed,sprite.object,0,0,0),"RGB565 bridge rejects indexed destination");
        check(DeleteVideoSurfaceFromIndex(indexed),"failed depth check releases its lock");
        // Real true-color manager objects retain original row pitch, ignore
        // offsets, and use literal0x1f as transparent16-bit key.
        std::array<UINT16,6> raw16{0xf800,0x001f,0x07e0,0xffff,0x1234,0x5678};
        ETRLEObject rawFrame{0,12,9,9,2,3};image_type rawImage{};
        rawImage.ubBitDepth=16;rawImage.fFlags=IMAGE_BITMAPDATA;rawImage.pImageData=raw16.data();
        rawImage.uiSizePixData=12;rawImage.pETRLEObject=&rawFrame;rawImage.usNumberOfObjects=1;
        ManagedObject raw(rawImage);before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,1,1,VO_BLT_SRCTRANSPARENCY,nullptr,2,2),"raw16 transparent crop original source pitch");
        c.expect(before,{{1,1,0xf800},{1,2,0xffff},{2,2,0x1234}});
        before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,0,0,0,nullptr,2,1),"raw16 opaque crop");
        c.expect(before,{{0,0,0xf800},{1,0,0x001f}});
        SetClippingRect(&strip);before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,0,0,0,nullptr,1,1),"raw16 retains original global clip independence");
        c.expect(before,{{0,0,0xf800}});SetClippingRect(&full);
        before=c.reset();
        check(BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,-1,3,0,nullptr,3,2),"raw16 actual-bound crop advances source coordinates");
        c.expect(before,{{0,3,0x001f},{1,3,0x07e0}});
        before=c.reset();
        check(BltVideoObjectOutlineShadowFromIndex(c.id,raw.id,0,0,0),"raw16 shadow skips key and uses actual table");
        c.expect(before,{{0,0,0x1111},{2,0,0x1111},{0,1,0x1111},{1,1,0x1111},{2,1,0x1111}});
        before=c.reset();auto* save=raw.object->p16BPPObject[0].p16BPPData;raw.object->p16BPPObject[0].p16BPPData=nullptr;
        check(!BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,0,0,0,nullptr,1,1),"raw null storage preflight");c.expect(before);
        raw.object->p16BPPObject[0].p16BPPData=save;
        raw.object->p16BPPObject[0].usHeight=0;
        check(!BltVideoObjectOutlineShadowFromIndex(c.id,raw.id,0,0,0),"zero true-color dimensions rejected");c.expect(before);
        raw.object->p16BPPObject[0].usHeight=2;
        raw.object->p16BPPObject[0].p16BPPData=reinterpret_cast<UINT16*>(locked);
        check(!BltVideoObjectFromIndexWithOverride(c.id,raw.id,0,0,0,0,nullptr,1,1),"true-color source/destination alias rejected");c.expect(before);
        raw.object->p16BPPObject[0].p16BPPData=save;
        std::array<UINT32,6> rgba{0x000000ff,0xff0000ff,0x800000ff,0xff00ff00,0xffffffff,0xff000000};
        rawFrame.uiDataLength=24;rawImage.ubBitDepth=32;rawImage.pImageData=rgba.data();rawImage.uiSizePixData=24;
        ManagedObject alpha(rawImage);before=c.reset(0);
        check(BltVideoObjectFromIndexWithOverride(c.id,alpha.id,0,1,1,0,nullptr,3,2),"RGBA uses real alpha-cache provider regardless flags");
        c.expect(before,{{2,1,0xf800},{3,1,0x8000},{1,2,0x07e0},{2,2,0xffff}});
        before=c.reset(0);
        check(BltVideoObjectFromIndexWithOverride(c.id,alpha.id,0,1,1,0,nullptr,2,2),"RGBA cropped width retains source row pitch");
        c.expect(before,{{2,1,0xf800},{1,2,0x07e0},{2,2,0xffff}});
        ShadeTable[0xffff]=0;before=c.reset(0xffff);
        check(BltVideoObjectOutlineShadowFromIndex(c.id,alpha.id,0,0,0),"RGBA alpha shadow actual provider");
        c.expect(before,{{1,0,0},{2,0,0x7bef},{0,1,0},{1,1,0},{2,1,0}});
    }
    check(ShutdownVideoObjectManager()&&ShutdownVideoSurfaceManager(),"owned objects and surfaces cleanly retire");
}
}
int main()
{
    try {test();std::cout<<checks<<" native video-object bridge checks passed\n";return 0;}
    catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
