/*
 * P710 native snapshot update: Tactical/dialogue_payload.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Borrowed process-local dialogue/NPC data; never a serialized event record.
#ifndef JA2_DIALOGUE_PAYLOAD_H
#define JA2_DIALOGUE_PAYLOAD_H
#include "types.h"
#if defined(_WIN32)
using DialogueEventData = UINT32; // Preserve the original Windows API/field ABI.
#else
#include <cstdint>
#include <type_traits>
using DialogueEventData = std::uintptr_t;
static_assert(sizeof(DialogueEventData)==sizeof(void*), "dialogue payload preserves pointers");
static_assert(std::is_unsigned<DialogueEventData>::value, "original dialogue payloads are unsigned");
static_assert(static_cast<DialogueEventData>(UINT32{0xffffffffu})==UINT32{0xffffffffu},
              "original unsigned32 scalar/sentinel bit patterns remain representable");
#endif
#endif
