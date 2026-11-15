/*
 * P710 native snapshot update: sgp/native_button_sounds.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation of 1dot13/source3394695a387a14c78e92b158cfdcf50e4068c96b
// sgp/Button Sound Control.cpp and the sound events in sgp/Button System.h.
#ifndef SGP_NATIVE_BUTTON_SOUNDS_H
#define SGP_NATIVE_BUTTON_SOUNDS_H
#include "core_types.h"
#include "Button Sound Control.h"

#ifndef BUTTON_SOUND_NONE
#define BUTTON_SOUND_NONE 0x00
#define BUTTON_SOUND_CLICKED_ON 0x01
#define BUTTON_SOUND_CLICKED_OFF 0x02
#define BUTTON_SOUND_MOVED_ONTO 0x04
#define BUTTON_SOUND_MOVED_OFF_OF 0x08
#define BUTTON_SOUND_DISABLED_CLICK 0x10
#define BUTTON_SOUND_DISABLED_MOVED_ONTO 0x20
#define BUTTON_SOUND_DISABLED_MOVED_OFF_OF 0x40
#define BUTTON_SOUND_ALREADY_PLAYED 0x80
#define BUTTON_SOUND_ALL_EVENTS 0xff
#define GUI_SND_CLK_ON BUTTON_SOUND_CLICKED_ON
#define GUI_SND_CLK_OFF BUTTON_SOUND_CLICKED_OFF
#define GUI_SND_MOV_ON BUTTON_SOUND_MOVED_ONTO
#define GUI_SND_MOV_OFF BUTTON_SOUND_MOVED_OFF_OF
#define GUI_SND_DCLK BUTTON_SOUND_DISABLED_CLICK
#define GUI_SND_DMOV BUTTON_SOUND_DISABLED_MOVED_ONTO
#endif

void SpecifyButtonSoundScheme(INT32 button, INT8 scheme);
void PlayButtonSound(INT32 button, INT32 event);

struct NativeButtonSoundSamplePair {
    UINT32 clickedOn = 0xffffffffu;
    UINT32 clickedOff = 0xffffffffu;
};
struct NativeButtonSoundServices {
    // Borrowed original guiCurrentScreen; must outlive the binding. A pointer
    // avoids calling arbitrary code during button construction/scheme selection.
    const UINT32* currentScreen = nullptr;
    UINT32 (*playSample)(UINT32, UINT32, UINT32, UINT32, UINT32) = nullptr;
    UINT32 (*playFile)(const CHAR8*, UINT32, UINT32, UINT32, UINT32) = nullptr;
    // Consecutive original schemes VERYSMALLSWITCH1..COMPUTERSWITCH1, minus2.
    // Engine integration supplies the original Sound Control.h enum constants;
    // do not invent sound IDs or filenames for the sample table.
    NativeButtonSoundSamplePair samples[8]{};
};
enum class NativeButtonSoundResult {
    Silent, InvalidButton, InvalidEvent, Unbound, MissingSample,
    Reentrant, PlaybackFailed, Submitted
};
// Main-thread only. Copies the table/function pointers. Pass {} to unbind before
// the borrowed screen goes away. Callbacks may remove buttons/rebind services;
// recursive PlayButtonSound is bounded and no button is read after a callback.
// Callbacks follow the existing native UI contract: they must not throw.
void NativeBindButtonSoundServices(const NativeButtonSoundServices& services);
NativeButtonSoundResult NativeButtonSoundLastResult();
UINT32 NativeButtonSoundLastPlaybackID();
#endif
