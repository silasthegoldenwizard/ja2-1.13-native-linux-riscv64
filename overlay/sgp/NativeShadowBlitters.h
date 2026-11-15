/*
 * P710 native snapshot update: sgp/NativeShadowBlitters.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_SHADOW_BLITTERS_H
#define P710_NATIVE_SHADOW_BLITTERS_H
#include "etrle_blitters.h"
// Scalar original TransShadow kernels. Color/Z must be real registered native
// buffer views of equal geometry/pitch. Full declared ETRLE validation and alias
// rejection precede writes. Explicit palette may differ from pShadeCurrent.
// Alpha bytes follow original encoded-byte cursor alignment, including its
// left-clipping control-byte omission. See SHADOW-IMPLEMENTATION.md for defined
// parity policy replacing the original uninitialized clipped checker flag.
#define SHADOW_PLAIN UINT16*,UINT32,HVOBJECT,INT32,INT32,UINT16
#define SHADOW_Z UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,INT32,INT32,UINT16
#define SHADOW_ALPHA UINT16*,UINT32,HVOBJECT,HVOBJECT,INT32,INT32,UINT16
#define SHADOW_ZALPHA UINT16*,UINT32,UINT16*,UINT16,HVOBJECT,HVOBJECT,INT32,INT32,UINT16
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadow(SHADOW_PLAIN,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowClip(SHADOW_PLAIN,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowAlpha(SHADOW_ALPHA,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowClipAlpha(SHADOW_ALPHA,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZ(SHADOW_Z,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZAlpha(SHADOW_ZALPHA,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZClip(SHADOW_Z,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZClipAlpha(SHADOW_ZALPHA,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNB(SHADOW_Z,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBAlpha(SHADOW_ZALPHA,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBClip(SHADOW_Z,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBClipAlpha(SHADOW_ZALPHA,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBObscured(SHADOW_Z,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredTest(SHADOW_Z,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredAlpha(SHADOW_ZALPHA,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClip(SHADOW_Z,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowZNBObscuredClipAlpha(SHADOW_ZALPHA,SGPRect*,UINT16*,BOOLEAN);
BOOLEAN Blt8BPPDataTo16BPPBufferTransShadowBelowOrEqualZNBClip(SHADOW_Z,SGPRect*,UINT16*);
#undef SHADOW_PLAIN
#undef SHADOW_Z
#undef SHADOW_ALPHA
#undef SHADOW_ZALPHA
#endif
