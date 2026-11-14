/*
 * P710 native notice preparation: sgp/etrle_blitters.h
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare portable ETRLE kernel interface.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject_blitters.h.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/etrle_blitters.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_ETRLE_BLITTERS_H
#define SGP_ETRLE_BLITTERS_H

#include "vobject_types.h"
#include "clip_state.h"
#include <cstddef>
#include <cstdint>

// Shared bounded ETRLE preflight and linear run traversal. Internal renderer API.
namespace sgp { namespace etrle {
struct Sprite
{
    const ETRLEObject* object;
    const UINT8* data;
    std::size_t length;
    std::int64_t x, y;
};
// Clipped callers may describe blank frames for their original no-area exit.
// Other callers retain the existing nonempty admission contract.
bool DescribeSprite(HVOBJECT, UINT16, INT32, INT32, Sprite&, bool allowEmpty = false);
bool BindSource(HVOBJECT, Sprite&);
bool ValidateEncoding(const Sprite&);
bool ValidDestination(UINT32, std::int64_t, std::int64_t, std::int64_t, std::int64_t);
// Requires successful ValidateEncoding; callback receives only opaque runs.
using LiteralRun = void (*)(void*, UINT32 row, UINT32 column, const UINT8*, UINT32 count);
void ForEachLiteralRun(const Sprite&, LiteralRun, void*);
}}

// The caller owns a writable RGB565 buffer extending through the last visible
// row and column. The legacy API provides a byte pitch, but no allocation size.
// Clipping rectangles use exclusive right and bottom edges. A null clip uses
// ClippingRect. Literal ETRLE pixels (including index zero) use pShadeCurrent.
// Invalid source metadata/encoding or an unsafe destination span returns FALSE
// before any writes. A fully clipped sprite returns TRUE without decoding it.
BOOLEAN Blt8BPPDataTo16BPPBufferTransparent(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
    HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex);
BOOLEAN Blt8BPPDataTo16BPPBufferTransparentClip(UINT16* pBuffer, UINT32 uiDestPitchBYTES,
    HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion);
// Same validated source/destination contract; palette index 254 is an outline
// marker and preserves destination pixels unless drawOutline is exactly TRUE.
BOOLEAN Blt8BPPDataTo16BPPBufferOutline(UINT16*, UINT32, HVOBJECT, INT32, INT32,
    UINT16, INT16, BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineClip(UINT16*, UINT32, HVOBJECT, INT32, INT32,
    UINT16, INT16, BOOLEAN, SGPRect*);

#endif
