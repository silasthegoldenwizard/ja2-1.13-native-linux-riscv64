/*
 * P710 native snapshot update: sgp/NativeTooltipText.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_TOOLTIP_TEXT_H
#define SGP_NATIVE_TOOLTIP_TEXT_H
#include "native_font.h"
#include <string>
#include <vector>
namespace sgp {
struct NativeTooltipRun { std::wstring text; INT32 x=0,y=0; bool bold=false; };
struct NativeTooltipLayout {
    std::vector<NativeTooltipRun> runs;
    INT32 width=0,lines=1,lineHeight=0;
};
bool ConfigureNativeTooltipText(INT32 regular,INT32 bold,UINT32 destination);
void ClearNativeTooltipText() noexcept;
bool ConfigureNativeScalableTooltipText(INT32 regular,INT32 bold,double scale);
void ClearNativeScalableTooltipText() noexcept;
bool NativeTooltipScalingEnabled() noexcept;
double NativeTooltipScale() noexcept;
bool LayoutNativeTooltipText(const wchar_t*,UINT32 screenHeight,NativeTooltipLayout&);
// Actual caller-owned bytes, not a size inferred from an opaque legacy pointer.
// Used on a private snapshot; publish only after every run has succeeded.
bool DrawNativeTooltipLayout(const NativeTooltipLayout&,BYTE*,std::size_t bytes,
    UINT32 width,UINT32 height,UINT32 pitch,INT32 x,INT32 y);
}
// Genuine original public mouse-help services, shared by soldier tooltips.
bool isTooltipScalingEnabled();
UINT16 GetScaledFontHeight();
INT16 GetNumberOfLinesInHeight(const STR16);
INT16 GetWidthOfString(const STR16);
void DisplayHelpTokenizedString(const STR16,INT16 x,INT16 y);
#endif
