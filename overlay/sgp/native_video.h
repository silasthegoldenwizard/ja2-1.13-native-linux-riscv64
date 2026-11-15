/*
 * P710 native notice preparation: sgp/native_video.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare native reserved-framebuffer/video API.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/native_video.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_VIDEO_H
#define SGP_NATIVE_VIDEO_H
#include "native_surface.h"
#include "SDLPresentation.h"
#include "NativeScroll.h"
#include <cstdint>
#define VIDEO_NO_CURSOR 0xFFFF
#define BUFFER_READY 0x00
#define BUFFER_BUSY 0x01
#define BUFFER_DIRTY 0x02
#define BUFFER_DISABLED 0x03
#define MAX_CURSOR_WIDTH 64
#define MAX_CURSOR_HEIGHT 64
extern UINT32 guiMouseBufferState;
extern BOOLEAN gfNextRefreshFullScreen;
extern INT32 giNumFrames;
PTR LockMouseBuffer(UINT32*);
void UnlockMouseBuffer();
BOOLEAN CanBlitToMouseBuffer();
BOOLEAN EraseMouseCursor();
BOOLEAN SetMouseCursorProperties(INT16,INT16,UINT16,UINT16);
BOOLEAN BltToMouseCursor(UINT32,UINT16,UINT16,UINT16);
BOOLEAN SetMouseCursorFromObject(UINT32,UINT16,UINT16,UINT16);
BOOLEAN HideMouseCursor();
BOOLEAN LoadCursorFile(STR8);
BOOLEAN SetCurrentCursor(UINT16,UINT16,UINT16);
void DirtyCursor();
// Retains original historical polarity: TRUE disables, FALSE enables.
void EnableCursor(BOOLEAN);
// Single-threaded, main-thread-only RGB565 subset; SDLPresentation owns SDL video.
// Initialize accepts only (nullptr, 0, nullptr); native callbacks are unsupported.
// Repeated valid initialization is a no-op. Settings report configured geometry.
BOOLEAN InitializeVideoManager(void* instance, UINT16 show, void* windowproc);
void ShutdownVideoManager();
void SuspendVideoManager();
BOOLEAN RestoreVideoManager();
void GetCurrentVideoSettings(UINT16*, UINT16*, UINT8*);
// Actual live primary RGB565 masks; false leaves outputs unchanged.
BOOLEAN GetPrimaryRGBDistributionMasks(UINT32*,UINT32*,UINT32*);
// The native display accepts RGB565 modes only; indexed offscreen palettes
// use SetVideoSurfacePalette. This display-palette request returns FALSE.
BOOLEAN Set8BPPPalette(SGPPaletteEntry*);
PTR LockPrimarySurface(UINT32*);
void UnlockPrimarySurface();
PTR LockBackBuffer(UINT32*);
void UnlockBackBuffer();
PTR LockFrameBuffer(UINT32*);
void UnlockFrameBuffer();
// Matches BUFFER_READY: false while a framebuffer transfer is pending or locked.
BOOLEAN CanBlitToFrameBuffer();
void InvalidateRegion(INT32, INT32, INT32, INT32);
// Original extended dirty rectangles share the same non-scrolling pixel copy,
// but have an independent128-entry budget and split at the viewport boundary.
// The original stores flags without consuming them; all current callers pass0.
void InvalidateRegionEx(INT32, INT32, INT32, INT32, UINT32);
void InvalidateRegions(SGPRect*, UINT32);
void InvalidateScreen();
void InvalidateFrameBuffer();
void StartFrameBufferRender();
void EndFrameBufferRender();
void RefreshScreen(void* unused);
namespace sgp
{
struct NativeVideoRegionState {
    INT32 viewportBottom; bool scrolling;
    NativeScrollRequest scroll{};
    UINT16* zBuffer=nullptr; // real owned InitZBuffer allocation, never borrowed metadata
    bool gameScreen=true; // genuine engine binds guiCurrentScreen==GAME_SCREEN
};
struct NativeVideoScrollHooks {
    // Bind only original engine rendering/overlay functions. Callbacks run with
    // video surfaces unlocked and must return with all acquired locks released.
    // They preserve reserved surface lifetimes/identity and the owned Z block.
    bool (*renderStrip)(const SGPRect&)=nullptr;
    // Begin atomically snapshots original overlay save areas/flags before pixel
    // changes. A false return/exception leaves no active transaction.
    bool (*beginOverlays)()=nullptr;
    // Original RestoreShifted/Save/Execute order, under the active transaction.
    bool (*finishOverlays)(INT32 shiftX,INT32 shiftY)=nullptr;
    // Rollback on every failure, commit only after successful presentation.
    void (*endOverlays)(bool commit) noexcept=nullptr;
    // Original pending-scroll flags clear after any successful frame present.
    void (*committedFrame)() noexcept=nullptr;
};
void SetNativeVideoScrollHooks(NativeVideoScrollHooks);
using NativeVideoRegionSource = NativeVideoRegionState (*)() noexcept;
// Borrowed main-thread state reader. The original engine binds its actual
// viewport/scroll globals; isolated consumers default to a non-scrolling frame.
// Scrolling requires the real owned Z allocation, direction/steps/viewport and
// original engine hooks. Missing/invalid binding retains pending work unchanged.
void SetNativeVideoRegionSource(NativeVideoRegionSource);
// Configuration is accepted only while inactive. Title is copied. Defaults are
// 640x480, RGB565, scale 1. Dimensions must fit UINT16, scaled SDL dimensions
// must fit int, and allocation arithmetic must fit size_t and ptrdiff_t.
bool ConfigureNativeVideo(UINT32 width, UINT32 height, UINT32 depth, UINT32 scale, const char* title,
                          NativePresentationOptions options = {});
// Idempotent. Refuses owned locks without closing the window or changing aliases.
// Releases only video-owned surfaces; leaves the surface manager initialized.
bool ShutdownNativeVideo();
// Preflights all three transfer locks before any copy. Suspended/locked failures
// preserve pixels and pending work. Scrolling snapshots PRIMARY/BACK/FRAME/Z and
// the bound original overlays; rendering/presentation failure rolls back those
// pixels and overlay state, retaining pending work. Bound callbacks must balance
// locks. Success alone clears invalidation. Transfers ignore drawing clip and row
// padding. A no-dirty refresh presents direct backbuffer work. Non-scrolling
// presentation failure retains pending work and may leave copied pixels.
bool RefreshNativeVideo();
// Converts the real MOUSE_BUFFER RGB565 bitmap to SDL color cursor pixels.
// Unsupported drivers retain BUFFER_DIRTY; success alone commits replacement.
// A cursor needing >4096 physical pixels per axis fails without replacement.
// Main-thread only. Bitmap zero is transparent, matching original mouse buffer.
bool RefreshNativeCursor();
std::uint64_t NativeVideoGeneration();
bool SaveNativeVideoWindow(const char* bmpPath);
const char* NativeVideoDriver();
bool NativeVideoQuitRequested();
// Borrowed, main-thread event boundary; no input implementation dependency.
// Geometry is logical framebuffer plus current actual SDL window dimensions.
bool NativeVideoInputInfo(SDL_WindowID&, int&, int&, int&, int&);
bool NativeVideoPresentationGeometry(NativePresentationGeometry&);
bool AttachNativeVideoInput(PresentationEventSink, void*);
void DetachNativeVideoInput(PresentationEventSink, void*);
void PumpNativeVideoEvents();
}
#endif
