/*
 * P710 native notice preparation: sgp/etrle_blitters.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: translate original v1.13 ETRLE assembly kernels to scalar code.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject_blitters.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/etrle_blitters.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "etrle_blitters.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

// Scalar translations of the original v1.13 transparent ETRLE kernels from
// sgp/vobject_blitters.cpp:10212 and :10526 at engine revision
// 3394695a387a14c78e92b158cfdcf50e4068c96b. Retain the engine's source license.
// No Stracciatella implementation is used. Other sprite families remain in
// vobject_blitters.cpp. Decode controls structurally, rather than searching
// literal payload bytes for zero (the old clipped assembly's row-skip bug).
namespace sgp { namespace etrle {

bool DescribeSprite(HVOBJECT source, UINT16 index, INT32 x, INT32 y, Sprite& sprite, bool allowEmpty)
{
    if (!source || !source->pETRLEObject || index >= source->usNumberOfObjects ||
        source->ubBitDepth != 8)
        return false;

    sprite.object = &source->pETRLEObject[index];
    if (!allowEmpty && (sprite.object->usWidth == 0 || sprite.object->usHeight == 0))
        return false;
    sprite.x = static_cast<std::int64_t>(x) + sprite.object->sOffsetX;
    sprite.y = static_cast<std::int64_t>(y) + sprite.object->sOffsetY;
    return true;
}

bool BindSource(HVOBJECT source, Sprite& sprite)
{
    const auto& object = *sprite.object;
    if (!source->pPixData || !source->pShadeCurrent || object.uiDataLength == 0 ||
        object.uiDataOffset > source->uiSizePixData ||
        object.uiDataLength > source->uiSizePixData - object.uiDataOffset)
        return false;

    sprite.data = static_cast<const UINT8*>(source->pPixData) + object.uiDataOffset;
    sprite.length = object.uiDataLength;
    return true;
}

// Verify every declared row before any destination mutation. Subimage bounds
// may include padding; bytes beyond the final declared row are not decoded.
bool ValidateEncoding(const Sprite& sprite)
{
    std::size_t cursor = 0;
    for (UINT32 row = 0; row < sprite.object->usHeight; ++row)
    {
        UINT32 column = 0;
        for (;;)
        {
            if (cursor == sprite.length)
                return false;
            const UINT8 control = sprite.data[cursor++];
            if (control == 0)
            {
                if (column != sprite.object->usWidth)
                    return false;
                break;
            }
            const UINT32 run = control & 0x7f;
            if (run > sprite.object->usWidth - column)
                return false;
            column += run;
            if ((control & 0x80) == 0)
            {
                if (run > sprite.length - cursor)
                    return false;
                cursor += run;
            }
        }
    }
    return true;
}

bool ValidDestination(UINT32 pitch, std::int64_t left, std::int64_t top,
    std::int64_t right, std::int64_t bottom)
{
    if (pitch == 0 || left < 0 || top < 0 || right <= left || bottom <= top ||
        static_cast<std::uint64_t>(right) * sizeof(UINT16) > pitch)
        return false;

    // Coordinates and pitch originate in at most 32-bit values, so this product
    // fits uint64_t even when signed coordinate+object-offset exceeds INT32_MAX.
    const std::uint64_t end = static_cast<std::uint64_t>(bottom - 1) * pitch +
        static_cast<std::uint64_t>(right) * sizeof(UINT16);
    return end <= static_cast<std::uint64_t>(std::numeric_limits<std::ptrdiff_t>::max());
}

void ForEachLiteralRun(const Sprite& sprite, LiteralRun visit, void* context)
{
    std::size_t cursor = 0;
    for (UINT32 row = 0; row < sprite.object->usHeight; ++row)
    {
        UINT32 column = 0;
        for (;;)
        {
            const UINT8 control = sprite.data[cursor++];
            if (!control) break;
            const UINT32 run = control & 0x7f;
            if (!(control & 0x80))
            {
                visit(context, row, column, sprite.data + cursor, run);
                cursor += run;
            }
            column += run;
        }
    }
}
}}
namespace {
using namespace sgp::etrle;

struct TransparentDraw
{
    UINT8* buffer;
    UINT32 pitch;
    const Sprite* sprite;
    const UINT16* palette;
    std::int64_t left, top, right, bottom;
    bool outline;
    UINT16 outlineColor;
    bool drawOutline;
};
void DrawTransparentRun(void* context, UINT32 row, UINT32 column,
    const UINT8* pixels, UINT32 count)
{
    auto& draw = *static_cast<TransparentDraw*>(context);
    const std::int64_t y = draw.sprite->y + row;
    if (y < draw.top || y >= draw.bottom) return;
    const std::int64_t runLeft = draw.sprite->x + column;
    const auto first = std::max(draw.left, runLeft);
    const auto last = std::min(draw.right, runLeft + count);
    for (std::int64_t x = first; x < last; ++x)
    {
        const UINT8 index = pixels[static_cast<std::size_t>(x-runLeft)];
        const UINT16 pixel = draw.outline && index == 254 ? draw.outlineColor : draw.palette[index];
        // Retain the accepted byte-pitch contract, including odd pitch.
        if (!draw.outline || index != 254 || draw.drawOutline)
            std::memcpy(draw.buffer + static_cast<std::size_t>(y)*draw.pitch +
                static_cast<std::size_t>(x)*sizeof(UINT16), &pixel, sizeof(pixel));
    }
}
void DrawSprite(UINT16* buffer, UINT32 pitch, const Sprite& sprite,
    const UINT16* palette, std::int64_t left, std::int64_t top,
    std::int64_t right, std::int64_t bottom, bool outline, UINT16 outlineColor,
    bool drawOutline)
{
    TransparentDraw draw{reinterpret_cast<UINT8*>(buffer),pitch,&sprite,palette,
        left,top,right,bottom,outline,outlineColor,drawOutline};
    ForEachLiteralRun(sprite,DrawTransparentRun,&draw);
}

BOOLEAN Blit(UINT16* buffer, UINT32 pitch, HVOBJECT source, INT32 x, INT32 y,
    UINT16 index, const SGPRect* clip, bool outline = false,
    UINT16 outlineColor = 0, bool drawOutline = false)
{
    Sprite sprite{};
    if (!buffer || !DescribeSprite(source, index, x, y, sprite, clip != nullptr))
        return FALSE;

    std::int64_t left = sprite.x;
    std::int64_t top = sprite.y;
    std::int64_t right = sprite.x + sprite.object->usWidth;
    std::int64_t bottom = sprite.y + sprite.object->usHeight;
    if (clip)
    {
        left = std::max(left, static_cast<std::int64_t>(clip->iLeft));
        top = std::max(top, static_cast<std::int64_t>(clip->iTop));
        right = std::min(right, static_cast<std::int64_t>(clip->iRight));
        bottom = std::min(bottom, static_cast<std::int64_t>(clip->iBottom));
        if (left >= right || top >= bottom)
            return TRUE;
    }

    if (!ValidDestination(pitch, left, top, right, bottom) ||
        !BindSource(source, sprite) || !ValidateEncoding(sprite))
        return FALSE;
    DrawSprite(buffer, pitch, sprite, source->pShadeCurrent, left, top, right, bottom,
        outline, outlineColor, drawOutline);
    return TRUE;
}
}

BOOLEAN Blt8BPPDataTo16BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
    HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex)
{
    return Blit(pBuffer, uiDestPitchBYTES, hSrcVObject, iX, iY, usIndex, nullptr);
}

BOOLEAN Blt8BPPDataTo16BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
    HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion)
{
    return Blit(pBuffer, uiDestPitchBYTES, hSrcVObject, iX, iY, usIndex,
        clipregion ? clipregion : &ClippingRect);
}

// Scalar translation of original vobject_blitters.cpp:11914 / :12024:
// literal index 254 writes the supplied color only when fDoOutline == TRUE.
BOOLEAN Blt8BPPDataTo16BPPBufferOutline(UINT16* buffer, UINT32 pitch, HVOBJECT source,
    INT32 x, INT32 y, UINT16 index, INT16 color, BOOLEAN drawOutline)
{
    return Blit(buffer,pitch,source,x,y,index,nullptr,true,static_cast<UINT16>(color),drawOutline==TRUE);
}
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineClip(UINT16* buffer, UINT32 pitch, HVOBJECT source,
    INT32 x, INT32 y, UINT16 index, INT16 color, BOOLEAN drawOutline, SGPRect* clip)
{
    return Blit(buffer,pitch,source,x,y,index,clip?clip:&ClippingRect,
        true,static_cast<UINT16>(color),drawOutline==TRUE);
}
