/*
 * P710 native snapshot update: sgp/NativeEffectBlitters.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_EFFECT_BLITTERS_H
#define P710_NATIVE_EFFECT_BLITTERS_H
#include "etrle_blitters.h"
// Real acquired RGB565 surfaces/owned background buffers only; bounded ETRLE
// validation and source/depth/destination alias rejection precede every write.
// Preserve original effect-specific index, depth and checkerboard quirks.
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClipTranslucent( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZTranslucent( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClipTranslucent( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBTranslucent( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadowClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion, UINT16 usForeground, UINT16 usBackground, UINT16 usShadow );
BOOLEAN Blt8BPPDataTo16BPPBufferTransZPixelate( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferTransZPixelateObscured( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClipPixelate( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBPixelate( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferTransZNBClipPixelate( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferShadowZ( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferShadowZClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferShadowZNB( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferShadowZNBClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClipLandGrad(UINT16* pBuffer, UINT32 uiDestPitchBYTES, UINT16* pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, const INT16* pGradPatch, UINT16 usGradTint, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect* clipregion, BOOLEAN fWriteZ);
BOOLEAN Blt8BPPDataTo16BPPBufferShadow( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex);
BOOLEAN Blt8BPPDataTo16BPPBufferTransMirror( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferShadowClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferMonoShadow( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, UINT16 usForeground, UINT16 usBackground);
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZPixelateObscuredClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, INT16 s16BPPColor, BOOLEAN fDoOutline, SGPRect *clipregion );
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineShadow( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineShadowClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion );
BOOLEAN Blt8BPPDataTo16BPPBufferOutlineZPixelateObscured( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, INT16 s16BPPColor, BOOLEAN fDoOutline );
BOOLEAN Blt8BPPDataTo16BPPBufferIntensityZ( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferIntensityZClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferIntensityZNB( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex );
BOOLEAN Blt8BPPDataTo16BPPBufferIntensityZNBClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferIntensityClip( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
BOOLEAN Blt8BPPDataTo16BPPBufferIntensity( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex);
BOOLEAN Blt8BPPDataTo16BPPBufferTransZClipPixelateObscured( UINT16 *pBuffer, UINT32 uiDestPitchBYTES, UINT16 *pZBuffer, UINT16 usZValue, HVOBJECT hSrcVObject, INT32 iX, INT32 iY, UINT16 usIndex, SGPRect *clipregion);
#endif
