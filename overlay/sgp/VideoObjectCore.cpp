/*
 * P710 native notice preparation: sgp/VideoObjectCore.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract and adapt original video-object ownership/manager boundary.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

// Original v1.13 video-object ownership and manager boundary. Surface/blit
// dispatch and legacy diagnostic list code remain in vobject.cpp.
#include "video_object_core.h"
#include "shading.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <unordered_map>

namespace
{
void release(void* memory) { if (memory) MemFree(memory); }

bool borrowedShade(HVOBJECT object, const UINT16* palette)
{
    if (!(object->fFlags & VOBJECT_FLAG_SHADETABLE_SHARED)) return false;
    return std::find(std::begin(object->pShades), std::end(object->pShades), palette) != std::end(object->pShades);
}

// Replacing the base palette must also update aliases to it. Shared shade
// tables belong to another object and may neither be freed nor overwritten.
void replaceBasePalette(HVOBJECT object, UINT16* palette)
{
    auto* old = object->p16BPPPalette;
    const bool borrowed = old && borrowedShade(object, old);
    if (!(object->fFlags & VOBJECT_FLAG_SHADETABLE_SHARED))
        for (auto*& shade : object->pShades) if (old && shade == old) shade = palette;
    if (old && !borrowed && object->pGlow == old) object->pGlow = palette;
    if (old && !borrowed) release(old);
    object->p16BPPPalette = palette;
    object->pShadeCurrent = palette;
}
}

#ifndef SGP_VIDEO_DEBUGGING
BOOLEAN gfVideoObjectsInit = FALSE;
UINT32 guiVObjectIndex = 1;
UINT32 guiVObjectSize = 0;
UINT32 guiVObjectTotalAdded = 0;
namespace { std::unordered_map<UINT32, HVOBJECT> objects; }

BOOLEAN InitializeVideoObjectManager()
{
    if (gfVideoObjectsInit || !objects.empty()) return FALSE;
    gfVideoObjectsInit = TRUE;
    return TRUE;
}
BOOLEAN ShutdownVideoObjectManager()
{
    for (auto& entry : objects) DeleteVideoObject(entry.second);
    objects.clear();
    guiVObjectIndex = 1;
    guiVObjectSize = guiVObjectTotalAdded = 0;
    gfVideoObjectsInit = FALSE;
    return TRUE;
}
UINT32 CountVideoObjectNodes() { return static_cast<UINT32>(objects.size()); }
BOOLEAN AddStandardVideoObject(VOBJECT_DESC* description, UINT32* index)
{
    if (!gfVideoObjectsInit || !description || !index || guiVObjectIndex >= 0xfffffff0u) return FALSE;
    auto* object = CreateVideoObject(description);
    if (!object) return FALSE;
    SetVideoObjectTransparencyColor(object, FROMRGB(0, 0, 0));
    try
    {
        const auto inserted = objects.emplace(guiVObjectIndex, object);
        if (!inserted.second) { DeleteVideoObject(object); return FALSE; }
    }
    catch (const std::bad_alloc&) { DeleteVideoObject(object); return FALSE; }
    *index = guiVObjectIndex++;
    ++guiVObjectSize;
    ++guiVObjectTotalAdded;
    return TRUE;
}
BOOLEAN GetVideoObject(HVOBJECT* object, UINT32 index)
{
    if (!object) return FALSE;
    const auto entry = objects.find(index);
    *object = entry == objects.end() ? nullptr : entry->second;
    return *object != nullptr;
}
BOOLEAN DeleteVideoObjectFromIndex(UINT32 index)
{
    const auto entry = objects.find(index);
    if (entry == objects.end()) return FALSE;
    DeleteVideoObject(entry->second);
    objects.erase(entry);
    --guiVObjectSize;
    return TRUE;
}
BOOLEAN SetVideoObjectTransparency(UINT32 index, COLORVAL color)
{
    HVOBJECT object;
    return GetVideoObject(&object, index) && SetVideoObjectTransparencyColor(object, color);
}
#endif

HVOBJECT CreateVideoObject(VOBJECT_DESC* description)
{
    if (!description) return nullptr;
    const bool fromFile = (description->fCreateFlags & VOBJECT_CREATE_FROMFILE) != 0;
    if (!fromFile && !(description->fCreateFlags & VOBJECT_CREATE_FROMHIMAGE)) return nullptr;
    HIMAGE image = nullptr;
    if (fromFile)
    {
        if (!std::memchr(description->ImageFile, 0, SGPFILENAME_LEN)) return nullptr;
        ImageFileType::TestOrder order = ImageFileType::JPC_FALLBACK;
        if (description->fCreateFlags & VOBJECT_CREATE_FROMJPC) order = ImageFileType::JPC;
        else if (description->fCreateFlags & VOBJECT_CREATE_FROMJPC_FALLBACK) order = ImageFileType::JPC_FALLBACK;
        else if (description->fCreateFlags & VOBJECT_CREATE_FROMPNG) order = ImageFileType::PNG;
        else if (description->fCreateFlags & VOBJECT_CREATE_FROMPNG_FALLBACK) order = ImageFileType::PNG_FALLBACK;
        image = CreateImage(description->ImageFile, IMAGE_ALLIMAGEDATA, order);
    }
    else image = description->hImage;
    if (!image) return nullptr;
    auto* object = static_cast<HVOBJECT>(MemAlloc(sizeof(SGPVObject)));
    if (object) std::memset(object, 0, sizeof(*object));
    auto fail = [&]() -> HVOBJECT {
        if (object) DeleteVideoObject(object);
        if (fromFile) DestroyImage(image);
        return nullptr;
    };
    if (!object) return fail();
    object->ubBitDepth = image->ubBitDepth;
    std::memcpy(object->ImageFile, image->ImageFile, sizeof(object->ImageFile));
    object->ImageFile[SGPFILENAME_LEN - 1] = 0;
    if (image->ubBitDepth == 16 || image->ubBitDepth == 32)
    {
        if (!(image->fFlags & IMAGE_BITMAPDATA) || (image->fFlags & (IMAGE_COMPRESSED | IMAGE_TRLECOMPRESSED)) ||
            !image->pImageData || !image->pETRLEObject || !image->usNumberOfObjects) return fail();
        // Original true-color reader contract: one descriptor holds the raw
        // 16/32-bit image. Keep its offsets/dimensions and deep-copy the pixels.
        const auto& frame = image->pETRLEObject[0];
        const std::size_t pixels = std::size_t(frame.usWidth) * frame.usHeight;
        const unsigned pixelBytes = image->ubBitDepth / 8u;
        if (!frame.usWidth || !frame.usHeight || pixels > (std::numeric_limits<UINT32>::max)() / pixelBytes ||
            frame.uiDataOffset > image->uiSizePixData ||
            frame.uiDataLength > image->uiSizePixData - frame.uiDataOffset ||
            pixels * pixelBytes > frame.uiDataLength) return fail();
        const std::size_t bytes = pixels * pixelBytes;
        object->p16BPPObject = static_cast<SixteenBPPObjectInfo*>(MemAlloc(sizeof(SixteenBPPObjectInfo)));
        if (!object->p16BPPObject) return fail();
        std::memset(object->p16BPPObject, 0, sizeof(SixteenBPPObjectInfo));
        object->usNumberOf16BPPObjects = 1;
        auto& region = object->p16BPPObject[0];
        region.p16BPPData = static_cast<UINT16*>(MemAlloc(bytes));
        if (!region.p16BPPData) return fail();
        std::memcpy(region.p16BPPData, static_cast<const UINT8*>(image->pImageData) + frame.uiDataOffset, bytes);
        region.sOffsetX = frame.sOffsetX;
        region.sOffsetY = frame.sOffsetY;
        region.usWidth = frame.usWidth;
        region.usHeight = frame.usHeight;
    }
    else
    {
        if (image->ubBitDepth != 8 || !(image->fFlags & IMAGE_TRLECOMPRESSED) || !image->pPalette) return fail();
        ETRLEData data{};
        if (!GetETRLEImageData(image, &data)) return fail();
        object->pPixData = data.pPixData;
        object->pETRLEObject = data.pETRLEObject;
        object->uiSizePixData = data.uiSizePixData;
        object->usNumberOfObjects = data.usNumberOfObjects;
        object->pShade8 = ubColorTables[DEFAULT_SHADE_LEVEL];
        object->pGlow8 = ubColorTables[0];
        if (!SetVideoObjectPalette(object, image->pPalette)) return fail();
    }
    if (fromFile) DestroyImage(image);
    return object;
}

BOOLEAN SetVideoObjectPalette(HVOBJECT object, SGPPaletteEntry* source)
{
    if (!object || !source) return FALSE;
    auto* palette = Create16BPPPalette(source);
    if (!palette) return FALSE;
    auto* entries = object->pPaletteEntry;
    if (!entries) entries = static_cast<SGPPaletteEntry*>(MemAlloc(sizeof(SGPPaletteEntry) * 256));
    if (!entries) { release(palette); return FALSE; }
    std::memmove(entries, source, sizeof(SGPPaletteEntry) * 256);
    object->pPaletteEntry = entries;
    replaceBasePalette(object, palette);
    return TRUE;
}
BOOLEAN SetVideoObjectTransparencyColor(HVOBJECT object, COLORVAL color)
{
    if (!object) return FALSE;
    object->TransparentColor = color;
    return TRUE;
}
BOOLEAN DestroyObjectPaletteTables(HVOBJECT object)
{
    if (!object) return FALSE;
    // A base palette may alias the neutral shade. Free each owned allocation
    // exactly once, even if the caller aliases more than one shade entry.
    auto* base = object->p16BPPPalette;
    const bool borrowed = base && borrowedShade(object, base);
    if (!(object->fFlags & VOBJECT_FLAG_SHADETABLE_SHARED))
    {
        for (unsigned i = 0; i < HVOBJECT_SHADE_TABLES; ++i)
        {
            auto* shade = object->pShades[i];
            if (!shade) continue;
            for (unsigned j = i + 1; j < HVOBJECT_SHADE_TABLES; ++j)
                if (object->pShades[j] == shade) object->pShades[j] = nullptr;
            if (shade == base) base = nullptr;
            release(shade);
            object->pShades[i] = nullptr;
        }
    }
    std::fill(std::begin(object->pShades), std::end(object->pShades), nullptr);
    if (base && !borrowed) release(base);
    object->p16BPPPalette = object->pShadeCurrent = object->pGlow = nullptr;
    return TRUE;
}
BOOLEAN DeleteVideoObject(HVOBJECT object)
{
    if (!object) return FALSE;
    DestroyObjectPaletteTables(object);
    release(object->pPaletteEntry);
    release(object->pPixData);
    release(object->pETRLEObject);
    if (object->ppZStripInfo)
    {
        for (unsigned i = 0; i < object->usNumberOfObjects; ++i)
            if (object->ppZStripInfo[i]) { release(object->ppZStripInfo[i]->pbZChange); release(object->ppZStripInfo[i]); }
        release(object->ppZStripInfo);
    }
    if (object->p16BPPObject)
    {
        for (unsigned i = 0; i < object->usNumberOf16BPPObjects; ++i) release(object->p16BPPObject[i].p16BPPData);
        release(object->p16BPPObject);
    }
    release(object);
    return TRUE;
}
UINT16 CreateObjectPaletteTables(HVOBJECT object, UINT32 type)
{
    if (!object || !object->pPaletteEntry || type > HVOBJECT_GLOW_RED ||
        (object->fFlags & VOBJECT_FLAG_SHADETABLE_SHARED)) return FALSE;
    UINT16* tables[16]{};
    const UINT32 glow[4][3] = {{0,255,0},{0,0,255},{255,255,0},{255,0,0}};
    const UINT32 scales[16] = {0,293,281,268,255,195,165,135,105,75,45,36,27,18,9,0};
    for (unsigned i = 0; i < 16; ++i)
    {
        tables[i] = i == 4 && object->p16BPPPalette ? object->p16BPPPalette :
            (i == 0 ? Create16BPPPaletteShaded(object->pPaletteEntry, glow[type][0], glow[type][1], glow[type][2], TRUE) :
                      Create16BPPPaletteShaded(object->pPaletteEntry, scales[i], scales[i], scales[i], FALSE));
        if (!tables[i])
        {
            for (auto* table : tables) if (table != object->p16BPPPalette) release(table);
            return FALSE;
        }
    }
    auto* glowTable = object->pGlow;
    for (unsigned i = 0; i < 16; ++i)
    {
        auto* old = object->pShades[i];
        if (old && old != object->p16BPPPalette && old == object->pGlow)
        { glowTable = tables[i]; break; }
    }
    // Only the original first 16 tables are generated. Preserve the historic
    // 48-entry completeness result; higher custom tables remain caller-owned.
    for (unsigned i = 0; i < 16; ++i)
    {
        auto* old = object->pShades[i];
        if (old && old != object->p16BPPPalette)
        {
            for (unsigned j = i + 1; j < HVOBJECT_SHADE_TABLES; ++j)
                if (object->pShades[j] == old) object->pShades[j] = nullptr;
            release(old);
        }
        object->pShades[i] = tables[i];
    }
    object->p16BPPPalette = tables[4];
    object->pShadeCurrent = tables[4];
    object->pGlow = glowTable;
    return std::all_of(std::begin(object->pShades), std::end(object->pShades), [](const UINT16* table){ return table != nullptr; });
}
UINT16 SetObjectShade(HVOBJECT object, UINT32 shade)
{
    if (!object || shade >= HVOBJECT_SHADE_TABLES || !object->pShades[shade]) return FALSE;
    object->pShadeCurrent = object->pShades[shade];
    return TRUE;
}
UINT16 SetObjectHandleShade(UINT32 handle, UINT32 shade)
{
    HVOBJECT object;
    return GetVideoObject(&object, handle) && SetObjectShade(object, shade);
}
BOOLEAN GetVideoObjectETRLEProperties(HVOBJECT object, ETRLEObject* region, UINT16 index)
{
    if (!object || !region || !object->pETRLEObject || index >= object->usNumberOfObjects) return FALSE;
    *region = object->pETRLEObject[index];
    return TRUE;
}
BOOLEAN GetVideoObjectETRLEPropertiesFromIndex(UINT32 handle, ETRLEObject* region, UINT16 index)
{
    HVOBJECT object;
    return GetVideoObject(&object, handle) && GetVideoObjectETRLEProperties(object, region, index);
}
BOOLEAN GetVideoObjectETRLESubregionProperties(UINT32 handle, UINT16 index, UINT16* width, UINT16* height)
{
    ETRLEObject region;
    if (!width || !height || !GetVideoObjectETRLEPropertiesFromIndex(handle, &region, index)) return FALSE;
    *width = region.usWidth; *height = region.usHeight;
    return TRUE;
}
BOOLEAN SetVideoObjectPalette8BPP(INT32 handle, SGPPaletteEntry* palette)
{
    HVOBJECT object;
    return GetVideoObject(&object, static_cast<UINT32>(handle)) && SetVideoObjectPalette(object, palette);
}
BOOLEAN GetVideoObjectPalette16BPP(INT32 handle, UINT16** palette)
{
    HVOBJECT object;
    if (!palette || !GetVideoObject(&object, static_cast<UINT32>(handle))) return FALSE;
    *palette = object->p16BPPPalette;
    return TRUE;
}
BOOLEAN CopyVideoObjectPalette16BPP(INT32 handle, UINT16* palette)
{
    UINT16* source;
    if (!palette || !GetVideoObjectPalette16BPP(handle, &source) || !source) return FALSE;
    std::memcpy(palette, source, sizeof(UINT16) * 256);
    return TRUE;
}
BOOLEAN CheckFor16BPPRegion(HVOBJECT object, UINT16 region, UINT8 shade, UINT16* index)
{
    if (!object || !object->p16BPPObject) return FALSE;
    for (UINT16 i = 0; i < object->usNumberOf16BPPObjects; ++i)
        if (object->p16BPPObject[i].usRegionIndex == region && object->p16BPPObject[i].ubShadeLevel == shade)
        { if (index) *index = i; return TRUE; }
    return FALSE;
}
void GetVideoObjectDimensions(HVOBJECT object, UINT16 index, UINT16& width, UINT16& height)
{
    width = height = 0;
    if (!object) return;
    if ((object->ubBitDepth == 16 || object->ubBitDepth == 32) && object->p16BPPObject && index < object->usNumberOf16BPPObjects)
    { width = object->p16BPPObject[index].usWidth; height = object->p16BPPObject[index].usHeight; }
    else if (object->ubBitDepth == 8 && object->pETRLEObject && index < object->usNumberOfObjects)
    { width = object->pETRLEObject[index].usWidth; height = object->pETRLEObject[index].usHeight; }
}
BOOLEAN GetETRLEPixelValue(UINT8* destination, HVOBJECT object, UINT16 index, UINT16 x, UINT16 y)
{
    ETRLEObject region;
    if (!destination || !GetVideoObjectETRLEProperties(object, &region, index) || object->ubBitDepth != 8 ||
        !object->pPixData || x >= region.usWidth || y >= region.usHeight ||
        region.uiDataOffset > object->uiSizePixData || region.uiDataLength > object->uiSizePixData - region.uiDataOffset) return FALSE;
    const auto* current = static_cast<const UINT8*>(object->pPixData) + region.uiDataOffset;
    const auto* end = current + region.uiDataLength;
    for (unsigned row = 0; row <= y; ++row)
    {
        unsigned column = 0;
        bool found = false;
        UINT8 value = 0;
        for (;;)
        {
            if (current == end) return FALSE;
            const UINT8 code = *current++;
            if (!code) break;
            const unsigned length = code & 0x7fu;
            // 0x80 is a legal zero-length transparent no-op; 0x00 alone
            // terminates the scanline. Existing STCI and blitters retain it.
            if (length > region.usWidth - column) return FALSE;
            if (!(code & 0x80u) && std::size_t(end - current) < length) return FALSE;
            if (row == y && x >= column && x < column + length)
            { value = code & 0x80u ? 0 : current[x - column]; found = true; }
            if (!(code & 0x80u)) current += length;
            column += length;
        }
        if (column != region.usWidth) return FALSE;
        if (row == y && found) { *destination = value; return TRUE; }
    }
    return FALSE;
}
