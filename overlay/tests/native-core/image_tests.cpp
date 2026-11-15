/*
 * P710 native notice preparation: tests/native-core/image_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add STCI/image/scalar ETRLE validation against authored expectations.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/image_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "FileManIO.h"
#include "FileManBackend.h"
#include "STCI.h"
#include "himage.h"
#include "etrle_blitters.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_allocator.h>
#include <vfs/Tools/vfs_log.h>
#include <zlib.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace
{
unsigned checks = 0, drawScenarios = 0, loadScenarios = 0;
fs::path root;
constexpr UINT16 background = 0x4210;
constexpr int surfaceWidth = 11, surfaceHeight = 10;
const std::array<UINT16, 7> colors{{0x0000, 0xf800, 0x07e0, 0x001f, 0xffff, 0x0001, 0xffe0}};
const std::array<UINT8, 8> app{{'A', 'P', 'P', 0, 'M', 'E', 'T', 'A'}};
const std::array<const char*, 12> valid{{
    "sprite.sti", "sprite_canvas.sti", "sprite_trailing.sti", "long_runs.sti",
    "indexed8.sti", "indexed8_zlib.sti", "rgb16.sti", "rgb16_zlib.sti",
    "rgb24.sti", "rgb24_zlib.sti", "rgb32.sti", "rgb32_zlib.sti"}};
const std::array<const char*, 28> malformed{{
    "bad_short_header.sti", "bad_short_palette.sti", "bad_short_table.sti", "bad_short_pixels.sti",
    "bad_short_app.sti", "bad_magic.sti", "bad_flags_none.sti", "bad_flags_both.sti",
    "bad_depth.sti", "bad_palette_count.sti", "bad_empty_object_offset.sti", "bad_empty_object_length.sti",
    "bad_object_offset.sti", "bad_object_length.sti", "bad_object_count_zero.sti", "bad_object_count_extra.sti",
    "bad_run_overflow.sti", "bad_run_underflow.sti", "bad_literal_overflow.sti", "bad_literal_truncated.sti",
    "bad_missing_eol.sti", "bad_etrle_zlib.sti", "bad_plain_zero_width.sti", "bad_plain_size.sti",
    "bad_deflate_header.sti", "bad_deflate_checksum.sti", "bad_deflate_size.sti", "bad_deflate_truncated.sti"}};

void check(bool condition, const std::string& label)
{
    ++checks;
    if (!condition) throw std::runtime_error(label);
}

std::vector<UINT8> disk(const std::string& name)
{
    std::ifstream input(root / name, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read independent fixture: " + name);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

UINT32 le32(const UINT8* data)
{
    return UINT32(data[0]) | (UINT32(data[1]) << 8) | (UINT32(data[2]) << 16) | (UINT32(data[3]) << 24);
}

void filename(image_type& image, const std::string& name)
{
    check(name.size() < sizeof(image.ImageFile), "fixture basename fits original filename buffer");
    std::memset(image.ImageFile, 0, sizeof(image.ImageFile));
    std::memcpy(image.ImageFile, name.data(), name.size());
}

struct Image
{
    image_type image{};
    explicit Image(const std::string& name) { filename(image, name); }
    ~Image() { ReleaseImageData(&image, IMAGE_ALLDATA); }
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    void load(UINT16 mask = IMAGE_ALLDATA)
    {
        ++loadScenarios;
        check(LoadSTCIFileToImage(&image, mask), std::string("load actual FileMan/STCI image: ") + image.ImageFile);
    }
};

bool isRGB(const std::string& name) { return name.rfind("rgb", 0) == 0; }
bool isSprite(const std::string& name) { return name.rfind("sprite", 0) == 0 || name == "long_runs.sti"; }

std::vector<UINT8> rawExpected(const std::string& name)
{
    if (!isRGB(name)) return {0, 1, 2, 3, 4, 5};
    if (name.rfind("rgb16", 0) == 0)
        return {0, 0, 0, 248, 224, 7, 31, 0, 255, 255, 224, 255};
    const std::vector<UINT8> bgr{0,0,0, 0,0,255, 0,255,0, 255,0,0, 255,255,255, 0,255,255};
    if (name.rfind("rgb24", 0) == 0) return bgr;
    std::vector<UINT8> bgra;
    for (unsigned i = 0; i < 6; ++i)
    {
        bgra.insert(bgra.end(), bgr.begin() + i * 3, bgr.begin() + i * 3 + 3);
        bgra.push_back(0);
    }
    return bgra;
}

void checkApp(const image_type& image, const std::string& label)
{
    check((image.fFlags & IMAGE_APPDATA) && image.pAppData && image.uiAppDataSize == app.size(), label + " app metadata");
    check(std::memcmp(image.pAppData, app.data(), app.size()) == 0, label + " literal APP NUL META bytes");
}

void checkPalette(image_type& image)
{
    const std::array<std::array<UINT8, 3>, 7> rgb{{{{0,0,0}}, {{255,0,0}}, {{0,255,0}},
        {{0,0,255}}, {{255,255,255}}, {{1,1,1}}, {{255,255,0}}}};
    check(image.pPalette != nullptr && (image.fFlags & IMAGE_PALETTE), "normal indexed palette allocated");
    for (unsigned i = 0; i < 256; ++i)
    {
        const auto expected = i < rgb.size() ? rgb[i] : std::array<UINT8, 3>{{0,0,0}};
        const auto& p = image.pPalette[i];
        check(p.peRed == expected[0] && p.peGreen == expected[1] && p.peBlue == expected[2] && p.peFlags == 0,
              "exact palette entry " + std::to_string(i));
    }
    std::unique_ptr<UINT16, decltype(&std::free)> converted(Create16BPPPalette(image.pPalette), &std::free);
    check(bool(converted), "production RGB565 palette allocation");
    for (unsigned i = 0; i < 256; ++i)
        check(converted.get()[i] == (i < colors.size() ? colors[i] : UINT16{0}), "independent RGB565 palette value");
}

void fullLoads()
{
    for (const auto* name : valid)
    {
        const auto wire = disk(name);
        check(wire.size() >= 64 && wire[45] == 0x4d && wire[46] == 0xa5 && wire[47] == 0x78,
              "original header alignment padding is deliberately nonzero");
        check(le32(wire.data() + 48) == app.size(), "literal application size at original wire offset 48");
        Image owner(name);
        owner.load();
        auto& image = owner.image;
        checkApp(image, name);
        check(image.fFlags & IMAGE_BITMAPDATA, "full load includes bitmap");
        if (!isRGB(name)) checkPalette(image);
        else check(image.pPalette == nullptr && !(image.fFlags & IMAGE_PALETTE), "RGB has no indexed palette");
        if (isSprite(name))
        {
            check(image.ubBitDepth == 8 && image.pPixData8 && image.pETRLEObject && (image.fFlags & IMAGE_TRLECOMPRESSED),
                  "ETRLE object structures loaded");
            if (std::string(name) == "long_runs.sti")
                check(image.usNumberOfObjects == 1 && image.pETRLEObject[0].usWidth == 130 &&
                      image.pETRLEObject[0].usHeight == 2, "maximum-run object dimensions");
            else
            {
                check(image.usNumberOfObjects == 2, "two sprite subimages");
                const auto& a = image.pETRLEObject[0];
                const auto& b = image.pETRLEObject[1];
                const bool trailing = std::string(name) == "sprite_trailing.sti";
                check(a.uiDataOffset == 0 && a.uiDataLength == (trailing ? 42u : 40u) &&
                      a.sOffsetX == -1 && a.sOffsetY == 2 && a.usWidth == 7 && a.usHeight == 5,
                      "frame 0 explicit disk metadata");
                check(b.uiDataOffset == (trailing ? 42u : 40u) && b.uiDataLength == 11 &&
                      b.sOffsetX == 2 && b.sOffsetY == -1 && b.usWidth == 3 && b.usHeight == 2,
                      "frame 1 explicit disk metadata");
                check(image.uiSizePixData == (trailing ? 53u : 51u), "stored ETRLE size");
                check(image.usWidth == (std::string(name) == "sprite_canvas.sti" ? 7 : 0) &&
                      image.usHeight == (std::string(name) == "sprite_canvas.sti" ? 5 : 0), "collection canvas including zero");
            }
        }
        else
        {
            check(image.usWidth == 3 && image.usHeight == 2 && image.usNumberOfObjects == 0 &&
                  image.pETRLEObject == nullptr, "plain image dimensions and absent object table");
            const auto expected = rawExpected(name);
            const UINT8 depth = !isRGB(name) ? 8 : (std::string(name).rfind("rgb16", 0) == 0 ? 16 :
                (std::string(name).rfind("rgb24", 0) == 0 ? 24 : 32));
            check(image.ubBitDepth == depth, "plain stored pixel bit depth");
            if (std::string(name).find("_zlib") != std::string::npos)
            {
                check(image.fFlags & IMAGE_COMPRESSED, "zlib flag retained");
                const auto bytes = disk(name);
                const std::size_t section = isRGB(name) ? 64 : 832;
                const auto stored = le32(bytes.data() + 8);
                check(image.uiSizePixData == stored && std::memcmp(image.pImageData, bytes.data() + section, stored) == 0,
                      "exact compressed representation retained");
                std::vector<UINT8> inflated(expected.size());
                uLongf size = inflated.size();
                check(uncompress(inflated.data(), &size, image.p8BPPData, stored) == Z_OK && size == expected.size(),
                      "independent installed zlib inflate");
                check(inflated == expected, "fixed raw pixels after independent inflate");
            }
            else
            {
                check(!(image.fFlags & IMAGE_COMPRESSED) && image.uiSizePixData == expected.size(), "plain size and compression flag");
                check(std::memcmp(image.pImageData, expected.data(), expected.size()) == 0, "fixed exact raw pixel payload");
            }
        }
        std::string query(name);
        check(bool(IsSTCIETRLEFile(query.data())) == isSprite(name), "production ETRLE type query");
    }
}

bool emptySections(const image_type& image)
{
    return !(image.fFlags & (IMAGE_ALLDATA | IMAGE_COMPRESSED | IMAGE_TRLECOMPRESSED)) &&
        !image.pPalette && !image.pui16BPPPalette && !image.pImageData && !image.pETRLEObject &&
        image.uiSizePixData == 0 && image.usNumberOfObjects == 0 && !image.pAppData && image.uiAppDataSize == 0;
}

void masksAndOwnership()
{
    for (const auto* name : valid)
    {
        for (unsigned bits = 0; bits < 8; ++bits)
        {
            const UINT16 mask = static_cast<UINT16>(((bits & 1) ? IMAGE_PALETTE : 0) |
                ((bits & 2) ? IMAGE_BITMAPDATA : 0) | ((bits & 4) ? IMAGE_APPDATA : 0));
            // Original RGB palette-only request has no valid palette to provide.
            if (isRGB(name) && (mask & IMAGE_PALETTE) && !(mask & IMAGE_BITMAPDATA)) continue;
            Image fresh(name);
            fresh.load(mask);
            const auto& image = fresh.image;
            check(bool(image.pPalette) == bool((mask & IMAGE_PALETTE) && !isRGB(name)), "subset palette ownership");
            check(bool(image.pImageData) == bool(mask & IMAGE_BITMAPDATA), "subset bitmap ownership");
            check(bool(image.pETRLEObject) == bool((mask & IMAGE_BITMAPDATA) && isSprite(name)), "subset object table ownership");
            check(bool(image.pAppData) == bool(mask & IMAGE_APPDATA), "subset app ownership");
            if (mask & IMAGE_APPDATA) checkApp(image, "subset app-only/table skip");
            Image incremental(name);
            incremental.load();
            const auto palette = incremental.image.pPalette;
            const auto pixels = incremental.image.pImageData;
            const auto objects = incremental.image.pETRLEObject;
            const auto application = incremental.image.pAppData;
            incremental.load(mask);
            if (!(mask & IMAGE_PALETTE)) check(incremental.image.pPalette == palette, "unrequested palette retained");
            if (!(mask & IMAGE_BITMAPDATA))
                check(incremental.image.pImageData == pixels && incremental.image.pETRLEObject == objects, "unrequested bitmap/table retained");
            if (!(mask & IMAGE_APPDATA)) check(incremental.image.pAppData == application, "unrequested app retained");
            checkApp(incremental.image, "incremental retained app");
        }
    }
    Image staged("sprite.sti");
    staged.load(IMAGE_APPDATA);
    const auto application = staged.image.pAppData;
    staged.load(IMAGE_PALETTE);
    const auto palette = staged.image.pPalette;
    staged.load(IMAGE_BITMAPDATA);
    check(staged.image.pAppData == application && staged.image.pPalette == palette &&
          staged.image.usNumberOfObjects == 2, "app then palette then bitmap staged load");
    staged.image.pui16BPPPalette = Create16BPPPalette(staged.image.pPalette);
    check(staged.image.pui16BPPPalette != nullptr, "owned image conversion palette");
    check(ReleaseImageData(&staged.image, IMAGE_BITMAPDATA), "release bitmap independently");
    check(!staged.image.pImageData && !staged.image.pETRLEObject && staged.image.usNumberOfObjects == 0 &&
          staged.image.uiSizePixData == 0 && staged.image.pPalette == palette && staged.image.pAppData == application,
          "bitmap release clears own fields and preserves other sections");
    check(ReleaseImageData(&staged.image, IMAGE_ALLDATA) && ReleaseImageData(&staged.image, IMAGE_ALLDATA), "release all twice safely");
    check(emptySections(staged.image), "repeat release leaves all ownership fields clear");
    auto* heap = static_cast<HIMAGE>(MemAlloc(sizeof(image_type)));
    check(heap != nullptr, "MemAlloc image object");
    std::memset(heap, 0, sizeof(*heap));
    filename(*heap, "sprite.sti");
    check(LoadSTCIFileToImage(heap, IMAGE_ALLDATA), "heap image actual load");
    ++loadScenarios;
    check(DestroyImage(heap), "DestroyImage owns MemAlloc image object");
}

void rejectedAndTransactional()
{
    for (const auto* name : malformed)
    {
        Image owner(name);
        std::array<UINT8, sizeof(image_type)> before{};
        std::memcpy(before.data(), &owner.image, before.size());
        ++loadScenarios;
        check(!LoadSTCIFileToImage(&owner.image, IMAGE_ALLDATA), std::string("reject malformed actual STCI: ") + name);
        check(std::memcmp(before.data(), &owner.image, before.size()) == 0 && emptySections(owner.image),
              "malformed full load exposes no partial allocations");
    }
    Image owner("sprite.sti");
    owner.load();
    filename(owner.image, "bad_run_underflow.sti");
    // Snapshot AFTER assigning the requested source name, before failed load.
    std::array<UINT8, sizeof(image_type)> before{};
    std::memcpy(before.data(), &owner.image, before.size());
    const std::vector<UINT8> pixels(owner.image.p8BPPData, owner.image.p8BPPData + owner.image.uiSizePixData);
    const std::vector<UINT8> palette(reinterpret_cast<const UINT8*>(owner.image.pPalette),
                                   reinterpret_cast<const UINT8*>(owner.image.pPalette + 256));
    std::array<ETRLEObject, 2> objects{{owner.image.pETRLEObject[0], owner.image.pETRLEObject[1]}};
    ++loadScenarios;
    check(!LoadSTCIFileToImage(&owner.image, IMAGE_ALLDATA), "failed replacement is rejected");
    check(std::memcmp(before.data(), &owner.image, before.size()) == 0, "failed load preserves all pointers and metadata");
    check(std::memcmp(pixels.data(), owner.image.pImageData, pixels.size()) == 0 &&
          std::memcmp(palette.data(), owner.image.pPalette, palette.size()) == 0 &&
          std::memcmp(objects.data(), owner.image.pETRLEObject, sizeof(objects)) == 0,
          "failed load preserves allocated pixel/palette/table bytes");
    checkApp(owner.image, "failed load retains original app bytes");
    check(!LoadSTCIFileToImage(nullptr, IMAGE_ALLDATA), "null load image rejected");
    check(!LoadSTCIFileToImage(&owner.image, 0xffff), "unsupported content flags rejected");
}

struct Grid
{
    int width, height, x, y;
    std::vector<UINT16> pixels;
    std::vector<UINT8> mask;
};

Grid independentGrid(const std::string& prefix, int width, int height, int x, int y)
{
    const auto bytes = disk(prefix + ".rgb565le");
    const auto mask = disk(prefix + ".mask");
    check(bytes.size() == std::size_t(width * height * 2) && mask.size() == std::size_t(width * height), "independent oracle sidecar sizes");
    Grid result{width, height, x, y, {}, mask};
    for (std::size_t i = 0; i < mask.size(); ++i)
    {
        check(mask[i] <= 1, "independent oracle boolean mask");
        result.pixels.push_back(static_cast<UINT16>(bytes[i * 2] | (UINT16(bytes[i * 2 + 1]) << 8)));
    }
    return result;
}

struct Surface
{
    unsigned pitch, origin;
    std::vector<UINT8> bytes;
    Surface(unsigned rowPitch, unsigned misalignment) : pitch(rowPitch), origin(64 + misalignment),
        bytes(origin + pitch * surfaceHeight + 64, 0xa5)
    {
        for (int y = 0; y < surfaceHeight; ++y)
            for (int x = 0; x < surfaceWidth; ++x)
                std::memcpy(bytes.data() + origin + std::size_t(y) * pitch + std::size_t(x) * 2, &background, 2);
    }
    UINT16* data() { return reinterpret_cast<UINT16*>(bytes.data() + origin); }
};

void blankFrames()
{
    Image owner("blank_frames.sti"); owner.load();
    auto& image = owner.image;
    check(image.usNumberOfObjects == 5 && image.uiSizePixData == 53,
          "blank layer descriptors retain every animation index and stored span");
    const std::array<UINT16,5> widths{{0,7,0,4,3}}, heights{{0,5,3,0,2}};
    const std::array<UINT32,5> offsets{{0,0,40,42,42}}, lengths{{0,40,2,0,11}};
    for (unsigned i=0;i<5;++i)
    {
        const auto& frame=image.pETRLEObject[i];
        check(frame.usWidth==widths[i] && frame.usHeight==heights[i] &&
              frame.uiDataOffset==offsets[i] && frame.uiDataLength==lengths[i],
              "literal blank/visible descriptor offsets and dimensions");
    }
    SGPVObject source{}; source.ubBitDepth=8; source.usNumberOfObjects=5;
    source.pETRLEObject=image.pETRLEObject; source.pPixData=image.pPixData8;
    source.uiSizePixData=image.uiSizePixData;
    Surface surface(27,1); const auto before=surface.bytes; SGPRect clip{0,0,11,10};
    // No palette is bound: these calls must return before payload/palette use.
    for (UINT16 i : {UINT16(0),UINT16(2),UINT16(3)})
    {
        check(Blt8BPPDataTo16BPPBufferTransparentClip(surface.data(),surface.pitch,&source,2,2,i,&clip),
              "each no-area frame is a successful clipped no-op");
        check(surface.bytes==before,"blank clipped draw preserves pixels, pitch padding and guards");
        check(Blt8BPPDataTo16BPPBufferOutlineClip(surface.data(),surface.pitch,&source,2,2,i,0xffff,TRUE,&clip),
              "blank outline clipped draw succeeds before palette access");
        check(surface.bytes==before,"blank outline preserves every destination byte");
        check(!Blt8BPPDataTo16BPPBufferTransparent(surface.data(),surface.pitch,&source,2,2,i),
              "unclipped empty admission remains unchanged");
    }
}

void expectedDraw(std::vector<UINT8>& destination, const Surface& surface, const Grid& grid,
                  INT32 x, INT32 y, const SGPRect& clip)
{
    for (int row = 0; row < grid.height; ++row)
        for (int column = 0; column < grid.width; ++column)
        {
            const auto index = std::size_t(row * grid.width + column);
            const std::int64_t dx = std::int64_t(x) + grid.x + column;
            const std::int64_t dy = std::int64_t(y) + grid.y + row;
            if (grid.mask[index] && dx >= clip.iLeft && dx < clip.iRight && dy >= clip.iTop && dy < clip.iBottom &&
                dx >= 0 && dx < surfaceWidth && dy >= 0 && dy < surfaceHeight)
                std::memcpy(destination.data() + surface.origin + std::size_t(dy) * surface.pitch + std::size_t(dx) * 2,
                            &grid.pixels[index], 2);
        }
}

struct Object
{
    Image owner;
    std::unique_ptr<UINT16, decltype(&std::free)> palette{nullptr, &std::free};
    SGPVObject object{};
    explicit Object(const std::string& name) : owner(name)
    {
        owner.load();
        palette.reset(Create16BPPPalette(owner.image.pPalette));
        check(bool(palette), "actual palette for borrowed HVOBJECT");
        object.pPixData = owner.image.pImageData;
        object.pETRLEObject = owner.image.pETRLEObject;
        object.uiSizePixData = owner.image.uiSizePixData;
        object.usNumberOfObjects = owner.image.usNumberOfObjects;
        object.ubBitDepth = owner.image.ubBitDepth;
        object.pShadeCurrent = palette.get();
    }
};

void drawCase(Object& source, UINT16 index, const Grid& grid, INT32 x, INT32 y,
              unsigned pitch, unsigned misalignment, const SGPRect& clip, bool clipped, bool shared = false)
{
    Surface surface(pitch, misalignment);
    auto expected = surface.bytes;
    expectedDraw(expected, surface, grid, x, y, clip);
    auto region = clip;
    const auto result = clipped ? Blt8BPPDataTo16BPPBufferTransparentClip(surface.data(), pitch, &source.object,
        x, y, index, shared ? nullptr : &region) :
        Blt8BPPDataTo16BPPBufferTransparent(surface.data(), pitch, &source.object, x, y, index);
    ++drawScenarios;
    check(result, "production ETRLE draw succeeds at " + std::to_string(x) + "," + std::to_string(y));
    check(surface.bytes == expected, "entire drawn buffer, row padding, prefix/suffix guards match literal oracle");
}

void drawing()
{
    Object source("sprite.sti");
    const std::array<Grid, 2> grids{{independentGrid("sprite_frame0", 7, 5, -1, 2),
                                    independentGrid("sprite_frame1", 3, 2, 2, -1)}};
    const std::array<SGPRect, 2> clips{{{0,0,11,10}, {2,2,9,8}}};
    for (const auto pitch : {26u, 27u})
        for (const auto alignment : {0u, 1u})
            for (UINT16 index = 0; index < grids.size(); ++index)
            {
                for (const auto& clip : clips)
                    for (INT32 y = -8; y <= 12; ++y)
                        for (INT32 x = -9; x <= 13; ++x)
                            drawCase(source, index, grids[index], x, y, pitch, alignment, clip, true);
                const auto& grid = grids[index];
                for (int top = 0; top + grid.height <= surfaceHeight; ++top)
                    for (int left = 0; left + grid.width <= surfaceWidth; ++left)
                        drawCase(source, index, grid, left - grid.x, top - grid.y,
                                 pitch, alignment, clips[0], false);
            }
    SGPRect shared{2,2,9,8}, actual{};
    SetClippingRect(&shared);
    GetClippingRect(&actual);
    check(actual.iLeft == 2 && actual.iTop == 2 && actual.iRight == 9 && actual.iBottom == 8, "shared clipping state round trip");
    for (UINT16 index = 0; index < grids.size(); ++index)
        drawCase(source, index, grids[index], 0, 0, 27, 1, shared, true, true);
    for (const INT32 extreme : {std::numeric_limits<INT32>::min(), std::numeric_limits<INT32>::max()})
        for (UINT16 index = 0; index < grids.size(); ++index)
        {
            drawCase(source, index, grids[index], extreme, 0, 27, 1, clips[0], true);
            drawCase(source, index, grids[index], 0, extreme, 27, 1, clips[0], true);
        }
    Object trailing("sprite_trailing.sti");
    drawCase(trailing, 0, grids[0], 2, 0, 27, 1, clips[0], false);
    drawCase(trailing, 1, grids[1], 0, 2, 26, 0, clips[1], true);
    Object longRuns("long_runs.sti");
    const auto longGrid = independentGrid("long_runs_frame0", 130, 2, 0, 0);
    for (const int x : {-130, -129, -127, -125, -119, -1, 0, 10, 11})
        for (const int y : {-2, -1, 0, 8, 9, 10})
            drawCase(longRuns, 0, longGrid, x, y, 27, 1, clips[0], true);

    // A saved preview is actual renderer output, separate from the oracle.
    Surface preview(26, 0);
    check(Blt8BPPDataTo16BPPBufferTransparent(preview.data(), preview.pitch, &source.object, 2, 0, 0), "draw actual preview frame 0");
    check(Blt8BPPDataTo16BPPBufferTransparent(preview.data(), preview.pitch, &source.object, 5, 7, 1), "draw actual preview frame 1");
    drawScenarios += 2;
    std::ofstream raw(root / "image-preview.rgb565le", std::ios::binary);
    std::ofstream ppm(root / "image-preview.ppm", std::ios::binary);
    ppm << "P6\n11 10\n255\n";
    for (int y = 0; y < surfaceHeight; ++y)
        for (int x = 0; x < surfaceWidth; ++x)
        {
            UINT16 pixel;
            std::memcpy(&pixel, preview.bytes.data() + preview.origin + std::size_t(y) * preview.pitch + std::size_t(x) * 2, 2);
            const char little[2]{static_cast<char>(pixel & 255), static_cast<char>(pixel >> 8)};
            raw.write(little, 2);
            const char rgb[3]{static_cast<char>(((pixel >> 11) & 31) * 255 / 31),
                              static_cast<char>(((pixel >> 5) & 63) * 255 / 63),
                              static_cast<char>((pixel & 31) * 255 / 31)};
            ppm.write(rgb, 3);
        }
    check(bool(raw) && bool(ppm), "write actual RGB565/PPM preview");
}

void invalidDraws()
{
    Object source("sprite.sti");
    SGPRect clip{0,0,11,10};
    auto rejected = [&](SGPVObject object, UINT32 pitch, INT32 x, INT32 y, UINT16 index, bool clipped)
    {
        Surface surface(27, 1);
        const auto before = surface.bytes;
        const auto result = clipped ? Blt8BPPDataTo16BPPBufferTransparentClip(surface.data(), pitch, &object, x, y, index, &clip) :
            Blt8BPPDataTo16BPPBufferTransparent(surface.data(), pitch, &object, x, y, index);
        ++drawScenarios;
        check(!result, "invalid public ETRLE request returns FALSE");
        check(before == surface.bytes, "invalid ETRLE request preserves every destination byte");
    };
    for (const bool clipped : {false, true})
    {
        rejected(source.object, 27, 2, 0, 2, clipped);
        rejected(source.object, 0, 2, 0, 0, clipped);
        rejected(source.object, 2, 2, 0, 0, clipped);
        auto modified = source.object;
        modified.pShadeCurrent = nullptr;
        rejected(modified, 27, 2, 0, 0, clipped);
        modified = source.object;
        modified.uiSizePixData = 39;
        rejected(modified, 27, 2, 0, 0, clipped);
        std::array<ETRLEObject, 2> table{{source.object.pETRLEObject[0], source.object.pETRLEObject[1]}};
        table[0].uiDataLength = 39;
        modified = source.object;
        modified.pETRLEObject = table.data();
        rejected(modified, 27, 2, 0, 0, clipped);
        table[0] = source.object.pETRLEObject[0];
        table[0].uiDataOffset = source.object.uiSizePixData;
        rejected(modified, 27, 2, 0, 0, clipped);
        table[0] = source.object.pETRLEObject[0];
        table[0].usWidth = 0;
        // Unclipped admission is unchanged; clipped no-area success/no-write
        // is covered for both dimensions and owned objects above.
        if (!clipped) rejected(modified, 27, 2, 0, 0, false);
        // Bounded bytes from the fixture, deliberately corrupt one early control.
        std::vector<UINT8> encoded(source.owner.image.p8BPPData,
                                   source.owner.image.p8BPPData + source.owner.image.uiSizePixData);
        encoded[1] = 0xff;
        modified = source.object;
        modified.pPixData = encoded.data();
        rejected(modified, 27, 2, 0, 0, clipped);
    }
    rejected(source.object, 27, 0, 0, 0, false); // signed offset gives negative x
    rejected(source.object, 27, 0, 0, 1, false); // signed offset gives negative y
    Surface surface(27, 1);
    check(!Blt8BPPDataTo16BPPBufferTransparent(nullptr, 27, &source.object, 2, 0, 0), "null destination rejected");
    check(!Blt8BPPDataTo16BPPBufferTransparent(surface.data(), 27, nullptr, 2, 0, 0), "null HVOBJECT rejected");
}

void paletteHelpers()
{
    const std::array<UINT32, 7> rgb{{0x000000, 0x0000ff, 0x00ff00, 0xff0000, 0xffffff, 0x010101, 0x00ffff}};
    for (unsigned i = 0; i < rgb.size(); ++i)
        check(Get16BPPColor(rgb[i]) == colors[i], "fixed Get16BPPColor RGB565 vector");
    const std::array<UINT16, 7> input{{0x0000,0xf800,0x07e0,0x001f,0xffff,0x1234,0x0020}};
    const std::array<UINT16, 7> expected555{{0x0000,0x7c00,0x03e0,0x001f,0x7fff,0x0914,0x0000}};
    const std::array<UINT16, 7> expected655{{0x0000,0xf800,0x03e0,0x001f,0xfbff,0x1114,0x0000}};
    const std::array<UINT16, 7> expected556{{0x0000,0xf800,0x07c0,0x003e,0xfffe,0x1228,0x0000}};
    auto converted = input;
    ConvertRGBDistribution565To555(converted.data(), converted.size());
    check(converted == expected555, "fixed original 565-to-555 vector");
    converted = input;
    ConvertRGBDistribution565To655(converted.data(), converted.size());
    check(converted == expected655, "fixed original 565-to-655 vector");
    converted = input;
    ConvertRGBDistribution565To556(converted.data(), converted.size());
    check(converted == expected556, "fixed original 565-to-556 vector");
}

void shutdown()
{
    fileman::CloseAll();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}

void mount()
{
    auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "initialize actual readonly fixture directory tree");
    auto profile = std::make_unique<vfs::CVirtualProfile>("images", vfs::Path(root.u8string()), false);
    profile->addLocation(directory.get());
    directory.release();
    auto* entry = profile.get();
    getVFS()->getProfileStack()->pushProfile(entry);
    profile.release();
    for (auto it = entry->begin(); !it.end(); it.next())
        check(getVFS()->addLocation(it.value(), entry), "mount actual VFS fixture location");
}
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("expected exact generated image fixture directory");
        root = fs::canonical(argv[1]);
        check(fs::exists(root / "image_manifest.json"), "independent synthetic image manifest exists");
        check(sizeof(void*) == 8, "native 64-bit integration executable");
        ConfigureRGB565ImageFormat();
        mount();
        fullLoads();
        blankFrames();
        masksAndOwnership();
        rejectedAndTransactional();
        paletteHelpers();
        drawing();
        invalidDraws();
        shutdown();
        std::ofstream result(root / "image-result.json");
        check(bool(result), "open native integration result");
        result << "{\"passed\":true,\"checks\":" << checks << ",\"load_scenarios\":" << loadScenarios
               << ",\"draw_scenarios\":" << drawScenarios << ",\"valid_fixture_count\":13,\"malformed_fixture_count\":28,"
               << "\"provider\":\"production FileManVFS + STCI + image ownership/palette + scalar ETRLE\","
               << "\"synthetic_only\":true,\"full_game_startup\":false}\n";
        if (!result) throw std::runtime_error("write native integration result");
        std::cout << "PASS: " << checks << " image checks, " << loadScenarios << " loads, " << drawScenarios << " guarded draws\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        shutdown();
        return 1;
    }
}
