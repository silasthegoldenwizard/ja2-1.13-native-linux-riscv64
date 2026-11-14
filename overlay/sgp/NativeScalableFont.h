/*
 * P710 native snapshot update: sgp/NativeScalableFont.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_SCALABLE_FONT_H
#define SGP_NATIVE_SCALABLE_FONT_H
#include "native_surface.h"
#include <cstddef>

namespace sgp {
struct NativeScalableFontMetrics {
    INT32 ascent=0, descent=0, height=0, internalLeading=0;
};
// Single-threaded. Copies bytes; scalable Unicode faces only. IDs are positive,
// never reused (including shutdown), and are not pointers. Returns -1 on failure.
// Bounds: 32 live fonts, 16 MiB/font, 1..512 pixel size, 4096 Unicode scalars/text,
// 16 MiB prepared masks/draw. Missing glyphs use this face's .notdef glyph;
// malformed Unicode fails. No additional-font fallback or shaping.
INT32 CreateNativeScalableFont(const BYTE*, std::size_t bytes, UINT32 pixels);
void DeleteNativeScalableFont(INT32) noexcept;
void ShutdownNativeScalableFonts() noexcept;
bool SetNativeScalableFontColor(INT32, COLORVAL);
bool GetNativeScalableFontColor(INT32, COLORVAL*);
bool GetNativeScalableFontMetrics(INT32, NativeScalableFontMetrics*);
bool MeasureNativeScalableText(INT32, const wchar_t*, INT32*);
// Top-left y uses the selected face's ascender as baseline. Metrics describe
// FreeType; exact GDI layout is not promised. All valid non-NUL scalars,
// including C0/C1, map literally through the face's cmap or missing glyph.
// No newline/tab/control actions are interpreted; NUL ends the string.
// Transparent background; normal hinted grayscale masks; RGB565 component
// interpolation with nearest rounding. Clips are half-open and surface-bounded.
// Complete layout/masks are prepared before writes. Failure leaves bytes alone.
bool DrawNativeScalableText(UINT32 dest, INT32 font, INT32 x, INT32 y,
                            const wchar_t*, COLORVAL, const SGPRect* clip=nullptr);
// Caller supplies the actual writable allocation extent (including full rows).
// Never infer an allocation size from a legacy pointer/pitch alone.
bool DrawNativeScalableTextBuffer(BYTE*, std::size_t bytes, UINT32 width,
                                  UINT32 height, UINT32 pitch, INT32 font,
                                  INT32 x, INT32 y, const wchar_t*, COLORVAL,
                                  const SGPRect* clip=nullptr);
}
#endif
