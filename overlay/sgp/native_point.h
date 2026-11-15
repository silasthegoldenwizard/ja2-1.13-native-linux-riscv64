/*
 * P710 native snapshot update: sgp/native_point.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Explicit original-client coordinate adapter; no Windows API is emulated.
#ifndef SGP_NATIVE_POINT_H
#define SGP_NATIVE_POINT_H
#if defined(_WIN32)
// Expanded only where the original source already requires the real SDK type.
#define SGP_RECT_LTRB RECT
#define SGP_POINT_XY POINT
#else
#include "native_input.h"
namespace sgp {
struct PointXY { INT32 x, y; };
struct RectLTRB { INT32 left, top, right, bottom; };
static_assert(sizeof(RectLTRB)==16, "Original drawing rectangles have four signed32 coordinates");
static_assert(sizeof(PointXY)==8, "Original client points have two 32-bit coordinates");
inline void ReadClientMousePoint(PointXY* point)
{
    if (!point) return;
    SGPPoint logical{};
    GetMousePos(&logical);
    point->x=logical.iX;
    point->y=logical.iY;
}
}
#define SGP_RECT_LTRB sgp::RectLTRB
#define SGP_POINT_XY sgp::PointXY
#endif
#endif
