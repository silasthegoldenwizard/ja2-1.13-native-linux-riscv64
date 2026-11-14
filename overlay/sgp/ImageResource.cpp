/*
 * P710 native notice preparation: sgp/ImageResource.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract and adapt original resource factory/loading path.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/himage.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/ImageResource.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production v1.13 image chooser, dispatch and ETRLE ownership operations.
// Extracted from himage.cpp; platform rendering remains in its original unit.
#include "himage.h"
#include "pcx.h"
#include "impTGA.h"
#include "STCI.h"
#include "PngLoader.h"
#include <vfs/Core/vfs.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

namespace
{
int readerFor(std::string& filename, ImageFileType::TestOrder order)
{
    auto dot = filename.find_last_of('.');
    const auto slash = filename.find_last_of("/\\");
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash) || dot + 1 == filename.size())
    {
        filename += (dot != std::string::npos && dot + 1 == filename.size()) ? "pcx" : ".pcx";
        dot = filename.find_last_of('.');
    }
    std::string ext = filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
        return static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
    });
    int reader = UNKNOWN_FILE_READER;
    if (ext == "pcx") reader = PCX_FILE_READER;
    else if (ext == "tga") reader = TGA_FILE_READER;
    else if (ext == "sti") reader = STCI_FILE_READER;
    else if (ext == "png") reader = PNG_FILE_READER;
    else if (ext == "7z" && dot >= 4)
    {
        std::string compound = filename.substr(dot - 4, 4);
        std::transform(compound.begin(), compound.end(), compound.begin(), [](unsigned char c) {
            return static_cast<char>(c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c);
        });
        if (compound == ".jpc") reader = JPC_FILE_READER;
    }
    if (reader == UNKNOWN_FILE_READER) return reader;
    if (order == ImageFileType::DEFAULT || reader != STCI_FILE_READER)
        return getVFS()->fileExists(filename) ? reader : UNKNOWN_FILE_READER;
    const bool jpc = order == ImageFileType::JPC || order == ImageFileType::JPC_FALLBACK;
    if (!jpc && order != ImageFileType::PNG && order != ImageFileType::PNG_FALLBACK)
        return UNKNOWN_FILE_READER;
    const std::string replacement = filename.substr(0, dot + 1) + (jpc ? "jpc.7z" : "png");
    if (getVFS()->fileExists(replacement))
    {
        filename = replacement;
        return jpc ? JPC_FILE_READER : PNG_FILE_READER;
    }
    if (order == ImageFileType::JPC_FALLBACK || order == ImageFileType::PNG_FALLBACK)
        return getVFS()->fileExists(filename) ? reader : UNKNOWN_FILE_READER;
    return UNKNOWN_FILE_READER;
}
}

HIMAGE CreateImage(SGPFILENAME imageFile, UINT16 contents, ImageFileType::TestOrder order)
try
{
    if (!imageFile || (contents & ~IMAGE_ALLDATA)) return nullptr;
    // The API's filename is a fixed SGPFILENAME. Never scan beyond that contract.
    std::size_t length = 0;
    while (length < SGPFILENAME_LEN && imageFile[length]) ++length;
    if (!length || length == SGPFILENAME_LEN) return nullptr;
    const char* end = imageFile + length;
    std::string filename(imageFile, static_cast<std::size_t>(end - imageFile));
    const int reader = readerFor(filename, order);
    if (reader == UNKNOWN_FILE_READER || filename.size() >= SGPFILENAME_LEN) return nullptr;
    HIMAGE image = static_cast<HIMAGE>(MemAlloc(sizeof(image_type)));
    if (!image) return nullptr;
    std::memset(image, 0, sizeof(*image));
    std::memcpy(image->ImageFile, filename.c_str(), filename.size() + 1);
    image->iFileLoader = static_cast<UINT32>(reader);
    if (!LoadImageData(image, contents))
    {
        DestroyImage(image);
        return nullptr;
    }
    return image;
}
catch (...) { return nullptr; }

BOOLEAN LoadImageData(HIMAGE image, UINT16 contents)
try
{
    if (!image || (contents & ~IMAGE_ALLDATA) ||
        !std::memchr(image->ImageFile, '\0', sizeof(image->ImageFile))) return FALSE;
    switch (image->iFileLoader)
    {
        case TGA_FILE_READER: return LoadTGAFileToImage(image, contents);
        case PCX_FILE_READER: return LoadPCXFileToImage(image, contents);
        case STCI_FILE_READER: return LoadSTCIFileToImage(image, contents);
        case PNG_FILE_READER: return LoadPNGFileToImage(image, contents) ? TRUE : FALSE;
        case JPC_FILE_READER: return LoadJPCFileToImage(image, contents) ? TRUE : FALSE;
        default: return FALSE;
    }
}
catch (...) { return FALSE; }

BOOLEAN GetETRLEImageData(HIMAGE image, ETRLEData* buffer)
{
    if (!image || !buffer || !(image->fFlags & IMAGE_TRLECOMPRESSED) ||
        !(image->fFlags & IMAGE_BITMAPDATA) || !image->pETRLEObject ||
        !image->pPixData8 || !image->usNumberOfObjects || !image->uiSizePixData) return FALSE;
    for (UINT16 i = 0; i < image->usNumberOfObjects; ++i)
    {
        const auto& object = image->pETRLEObject[i];
        if (object.uiDataOffset > image->uiSizePixData ||
            object.uiDataLength > image->uiSizePixData - object.uiDataOffset) return FALSE;
    }
    ETRLEData copy{};
    const std::size_t objectBytes = sizeof(ETRLEObject) * image->usNumberOfObjects;
    copy.pETRLEObject = static_cast<ETRLEObject*>(MemAlloc(objectBytes));
    if (!copy.pETRLEObject) return FALSE;
    copy.pPixData = MemAlloc(image->uiSizePixData);
    if (!copy.pPixData)
    {
        MemFree(copy.pETRLEObject);
        return FALSE;
    }
    copy.usNumberOfObjects = image->usNumberOfObjects;
    copy.uiSizePixData = image->uiSizePixData;
    std::memcpy(copy.pETRLEObject, image->pETRLEObject, objectBytes);
    std::memcpy(copy.pPixData, image->pPixData8, copy.uiSizePixData);
    *buffer = copy;
    return TRUE;
}
