/*
 * P710 native snapshot update: sgp/NativeWorldBlitters.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_WORLD_BLITTERS_H
#define P710_NATIVE_WORLD_BLITTERS_H
#include "etrle_blitters.h"
// Original world Z kernels, translated from engine 3394695. Destination and Z
// must be actual owned background buffers or acquired native surface locks,
// with equal pitch and geometry. No source/destination/Z aliasing is accepted.
// Invalid input and malformed declared rows fail before any pixel is written.
// OutlineZ preserves the original AX low-byte replacement in ordinary Z writes:
// (depth & 0xff00) | paletteIndex. Outline markers never write Z.
BOOLEAN Blt8BPPDataTo16BPPBufferTransZ(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNB(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClip(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,SGPRect*);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClip(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,SGPRect*);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBColor(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,UINT16);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClipColor(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,SGPRect*,UINT16);
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZ(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,INT16,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZNB(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,INT16,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZClip(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16,INT16,BOOLEAN,SGPRect*);
#endif
