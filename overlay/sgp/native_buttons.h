/*
 * P710 native snapshot update: sgp/native_buttons.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation, 2026-11-12/13: quick-image/checkbox/hotspot button manager.
// Based on 1dot13/source at3394695a387a14c78e92b158cfdcf50e4068c96b.
#ifndef SGP_NATIVE_BUTTONS_H
#define SGP_NATIVE_BUTTONS_H
#include "vobject.h"
#include "native_mousesystem.h"
#include "native_button_sounds.h"
#include <vector>
#define BUTTON_USE_DEFAULT		-1
#define BUTTON_NO_FILENAME		NULL
#define BUTTON_NO_CALLBACK		NULL
#define BUTTON_NO_IMAGE				-1
#define BUTTON_NO_SLOT				-1

#define BUTTON_INIT						1
#define BUTTON_WAS_CLICKED		2

//affects how the button is rendered.
#define BUTTON_TYPES						( BUTTON_QUICK | BUTTON_GENERIC | BUTTON_HOT_SPOT | BUTTON_CHECKBOX )
//affects how the button is processed
#define BUTTON_TYPE_MASK				(BUTTON_NO_TOGGLE| BUTTON_ALLOW_DISABLED_CALLBACK | BUTTON_CHECKBOX | BUTTON_IGNORE_CLICKS )

//button flags
#define BUTTON_TOGGLE										0x00000000
#define BUTTON_QUICK										0x00000000
#define BUTTON_ENABLED									0x00000001
#define BUTTON_CLICKED_ON								0x00000002
#define BUTTON_NO_TOGGLE								0x00000004
#define BUTTON_CLICK_CALLBACK						0x00000008
#define BUTTON_MOVE_CALLBACK						0x00000010
#define BUTTON_GENERIC									0x00000020
#define BUTTON_HOT_SPOT									0x00000040
#define BUTTON_SELFDELETE_IMAGE					0x00000080
#define BUTTON_DELETION_PENDING					0x00000100
#define BUTTON_ALLOW_DISABLED_CALLBACK	0x00000200
#define BUTTON_DIRTY										0x00000400
#define BUTTON_SAVEBACKGROUND						0x00000800
#define BUTTON_CHECKBOX									0x00001000
#define BUTTON_NEWTOGGLE								0x00002000
#define BUTTON_FORCE_UNDIRTY						0x00004000 // no matter what happens this buttons does NOT get marked dirty
#define BUTTON_IGNORE_CLICKS						0x00008000 // Ignore any clicks on this button
#define BUTTON_16BPPCOLOURTEXT						0x00010000 // Use 16 bit colours for the text of this button
#define BUTTON_DISABLED_CALLBACK				0x80000000

typedef void (*GUI_CALLBACK)(struct _GUI_BUTTON *,INT32);

// GUI_BUTTON structure definitions.
typedef struct _GUI_BUTTON {
	INT32					IDNum;						// ID Number, contains it's own button number
	UINT32				ImageNum;					// Image number to use (see DOCs for details)
	MOUSE_REGION	Area;							// Mouse System's mouse region to use for this button
	GUI_CALLBACK	ClickCallback;		// Button Callback when button is clicked
	GUI_CALLBACK	MoveCallback;			// Button Callback when mouse moved on this region
	INT16					Cursor;						// Cursor to use for this button
	UINT32				uiFlags;					// Button state flags etc.( 32-bit )
	UINT32				uiOldFlags;				// Old flags from previous render loop
	INT16					XLoc;							// Coordinates where button is on the screen
	INT16					YLoc;
	NativeUIPayload					UserData[4];			// Place holder for user data etc.
	INT16					Group;						// Group this button belongs to (see DOCs)
	INT8					bDefaultStatus;
	//Button disabled style
	INT8					bDisabledStyle;
	//For buttons with text
	CHAR16				*string;					//the string
	UINT16				usFont;						//font for text 
	BOOLEAN				fMultiColor;			//font is a multi-color font
	INT16					sForeColor;				//text colors if there is text
	INT16					sShadowColor;			
	INT16					sForeColorDown;		//text colors when button is down (optional)
	INT16					sShadowColorDown;
	INT16					sForeColorHilited;		//text colors when button is down (optional)
	INT16					sShadowColorHilited;
	INT8					bJustification;		// BUTTON_TEXT_LEFT, BUTTON_TEXT_CENTER, BUTTON_TEXT_RIGHT
	INT8					bTextXOffset;
	INT8					bTextYOffset;
	INT8					bTextXSubOffSet;
	INT8					bTextYSubOffSet;
	BOOLEAN				fShiftText;
	INT16					sWrappedWidth;
	//For buttons with icons (don't confuse this with quickbuttons which have up to 5 states )
	INT32					iIconID;							
	INT16					usIconIndex;
	INT8					bIconXOffset; //-1 means horizontally centered
	INT8					bIconYOffset; //-1 means vertically centered
	BOOLEAN				fShiftImage;  //if true, icon is shifted +1,+1 when button state is down.

	UINT8					ubToggleButtonOldState;		// Varibles for new toggle buttons that work
	UINT8					ubToggleButtonActivated;

	INT32					BackRect;					// Handle to a Background Rectangle
	UINT8					ubSoundSchemeID;
} GUI_BUTTON;


#define BUTTON_TEXT_LEFT -1
#define BUTTON_TEXT_CENTER 0
#define BUTTON_TEXT_RIGHT 1
#define TEXT_LJUSTIFIED BUTTON_TEXT_LEFT
#define TEXT_CJUSTIFIED BUTTON_TEXT_CENTER
#define TEXT_RJUSTIFIED BUTTON_TEXT_RIGHT
// Exact original Button System.h disabled-style enum values.
enum { DISABLED_STYLE_NONE, DISABLED_STYLE_DEFAULT, DISABLED_STYLE_HATCHED, DISABLED_STYLE_SHADED };
#define MAX_BUTTONS 400
extern std::vector<GUI_BUTTON*> ButtonList;
inline GUI_BUTTON* GetButtonPtr(INT32 id) {
    return id >= 0 && id < MAX_BUTTONS && static_cast<std::size_t>(id) < ButtonList.size() ? ButtonList[static_cast<std::size_t>(id)] : nullptr;
}
typedef struct {
	HVOBJECT	vobj;						// The Image itself
	INT32			Grayed;					// Index to use for a "Grayed-out" button
	INT32			OffNormal;			// Index to use when button is OFF
	INT32			OffHilite;			// Index to use when button is OFF w/ hilite on it
	INT32			OnNormal;				// Index to use when button is ON
	INT32			OnHilite;				// Index to use when button is ON w/ hilite on it
	UINT32		MaxWidth;				// Width of largest image in use
 	UINT32		MaxHeight;			// Height of largest image in use
	UINT32		fFlags;					// Special image flags
} BUTTON_PICS;

#define MAX_BUTTON_PICS			256

extern std::vector<BUTTON_PICS> ButtonPictures;

// Main thread only; all supplied callbacks MUST NOT throw. Nested callbacks are
// depth-guarded; nested event dispatch is rejected by MSYS. Remove unlinks at
// once, but frees/reuses its slot only after outer event/callback completion.
// Init MSYS and resource/surface managers first. Shutdown buttons before MSYS,
// objects and surfaces. Reinitialize explicitly after UI shutdown.
// Borrowed objects must remain alive and unmodified until image unload. Unload
// of a referenced image is rejected (void API: slot remains unchanged). Owned
// objects are deleted only after every sharing image slot is unloaded; ownership
// transfers when the original slot unloads before its aliases. UseLoaded rejects
// external/borrowed image slots, as in the original. Public IDs are reusable slot
// numbers, not generation handles; unloaded IDs are invalid until slot reuse.
// Generic skins/icons are owned by this manager and cannot unload while
// referenced. Quick-button icons borrow indexed video objects; callers retain
// their lifetime. Default generic skin loads lazily from GENBUTN[2-4].STI.
// Background-saving remains unsupported and its creation flag is rejected. Cursor updates retained region state; no cursor sprite
// renderer is supplied. Quick image+text uses the native bitmap font manager.
// Unsupported creation flags are rejected, rather than stripped.
// Missing gray image uses source OffNormal fallback (checkbox uses hilite),
// then source disabled-style hatch/shade after text. Existing gray frames
// suppress overlays. DEFAULT chooses shade for text, hatch otherwise (checkbox
// always hatches). Pattern phase restarts at the clipped rectangle origin.
BOOLEAN InitButtonSystem();
void ShutdownButtonSystem();
#define MAX_GENERIC_PICS 40
#define MAX_BUTTON_ICONS 40
#define DEFAULT_GENERIC_BUTTON_OFF "GENBUTN.STI"
#define DEFAULT_GENERIC_BUTTON_ON "GENBUTN2.STI"
#define DEFAULT_GENERIC_BUTTON_OFF_HI "GENBUTN3.STI"
#define DEFAULT_GENERIC_BUTTON_ON_HI "GENBUTN4.STI"
INT16 LoadGenericButtonImages(const char*,const char*,const char*,const char*,const char*,const char*,INT16,INT16,INT16);
BOOLEAN UnloadGenericButtonImage(INT16);
INT16 LoadGenericButtonIcon(const char*);
BOOLEAN UnloadGenericButtonIcon(INT16);
INT32 CreateTextButton(STR16,UINT32,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT32,INT16,GUI_CALLBACK,GUI_CALLBACK);
INT32 CreateIconButton(INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT32,INT16,GUI_CALLBACK,GUI_CALLBACK);
BOOLEAN SpecifyButtonIcon(INT32,INT32,UINT16,INT8,INT8,BOOLEAN);
void SpecifyButtonTextWrappedWidth(INT32,INT16);
void SetBtnHelpEndCallback(INT32,MOUSE_HELPTEXT_DONE_CALLBACK);
INT32 LoadButtonImage(const char*,INT32,INT32,INT32,INT32,INT32);
INT32 UseLoadedButtonImage(INT32,INT32,INT32,INT32,INT32,INT32);
INT32 UseVObjAsButtonImage(HVOBJECT,INT32,INT32,INT32,INT32,INT32);
void UnloadButtonImage(INT32);
// iSlot is a raw subimage/frame index, not a button-state position. Return the
// descriptor's width/height without offsets, or zero for invalid IDs/frames.
UINT16 GetWidthOfButtonPic(UINT16,INT32);
UINT16 GetHeightOfButtonPic(UINT16,INT32);
INT32 QuickCreateButton(UINT32,INT16,INT16,INT32,INT16,GUI_CALLBACK,GUI_CALLBACK);
INT32 CreateHotSpot(INT16,INT16,INT16,INT16,INT16,GUI_CALLBACK,GUI_CALLBACK);
INT32 CreateCheckBoxButton(INT16,INT16,const char*,INT16,GUI_CALLBACK);
// These names describe the temporary visual press, not logical check state.
// As in the source, each leaves MOUSE_IN_AREA set, restores held-button state,
// and draws through DrawButton without toggling BUTTON_CLICKED_ON.
void DrawCheckBoxButtonOn(INT32);
void DrawCheckBoxButtonOff(INT32);
// MSYS owns the copied help text and its rendering lifetime.
void SetButtonFastHelpText(INT32,STR16);
// Pinned source quirk: either Boolean value enables disabled fast help.
void AllowDisabledButtonFastHelp(INT32,BOOLEAN);
#define RenderButtonsFastHelp() RenderFastHelp()
INT32 CreateIconAndTextButton(INT32, const STR16, UINT32, INT16, INT16,
    INT16, INT16, INT8, INT16, INT16, INT32, INT16, GUI_CALLBACK, GUI_CALLBACK);
BOOLEAN SetButtonCursor(INT32,UINT16);
// Stores original validated style and dirties the button. Rendering applies
// the selected overlay only when no explicit gray image exists.
void SpecifyDisabledButtonStyle(INT32,INT8);
void SpecifyButtonText(INT32,STR16);
void RemoveTextFromButton(INT32);
void SpecifyButtonFont(INT32,UINT32);
void SpecifyButtonMultiColorFont(INT32,BOOLEAN);
void SpecifyButtonUpTextColors(INT32,INT16,INT16);
void SpecifyButtonDownTextColors(INT32,INT16,INT16);
void SpecifyButtonHilitedTextColors(INT32,INT16,INT16);
void SpecifyButtonTextJustification(INT32,INT8);
void SpecifyButtonTextOffsets(INT32,INT8,INT8,BOOLEAN);
void SpecifyButtonTextSubOffsets(INT32,INT8,INT8,BOOLEAN);
void SpecifyFullButtonTextAttributes(INT32,STR16,INT32,INT16,INT16,INT16,INT16,INT8);
void SpecifyGeneralButtonTextAttributes(INT32,STR16,INT32,INT16,INT16);
BOOLEAN SpecifyButtonImage(INT32,UINT32);
BOOLEAN SpecifyButtonImage(GUI_BUTTON*,UINT32);
void RemoveButton(INT32);
BOOLEAN EnableButton(INT32);
BOOLEAN DisableButton(INT32);
void HideButton(INT32);
void ShowButton(INT32);
void SetButtonPosition(INT32,INT16,INT16);
BOOLEAN SetButtonDestBuffer(UINT32);
BOOLEAN DrawButton(INT32);
void RenderButtons();
void MarkAButtonDirty(INT32);
void MarkButtonsDirty();
void UnMarkButtonDirty(INT32);
void UnmarkButtonsDirty();
void ForceButtonUnDirty(INT32);
void MSYS_SetBtnUserData(INT32,INT32,NativeUIPayload);
NativeUIPayload MSYS_GetBtnUserData(GUI_BUTTON*,INT32);
BOOLEAN GetButtonArea(INT32,SGPRect*);
INT32 GetButtonWidth(INT32);
INT32 GetButtonHeight(INT32);
INT32 GetButtonX(INT32);
INT32 GetButtonY(INT32);
void QuickButtonCallbackMMove(MOUSE_REGION*,INT32);
void QuickButtonCallbackMButn(MOUSE_REGION*,INT32);
void BtnGenericMouseMoveButtonCallback(GUI_BUTTON*,INT32);
#define DEFAULT_MOVE_CALLBACK BtnGenericMouseMoveButtonCallback
extern UINT32 ButtonDestBuffer;
extern BOOLEAN gfRenderHilights;
#define EnableHilightsAndHelpText() (gfRenderHilights = TRUE)
#define DisableHilightsAndHelpText() (gfRenderHilights = FALSE)
void ReleaseAnchorMode();
namespace sgp {
void CancelNativeButtonInput();
// MSYS calls after completing the outer event and clearing its active flag.
void FinishNativeButtonDispatch();
}
#endif
