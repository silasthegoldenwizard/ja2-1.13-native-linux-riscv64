/*
 * P710 native notice preparation: sgp/ImageData.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original image ownership operations.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/himage.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/ImageData.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original v1.13 image ownership operations, without file-format/platform code.
#include "himage.h"
#include <cassert>

BOOLEAN ReleaseImageData(HIMAGE image, UINT16 contents)
{
    assert(image != nullptr);
    if (!image) return FALSE;
    if ((contents & IMAGE_PALETTE) && (image->fFlags & IMAGE_PALETTE))
    {
        if (image->pPalette) MemFree(image->pPalette);
        if (image->pui16BPPPalette) MemFree(image->pui16BPPPalette);
        image->pPalette = nullptr;
        image->pui16BPPPalette = nullptr;
        image->fFlags &= static_cast<UINT16>(~IMAGE_PALETTE);
    }
    if ((contents & IMAGE_BITMAPDATA) && (image->fFlags & IMAGE_BITMAPDATA))
    {
        MemFree(image->pImageData);
        if (image->usNumberOfObjects) MemFree(image->pETRLEObject);
        image->pImageData = nullptr;
        image->pETRLEObject = nullptr;
        image->uiSizePixData = 0;
        image->usNumberOfObjects = 0;
        image->fFlags &= static_cast<UINT16>(~(IMAGE_BITMAPDATA | IMAGE_TRLECOMPRESSED | IMAGE_COMPRESSED));
    }
    if ((contents & IMAGE_APPDATA) && (image->fFlags & IMAGE_APPDATA))
    {
        if (image->pAppData) MemFree(image->pAppData);
        image->pAppData = nullptr;
        image->uiAppDataSize = 0;
        image->fFlags &= static_cast<UINT16>(~IMAGE_APPDATA);
    }
    return TRUE;
}

BOOLEAN DestroyImage(HIMAGE image)
{
    assert(image != nullptr);
    if (!image) return FALSE;
    ReleaseImageData(image, IMAGE_ALLDATA);
    MemFree(image);
    return TRUE;
}
