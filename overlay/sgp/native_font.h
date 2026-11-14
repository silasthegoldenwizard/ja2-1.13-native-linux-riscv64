/*
 * P710 native notice preparation: sgp/native_font.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare bounded native bitmap-font API.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/native_font.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_FONT_H
#define SGP_NATIVE_FONT_H
#include "vobject.h"
#include "native_surface.h"
#ifndef _SIRTECH_TYPES_
// Process-local text only: never use wchar_t for resource/wire structures.
using CHAR16 = wchar_t;
using STR16 = CHAR16*;
#endif
#define MAX_FONTS 25
#define DEFAULT_SHADOW 2
#define MILITARY_SHADOW 67
#define NO_SHADOW 0

// Original Font.h palette indices, shared by native bitmap rendering.
#define		FONT_FCOLOR_WHITE			208
#define		FONT_FCOLOR_RED				162
#define		FONT_FCOLOR_NICERED		164
#define		FONT_FCOLOR_BLUE			203
#define		FONT_FCOLOR_GREEN			184
#define		FONT_FCOLOR_YELLOW		144
#define		FONT_FCOLOR_BROWN			184
#define		FONT_FCOLOR_ORANGE		76
#define		FONT_FCOLOR_PURPLE		160
#define		FONT_BCOLOR_WHITE			208
#define		FONT_BCOLOR_RED				162
#define		FONT_BCOLOR_BLUE			203
#define		FONT_BCOLOR_GREEN			184
#define		FONT_BCOLOR_YELLOW		144
#define		FONT_BCOLOR_BROWN			80
#define		FONT_BCOLOR_ORANGE		76
#define		FONT_BCOLOR_PURPLE		160
struct FontTranslationTable
{
    UINT16 usNumberOfSymbols;
    UINT16* DynamicArrayOf16BitValues;
};
extern INT32 FontDefault;
extern UINT32 FontDestBuffer, FontDestPitch, FontDestBPP;
extern SGPRect FontDestRegion;
extern BOOLEAN FontDestWrap;
BOOLEAN InitializeFontManager(UINT16, FontTranslationTable*);
void ShutdownFontManager();
INT32 LoadFontFile(const STR8);
void UnloadFont(UINT32);
BOOLEAN IsFontLoaded(INT32);
HVOBJECT GetFontObject(INT32);
BOOLEAN SetFont(INT32);
INT16 GetIndex(CHAR16);
UINT32 GetWidth(HVOBJECT, INT16);
UINT16 GetFontHeight(INT32);
INT16 StringPixLength(const CHAR16*, INT32);
INT16 StringNPixLength(STR16, UINT32, INT32);
INT16 SubstringPixLength(STR16, UINT32, UINT32, INT32);
INT16 StringPixLengthArg(INT32, UINT32, STR16, ...);
void FindFontRightCoordinates(INT16, INT16, INT16, INT16, const STR16, INT32, INT16*, INT16*);
void FindFontCenterCoordinates(INT16, INT16, INT16, INT16, const STR16, INT32, INT16*, INT16*);
void VarFindFontRightCoordinates(INT16, INT16, INT16, INT16, INT32, INT16*, INT16*, const CHAR16*, ...);
void VarFindFontCenterCoordinates(INT16, INT16, INT16, INT16, INT32, INT16*, INT16*, const CHAR16*, ...);
// Metrics saturate at INT16_MAX. Substring endpoints are inclusive (the legacy
// endpoint equal to wcslen counts the NUL's fallback glyph). Invalid metric
// arguments return 0. Positioning leaves both outputs unchanged on null text,
// invalid font, missing output, or formatting failure; valid results saturate
// to INT16 range. Var forms use the same 511-character bound as printing.
// These read-only helpers never select a font or change drawing/saved state.
BOOLEAN SetFontDestBuffer(UINT32, INT32, INT32, INT32, INT32, BOOLEAN);
void SetFontColors(UINT16);
void SetFontForeground(UINT8);
void SetFontBackground(UINT8);
void SetFontShadow(UINT8);
void SetRGBFontForeground(UINT32, UINT32, UINT32);
void SetRGBFontBackground(UINT32, UINT32, UINT32);
void SetRGBFontShadow(UINT32, UINT32, UINT32);
void SetRGBFontForeground(UINT16);
void SetRGBFontShadow(UINT16);
void SaveFontSettings();
void RestoreFontSettings();
UINT32 gprintf(INT32, INT32, const STR16, ...);
UINT32 mprintf(INT32, INT32, const CHAR16*, ...);
UINT32 mprintf_coded(INT32, INT32, const STR16, ...);
UINT32 gprintf_buffer(UINT8*, UINT32, UINT32, INT32, INT32, const STR16, ...);
UINT32 mprintf_buffer(UINT8*, UINT32, UINT32, INT32, INT32, const STR16, ...);
UINT32 mprintf_buffer_coded(UINT8*, UINT32, UINT32, INT32, INT32, const STR16, ...);
// Prints return 0 on success (legacy convention), UINT32_MAX on failure.
// Format result must fit 511 wchar_t plus NUL. Failure makes no pixel writes.
// Bitmap uncoded buffer forms use explicit FontType; coded retains FontDefault.
// Mapped mprintf_buffer/_coded use current FontDefault, as the original Win
// branch did; only a real locked native RGB565 surface pointer is accepted.
// gprintf_buffer remains bitmap-only (the original has no Win branch).
// Caller supplies writable storage through
// (FontDestRegion.bottom-1)*pitch + FontDestRegion.right*2 bytes, nonnegative
// region coordinates and pitch >= right*2. Allocation size is not in this ABI.
// All manager APIs are single-threaded; returned font objects are borrowed.
#define SetFontDestObject(x) SetFontDestBuffer(x,FontDestRegion.iLeft,FontDestRegion.iTop,FontDestRegion.iRight,FontDestRegion.iBottom,FontDestWrap)
#define SetFontDestClip(l,t,r,b) SetFontDestBuffer(FontDestBuffer,l,t,r,b,FontDestWrap)
#define SetFontDestWrap(x) SetFontDestBuffer(FontDestBuffer,FontDestRegion.iLeft,FontDestRegion.iTop,FontDestRegion.iRight,FontDestRegion.iBottom,x)
// Only the declarations above are implemented by this bounded native slice.
// The optional scalable replacement is explicitly registered below.

namespace sgp {
// Explicit, default-off mapping: requires loaded bitmap ID and live scalable
// handle; -1 clears. Does not own/delete either font. Unload/shutdown clear it.
// Full-string/height metrics and ordinary draws use the real scalable face;
// NPix/substrings preserve the original bitmap-only metric policy.
// Optional bridge-owned metric policy retains original language/ID behavior
// without importing game globals into this manager. Callbacks receive the real
// core handle, have the original WinFont helper signatures, and must not call
// this Font API recursively. Empty entries use actual core metrics directly.
struct NativeFontReplacementPolicy {
    INT16 (*measure)(const CHAR16*,INT32)=nullptr;
    INT16 (*height)(INT32)=nullptr;
};
bool SetNativeFontReplacement(INT32 bitmapFont, INT32 scalableHandle,
                              NativeFontReplacementPolicy policy={});
void ClearNativeFontReplacements() noexcept;
// Read-only validated lookup; absent bitmap or deleted scalable handle is -1.
INT32 GetNativeFontReplacement(INT32 bitmapFont);
// Internal composition state. Unlike SaveFontSettings this does not overwrite
// the caller's legacy single saved slot. Restores raw destination values too.
struct NativeFontState {
    INT32 font; std::uint64_t generation;
    UINT32 dest, pitch, bpp; SGPRect region; BOOLEAN wrap;
    UINT16 foreground, background, shadow;
    INT32 scalable=-1; COLORVAL scalableColor=0;
};
NativeFontState CaptureNativeFontState();
void RestoreNativeFontState(const NativeFontState&);
// Literal text through the same validated glyph renderer as mprintf/gprintf.
UINT32 DrawNativeFontText(INT32, INT32, const CHAR16*, bool multicolor);
}

#endif
