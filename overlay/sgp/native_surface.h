/*
 * P710 native notice preparation: sgp/native_surface.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare native software-surface API.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/native_surface.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_SURFACE_H
#define SGP_NATIVE_SURFACE_H
#include "vobject_types.h"
#include <cstddef>

namespace sgp {
struct NativeSurfaceView {
    BYTE* data = nullptr;
    std::size_t bytes = 0;
    UINT32 width = 0, height = 0, pitch = 0;
};
// Exact allocation base of a live, currently locked RGB565 surface only.
// No lock is acquired; the view expires on the owner's matching unlock.
// Failure leaves output unchanged. Callers compare the actual returned pitch.
bool LockedNativeSurfaceView(void* pixels, NativeSurfaceView* output);
}

#define PRIMARY_SURFACE 0xfffffff0u
#define BACKBUFFER 0xfffffff1u
#define FRAME_BUFFER 0xfffffff2u
#define MOUSE_BUFFER 0xfffffff3u
#define VS_BLT_COLORFILL 0x20u
#define VS_BLT_USECOLORKEY 0x2u
#define VS_BLT_USEDESTCOLORKEY 0x200u
#define VS_BLT_FAST 0x4u
#define VS_BLT_CLIPPED 0x8u
#define VS_BLT_SRCREGION 0x10u
#define VS_BLT_DESTREGION 0x80u
#define VS_BLT_SRCSUBRECT 0x40u
#define VS_BLT_COLORFILLRECT 0x100u
#define VS_BLT_MIRROR_Y 0x1000u
#define VSURFACE_DEFAULT_MEM_USAGE 0x1u
#define VSURFACE_VIDEO_MEM_USAGE 0x2u
#define VSURFACE_SYSTEM_MEM_USAGE 0x4u
#define VSURFACE_RESERVED_SURFACE 0x100u
#define VSURFACE_CREATE_DEFAULT 0x20u
#define VSURFACE_CREATE_FROMFILE 0x40u
// Explicit opt-in: FROMFILE preserves indexed8 samples; rejects true-color input.
#define VSURFACE_NATIVE_INDEXED8 0x00400000u
#define VSURFACE_CREATE_FROMJPC 0x00010000u
#define VSURFACE_CREATE_FROMPNG 0x00020000u
#define VSURFACE_CREATE_FROMJPC_FALLBACK 0x00100000u
#define VSURFACE_CREATE_FROMPNG_FALLBACK 0x00200000u

struct NativeSurface;
using HVSURFACE = NativeSurface*;
// Borrowed actual FRAME_BUFFER capability; null while unbound. Platform-owned.
extern HVSURFACE ghFrameBuffer;
struct VSURFACE_DESC
{
    UINT32 fCreateFlags;
    SGPFILENAME ImageFile;
    UINT32 usWidth, usHeight;
    UINT8 ubBitDepth;
};
struct blt_vs_fx
{
    COLORVAL ColorFill;
    SGPRect SrcRect, FillRect;
    UINT16 DestRegion;
};

// Single-threaded manager. Init is idempotent. IDs never repeat, even across
// shutdown. Get/Create pointers are borrowed capabilities; deleted pointers
// remain invalid forever. Lock is exclusive and non-nestable, across both API
// forms. Unlock invalidates the byte view. Drawing/deletion reject locked
// surfaces. Shutdown fails without changing anything while any lock exists.
// DEFAULT accepts 8 or 16; FROMFILE defaults to owned RGB565 pixels;
// NATIVE_INDEXED8 preserves indexed input and its deep-copied palette; supported
// memory hints are DEFAULT and SYSTEM. Image palettes are deep-copied. No
// reserved surface is implicitly bound; FROMFILE ignores descriptor dimensions.
// PNG/JPC RGBA composites over the new black surface with the original
// Blt32BPPTo16BPPTrans alpha-cache arithmetic; alpha zero remains black.
// ETRLE skip runs retain zero/key pixels regardless of palette entry zero;
// ordinary uncompressed index zero still uses its palette color.
BOOLEAN InitializeVideoSurfaceManager();
BOOLEAN ShutdownVideoSurfaceManager();
BOOLEAN AddStandardVideoSurface(VSURFACE_DESC*, UINT32*);
#define AddVideoSurface(a, b) AddStandardVideoSurface(a, b)
BOOLEAN GetVideoSurface(HVSURFACE*, UINT32);
HVSURFACE CreateVideoSurface(VSURFACE_DESC*);
BOOLEAN DeleteVideoSurface(HVSURFACE);
BOOLEAN DeleteVideoSurfaceFromIndex(UINT32);
BYTE* LockVideoSurface(UINT32, UINT32*);
void UnLockVideoSurface(UINT32);
BYTE* LockVideoSurfaceBuffer(HVSURFACE, UINT32*);
void UnLockVideoSurfaceBuffer(HVSURFACE);
BOOLEAN GetVideoSurfaceDescription(UINT32, UINT16*, UINT16*, UINT8*);
// Dimensions validate a live unlocked handle; zero indicates failure.
UINT16 GetVideoSurfaceWidth(HVSURFACE);
UINT16 GetVideoSurfaceHeight(HVSURFACE);
// Full palette replacement and readback require an unlocked live surface.
// Both APIs copy all 256 entries. RGB565 palette selection copies its input,
// accepts a locked indexed source for map shading, and never owns caller memory.
BOOLEAN SetVideoSurfacePalette(HVSURFACE, SGPPaletteEntry*);
BOOLEAN GetVSurfacePaletteEntries(HVSURFACE, SGPPaletteEntry*);
BOOLEAN GetVideoSurfacePalette16(HVSURFACE, UINT16*);
BOOLEAN SetVideoSurfacePalette16(HVSURFACE, const UINT16*);
BOOLEAN GetPrimaryRGBDistributionMasks(UINT32*,UINT32*,UINT32*);
// Source rectangle is half-open in indexed source units. Samples are taken at
// rect.left/top + 2*n; odd trailing rows/columns are dropped, as in v1.13.
// Source must be locked with exactly the passed pointer/pitch. For native
// surface destination buffers, require an RGB565 lock and validate full bounds.
// Other raw destinations retain the original caller-owned extent contract.
// No global clip is used. Invalid metadata fails before writing.
// Same validated contract over the complete source; samples even source
// coordinates, drops odd trailing rows/columns, ignores the global clip.
// Full-size conversion uses the same live indexed lock and owned palette.
BOOLEAN Blt8BPPDataTo16BPPBuffer(UINT16*,UINT32,HVSURFACE,UINT8*,INT32,INT32);
BOOLEAN Blt8BPPDataSubTo16BPPBuffer(UINT16*,UINT32,HVSURFACE,UINT8*,UINT32,INT32,INT32,SGPRect*);
BOOLEAN Blt8BPPDataTo16BPPBufferHalf(UINT16*, UINT32, HVSURFACE, UINT8*, UINT32, INT32, INT32);
BOOLEAN Blt8BPPDataTo16BPPBufferHalfRect(UINT16*, UINT32, HVSURFACE, UINT8*, UINT32,
                                      INT32, INT32, SGPRect*);
BOOLEAN SetVideoSurfaceTransparency(UINT32, COLORVAL);
BOOLEAN SetVideoSurfaceTransparencyColor(HVSURFACE, COLORVAL);
BOOLEAN ColorFillVideoSurfaceArea(UINT32, INT32, INT32, INT32, INT32, UINT16);
BOOLEAN BltVideoSurface(UINT32, UINT32, UINT16, INT32, INT32, UINT32, blt_vs_fx*);
BOOLEAN BltVideoSurfaceToVideoSurface(HVSURFACE, HVSURFACE, UINT16, INT32, INT32, INT32, blt_vs_fx*);
// Legacy DD-named caller adapter, backed by the same real software copy.
// Flags: zero or VS_BLT_USECOLORKEY; optional source rect is half-open.
BOOLEAN BltVSurfaceUsingDD(HVSURFACE, HVSURFACE, UINT32, INT32, INT32, SGPRect*);
BOOLEAN BltStretchVideoSurface(UINT32, UINT32, INT32, INT32, UINT32, SGPRect*, SGPRect*);
// Original stretch contract: x/y are unused; rectangles determine placement.
// Nearest-neighbor, half-open rectangles; supports USECOLORKEY and CLIPPED.
// Out-of-source samples are skipped without moving the original scale origin.
// Original Image API shades two drop-shadow strips using the FIRST object's
// width/height, ignoring offsets; their shared corner is shaded twice. Valid
// clipped/offscreen geometry succeeds as in the source helper. Invalid image,
// invalid/locked destination or overflowing coordinates fail before any write.
BOOLEAN ShadowVideoSurfaceImage(UINT32,HVOBJECT,INT32,INT32);
BOOLEAN ShadowVideoSurfaceRect(UINT32, INT32, INT32, INT32, INT32);
// Normal shadow uses the source-built ShadeTable (default 0.48), not IntensityTable.
BOOLEAN ShadowVideoSurfaceRectUsingLowPercentTable(UINT32, INT32, INT32, INT32, INT32);
// Original shadow API has inclusive right/bottom endpoints and requires both
// input spans > 0 after surface clipping. Uses the original IntensityTable,
// initialized by BuildIntensityTable, and intersects the half-open global clip.
// Half-open rectangles. FillArea always intersects the global clip; blits do so
// only with CLIPPED. COLORFILL fills the destination; COLORFILLRECT fills
// FillRect in destination coordinates. These two flags are mutually exclusive
// and cannot be combined with source flags. Empty clipped operations succeed.
// Transparency COLORVAL uses FROMRGB, converted to RGB565, not a packed 565 word.

// Internal main-thread video-manager integration, not application ownership APIs.
// Bind atomically protects three equal-sized framebuffer IDs and one cursor ID.
// All four must be distinct and unlocked; cursor size is independent.
// Aliases and backing IDs share pointers/locks; all external deletion is rejected.
namespace sgp
{
bool BindReservedVideoSurfaces(const UINT32 (&ids)[4]);
bool ReservedVideoSurfacesReady();
bool ReleaseReservedVideoSurfaces(); // Refuses locks; unbinds and deletes only its four surfaces.
// Read-only diagnostic capability; zero while inactive. Does not transfer ownership.
UINT32 ReservedVideoSurfaceBackingId(UINT32 alias);
// Read-only pitch metadata for a live handle, including while it is locked.
// Takes/releases no pixel lock and leaves output unchanged on failure.
bool GetNativeSurfacePitch(HVSURFACE surface, UINT32* pitch);
}

#endif
