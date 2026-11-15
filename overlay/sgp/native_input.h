/*
 * P710 native snapshot update: sgp/native_input.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_INPUT_H
#define SGP_NATIVE_INPUT_H
#include "types.h"
#include <cstdint>
// Native observation added 2026-11-12; process-local, saturates at UINT64_MAX.
// Changes on input recovery/shutdown; never reset by initialization.
namespace sgp {
std::uint64_t NativeInputResetGeneration();
// True after close/destruction until explicit shutdown/reinitialization.
bool NativeInputTerminated();
bool NativeInputFocused();
// Original binding-file virtual-key byte, queried from focused owned-window
// held state. No event is consumed or desktop-global key state consulted.
// Keypad numeric/navigation identity retains the bank captured on key-down.
// Unsupported codes return false; no unreliable Windows transition bit.
bool NativeBindingKeyDown(UINT8 historicalCode);
}
// Main-thread-only native subset. No string editing, Win32 scan-code bridge,
// double-click, button timers, or text/IME. Restriction and pointer warps target
// only the owned SDL window; no desktop-global confinement is requested.
// Constants remain available even where synthesis of that event is unsupported.
#define KEY_DOWN 0x0001
#define KEY_UP 0x0002
#define KEY_REPEAT 0x0004
#define LEFT_BUTTON_DOWN 0x0008
#define LEFT_BUTTON_UP 0x0010
#define LEFT_BUTTON_DBL_CLK 0x0020
#define LEFT_BUTTON_REPEAT 0x0040
#define RIGHT_BUTTON_DOWN 0x0080
#define RIGHT_BUTTON_UP 0x0100
#define RIGHT_BUTTON_REPEAT 0x0200
#define MOUSE_POS 0x0400
#define MOUSE_WHEEL 0x0800
#define MOUSE_WHEEL_UP 0x0800
#define MOUSE_WHEEL_DOWN 0x1000
#define MIDDLE_BUTTON_DOWN 0x2000
#define MIDDLE_BUTTON_UP 0x4000
#define MIDDLE_BUTTON_REPEAT 0x8000
#define X1_BUTTON_DOWN 0x8010
#define X1_BUTTON_UP 0x8020
#define X1_BUTTON_REPEAT 0x8030
#define X2_BUTTON_DOWN 0x8040
#define X2_BUTTON_UP 0x8050
#define X2_BUTTON_REPEAT 0x8060
#define SHIFT_DOWN 0x01
#define CTRL_DOWN 0x02
#define ALT_DOWN 0x04
typedef struct InputAtom
{
    UINT32 uiTimeStamp;
    UINT16 usKeyState;
    UINT16 usEvent;
    UINT32 usParam;
    UINT32 uiParam;
} InputAtom;
namespace sgp {
// Exact enum-tag checks: X1/X2 tags overlap the legacy bit-mask values.
bool IsNativeGameMouseEvent(UINT16);
// Pumps input, then consumes only a mouse head. Null/miss leaves FIFO intact.
bool DequeueNativeGameMouseEvent(InputAtom*);
// Main-thread observation without pumping; used after a prefix drain.
bool NativeInputQueueEmpty();
}
using NativeMouseInputHook = void (*)(UINT16,UINT16,UINT16,BOOLEAN,BOOLEAN);
extern NativeMouseInputHook MouseSystemHook;
void SetMouseInputHook(NativeMouseInputHook);
namespace sgp { void ClearNativeMouseInputHook(NativeMouseInputHook expected); }
extern "C" {
BOOLEAN InitializeInputManager(void);
void ShutdownInputManager(void);
void QueueEvent(UINT16, UINT32, UINT32);
BOOLEAN DequeueEvent(InputAtom*);
BOOLEAN DequeueSpecificEvent(InputAtom*, UINT32);
BOOLEAN PeekSpecificEvent(UINT32);
// Historical name: pumps, then drains the WHOLE queue; held state is retained.
void DequeueAllKeyBoardEvents();
void GetMousePos(SGPPoint*);
void RestrictMouseToXYXY(UINT16,UINT16,UINT16,UINT16);
void RestrictMouseCursor(SGPRect*);
void FreeMouseCursor(BOOLEAN);
void RestoreCursorClipRect();
void GetRestrictedClipCursor(SGPRect*);
BOOLEAN IsCursorRestricted();
void SimulateMouseMovement(UINT32,UINT32);
BOOLEAN InputEventInside(InputAtom*, UINT32, UINT32, UINT32, UINT32);
extern BOOLEAN gfKeyState[256];
extern BOOLEAN gfLeftButtonState, gfRightButtonState, gfMiddleButtonState;
extern INT16 gusMouseXPos, gusMouseYPos, gsMouseWheelDeltaValue;
extern BOOLEAN gfSGPInputReceived, gfTrackMousePos;
}
// Portable extraction of the original unsigned, packed 16-bit coordinates.
#define GETXPOS(a) ((UINT16)((a)->uiParam & 0xffffu))
#define GETYPOS(a) ((UINT16)(((a)->uiParam >> 16) & 0xffffu))
inline BOOLEAN NativeInputKeyDown(UINT32 key) { return key < 256 ? gfKeyState[key] : FALSE; }
#define _KeyDown(a) NativeInputKeyDown((UINT32)(a))
#define _LeftButtonDown gfLeftButtonState
#define _RightButtonDown gfRightButtonState
#define _MiddleButtonDown gfMiddleButtonState
#define _WheelValue gsMouseWheelDeltaValue
#define _MouseXPos gusMouseXPos
#define _MouseYPos gusMouseYPos
#define _gusMouseInside(x1,y1,x2,y2) ((gusMouseXPos >= (x1)) && (gusMouseXPos <= (x2)) && (gusMouseYPos >= (y1)) && (gusMouseYPos <= (y2)))
#define _EvType(a) (((InputAtom*)(a))->usEvent)
#define _EvTimeStamp(a) (((InputAtom*)(a))->uiTimeStamp)
#define _EvKey(a) (((InputAtom*)(a))->usParam)
#define _EvMouseX(a) GETXPOS((InputAtom*)(a))
#define _EvMouseY(a) GETYPOS((InputAtom*)(a))
#define _EvShiftDown(a) (((InputAtom*)(a))->usKeyState & SHIFT_DOWN)
#define _EvCtrlDown(a) (((InputAtom*)(a))->usKeyState & CTRL_DOWN)
#define _EvAltDown(a) (((InputAtom*)(a))->usKeyState & ALT_DOWN)
// Init requires the current owned video window and logical dimensions <=32768.
// Repeated init preserves active state. Shutdown clears everything and detaches.
// Video destruction invalidates the binding; explicitly shutdown/reinitialize
// before using a subsequently opened window. Null dequeue outputs never drain.
// Specific/peek inspect only the head. Specific copies the head on a mask miss.
// Wheel polling reports the latest vertical delta, truncated and INT16-saturated.
// Focus recovery drops stale presses, emits current held releases first, then
// retains undelivered release atoms, deduplicated within the 256-slot bound.
#endif
