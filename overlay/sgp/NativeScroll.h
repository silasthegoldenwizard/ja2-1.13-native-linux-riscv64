/*
 * P710 native snapshot update: sgp/NativeScroll.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_SCROLL_H
#define P710_NATIVE_SCROLL_H
#include "native_background.h"
#include <array>
namespace sgp {
// Original renderworld direction values, transported without including the
// engine renderer in the platform library.
struct NativeScrollRequest {
    SGPRect viewport{};
    UINT32 direction=0;
    INT32 incrementX=0,incrementY=0;
};
struct NativeScrollPlan {
    SGPRect viewport{},source{},destination{};
    std::array<SGPRect,2> strips{};
    UINT32 stripCount=0;
    INT32 shiftX=0,shiftY=0;
};
// Half-open viewport, clipped to actual visible storage. Unsupported directions,
// negative/zero used increments or increments spanning the viewport fail. Output
// remains unchanged on failure. No global drawing clip or row padding is used.
bool PlanNativeScroll(const BackgroundView& back,const BackgroundView& z,
    const NativeScrollRequest&,NativeScrollPlan&);
// Actual production overlap-safe RGB565 shift and original exposed-only Z clear.
// Retained Z is intentionally not shifted: this matches sgp/video.cpp. Validates
// both real views before writes; an allocation failure leaves both untouched.
bool ApplyNativeScroll(BackgroundView back,BackgroundView z,
    const NativeScrollRequest&,NativeScrollPlan* applied=nullptr);
}
#endif
