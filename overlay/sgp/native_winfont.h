/*
 * P710 native snapshot update: sgp/native_winfont.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_WINFONT_H
#define SGP_NATIVE_WINFONT_H
#include "native_font.h"
#include <string>

// Typed native request: signed Height, Weight and requested family retain the
// original INI meanings. FreeType pixel size uses abs(height), not GDI cell size.
// fileName is optional; absolute paths use the host, game paths use actual VFS.
struct NativeWinFontDescriptor {
    std::string faceName="ja2font3";
    INT32 height=-12;
    INT32 weight=400;
    std::string fileName;
};
void InitWinFonts();
void ShutdownWinFonts();
void InitTooltipFonts();
void ShutdownTooltipFonts();
INT32 CreateWinFont(const NativeWinFontDescriptor&);
void DeleteWinFont(INT32);
void SetWinFontBackColor(INT32, COLORVAL*);
void SetWinFontForeColor(INT32, COLORVAL*);
void PrintWinFont(UINT32, INT32, INT32, INT32, STR16, ...);
INT16 WinFontStringPixLength(const CHAR16*, INT32);
INT16 GetWinFontHeight(INT32);

enum {
WIN_LARGEFONT1=0, WIN_SMALLFONT1, WIN_TINYFONT1, WIN_12POINTFONT1,
WIN_COMPFONT, WIN_SMALLCOMPFONT, WIN_10POINTROMAN, WIN_12POINTROMAN,
WIN_14POINTSANSSERIF, WIN_10POINTARIAL, WIN_14POINTARIAL, WIN_12POINTARIAL,
WIN_BLOCKYFONT, WIN_BLOCKYFONT2, WIN_10POINTARIALBOLD,
WIN_12POINTARIALFIXEDFONT, WIN_16POINTARIAL, WIN_BLOCKFONTNARROW,
WIN_14POINTHUMANIST, WIN_HUGEFONT, WIN_LASTFONT
};
#define MAX_WINFONTMAP 25
extern INT32 WinFontMap[MAX_WINFONTMAP];
extern INT32 TOOLTIP_IFONT, TOOLTIP_IFONT_BOLD;

bool NativeWinFontsReady() noexcept;
bool NativeTooltipFontsReady() noexcept;
// IDs are unique core handles, not reusable slots; stale/invalid IDs return -1.
INT32 GetNativeWinFontScalableHandle(INT32) noexcept;
bool GetNativeWinFontBackColor(INT32, COLORVAL*) noexcept;
struct NativeWinFontSubstitutionInfo {
    std::string requestedFace, resolvedFile;
    INT32 requestedHeight=0, requestedWeight=0;
    // Family-to-system resolution is intentionally absent. The explicit file
    // supplies glyphs; it never establishes Arial/ja2font3/GDI pixel parity.
    bool explicitFile=false;
};
bool GetNativeWinFontSubstitutionInfo(INT32, NativeWinFontSubstitutionInfo*);
// Literal path: callers that already formatted text must not format it again.
bool PrintNativeWinFontText(UINT32, INT32, INT32, INT32, const CHAR16*, const SGPRect* clip=nullptr);
#endif
