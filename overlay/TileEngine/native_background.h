/*
 * P710 native snapshot update: TileEngine/native_background.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_BACKGROUND_H
#define P710_NATIVE_BACKGROUND_H
#include "native_surface.h"
#include <cstddef>
namespace sgp {
// Explicit writable allocation view, never a pointer-derived surface ID.
// bytes is the real allocation extent; width excludes pitch padding.
struct BackgroundView {
    BYTE* data=nullptr;
    std::size_t bytes=0;
    UINT32 width=0,height=0,pitch=0;
};
bool ValidBackgroundView(const BackgroundView&);
// Half-open copy with correlated source/destination clipping to their real
// bounds. No global clip. Overlap uses a snapshot; invalid views fail before
// writes. Source/destination padding is never treated as visible pixels.
bool CopyBackgroundRect(BackgroundView destination, BackgroundView source,
    INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 width,UINT32 height);
// Legacy dirty-module boundary. Allocation extents come from actual owned
// malloc blocks; surface extents come from actual acquired manager locks.
// Main-thread/exclusive as the native manager: pair unlock only with a
// successful acquisition. Failed acquisition records no ownership. Views are
// borrowed only until the matching helper unlock or owned-buffer free.
INT16* AllocateBackgroundBuffer(UINT32 width,UINT32 height);
bool FreeBackgroundBuffer(void*);
BYTE* LockBackgroundSurface(UINT32 id,UINT32* pitch);
void UnlockBackgroundSurface(UINT32 id);
bool BackgroundBufferView(void* pointer,BackgroundView* out);
// Allocation-only lookups; reject surfaces and other buffer ownership kinds.
// No allocation; failed lookup resets a non-null output to an empty view.
bool OwnedBackgroundBufferView(void* pointer,BackgroundView* out);
bool OwnedZBufferView(void* pointer,BackgroundView* out);
BOOLEAN CopyBackgroundBuffers(UINT16*,UINT32,UINT16*,UINT32,INT32,INT32,INT32,INT32,UINT32,UINT32);
}
// Original Z-buffer API, implemented with owned bounded native storage.
// Requires reserved FRAME_BUFFER, matching byte pitch and full height.
// Works with a locked or unlocked frame; never changes its lock ownership.
UINT16* InitZBuffer(UINT32 pitch,UINT32 height);
BOOLEAN ShutdownZBuffer(UINT16*);
#endif
