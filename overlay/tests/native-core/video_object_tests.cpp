/*
 * P710 native notice preparation: tests/native-core/video_object_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add video-object ownership, palette/shading and guarded draw tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/video_object_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "video_object_core.h"
#include "etrle_blitters.h"
#include "shading.h"
#include "FileManIO.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_allocator.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <vector>
namespace fs = std::filesystem;
namespace
{
unsigned checks = 0, draws = 0;
fs::path root;
void check(bool condition, const char* message)
{ ++checks; if (!condition) throw std::runtime_error(message); }
struct Object
{
    HVOBJECT value = nullptr;
    explicit Object(HVOBJECT object) : value(object) { check(value, "create production owned object"); }
    ~Object() { if (value) DeleteVideoObject(value); }
};
std::vector<UINT8> read(const std::string& file)
{
    std::ifstream input(root / file, std::ios::binary);
    check(bool(input), "read existing independent literal oracle");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
HIMAGE load(const char* file)
{
    SGPFILENAME name{}; std::strncpy(name, file, sizeof(name) - 1);
    auto* image = CreateImage(name, IMAGE_ALLIMAGEDATA, ImageFileType::DEFAULT);
    check(image, "CreateImage actual FileMan/STCI fixture");
    return image;
}
VOBJECT_DESC fromImage(HIMAGE image)
{ VOBJECT_DESC result{}; result.fCreateFlags = VOBJECT_CREATE_FROMHIMAGE; result.hImage = image; return result; }
VOBJECT_DESC fromFile(const char* name, UINT32 options = 0)
{
    VOBJECT_DESC result{}; result.fCreateFlags = VOBJECT_CREATE_FROMFILE | options;
    std::strncpy(result.ImageFile, name, sizeof(result.ImageFile) - 1); return result;
}
void blankOwnership()
{
    HIMAGE image=load("blank_frames.sti");
    auto desc=fromImage(image); Object object(CreateVideoObject(&desc));
    check(object.value->usNumberOfObjects==5,"owned logical layer retains all blank frame indices");
    check(object.value->pETRLEObject!=image->pETRLEObject && object.value->pPixData!=image->pImageData,
          "blank descriptors and stream remain independent owned copies");
    DestroyImage(image);
    const std::array<UINT16,5> widths{{0,7,0,4,3}},heights{{0,5,3,0,2}};
    for(unsigned i=0;i<5;++i)
        check(object.value->pETRLEObject[i].usWidth==widths[i] && object.value->pETRLEObject[i].usHeight==heights[i],
              "visible and blank object indices survive image destruction");
    std::array<UINT16,64> pixels; pixels.fill(0x5a5a);const auto before=pixels;SGPRect clip{0,0,8,8};
    for(UINT16 i:{UINT16(0),UINT16(2),UINT16(3)}) {
        check(Blt8BPPDataTo16BPPBufferTransparentClip(pixels.data(),16,object.value,2,2,i,&clip),
              "owned empty logical frame clipped draw succeeds");
        check(pixels==before,"owned empty frame leaves entire destination unchanged");
    }
    check(Blt8BPPDataTo16BPPBufferTransparentClip(pixels.data(),16,object.value,0,2,4,&clip),
          "visible frame after blank descriptors still renders");
    check(pixels!=before,"preserved final visible index is not blanked or skipped");
}
void drawOracle(HVOBJECT object)
{
    const SGPRect clip{1,1,17,12};
    for (UINT16 frame = 0; frame < 2; ++frame)
    {
        const auto pixels = read("sprite_frame" + std::to_string(frame) + ".rgb565le");
        const auto mask = read("sprite_frame" + std::to_string(frame) + ".mask");
        const unsigned width = frame == 0 ? 7u : 3u, height = frame == 0 ? 5u : 2u;
        check(pixels.size() == width * height * 2 && mask.size() == width * height, "oracle dimensions");
        for (int x = -5; x < 20; ++x) for (int y = -5; y < 16; ++y)
        {
            constexpr unsigned pitch = 43, prefix = 18, rows = 15;
            std::vector<UINT8> actual(prefix + pitch * rows + 19, 0xa5), expected = actual;
            const int ox = frame == 0 ? -1 : 2, oy = frame == 0 ? 2 : -1;
            for (unsigned row = 0; row < height; ++row) for (unsigned column = 0; column < width; ++column)
            {
                const int dx = x + ox + int(column), dy = y + oy + int(row);
                if (!mask[row * width + column] || dx < clip.iLeft || dx >= clip.iRight || dy < clip.iTop || dy >= clip.iBottom) continue;
                const unsigned offset = prefix + unsigned(dy) * pitch + unsigned(dx) * 2;
                expected[offset] = pixels[(row * width + column) * 2];
                expected[offset + 1] = pixels[(row * width + column) * 2 + 1];
            }
            auto usedClip = clip;
            check(Blt8BPPDataTo16BPPBufferTransparentClip(reinterpret_cast<UINT16*>(actual.data() + prefix), pitch,
                object, x, y, frame, &usedClip), "owned object uses production transparent draw");
            check(actual == expected, "entire padded/guarded destination matches literal independent sprite");
            ++draws;
        }
    }
}
void imageOwnership()
{
    auto* image = load("sprite.sti");
    auto description = fromImage(image);
    Object object(CreateVideoObject(&description));
    check(object.value->pPixData != image->pPixData8 && object.value->pETRLEObject != image->pETRLEObject &&
          object.value->pPaletteEntry != image->pPalette, "FROMHIMAGE deep-copies all borrowed allocations");
    check(object.value->pShade8 == ubColorTables[DEFAULT_SHADE_LEVEL] && object.value->pGlow8 == ubColorTables[0], "actual production shade state");
    std::memset(image->pPixData8, 0xee, image->uiSizePixData);
    std::memset(image->pPalette, 0xdd, sizeof(SGPPaletteEntry) * 256);
    check(DestroyImage(image), "destroy independently owned source image");
    drawOracle(object.value);
    ETRLEObject frame{};
    check(GetVideoObjectETRLEProperties(object.value, &frame, 0) && frame.usWidth == 7 && frame.usHeight == 5 && frame.sOffsetX == -1, "actual subimage properties");
    check(!GetVideoObjectETRLEProperties(nullptr, &frame, 0) && !GetVideoObjectETRLEProperties(object.value, nullptr, 0) &&
          !GetVideoObjectETRLEProperties(object.value, &frame, 2), "invalid property access rejected");
    // Literal index sidecar is reconstructed from the RGB565 oracle and mask;
    // index zero pixels and transparent runs both return zero from this API.
    const auto rgb = read("sprite_frame0.rgb565le");
    const auto mask = read("sprite_frame0.mask");
    const std::array<UINT16,7> colors{{0,0xf800,0x07e0,0x001f,0xffff,1,0xffe0}};
    for (UINT16 y = 0; y < 5; ++y) for (UINT16 x = 0; x < 7; ++x)
    {
        const auto pos = std::size_t(y) * 7 + x;
        const UINT16 color = UINT16(rgb[pos*2]) | UINT16(UINT16(rgb[pos*2+1]) << 8);
        const auto found = std::find(colors.begin(), colors.end(), color);
        const UINT8 index = mask[pos] ? UINT8(found - colors.begin()) : 0;
        UINT8 actual = 99;
        check(GetETRLEPixelValue(&actual, object.value, 0, x, y) && actual == index, "pixel lookup across literal/transparent runs and scanlines");
        const bool originalHit = mask[y*7+x] && (x != 0 || y == 0);
        check(bool(CheckVideoObjectScreenCoordinateInData(object.value, 0, x, 5-y)) == originalHit,
              "interactive original bottom-up hit test preserves literal zero and EOL first-column quirk");
    }
    UINT8 noopPixel = 99;
    check(static_cast<const UINT8*>(object.value->pPixData)[0] == 0x80 &&
          GetETRLEPixelValue(&noopPixel, object.value, 0, 1, 0) && noopPixel == 1,
          "legal 0x80 no-op preserves first literal pixel");
    UINT8 pixel = 42;
    check(!GetETRLEPixelValue(&pixel, object.value, 0, 7, 0) && pixel == 42 &&
          !GetETRLEPixelValue(nullptr, object.value, 0, 0, 0) && !GetETRLEPixelValue(&pixel, object.value, 2, 0, 0), "pixel bounds/null semantics");
    auto invalid = *object.value;
    invalid.uiSizePixData = 1;
    check(!GetETRLEPixelValue(&pixel, &invalid, 0, 0, 0), "pixel rejects truncated resource");
    check(!CheckVideoObjectScreenCoordinateInData(object.value, 0, -1, 5) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 0, 7, 5) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 0, 0, 0) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 0, 0, 6) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 0, 0, INT32_MIN) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 0, 0, INT32_MAX) &&
          !CheckVideoObjectScreenCoordinateInData(nullptr, 0, 0, 1) &&
          !CheckVideoObjectScreenCoordinateInData(object.value, 2, 0, 1) &&
          !CheckVideoObjectScreenCoordinateInData(&invalid, 0, 0, 1),
          "interactive hit test rejects invalid objects, subimages, encoding and signed endpoint coordinates");
}
void rejectedOwnership()
{
    auto* image = load("indexed8.sti");
    auto description = fromImage(image);
    auto* raw = image->pImageData;
    check(!CreateVideoObject(&description), "plain8 unsupported sprite format rejected");
    check(image->pImageData == raw && image->p8BPPData[1] == 1, "failed FROMHIMAGE keeps source alive");
    DestroyImage(image);
    image = load("rgb16.sti"); description = fromImage(image);
    check(!CreateVideoObject(&description), "plain STCI RGB without original object descriptor rejected");
    DestroyImage(image);
    description = {}; check(!CreateVideoObject(&description) && !CreateVideoObject(nullptr), "invalid/default creation fails");
    description.fCreateFlags = VOBJECT_CREATE_FROMHIMAGE;
    check(!CreateVideoObject(&description), "null source image rejected");
    description = fromFile("bad_short_pixels.sti"); check(!CreateVideoObject(&description), "malformed FROMFILE cleans source/object");
    description = fromFile("sprite.sti", VOBJECT_CREATE_FROMPNG);
    check(!CreateVideoObject(&description), "strict PNG chooser does not silently fall back to STI");
    description = fromFile("sprite.sti", VOBJECT_CREATE_FROMJPC);
    check(!CreateVideoObject(&description), "strict JPC chooser does not silently fall back to STI");
    for (auto flag : {UINT32(0), UINT32(VOBJECT_CREATE_FROMPNG_FALLBACK), UINT32(VOBJECT_CREATE_FROMJPC_FALLBACK)})
    { description = fromFile("sprite.sti", flag); Object object(CreateVideoObject(&description)); }
}
void trueColorOwnership()
{
    for (const UINT8 depth : {UINT8(16), UINT8(32)})
    {
        std::array<UINT8,24> bytes{};
        for (unsigned i=0;i<bytes.size();++i) bytes[i] = UINT8(i*7);
        const auto expected = bytes;
        ETRLEObject frame{0, UINT32(6*(depth/8)), -2, 3, 2, 3};
        image_type image{}; image.ubBitDepth = depth; image.fFlags = IMAGE_BITMAPDATA;
        image.pImageData = bytes.data(); image.uiSizePixData = frame.uiDataLength; image.pETRLEObject = &frame; image.usNumberOfObjects = 1;
        auto description = fromImage(&image);
        Object object(CreateVideoObject(&description));
        check(object.value->ubBitDepth == depth && object.value->usNumberOf16BPPObjects == 1, "retained truecolor object path");
        check(reinterpret_cast<void*>(object.value->p16BPPObject->p16BPPData) != bytes.data(), "truecolor pixels owned independently");
        bytes.fill(0); frame.usWidth = 99;
        check(std::memcmp(object.value->p16BPPObject->p16BPPData, expected.data(), 6*(depth/8)) == 0, "16/32 bytes survive source mutation");
        UINT16 width=0,height=0,index=9;
        GetVideoObjectDimensions(object.value,0,width,height);
        check(width==3 && height==2 && object.value->p16BPPObject->sOffsetX==-2 && object.value->p16BPPObject->sOffsetY==3, "truecolor dimensions/offsets");
        check(CheckFor16BPPRegion(object.value,0,0,&index) && index==0 && !CheckFor16BPPRegion(object.value,1,0,nullptr), "truecolor region lookup");
    }
}
void trueColorDescriptors()
{
    for (const UINT8 depth : {UINT8(16), UINT8(32)})
    {
        const UINT32 bytes = 6 * (depth / 8);
        const UINT32 backing = 5 + bytes + 3;
        std::unique_ptr<image_type, decltype(&DestroyImage)> source(load("rgb16.sti"), &DestroyImage);
        MemFree(source->pImageData);
        source->pImageData = MemAlloc(backing);
        source->pETRLEObject = static_cast<ETRLEObject*>(MemAlloc(sizeof(ETRLEObject)));
        check(source->pImageData && source->pETRLEObject, "allocate owned truecolor descriptor fixture");
        source->ubBitDepth = depth;
        source->uiSizePixData = backing;
        source->usNumberOfObjects = 1;
        std::vector<UINT8> expected(backing, 0xa6);
        for (UINT32 i = 0; i < bytes; ++i) expected[5 + i] = UINT8(0x30 + i);
        std::memcpy(source->pImageData, expected.data(), backing);
        const ETRLEObject frame{5, bytes + 3, -2, 3, 2, 3};
        auto description = fromImage(source.get());
        auto* pixels = source->pImageData; auto* descriptor = source->pETRLEObject;
        std::array<ETRLEObject,9> bad{};
        bad.fill(frame);
        bad[0].usWidth = 0; bad[1].usHeight = 0;
        bad[2].uiDataLength = bytes - 1;
        bad[3].uiDataOffset = backing + 1;
        bad[4].uiDataOffset = backing; bad[4].uiDataLength = bytes;
        bad[5].uiDataOffset = 6; bad[5].uiDataLength = backing - 5;
        bad[6].uiDataOffset = 0xfffffff0u; bad[6].uiDataLength = 32;
        bad[7].uiDataLength = 0xffffffffu;
        bad[8].usWidth = 65535; bad[8].usHeight = 65535;
        for (const auto& invalid : bad)
        {
            *descriptor = invalid;
            auto* rejected = CreateVideoObject(&description);
            if (rejected) DeleteVideoObject(rejected);
            check(!rejected, "reject malformed truecolor descriptor span/dimensions");
            check(source.get() == description.hImage && source->pImageData == pixels && source->pETRLEObject == descriptor && source->uiSizePixData == backing && source->ubBitDepth == depth && source->usNumberOfObjects == 1, "failed truecolor creation preserves borrowed source ownership");
            check(std::memcmp(pixels, expected.data(), backing) == 0 && descriptor->uiDataOffset == invalid.uiDataOffset && descriptor->uiDataLength == invalid.uiDataLength && descriptor->usWidth == invalid.usWidth && descriptor->usHeight == invalid.usHeight, "failed truecolor creation leaves source bytes/descriptor unchanged");
        }
        *descriptor = frame;
        Object object(CreateVideoObject(&description));
        check(source->pImageData == pixels && source->pETRLEObject == descriptor && reinterpret_cast<void*>(object.value->p16BPPObject->p16BPPData) != pixels, "successful offset copy keeps borrowed source and owns separate pixels");
        std::memset(pixels, 0xee, backing);
        descriptor->usWidth = 99;
        source.reset();
        const auto& region = *object.value->p16BPPObject;
        check(std::memcmp(region.p16BPPData, expected.data() + 5, bytes) == 0, "offset16/32 copies exclude distinct prefix and survive source mutation/destruction");
        check(region.sOffsetX == -2 && region.sOffsetY == 3 && region.usWidth == 3 && region.usHeight == 2, "offset truecolor copy preserves rectangle");
        UINT16 width = 0, height = 0;
        GetVideoObjectDimensions(object.value, 0, width, height);
        check(width == 3 && height == 2, "offset truecolor dimensions remain independent of source descriptor");
    }
}

void paletteAndManager()
{
    check(ShutdownVideoObjectManager() && CountVideoObjectNodes()==0 && !gfVideoObjectsInit, "shutdown empty manager");
    auto description = fromFile("sprite.sti"); UINT32 id=99;
    check(!AddStandardVideoObject(&description,&id) && id==99, "manager creation requires initialization");
    check(InitializeVideoObjectManager() && !InitializeVideoObjectManager(), "single manager initialization");
    check(AddStandardVideoObject(&description,&id) && id==1 && CountVideoObjectNodes()==1, "production AddStandardVideoObject ID/count");
    HVOBJECT object=nullptr;
    check(GetVideoObject(&object,id) && object, "get real owned object");
    check(!CreateObjectPaletteTables(object,999), "invalid glow type rejected");
    check(!CreateObjectPaletteTables(object,HVOBJECT_GLOW_GREEN), "original 16-generation/48-completeness return retained");
    for (unsigned i=0;i<16;++i) check(object->pShades[i], "original generated shade entry exists");
    check(object->pShades[4]==object->p16BPPPalette && object->pShadeCurrent==object->p16BPPPalette, "neutral shade/base aliases");
    check(SetObjectHandleShade(id,15) && object->pShadeCurrent==object->pShades[15], "manager shade selection");
    check(!SetObjectShade(object,48) && !SetObjectShade(object,47) && !SetObjectHandleShade(999,4), "invalid shade indices/handles");
    check(SetVideoObjectTransparency(id,0x112233) && object->TransparentColor==0x112233, "manager transparency");
    SGPPaletteEntry palette[256]{}; palette[1].peRed=255;
    check(SetVideoObjectPalette8BPP(INT32(id),palette) && object->pShades[4]==object->p16BPPPalette, "palette replacement repairs shade4 alias");
    UINT16* borrowed=nullptr; std::array<UINT16,256> copy{};
    check(GetVideoObjectPalette16BPP(INT32(id),&borrowed) && borrowed==object->p16BPPPalette &&
          CopyVideoObjectPalette16BPP(INT32(id),copy.data()) && copy[1]==0xf800, "palette readback and copy");
    // Shared custom shades are borrowed; the borrower may use its own base
    // palette, or alias an owner's neutral palette, without freeing the owner.
    {
        Object owner(CreateVideoObject(&description)); CreateObjectPaletteTables(owner.value,HVOBJECT_GLOW_BLUE);
        Object borrower(CreateVideoObject(&description));
        DestroyObjectPaletteTables(borrower.value);
        borrower.value->fFlags |= VOBJECT_FLAG_SHADETABLE_SHARED;
        std::copy(std::begin(owner.value->pShades),std::end(owner.value->pShades),std::begin(borrower.value->pShades));
        borrower.value->p16BPPPalette = owner.value->p16BPPPalette;
        borrower.value->pShadeCurrent = owner.value->pShades[2];
        check(DestroyObjectPaletteTables(borrower.value), "detach shared shade/base aliases");
        check(owner.value->p16BPPPalette[1]==0xf800 && SetObjectShade(owner.value,2), "borrower destruction preserves original owner");
        check(!CreateObjectPaletteTables(borrower.value,HVOBJECT_GLOW_GREEN), "shared shade table mutation refused");
    }
    // Multiple aliases owned by one object must not trigger repeated frees.
    object->pShades[20]=object->pShades[2];
    UINT16 width=0,height=0; ETRLEObject region{};
    check(GetVideoObjectETRLEPropertiesFromIndex(id,&region,1) && GetVideoObjectETRLESubregionProperties(id,1,&width,&height) && width==3 && height==2, "manager properties");
    check(!GetVideoObjectETRLESubregionProperties(id,1,nullptr,&height), "manager properties null rejected");
    check(DeleteVideoObjectFromIndex(id) && !DeleteVideoObjectFromIndex(id) && CountVideoObjectNodes()==0, "delete ID once");
    object=reinterpret_cast<HVOBJECT>(1);
    check(!GetVideoObject(&object,id) && !object && !GetVideoObject(nullptr,id), "stale ID and null output rejected");
    check(AddStandardVideoObject(&description,&id) && id==2 && AddStandardVideoObject(&description,&id) && id==3, "IDs monotonic across deletes");
    check(ShutdownVideoObjectManager() && !gfVideoObjectsInit && CountVideoObjectNodes()==0, "shutdown destroys remaining owned objects");
}
void glowAliases()
{
    auto description = fromFile("sprite.sti");
    SGPPaletteEntry palette[256]{}; palette[1].peRed = 255;
    Object object(CreateVideoObject(&description));
    object.value->pGlow = object.value->p16BPPPalette;
    check(SetVideoObjectPalette(object.value, palette) && object.value->pGlow == object.value->p16BPPPalette && object.value->pGlow[1] == 0xf800, "base replacement rebinds owned glow alias");
    CreateObjectPaletteTables(object.value, HVOBJECT_GLOW_GREEN);
    for (unsigned pass = 0; pass < 3; ++pass)
        for (unsigned shade : {0u, 2u, 4u})
        {
            object.value->pGlow = object.value->pShades[shade];
            CreateObjectPaletteTables(object.value, pass % 2 ? HVOBJECT_GLOW_BLUE : HVOBJECT_GLOW_GREEN);
            check(object.value->pGlow == object.value->pShades[shade] && object.value->pGlow[1] == object.value->pShades[shade][1], "repeated shade rebuild rebinds corresponding glow table");
        }
    std::array<UINT16,256> external{};
    object.value->pGlow = external.data();
    CreateObjectPaletteTables(object.value, HVOBJECT_GLOW_RED);
    check(object.value->pGlow == external.data(), "shade rebuild preserves external glow alias");
    object.value->pShades[20] = static_cast<UINT16*>(MemAlloc(sizeof(UINT16) * 256));
    check(object.value->pShades[20], "allocate custom retained shade");
    object.value->pGlow = object.value->pShades[20];
    CreateObjectPaletteTables(object.value, HVOBJECT_GLOW_GREEN);
    check(object.value->pGlow == object.value->pShades[20], "shade rebuild preserves unreplaced custom glow table");
    check(DestroyObjectPaletteTables(object.value) && DestroyObjectPaletteTables(object.value), "repeated owned palette destruction after glow rebinding");
    Object owner(CreateVideoObject(&description));
    check(SetVideoObjectPalette(owner.value, palette), "set literal shared owner palette");
    CreateObjectPaletteTables(owner.value, HVOBJECT_GLOW_BLUE);
    for (bool aliasBase : {false, true})
    {
        Object borrower(CreateVideoObject(&description));
        if (aliasBase) DestroyObjectPaletteTables(borrower.value);
        borrower.value->fFlags |= VOBJECT_FLAG_SHADETABLE_SHARED;
        std::copy(std::begin(owner.value->pShades), std::end(owner.value->pShades), std::begin(borrower.value->pShades));
        if (aliasBase) borrower.value->p16BPPPalette = owner.value->p16BPPPalette;
        borrower.value->pGlow = owner.value->pShades[4];
        for (unsigned i = 0; i < 2; ++i)
        {
            check(SetVideoObjectPalette(borrower.value, palette), "replace palette with borrowed shades and owned/borrowed initial base");
            check(borrower.value->pGlow == owner.value->pShades[4] && std::equal(std::begin(owner.value->pShades), std::end(owner.value->pShades), std::begin(borrower.value->pShades)) && owner.value->p16BPPPalette[1] == 0xf800, "palette replacement preserves shared shade allocations and glow aliases");
        }
        borrower.value->pGlow = borrower.value->p16BPPPalette;
        check(SetVideoObjectPalette(borrower.value, palette) && borrower.value->pGlow == borrower.value->p16BPPPalette && borrower.value->pGlow[1] == 0xf800, "shared-shade borrower rebinds glow alias to its replaced owned base");
        borrower.value->pGlow = external.data();
        check(SetVideoObjectPalette(borrower.value, palette) && borrower.value->pGlow == external.data(), "base replacement preserves external glow alias");
        check(DestroyObjectPaletteTables(borrower.value) && DestroyObjectPaletteTables(borrower.value), "shared borrower repeated destruction is safe");
        check(owner.value->p16BPPPalette[1] == 0xf800 && SetObjectShade(owner.value, 2), "shared owner survives borrower replacement and destruction");
    }
}

void shadeMath()
{
    SGPPaletteEntry palette[256]{};
    // Many equal colors force the original lowest-nonzero-index tie behavior.
    palette[1]={255,0,0,0}; palette[2]={255,0,0,0}; palette[3]={0,255,0,0}; palette[4]={0,0,255,0};
    check(ShadesCalculateTables(palette), "production shade remapping");
    const unsigned scale[16]={0,293,281,268,255,195,165,135,105,75,45,36,27,18,9,0};
    for (unsigned table=0;table<16;++table) for (unsigned color=0;color<256;++color)
    {
        unsigned r=0,g=0,b=0;
        if (!table) g=palette[color].peRed*299/1000+palette[color].peGreen*587/1000+palette[color].peBlue*114/1000;
        else { r=std::min(255u,palette[color].peRed*scale[table]/255); g=std::min(255u,palette[color].peGreen*scale[table]/255); b=std::min(255u,palette[color].peBlue*scale[table]/255); }
        const auto& actual=Shaded8BPPPalettes[table][color];
        check(actual.peRed==r && actual.peGreen==g && actual.peBlue==b, "original shade luminance/scaling");
        unsigned best=1000,index=0;
        if (color) for(unsigned j=1;j<256;++j)
        {
            const unsigned distance=unsigned(std::abs(int(r)-palette[j].peRed)+std::abs(int(g)-palette[j].peGreen)+std::abs(int(b)-palette[j].peBlue));
            if(distance<best){best=distance;index=j;}
        }
        check(ubColorTables[table][color]==index, "scalar shade index matches independent distance/tie oracle");
    }
    check(ubColorTables[4][1]==1 && ubColorTables[4][2]==1, "first-index tie rule");
    BuildShadeTable(); BuildIntensityTable();
    check(ShadeTable[0xffff]==Get16BPPColor(FROMRGB(252*guiShadePercent,252*guiShadePercent,252*guiShadePercent)), "original darkening lookup");
    check(IntensityTable[0xffff]==Get16BPPColor(FROMRGB(252*0.8f,252*0.8f,252*0.8f)), "original intensity lookup");
    check(White16BPPPalette[0]==0xffff && White16BPPPalette[255]==0xffff, "production white table");
}
void rgb565Conversion()
{
    std::vector<UINT16> identity(65536);
    for (unsigned i = 0; i < identity.size(); ++i) identity[i] = UINT16(i);
    ConvertRGBDistribution565ToAny(identity.data(), UINT32(identity.size()));
    for (unsigned i = 0; i < identity.size(); ++i) check(identity[i] == i, "exhaustive literal RGB565 identity including transparent black");
    const std::array<UINT16,9> input{{0, 0xf800, 0x07e0, 0x001f, 0xffff, 0x8410, 0x0020, 0x0040, 1}};
    auto colors = input;
    gusAlphaMask = 0x8000;
    gusRedMask = 0x7c00; gusGreenMask = 0x03e0; gusBlueMask = 0x001f;
    gusRedShift = 7; gusGreenShift = 2; gusBlueShift = -3;
    ConvertRGBDistribution565ToAny(colors.data(), UINT32(colors.size()));
    ConfigureRGB565ImageFormat();
    const std::array<UINT16,9> expected{{0, 0xfc00, 0x83e0, 0x801f, 0xffff, 0xc210, 0x8001, 0x8020, 0x8001}};
    check(colors == expected, "literal RGB565-to-ARGB1555 primaries, white, midtone, black and nonblack-underflow colors");
}

void mount()
{
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(), "actual VFS fixture directory");
    auto profile=std::make_unique<vfs::CVirtualProfile>("vobjects",vfs::Path(root.u8string()),false);
    profile->addLocation(directory.get());directory.release();auto* entry=profile.get();
    getVFS()->getProfileStack()->pushProfile(entry);profile.release();
    for(auto it=entry->begin();!it.end();it.next())check(getVFS()->addLocation(it.value(),entry),"actual VFS mount");
}
void cleanup()
{ ShutdownVideoObjectManager(); fileman::CloseAll();vfs::Log::flushDeleteAll();vfs::CVirtualFileSystem::shutdownVFS();vfs::ObjectAllocator::clear(); }
}
int main(int argc,char** argv)
{
    try
    {
        if(argc!=2)throw std::runtime_error("expected existing generated image fixture root");
        root=fs::canonical(argv[1]);ConfigureRGB565ImageFormat();mount();
        imageOwnership();blankOwnership();rejectedOwnership();trueColorOwnership();trueColorDescriptors();paletteAndManager();glowAliases();shadeMath();rgb565Conversion();cleanup();
        std::cout<<"PASS: "<<checks<<" video-object/shading checks, "<<draws<<" whole-buffer owned-object draws\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n';cleanup();return 1;}
}
