/*
 * P710 native snapshot update: sgp/native_presentation.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_PRESENTATION_H
#define SGP_NATIVE_PRESENTATION_H
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>

namespace sgp {
struct NativePresentationGeometry {
    int logicalWidth=0, logicalHeight=0;
    int windowWidth=0, windowHeight=0;
    int pixelWidth=0, pixelHeight=0;
    int contentX=0, contentY=0, contentWidth=0, contentHeight=0;
};
inline bool operator==(const NativePresentationGeometry& a, const NativePresentationGeometry& b) noexcept {
    return std::tie(a.logicalWidth,a.logicalHeight,a.windowWidth,a.windowHeight,a.pixelWidth,a.pixelHeight,a.contentX,a.contentY,a.contentWidth,a.contentHeight)==
           std::tie(b.logicalWidth,b.logicalHeight,b.windowWidth,b.windowHeight,b.pixelWidth,b.pixelHeight,b.contentX,b.contentY,b.contentWidth,b.contentHeight);
}
inline bool operator!=(const NativePresentationGeometry& a, const NativePresentationGeometry& b) noexcept { return !(a==b); }
// Fit in actual drawable pixels. Window-point dimensions are separate for
// HiDPI event coordinates; all products fit signed64 for positive int inputs.
inline bool MakeNativePresentationGeometry(int lw,int lh,int ww,int wh,int pw,int ph,
                                           NativePresentationGeometry& out) noexcept {
    if (lw<=0 || lh<=0 || ww<=0 || wh<=0 || pw<=0 || ph<=0) return false;
    NativePresentationGeometry g{lw,lh,ww,wh,pw,ph};
    if (std::int64_t(pw)*lh<=std::int64_t(ph)*lw) {
        g.contentWidth=pw; g.contentHeight=static_cast<int>(std::int64_t(pw)*lh/lw);
    } else {
        g.contentHeight=ph; g.contentWidth=static_cast<int>(std::int64_t(ph)*lw/lh);
    }
    if (!g.contentWidth || !g.contentHeight) return false;
    g.contentX=(pw-g.contentWidth)/2;g.contentY=(ph-g.contentHeight)/2;
    out=g;return true;
}
inline bool NativeWindowToLogical(const NativePresentationGeometry& g,double x,double y,
                                  double& lx,double& ly) noexcept {
    if (!std::isfinite(x) || !std::isfinite(y) || g.windowWidth<=0 || g.windowHeight<=0 || g.pixelWidth<=0 || g.pixelHeight<=0 ||
        g.contentWidth<=0 || g.contentHeight<=0 || g.logicalWidth<=0 || g.logicalHeight<=0) return false;
    lx=(x*g.pixelWidth/g.windowWidth-g.contentX)*g.logicalWidth/g.contentWidth;
    ly=(y*g.pixelHeight/g.windowHeight-g.contentY)*g.logicalHeight/g.contentHeight;
    return std::isfinite(lx) && std::isfinite(ly);
}
inline bool NativeLogicalToWindow(const NativePresentationGeometry& g,double lx,double ly,
                                  double& x,double& y) noexcept {
    if (!std::isfinite(lx) || !std::isfinite(ly) || g.pixelWidth<=0 || g.pixelHeight<=0 || g.windowWidth<=0 || g.windowHeight<=0 ||
        g.contentWidth<=0 || g.contentHeight<=0 || g.logicalWidth<=0 || g.logicalHeight<=0) return false;
    x=(g.contentX+lx*g.contentWidth/g.logicalWidth)*g.windowWidth/g.pixelWidth;
    y=(g.contentY+ly*g.contentHeight/g.logicalHeight)*g.windowHeight/g.pixelHeight;
    return std::isfinite(x) && std::isfinite(y);
}
inline bool NativePresentationContains(const NativePresentationGeometry& g,double x,double y) noexcept {
    double lx=0,ly=0;
    return NativeWindowToLogical(g,x,y,lx,ly) && lx>=0 && ly>=0 && lx<g.logicalWidth && ly<g.logicalHeight;
}
}
#endif
