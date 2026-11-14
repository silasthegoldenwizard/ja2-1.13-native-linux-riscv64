/*
 * P710 native snapshot update: sgp/NativeRawBlitters.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_RAW_BLITTERS_H
#define P710_NATIVE_RAW_BLITTERS_H
#include "core_types.h"

// Original RGB565 alpha helper: alpha is narrowed to a signed FILD word, as in
// the x87 implementation. Pixel alpha callers normally supply 0..255.
unsigned short blendWithAlpha(unsigned int, unsigned int, unsigned int);
BOOLEAN Blt16BPPBufferShadowRectAlternateTable(UINT16*,UINT32,SGPRect*);
extern UINT32 guiTranslucentMask;
extern INT32 gLeftSkip, gRightSkip, gTopSkip, gBottomSkip;
extern BOOLEAN gfUsePreCalcSkips;
#endif
