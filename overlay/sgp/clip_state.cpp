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

