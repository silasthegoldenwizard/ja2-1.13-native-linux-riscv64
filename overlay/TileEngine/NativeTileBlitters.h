/*
 * P710 native snapshot update: TileEngine/NativeTileBlitters.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_TILE_BLITTERS_H
#define P710_NATIVE_TILE_BLITTERS_H
#include "etrle_blitters.h"
// Original renderworld x86 kernels translated to portable C++. Buffers must be
// actual owned background allocations or acquired native surface locks.
#define TILE_ARGS UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClip(TILE_ARGS,SGPRect*);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncClipZSameZBurnsThrough(TILE_ARGS,SGPRect*,INT16);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZIncObscureClip(TILE_ARGS,SGPRect*);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClip(TILE_ARGS,SGPRect*,INT16,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClip(TILE_ARGS,SGPRect*,INT16,UINT16*,BOOLEAN);
#undef TILE_ARGS
#define TILE_ALPHA_ARGS UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,HVOBJECT,INT32,INT32,UINT16
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncObscureClipAlpha(TILE_ALPHA_ARGS,SGPRect*,INT16,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTransShadowIncClipAlpha(TILE_ALPHA_ARGS,SGPRect*,INT16,UINT16*,BOOLEAN);
#undef TILE_ALPHA_ARGS
BOOLEAN Zero8BPPDataTo16BPPBufferTransparent(UINT16*,UINT32,HVOBJECT,INT32,INT32,UINT16);
BOOLEAN Blt8BPPDataTo16BPPBufferTransInvZ(UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16);
BOOLEAN IsTileRedundent(UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16);
#endif
