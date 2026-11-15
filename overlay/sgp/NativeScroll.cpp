/*
 * P710 native snapshot update: sgp/NativeScroll.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "NativeScroll.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
namespace sgp {
bool PlanNativeScroll(const BackgroundView& back,const BackgroundView& z,
    const NativeScrollRequest& request,NativeScrollPlan& output)
{
    if (!ValidBackgroundView(back) || !ValidBackgroundView(z) ||
        back.width!=z.width || back.height!=z.height ||
        back.width>UINT32(std::numeric_limits<INT32>::max()) ||
        back.height>UINT32(std::numeric_limits<INT32>::max())) return false;
    const auto ba=reinterpret_cast<std::uintptr_t>(back.data);
    const auto za=reinterpret_cast<std::uintptr_t>(z.data);
    if (ba<=za ? za-ba<back.bytes : ba-za<z.bytes) return false;
    if (request.incrementX<0 || request.incrementY<0) return false;
    INT32 x=0,y=0;
    switch (request.direction) {
        case 0x08: x=1;break; // SCROLL_LEFT
        case 0x04: x=-1;break;
        case 0x01: y=1;break;
        case 0x02: y=-1;break;
        case 0x20: x=1;y=1;break;
        case 0x40: x=-1;y=1;break;
        case 0x80: x=1;y=-1;break;
        case 0x200: x=-1;y=-1;break;
        default:return false;
    }
    NativeScrollPlan plan{};
    plan.viewport={std::max<INT32>(0,request.viewport.iLeft),
        std::max<INT32>(0,request.viewport.iTop),
        std::min<INT32>(INT32(back.width),request.viewport.iRight),
        std::min<INT32>(INT32(back.height),request.viewport.iBottom)};
    const auto& v=plan.viewport;
    if (v.iLeft>=v.iRight || v.iTop>=v.iBottom ||
        (x && (!request.incrementX || request.incrementX>=v.iRight-v.iLeft)) ||
        (y && (!request.incrementY || request.incrementY>=v.iBottom-v.iTop))) return false;
    plan.shiftX=x*request.incrementX;plan.shiftY=y*request.incrementY;
    plan.source={v.iLeft+std::max<INT32>(0,-plan.shiftX),
        v.iTop+std::max<INT32>(0,-plan.shiftY),
        v.iRight-std::max<INT32>(0,plan.shiftX),
        v.iBottom-std::max<INT32>(0,plan.shiftY)};
    plan.destination={plan.source.iLeft+plan.shiftX,plan.source.iTop+plan.shiftY,
        plan.source.iRight+plan.shiftX,plan.source.iBottom+plan.shiftY};
    if (x) {
        auto strip=v;
        if (x>0) strip.iRight=v.iLeft+request.incrementX;
        else strip.iLeft=v.iRight-request.incrementX;
        plan.strips[plan.stripCount++]=strip;
    }
    if (y) {
        auto strip=v;
        if (y>0) strip.iBottom=v.iTop+request.incrementY;
        else strip.iTop=v.iBottom-request.incrementY;
        // The first X-edge strip owns the corner; the Y-edge strip excludes it.
        if (x>0) strip.iLeft+=request.incrementX;
        else if (x<0) strip.iRight-=request.incrementX;
        plan.strips[plan.stripCount++]=strip;
    }
    output=plan;return true;
}
bool ApplyNativeScroll(BackgroundView back,BackgroundView z,
    const NativeScrollRequest& request,NativeScrollPlan* applied)
{
    NativeScrollPlan plan;
    if (!PlanNativeScroll(back,z,request,plan)) return false;
    const auto& source=plan.source;const auto& dest=plan.destination;
    if (!CopyBackgroundRect(back,back,dest.iLeft,dest.iTop,source.iLeft,source.iTop,
        UINT32(source.iRight-source.iLeft),UINT32(source.iBottom-source.iTop))) return false;
    for (UINT32 i=0;i<plan.stripCount;++i) {
        const auto& strip=plan.strips[i];
        for (INT32 row=strip.iTop;row<strip.iBottom;++row)
            std::memset(z.data+std::size_t(row)*z.pitch+std::size_t(strip.iLeft)*2,
                0,std::size_t(strip.iRight-strip.iLeft)*2);
    }
    if (applied) *applied=plan;
    return true;
}
}
