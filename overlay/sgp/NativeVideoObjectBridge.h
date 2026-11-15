/*
 * P710 native snapshot update: sgp/NativeVideoObjectBridge.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_VIDEO_OBJECT_BRIDGE_H
#define P710_NATIVE_VIDEO_OBJECT_BRIDGE_H
#include "vobject.h"
#include "native_surface.h"

// Implements the remaining original high-level object interfaces. Overrides
// crop source extents; they never scale. The 8-bit original clip-selection and
// OutlineShadow's clipped/unclipped index254 distinction are retained.
// Surface locks are exclusive and acquired/released only by these operations.
// Malformed ETRLE is rejected in full before any destination writes, including
// fully clipped calls. True-color sources retain their caller-owned readable
// allocation contract: the original struct has no byte-capacity field.
#endif
