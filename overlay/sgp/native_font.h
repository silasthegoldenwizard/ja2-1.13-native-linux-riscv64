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
UINT32 gprintf_buffer(UINT8*, UINT32, UINT32, INT32, INT32, const STR16, ...);
UINT32 mprintf_buffer(UINT8*, UINT32, UINT32, INT32, INT32, const STR16, ...);
// Prints return 0 on success (legacy convention), UINT32_MAX on failure.
// Format result must fit 511 wchar_t plus NUL. Failure makes no pixel writes.
// Buffer forms use explicit FontType. Caller supplies writable storage through
// (FontDestRegion.bottom-1)*pitch + FontDestRegion.right*2 bytes, nonnegative
// region coordinates and pitch >= right*2. Allocation size is not in this ABI.
// All manager APIs are single-threaded; returned font objects are borrowed.
#define SetFontDestObject(x) SetFontDestBuffer(x,FontDestRegion.iLeft,FontDestRegion.iTop,FontDestRegion.iRight,FontDestRegion.iBottom,FontDestWrap)
#define SetFontDestClip(l,t,r,b) SetFontDestBuffer(FontDestBuffer,l,t,r,b,FontDestWrap)
#define SetFontDestWrap(x) SetFontDestBuffer(FontDestBuffer,FontDestRegion.iLeft,FontDestRegion.iTop,FontDestRegion.iRight,FontDestRegion.iBottom,x)
// Only the declarations above are implemented by this bounded native slice.
// In particular no WinFont, coded-string, dirty-region or English-map factory.

#endif
