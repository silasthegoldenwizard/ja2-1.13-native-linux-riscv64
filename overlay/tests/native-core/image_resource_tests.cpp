/*
 * P710 native notice preparation: tests/native-core/image_resource_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add resource chooser/factory, PCX/TGA and image ownership integration tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/image_resource_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "himage.h"
#include "pcx.h"
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
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
unsigned checks = 0;
void check(bool condition, const std::string& label)
{
    ++checks;
    if (!condition) throw std::runtime_error(label);
}
using Image = std::unique_ptr<image_type, decltype(&DestroyImage)>;
Image load(const std::string& name, UINT16 mask = IMAGE_ALLDATA,
           ImageFileType::TestOrder order = ImageFileType::DEFAULT)
{
    SGPFILENAME filename{};
    check(name.size() < sizeof(filename), "test filename fits API");
    std::memcpy(filename, name.c_str(), name.size() + 1);
    return Image(CreateImage(filename, mask, order), &DestroyImage);
}
void mount(const std::filesystem::path& root)
{
    auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "readonly actual fixture directory");
    auto profile = std::make_unique<vfs::CVirtualProfile>("resource-inputs", vfs::Path(root.u8string()), false);
    profile->addLocation(directory.get());
    directory.release();
    auto* entry = profile.get();
    getVFS()->getProfileStack()->pushProfile(entry);
    profile.release();
    for (auto it = entry->begin(); !it.end(); it.next())
        check(getVFS()->addLocation(it.value(), entry), "actual VFS resource mount");
}
void shutdown()
{
    fileman::CloseAll();
    vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}
void chooser()
{
    for (const auto order : {ImageFileType::DEFAULT, ImageFileType::JPC_FALLBACK, ImageFileType::PNG_FALLBACK})
    {
        auto image = load("fallback.sti", IMAGE_ALLDATA, order);
        check(image && image->iFileLoader == STCI_FILE_READER && image->usWidth == 3, "STCI fallback policy");
    }
    check(!load("fallback.sti", IMAGE_ALLDATA, ImageFileType::JPC), "strict JPC has no STI fallback");
    check(!load("fallback.sti", IMAGE_ALLDATA, ImageFileType::PNG), "strict PNG has no STI fallback");
    for (const auto order : {ImageFileType::PNG, ImageFileType::PNG_FALLBACK})
    {
        auto image = load("chooser.sti", IMAGE_ALLDATA, order);
        check(image && image->iFileLoader == PNG_FILE_READER && image->usWidth == 2 && image->usHeight == 1,
              "PNG replacement selected rather than original STI");
    }
    auto original = load("chooser.sti", IMAGE_ALLDATA, ImageFileType::DEFAULT);
    check(original && original->iFileLoader == STCI_FILE_READER && original->usWidth == 3, "DEFAULT retains STI");
    auto direct = load("direct.png");
    check(direct && direct->iFileLoader == PNG_FILE_READER && direct->usWidth == 2, "direct PNG dispatch");
    check(!load("broken.sti", IMAGE_ALLDATA, ImageFileType::PNG_FALLBACK),
          "present malformed PNG fails instead of silently falling back after decode error");
    auto noExtension = load("default");
    check(noExtension && noExtension->iFileLoader == PCX_FILE_READER, "default PCX extension");
    auto upper = load("UPPER.PCX");
    check(upper && upper->iFileLoader == PCX_FILE_READER, "case-insensitive extension");
    check(!load("missing.sti"), "missing resource rejected");
    check(!load("padded.pcx", 0x8000), "unknown image contents rejected");
    SGPFILENAME unterminated; std::memset(unterminated, 'x', sizeof(unterminated));
    check(!CreateImage(unterminated, IMAGE_ALLDATA), "unterminated fixed filename rejected");
    check(!CreateImage(nullptr, IMAGE_ALLDATA), "null fixed filename rejected");
    SGPFILENAME extensionOverflow; std::memset(extensionOverflow, 'x', sizeof(extensionOverflow));
    extensionOverflow[sizeof(extensionOverflow) - 1] = 0;
    check(!CreateImage(extensionOverflow, IMAGE_ALLDATA), "added extension cannot overflow filename");
    image_type unknown{}; unknown.iFileLoader = UNKNOWN_FILE_READER;
    check(!LoadImageData(&unknown, IMAGE_ALLDATA), "unknown loader rejected");
    check(!LoadImageData(nullptr, IMAGE_ALLDATA), "null image rejected");
}
void pcx()
{
    const std::array<UINT8, 6> expected{{1, 2, 3, 193, 0, 0}};
    for (UINT16 mask = 0; mask <= IMAGE_ALLDATA; mask = static_cast<UINT16>(mask + 4))
    {
        auto image = load("padded.pcx", mask);
        check(image && image->usWidth == 3 && image->usHeight == 2 && image->ubBitDepth == 8, "PCX dimensions and mask load");
        check(bool(image->fFlags & IMAGE_BITMAPDATA) == bool(mask & IMAGE_BITMAPDATA), "PCX bitmap mask honored");
        check(bool(image->fFlags & IMAGE_PALETTE) == bool(mask & IMAGE_PALETTE), "PCX palette mask honored");
        check(!(image->fFlags & IMAGE_APPDATA), "PCX has no invented appdata");
        if (mask & IMAGE_BITMAPDATA)
            check(image->uiSizePixData == expected.size() && std::memcmp(image->p8BPPData, expected.data(), expected.size()) == 0,
                  "PCX literal pixels skip scanline padding and decode escaped 0xc1");
        if (mask & IMAGE_PALETTE)
        {
            check(image->pPalette && image->pui16BPPPalette, "PCX both original palette representations");
            check(image->pPalette[1].peRed == 255 && image->pPalette[1].peGreen == 0 &&
                  image->pPalette[1].peBlue == 0 && image->pui16BPPPalette[1] == 0xf800, "PCX authored red palette");
        }
        check(LoadImageData(image.get(), IMAGE_ALLDATA), "PCX incremental/replacement load");
        check(std::memcmp(image->p8BPPData, expected.data(), expected.size()) == 0, "PCX full load after any mask");
        check(LoadImageData(image.get(), IMAGE_BITMAPDATA), "PCX bitmap replacement retains palette");
        check(image->pPalette && image->pui16BPPPalette, "PCX retained palette ownership");
    }
    SGPFILENAME name = "padded.pcx";
    PcxObject* object = LoadPcx(name);
    check(object && object->usBytesPerLine == 4, "exported PCX object has wire stride");
    std::array<UINT8, 25> clipped; clipped.fill(99);
    check(BlitPcxToBuffer(object, clipped.data() + 1, 4, 4, 2, 1, TRUE), "PCX clipped transparent API");
    check(clipped[0] == 99 && clipped[17] == 99 && clipped[7] == 1 && clipped[8] == 2 &&
          clipped[11] == 193 && clipped[12] == 99, "PCX literal clip/transparency positions with guards");
    const UINT8 saved = object->pPcxBuffer[0]; object->pPcxBuffer[0] = 0xc0;
    const auto before = clipped;
    check(!BlitPcxToBuffer(object, clipped.data() + 1, 4, 4, 0, 0, FALSE) && clipped == before,
          "invalid exported PCX leaves destination untouched");
    object->pPcxBuffer[0] = saved;
    MemFree(object->pPcxBuffer); MemFree(object);
}
void tga()
{
    const std::array<UINT8, 18> rgb{{255,0,0, 0,255,0, 255,255,0, 0,0,255, 255,255,255, 0,0,0}};
    const std::array<UINT16, 6> words{{0x7c00, 0x03e0, 0x7fe0, 0x001f, 0x7fff, 0}};
    for (const char* name : {"rgb24.tga", "top.tga", "right.tga", "top_right.tga", "rgb16.tga"})
    {
        for (UINT16 mask = 0; mask <= IMAGE_ALLDATA; mask = static_cast<UINT16>(mask + 4))
        {
            auto image = load(name, mask);
            const bool sixteen = std::string(name) == "rgb16.tga";
            check(image && image->usWidth == 3 && image->usHeight == 2 && image->ubBitDepth == (sixteen ? 16 : 24), "TGA dimensions/depth");
            check(!(image->fFlags & (IMAGE_APPDATA | IMAGE_PALETTE)), "TGA no false palette/appdata flags");
            check(bool(image->fFlags & IMAGE_BITMAPDATA) == bool(mask & IMAGE_BITMAPDATA), "TGA contents mask");
            if (mask & IMAGE_BITMAPDATA)
            {
                check(image->uiSizePixData == (sixteen ? 12 : 18), "TGA bounded byte count");
                check(sixteen ? std::memcmp(image->p16BPPData, words.data(), sizeof(words)) == 0 :
                               std::memcmp(image->p8BPPData, rgb.data(), rgb.size()) == 0,
                      "TGA literal RGB/sample oracle across origins");
            }
            check(LoadImageData(image.get(), IMAGE_BITMAPDATA), "TGA incremental and repeated load");
        }
    }
}
void errorsAndOwnership()
{
    for (const char* name : {"bad_short.pcx", "bad_palette.pcx", "bad_marker.pcx", "bad_zero_run.pcx", "bad_cross_row.pcx",
            "bad_truncated_run.pcx", "bad_underflow.pcx", "bad_overflow.pcx", "bad_stride.pcx", "bad_trailing.pcx",
            "bad_zero_size.tga", "bad_short.tga", "bad_header.tga", "bad_rle.tga", "bad_depth.tga", "bad_interleave.tga"})
        for (UINT16 mask = 0; mask <= IMAGE_ALLDATA; mask = static_cast<UINT16>(mask + 4))
            check(!load(name, mask), std::string("malformed resource rejected for any section: ") + name);
    for (const auto& pair : {std::pair<const char*, const char*>{"padded.pcx", "bad_short.pcx"},
                           {"rgb24.tga", "bad_short.tga"}})
    {
        auto image = load(pair.first);
        const auto original = *image;
        std::strcpy(image->ImageFile, pair.second);
        check(!LoadImageData(image.get(), IMAGE_ALLDATA), "failed resource replacement rejected");
        check(image->p8BPPData == original.p8BPPData && image->pPalette == original.pPalette &&
              image->fFlags == original.fFlags && image->usWidth == original.usWidth, "failure preserves image allocations");
    }
    auto sprite = load("sprite.sti");
    check(bool(sprite), "CreateImage actual STCI compressed sprite");
    ETRLEData copy{};
    check(GetETRLEImageData(sprite.get(), &copy), "production ETRLE deep copy");
    check(copy.pPixData != sprite->pPixData8 && copy.pETRLEObject != sprite->pETRLEObject &&
          copy.usNumberOfObjects == 2, "ETRLE independent allocations");
    const std::array<UINT8, 11> frame1{{1,1,0x81,1,2,0,3,0,3,4,0}};
    const auto frame = copy.pETRLEObject[1];
    check(frame.usWidth == 3 && frame.usHeight == 2 && frame.sOffsetX == 2 && frame.sOffsetY == -1 &&
          frame.uiDataLength == frame1.size(), "ETRLE literal descriptor");
    check(std::memcmp(static_cast<UINT8*>(copy.pPixData) + frame.uiDataOffset, frame1.data(), frame1.size()) == 0,
          "ETRLE literal copied stream");
    sprite->pPixData8[frame.uiDataOffset] = 99; sprite->pETRLEObject[1].sOffsetX = 99;
    sprite.reset();
    check(copy.pETRLEObject[1].sOffsetX == 2 && static_cast<UINT8*>(copy.pPixData)[frame.uiDataOffset] == 1,
          "ETRLE deep copy survives source mutation and destruction");
    MemFree(copy.pETRLEObject); MemFree(copy.pPixData);
    ETRLEData untouched{}; untouched.uiSizePixData = 123;
    check(!GetETRLEImageData(nullptr, &untouched) && untouched.uiSizePixData == 123, "invalid ETRLE does not alter output");
    auto plain = load("padded.pcx");
    check(!GetETRLEImageData(plain.get(), &untouched), "plain image is not mistaken for compressed data");
}
}
int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("expected generated resource fixture directory");
        const auto root = std::filesystem::canonical(argv[1]);
        check(std::filesystem::is_regular_file(root / "resource_manifest.json"), "synthetic fixture manifest");
        ConfigureRGB565ImageFormat();
        mount(root);
        chooser(); pcx(); tga(); errorsAndOwnership();
        shutdown();
        std::cout << "PASS: " << checks << " production resource/ownership checks\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        shutdown();
        return 1;
    }
}
