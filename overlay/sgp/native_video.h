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

#ifndef SGP_NATIVE_VIDEO_H
#define SGP_NATIVE_VIDEO_H
#include "native_surface.h"
// Single-threaded, main-thread-only RGB565 subset; SDLPresentation owns SDL video.
// Initialize accepts only (nullptr, 0, nullptr); native callbacks are unsupported.
// Repeated valid initialization is a no-op. Settings report configured geometry.
BOOLEAN InitializeVideoManager(void* instance, UINT16 show, void* windowproc);
void ShutdownVideoManager();
void SuspendVideoManager();
BOOLEAN RestoreVideoManager();
void GetCurrentVideoSettings(UINT16*, UINT16*, UINT8*);
PTR LockPrimarySurface(UINT32*);
void UnlockPrimarySurface();
PTR LockBackBuffer(UINT32*);
void UnlockBackBuffer();
PTR LockFrameBuffer(UINT32*);
void UnlockFrameBuffer();
// Matches BUFFER_READY: false while a framebuffer transfer is pending or locked.
BOOLEAN CanBlitToFrameBuffer();
void InvalidateRegion(INT32, INT32, INT32, INT32);
void InvalidateRegions(SGPRect*, UINT32);
void InvalidateScreen();
void InvalidateFrameBuffer();
void StartFrameBufferRender();
void EndFrameBufferRender();
void RefreshScreen(void* unused);
namespace sgp
{
// Configuration is accepted only while inactive. Title is copied. Defaults are
// 640x480, RGB565, scale 1. Dimensions must fit UINT16, scaled SDL dimensions
// must fit int, and allocation arithmetic must fit size_t and ptrdiff_t.
bool ConfigureNativeVideo(UINT32 width, UINT32 height, UINT32 depth, UINT32 scale, const char* title);
// Idempotent. Refuses owned locks without closing the window or changing aliases.
// Releases only video-owned surfaces; leaves the surface manager initialized.
bool ShutdownNativeVideo();
// Preflights all three locks before any copy. Suspended/locked failures preserve
// pixels and pending work. Presentation failure can leave copied pixels, but
// retains pending work. Success alone clears invalidation. Transfers ignore the
// drawing clip and row padding. A no-dirty refresh presents direct backbuffer work.
bool RefreshNativeVideo();
bool SaveNativeVideoWindow(const char* bmpPath);
const char* NativeVideoDriver();
bool NativeVideoQuitRequested();
}
#endif
