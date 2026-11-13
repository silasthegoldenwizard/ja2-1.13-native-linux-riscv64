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

#ifndef SGP_ETRLE_BLITTERS_H
#define SGP_ETRLE_BLITTERS_H

#include "vobject_types.h"
#include "clip_state.h"

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

#endif
