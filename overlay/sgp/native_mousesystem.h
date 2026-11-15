/*
 * P710 native snapshot update: sgp/native_mousesystem.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation, 2026-11-12: bounded original SGP region dispatch.
// Based on 1dot13/source at3394695a387a14c78e92b158cfdcf50e4068c96b.
#ifndef SGP_NATIVE_MOUSESYSTEM_H
#define SGP_NATIVE_MOUSESYSTEM_H
#include "native_input.h"
#include "native_ui_payload.h"
#include <cstdint>
#ifndef _SIRTECH_TYPES_
using CHAR16 = wchar_t;
using STR16 = CHAR16*;
#endif
typedef void (*MOUSE_CALLBACK)(struct _MOUSE_REGION *,INT32);	// Define MOUSE_CALLBACK type as pointer to void 
typedef void (*MOUSE_HELPTEXT_DONE_CALLBACK)( ); // the help is done callback

typedef struct _MOUSE_REGION {
	UINT16						IDNumber;				// Region's ID number, set by mouse system
	INT8						PriorityLevel;			// Region's Priority, set by system and/or caller
	UINT32						uiFlags;				// Region's state flags
	INT16						RegionTopLeftX;			// Screen area affected by this region (absolute coordinates)
	INT16						RegionTopLeftY;
	INT16						RegionBottomRightX;
	INT16						RegionBottomRightY;
	INT16						MouseXPos;				// Mouse's Coordinates in absolute screen coordinates
	INT16						MouseYPos;
	INT16						RelativeXPos;			// Mouse's Coordinates relative to the Top-Left corner of the region
	INT16						RelativeYPos;
	UINT16						ButtonState;			// Current state of the mouse buttons
	INT16						WheelState;				// Wheel state +/-number of wheel units
	UINT16						Cursor;					// Cursor to use when mouse in this region (see flags)
	MOUSE_CALLBACK				MovementCallback;		// Pointer to callback function if movement occured in this region
	MOUSE_CALLBACK				ButtonCallback;			// Pointer to callback function if button action occured in this region
	NativeUIPayload									UserData[4];	// User Data, can be set to anything!
	
	//Fast help vars.
	INT16					FastHelpTimer;		// Countdown timer for FastHelp text
	CHAR16				*FastHelpText;		// Text string for the FastHelp (describes buttons if left there a while)
	INT32					FastHelpRect;
	MOUSE_HELPTEXT_DONE_CALLBACK HelpDoneCallback;

	struct _MOUSE_REGION	*next;							// List maintenance, do NOT touch these entries
	struct _MOUSE_REGION	*prev;
} MOUSE_REGION;
// Mouse Region Flags
#define MSYS_NO_FLAGS						0x00000000
#define MSYS_MOUSE_IN_AREA					0x00000001
#define MSYS_SET_CURSOR						0x00000002
#define MSYS_MOVE_CALLBACK					0x00000004
#define MSYS_BUTTON_CALLBACK				0x00000008
#define MSYS_REGION_EXISTS					0x00000010
#define MSYS_SYSTEM_INIT					0x00000020
#define MSYS_REGION_ENABLED					0x00000040
#define MSYS_FASTHELP						0x00000080
#define MSYS_GOT_BACKGROUND					0x00000100
#define MSYS_HAS_BACKRECT					0x00000200
#define MSYS_FASTHELP_RESET					0x00000400
#define MSYS_ALLOW_DISABLED_FASTHELP		0x00000800

// Mouse region IDs
#define MSYS_ID_BASE					1
#define MSYS_ID_MAX						0xfffffff // ( INT32 max )
#define MSYS_ID_SYSTEM					0

// Mouse region priorities
#define MSYS_PRIORITY_LOWEST		0
#define MSYS_PRIORITY_LOW			15
#define MSYS_PRIORITY_BASE			31
#define MSYS_PRIORITY_NORMAL		31
#define MSYS_PRIORITY_HIGH			63
#define MSYS_PRIORITY_HIGHEST		127
#define MSYS_PRIORITY_SYSTEM		-1
#define MSYS_PRIORITY_AUTO			-1

// Mouse system defines used during updates
#define MSYS_NO_ACTION			0
#define MSYS_DO_MOVE			1
#define MSYS_DO_LBUTTON_DWN		2
#define MSYS_DO_LBUTTON_UP		4
#define MSYS_DO_RBUTTON_DWN		8
#define MSYS_DO_RBUTTON_UP		16
#define MSYS_DO_LBUTTON_REPEAT	32
#define MSYS_DO_RBUTTON_REPEAT	64
#define MSYS_DO_MBUTTON_DWN			128
#define MSYS_DO_MBUTTON_UP			256
#define MSYS_DO_MBUTTON_REPEAT		512

#define MSYS_DO_X1BUTTON_DWN			1024
#define MSYS_DO_X1BUTTON_UP			2048
#define MSYS_DO_X1BUTTON_REPEAT		4096
#define MSYS_DO_X2BUTTON_DWN			8192
#define MSYS_DO_X2BUTTON_UP			16384
#define MSYS_DO_X2BUTTON_REPEAT		32768

#define MSYS_DO_WHEEL_UP			65536
#define MSYS_DO_WHEEL_DOWN			131072

#define MSYS_DO_BUTTONS				(MSYS_DO_LBUTTON_DWN|MSYS_DO_LBUTTON_UP|MSYS_DO_LBUTTON_REPEAT|\
									MSYS_DO_RBUTTON_DWN|MSYS_DO_RBUTTON_UP|MSYS_DO_RBUTTON_REPEAT|\
									MSYS_DO_MBUTTON_DWN|MSYS_DO_MBUTTON_UP|MSYS_DO_MBUTTON_REPEAT|\
									MSYS_DO_X1BUTTON_DWN|MSYS_DO_X1BUTTON_UP|MSYS_DO_X1BUTTON_REPEAT|\
									MSYS_DO_X2BUTTON_DWN|MSYS_DO_X2BUTTON_UP|MSYS_DO_X2BUTTON_REPEAT)

#define MSYS_DO_WHEEL				(MSYS_DO_WHEEL_UP|MSYS_DO_WHEEL_DOWN)
//#define MSYS_DO_BUTTONS			(MSYS_DO_LBUTTON_DWN|MSYS_DO_LBUTTON_UP|MSYS_DO_RBUTTON_DWN|MSYS_DO_RBUTTON_UP|MSYS_DO_RBUTTON_REPEAT|MSYS_DO_LBUTTON_REPEAT)

// Mouse system button masks
#define MSYS_LEFT_BUTTON		1
#define MSYS_RIGHT_BUTTON		2
#define MSYS_MIDDLE_BUTTON		4
#define MSYS_X1_BUTTON			8
#define MSYS_X2_BUTTON			16

// Mouse system special values
#define MSYS_NO_CALLBACK				NULL
#define MSYS_NO_CURSOR					65534

// Mouse system callback reasons
#define MSYS_CALLBACK_REASON_NONE									0
#define MSYS_CALLBACK_REASON_INIT									1
#define MSYS_CALLBACK_REASON_MOVE									2
#define MSYS_CALLBACK_REASON_LBUTTON_DWN					4
#define MSYS_CALLBACK_REASON_LBUTTON_UP						8
#define MSYS_CALLBACK_REASON_RBUTTON_DWN					16
#define MSYS_CALLBACK_REASON_RBUTTON_UP						32
//#define MSYS_CALLBACK_REASON_BUTTONS							(MSYS_CALLBACK_REASON_LBUTTON_DWN|MSYS_CALLBACK_REASON_LBUTTON_UP| \
//																									MSYS_CALLBACK_REASON_RBUTTON_DWN|MSYS_CALLBACK_REASON_RBUTTON_UP)
#define MSYS_CALLBACK_REASON_LOST_MOUSE						64
#define MSYS_CALLBACK_REASON_GAIN_MOUSE						128

#define	MSYS_CALLBACK_REASON_LBUTTON_REPEAT				256
#define	MSYS_CALLBACK_REASON_RBUTTON_REPEAT				512

//Kris:	Nov 31, 1999
//Added support for double clicks.	The DOUBLECLICK event is passed combined with
//the LBUTTON_DWN event if two LBUTTON_DWN events are detected on the same button/region
//within the delay defined by MSYS_DOUBLECLICK_DELAY (in milliseconds).	If your button/region
//supports double clicks and single clicks, make sure the DOUBLECLICK event is checked first (rejecting
//the LBUTTON_DWN event if detected)
#define MSYS_CALLBACK_REASON_LBUTTON_DOUBLECLICK	1024
#define MSYS_CALLBACK_REASON_MBUTTON_DWN			2048
#define MSYS_CALLBACK_REASON_MBUTTON_UP				4096
#define	MSYS_CALLBACK_REASON_MBUTTON_REPEAT			8192
#define MSYS_CALLBACK_REASON_WHEEL_UP				16384
#define MSYS_CALLBACK_REASON_WHEEL_DOWN				32768
#define MSYS_CALLBACK_REASON_X1BUTTON_DWN			65536
#define MSYS_CALLBACK_REASON_X1BUTTON_UP			131072
#define	MSYS_CALLBACK_REASON_X1BUTTON_REPEAT		262144
#define MSYS_CALLBACK_REASON_X2BUTTON_DWN			524288
#define MSYS_CALLBACK_REASON_X2BUTTON_UP			1048576
#define	MSYS_CALLBACK_REASON_X2BUTTON_REPEAT		2097152


#define MSYS_CALLBACK_REASON_BUTTONS				(MSYS_CALLBACK_REASON_LBUTTON_DWN|MSYS_CALLBACK_REASON_LBUTTON_UP| \
													MSYS_CALLBACK_REASON_RBUTTON_DWN|MSYS_CALLBACK_REASON_RBUTTON_UP| \
													MSYS_CALLBACK_REASON_MBUTTON_DWN|MSYS_CALLBACK_REASON_MBUTTON_UP|\
													MSYS_CALLBACK_REASON_X1BUTTON_DWN|MSYS_CALLBACK_REASON_X1BUTTON_UP|\
													MSYS_CALLBACK_REASON_X2BUTTON_DWN|MSYS_CALLBACK_REASON_X2BUTTON_UP)

#define MSYS_CALLBACK_REASON_WHEEL					(MSYS_CALLBACK_REASON_WHEEL_UP|MSYS_CALLBACK_REASON_WHEEL_DOWN)


// Mouse grabbing return codes
#define MSYS_GRABBED_OK						0
#define MSYS_ALREADY_GRABBED			1
#define MSYS_REGION_NOT_IN_LIST		2
// All APIs are main-thread-only. Callbacks must not throw. Remove user-owned
// regions before destruction. Nested dispatch/refresh is rejected until return;
// selection-changing mutations end the event and request a later refresh;
// cosmetic cursor changes and idempotent setters do not abort. GAIN precedes
// coordinate assignment. Shutdown inside callbacks is supported.
// IDs stay within UINT16 with collision checks; priority changes do not reorder.
// Cursor selection uses the bound original database service; doubleclick
// synthesis remains unsupported.
INT32 MSYS_Init();
void MSYS_Shutdown();
void MSYS_DefineRegion(MOUSE_REGION*,UINT16,UINT16,UINT16,UINT16,INT8,UINT16,MOUSE_CALLBACK,MOUSE_CALLBACK);
INT32 MSYS_AddRegion(MOUSE_REGION*);
void MSYS_RemoveRegion(MOUSE_REGION*);
void MSYS_EnableRegion(MOUSE_REGION*);
void MSYS_DisableRegion(MOUSE_REGION*);
void MSYS_ChangeRegionPriority(MOUSE_REGION*,INT8);
void MSYS_ChangeRegionCursor(MOUSE_REGION*,UINT16);
// Text is copied and owned until replacement/remove/shutdown. Null/empty clears
// text; invalid/unregistered regions are ignored. The pinned source explicitly
// disables help-end callback execution; native preserves that behavior.
void SetRegionFastHelpText(MOUSE_REGION*,const STR16);
void SetRegionHelpEndCallback(MOUSE_REGION*,MOUSE_HELPTEXT_DONE_CALLBACK);
void ExecuteMouseHelpEndCallBack(MOUSE_REGION*);
void RenderFastHelp();
void SetFastHelpDelay(INT16);
void EnableMouseFastHelp();
void DisableMouseFastHelp();
void MSYS_SetRegionUserData(MOUSE_REGION*,INT32,NativeUIPayload);
NativeUIPayload MSYS_GetRegionUserData(MOUSE_REGION*,INT32);
void MSYS_SetCurrentCursor(UINT16);
INT32 MSYS_RegionInList(MOUSE_REGION*);
INT32 MSYS_GrabMouse(MOUSE_REGION*);
void MSYS_ReleaseMouse(MOUSE_REGION*);
void MSYS_MoveMouseRegionTo(MOUSE_REGION*,INT16,INT16);
void MSYS_MoveMouseRegionBy(MOUSE_REGION*,INT16,INT16);
void MSYS_AllowDisabledRegionFastHelp(MOUSE_REGION*,BOOLEAN);
void MSYS_UpdateMouseRegion();
void RefreshMouseRegions();
void ResetClickedMode();
void ResetWheelState(MOUSE_REGION*);
MOUSE_REGION* get_first_entry_in_MSYS_RegList();
MOUSE_REGION* get_next_entry_in_MSYS_RegList(MOUSE_REGION*);
extern MOUSE_REGION *MSYS_RegList, *MSYS_PrevRegion, *MSYS_CurrRegion;
extern MOUSE_REGION MSYS_SystemBaseRegion;
extern INT16 MSYS_CurrentMX, MSYS_CurrentMY;
extern "C" void MSYS_SGP_Mouse_Handler_Hook(UINT16,UINT16,UINT16,BOOLEAN,BOOLEAN);
namespace sgp {
// Bind the original SetCurrentCursorFromDatabase after InitCursors; clear before
// its database is destroyed. No ownership transfer; main thread, no throwing.
using NativeMouseCursorSelector = BOOLEAN (*)(UINT32);
void SetNativeMouseCursorSelector(NativeMouseCursorSelector);
// No pump, dequeue or polling-state reads. Supports L/R/middle/X1/X2
// down/up/repeat, wheel direction atoms and POS. WheelState counts events.
// Original extended state/wheel behavior has no extended button/wheel callbacks.
// Reset generation changes cancel anchors and suppress unpaired release atoms.
// Terminal input backlogs are ignored until explicit input reinitialization.
// Already-copied application atoms have no reset metadata; do not replay them
// across focus/input resets. Direct hook intentionally keeps original semantics.
bool DispatchNativeMouseEvent(const InputAtom&);
// Original native GameLoop bridge: pumps and drains exact-tag mouse prefix;
// retains keyboard head/order, synchronizes empty-queue focus/terminal resets.
// When FIFO is empty, coalesces untracked hover using current coordinates and
// replayed button state. No final polling state is used for queued atoms.
void DispatchQueuedNativeGameMouseEvents(BOOLEAN extendedMouseEnabled);
void CancelNativeMouseInput();
// Registration identity allows button callbacks to reject removed/reused areas.
std::uint64_t NativeMouseRegionGeneration(const MOUSE_REGION*);
bool NativeMouseDispatchActive();
// Bind real loaded bitmap fonts and a real RGB565 surface. Font generations and
// backing IDs prevent slot/alias reuse. Call after font/video initialization.
bool ConfigureNativeFastHelp(INT32 regularFont, INT32 boldFont, UINT32 destination);
// Actual scalable tooltip handles and original >1 scaling predicate.
bool ConfigureNativeScalableFastHelp(INT32 regularCore, INT32 boldCore, double scale);
void ClearNativeScalableFastHelp() noexcept;
// Call before painting the next scene. Locked destinations retain a pending
// restoration; pixels changed by the scene are preserved. RenderFastHelp also
// performs this restoration before drawing. No scene graph is synthesized.
bool RestoreNativeFastHelpBackground();
// Deterministic monotonic-millisecond seam; nullptr restores steady_clock.
// Replacing the clock restarts the current hover delay, never replays input.
using NativeFastHelpClock = std::uint64_t (*)();
void SetNativeFastHelpClockForTesting(NativeFastHelpClock);
}
#endif
