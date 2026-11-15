/*
 * P710 native snapshot update: Utils/NativeCinematicsBink.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_CINEMATICS_BINK_H
#define P710_NATIVE_CINEMATICS_BINK_H
#include "core_types.h"
#define BINK_FLIC_OPEN 0x00000001u
#define BINK_FLIC_PLAYING 0x00000002u
#define BINK_FLIC_LOOP 0x00000004u
#define BINK_FLIC_AUTOCLOSE 0x00000008u
#define BINK_FLIC_CENTER_VERTICAL 0x00000010u
// Four stable main-thread slots. Like the original, the borrowed pointer may
// refer to a zeroed slot after AUTOCLOSE; Close tolerates that Intro protocol.
// Filename is provider-owned until close. No Windows/Bink SDK handles escape.
struct BINKFLIC {
    const CHAR8* cFilename=nullptr;
    UINT32 uiFlags=0,uiFrame=0,uiLeft=0,uiTop=0;
};
void BinkInitialize(void*,UINT32 width,UINT32 height);
void BinkShutdown();
void BinkShutdownVideo();
void BinkSetupVideo();
BINKFLIC* BinkOpenFlic(const CHAR8* filename);
BINKFLIC* BinkGetFreeFlic();
BINKFLIC* BinkPlayFlic(const CHAR8*,UINT32 left,UINT32 top,UINT32 flags);
void BinkSetBlitPosition(BINKFLIC*,UINT32 left,UINT32 top);
void BinkCloseFlic(BINKFLIC*);
BOOLEAN BinkPollFlics();
const char* NativeBinkLastError();
// Identical production poll with an explicit monotonic32-bit tick. Deterministic
// verification calls this; normal game callers use BinkPollFlics/GetClock.
BOOLEAN NativeBinkPollAt(UINT32 now);
#endif
