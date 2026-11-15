/*
 * P710 native notice preparation: sgp/clip_state.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: separate original shared clipping state.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/vobject_blitters.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/clip_state.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "clip_state.h"
#include <cassert>
#include <cstring>

SGPRect ClippingRect{};

void SetClippingRect(SGPRect* clip)
{
    assert(clip != nullptr);
    if (clip) std::memcpy(&ClippingRect, clip, sizeof(ClippingRect));
}

void GetClippingRect(SGPRect* clip)
{
    assert(clip != nullptr);
    if (clip) std::memcpy(clip, &ClippingRect, sizeof(ClippingRect));
}

