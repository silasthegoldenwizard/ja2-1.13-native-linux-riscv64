/*
 * P710 native snapshot update: sgp/native_ui_payload.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native UI values are process-local callback state, never resource/save data.
#ifndef SGP_NATIVE_UI_PAYLOAD_H
#define SGP_NATIVE_UI_PAYLOAD_H
#include <cstdint>
#include <type_traits>
using NativeUIPayload = std::intptr_t;
static_assert(sizeof(NativeUIPayload) == sizeof(void*), "UI payload must preserve pointers");
static_assert(std::is_signed<NativeUIPayload>::value, "UI payload preserves signed scalar values");
#endif
