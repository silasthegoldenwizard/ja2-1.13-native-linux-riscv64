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
namespace
{
struct Sprite
{
    const ETRLEObject* object;
    const UINT8* data;
    std::size_t length;
    std::int64_t x;
    std::int64_t y;
};

bool DescribeSprite(HVOBJECT source, UINT16 index, INT32 x, INT32 y, Sprite& sprite)
{
    if (!source || !source->pETRLEObject || index >= source->usNumberOfObjects ||
        source->ubBitDepth != 8)
        return false;

    sprite.object = &source->pETRLEObject[index];
    if (sprite.object->usWidth == 0 || sprite.object->usHeight == 0)
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

void DrawSprite(UINT16* buffer, UINT32 pitch, const Sprite& sprite,
    const UINT16* palette, std::int64_t left, std::int64_t top,
    std::int64_t right, std::int64_t bottom)
{
    std::size_t cursor = 0;
    auto* bytes = reinterpret_cast<UINT8*>(buffer);
    // Encoding has already been bounded and validated. Parse skipped rows and
    // skipped portions as runs too, so a zero-valued literal stays a pixel.
    for (UINT32 row = 0; row < sprite.object->usHeight; ++row)
    {
        const std::int64_t y = sprite.y + row;
        if (y >= bottom)
            break;
        UINT32 column = 0;
        for (;;)
        {
            const UINT8 control = sprite.data[cursor++];
            if (control == 0)
                break;
            const UINT32 run = control & 0x7f;
            if ((control & 0x80) == 0)
            {
                if (y >= top)
                {
                    const std::int64_t runLeft = sprite.x + column;
                    const std::int64_t first = std::max(left, runLeft);
                    const std::int64_t last = std::min(right, runLeft + run);
                    if (first < last)
                    {
                        auto* destination = bytes + static_cast<std::size_t>(y) * pitch +
                            static_cast<std::size_t>(first) * sizeof(UINT16);
                        std::size_t sourcePixel = cursor + static_cast<std::size_t>(first - runLeft);
                        for (std::int64_t x = first; x < last; ++x)
                        {
                            const UINT16 pixel = palette[sprite.data[sourcePixel++]];
                            // The API specifies byte pitch. memcpy also permits
                            // an odd pitch without unaligned UINT16 accesses.
                            std::memcpy(destination, &pixel, sizeof(pixel));
                            destination += sizeof(pixel);
                        }
                    }
                }
                cursor += run;
            }
            column += run;
        }
    }
}

BOOLEAN Blit(UINT16* buffer, UINT32 pitch, HVOBJECT source, INT32 x, INT32 y,
    UINT16 index, const SGPRect* clip)
{
    Sprite sprite{};
    if (!buffer || !DescribeSprite(source, index, x, y, sprite))
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
    DrawSprite(buffer, pitch, sprite, source->pShadeCurrent, left, top, right, bottom);
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
