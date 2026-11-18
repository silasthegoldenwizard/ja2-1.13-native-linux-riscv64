/*
 * P710 native snapshot update: tests/native-core/native_render_resource_guard_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-18.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "renderworld.h"
#include "vobject.h"

#include <cstdlib>
#include <iostream>

namespace {
unsigned checks = 0;

void expect(bool ok, const char* why) {
    ++checks;
    if (!ok) {
        std::cerr << "FAIL: " << why << '\n';
        std::abort();
    }
}
}

int main() {
    UINT8 pixel = 0;
    ETRLEObject frame{};
    SGPVObject object{};

    expect(!IsValidRenderVideoObjectFrame(nullptr, 0), "null video object is rejected");

    object.usNumberOfObjects = 1;
    expect(!IsValidRenderVideoObjectFrame(&object, 0), "missing frame table is rejected");

    object.pETRLEObject = &frame;
    expect(!IsValidRenderVideoObjectFrame(&object, 0), "missing pixel payload is rejected");

    object.pPixData = &pixel;
    object.usNumberOfObjects = 0;
    expect(!IsValidRenderVideoObjectFrame(&object, 0), "empty frame table is rejected");

    object.usNumberOfObjects = 1;
    expect(IsValidRenderVideoObjectFrame(&object, 0), "existing frame is accepted");
    expect(!IsValidRenderVideoObjectFrame(&object, 1), "out-of-range frame is rejected");

    std::cout << checks << " render resource guard assertions passed\n";
}
