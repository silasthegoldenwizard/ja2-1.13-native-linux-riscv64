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

#ifndef SGP_NATIVE_SURFACE_H
#define SGP_NATIVE_SURFACE_H
#include "vobject_types.h"

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

struct NativeSurface;
using HVSURFACE = NativeSurface*;
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
// Only DEFAULT/16 creation is supported; no reserved surface is implicitly bound.
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
BOOLEAN SetVideoSurfaceTransparency(UINT32, COLORVAL);
BOOLEAN SetVideoSurfaceTransparencyColor(HVSURFACE, COLORVAL);
BOOLEAN ColorFillVideoSurfaceArea(UINT32, INT32, INT32, INT32, INT32, UINT16);
BOOLEAN BltVideoSurface(UINT32, UINT32, UINT16, INT32, INT32, UINT32, blt_vs_fx*);
BOOLEAN BltVideoSurfaceToVideoSurface(HVSURFACE, HVSURFACE, UINT16, INT32, INT32, INT32, blt_vs_fx*);
// Half-open rectangles. FillArea always intersects the global clip; blits do so
// only with CLIPPED. COLORFILL fills the destination; COLORFILLRECT fills
// FillRect in destination coordinates. These two flags are mutually exclusive
// and cannot be combined with source flags. Empty clipped operations succeed.
// Transparency COLORVAL uses FROMRGB, converted to RGB565, not a packed 565 word.

// Internal main-thread video-manager integration, not application ownership APIs.
// Bind atomically protects three distinct, unlocked, equal-sized ordinary IDs.
// Aliases and backing IDs share pointers/locks; all external deletion is rejected.
namespace sgp
{
bool BindReservedVideoSurfaces(const UINT32 (&ids)[3]);
bool ReservedVideoSurfacesReady();
bool ReleaseReservedVideoSurfaces(); // Refuses locks; unbinds and deletes only its three surfaces.
// Read-only diagnostic capability; zero while inactive. Does not transfer ownership.
UINT32 ReservedVideoSurfaceBackingId(UINT32 alias);
}

#endif
