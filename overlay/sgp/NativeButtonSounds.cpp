/*
 * P710 native snapshot update: sgp/NativeButtonSounds.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Source-faithful sound selection from 1dot13/source at
//3394695a387a14c78e92b158cfdcf50e4068c96b, sgp/Button Sound Control.cpp.
// Engine sample IDs/services are supplied at integration, rather than linking
// engine gameplay headers or a counterfeit implementation into the foundation.
#include "native_button_sounds.h"
#include "native_buttons.h"
#include "../Ja2/screenids.h"

namespace {
NativeButtonSoundServices services;
NativeButtonSoundResult lastResult = NativeButtonSoundResult::Silent;
UINT32 lastPlayback = 0xffffffffu;
bool dispatching = false;
GUI_BUTTON* live(INT32 id) {
    GUI_BUTTON* b = GetButtonPtr(id);
    return b && !(b->uiFlags & BUTTON_DELETION_PENDING) ? b : nullptr;
}
UINT8 genericScheme(UINT32 screen) {
    switch (screen) {
    case MAINMENU_SCREEN: case OPTIONS_SCREEN: case LOADSAVE_SCREEN:
    case SAVE_LOAD_SCREEN: case INIT_SCREEN:
        return BUTTON_SOUND_SCHEME_BIGSWITCH3;
    case LAPTOP_SCREEN:
        return BUTTON_SOUND_SCHEME_COMPUTERBEEP2;
    case AUTORESOLVE_SCREEN: case MAP_SCREEN: case GAME_SCREEN: case SHOPKEEPER_SCREEN:
        return BUTTON_SOUND_SCHEME_SMALLSWITCH2;
    case GAME_INIT_OPTIONS_SCREEN:
        return BUTTON_SOUND_SCHEME_VERYSMALLSWITCH2;
    default:
        // Original fallback assigns NONE to a local variable only: the stored
        // field remains GENERIC. Preserve that visible field and silent result.
        return BUTTON_SOUND_SCHEME_GENERIC;
    }
}
struct DispatchScope {
    DispatchScope() { dispatching = true; }
    ~DispatchScope() { dispatching = false; }
};
}

void NativeBindButtonSoundServices(const NativeButtonSoundServices& value) {
    services = value;
}
NativeButtonSoundResult NativeButtonSoundLastResult() { return lastResult; }
UINT32 NativeButtonSoundLastPlaybackID() { return lastPlayback; }

void SpecifyButtonSoundScheme(INT32 id, INT8 scheme) {
    GUI_BUTTON* b = live(id);
    if (!b) return;
    if (scheme < BUTTON_SOUND_SCHEME_NONE || scheme > BUTTON_SOUND_SCHEME_COMPUTERSWITCH1) return;
    b->ubSoundSchemeID = static_cast<UINT8>(scheme);
    if (scheme == BUTTON_SOUND_SCHEME_GENERIC && services.currentScreen)
        b->ubSoundSchemeID = genericScheme(*services.currentScreen);
}

void PlayButtonSound(INT32 id, INT32 event) {
    lastPlayback = 0xffffffffu;
    if (dispatching) { lastResult = NativeButtonSoundResult::Reentrant; return; }
    const GUI_BUTTON* b = live(id);
    if (!b) { lastResult = NativeButtonSoundResult::InvalidButton; return; }
    const UINT8 scheme = b->ubSoundSchemeID;
    if (scheme == BUTTON_SOUND_SCHEME_NONE || scheme == BUTTON_SOUND_SCHEME_GENERIC) {
        lastResult = NativeButtonSoundResult::Silent;
        return;
    }
    if (scheme < BUTTON_SOUND_SCHEME_VERYSMALLSWITCH1 || scheme > BUTTON_SOUND_SCHEME_COMPUTERSWITCH1) {
        lastResult = NativeButtonSoundResult::Silent;
        return;
    }
    if (event != BUTTON_SOUND_CLICKED_ON && event != BUTTON_SOUND_CLICKED_OFF &&
        event != BUTTON_SOUND_DISABLED_CLICK) {
        // Original switch has no hover sounds and accepts exact event values,
        // not ORed masks. Keep those events silent.
        lastResult = event == BUTTON_SOUND_NONE || event == BUTTON_SOUND_MOVED_ONTO ||
            event == BUTTON_SOUND_MOVED_OFF_OF || event == BUTTON_SOUND_DISABLED_MOVED_ONTO ||
            event == BUTTON_SOUND_DISABLED_MOVED_OFF_OF || event == BUTTON_SOUND_ALREADY_PLAYED ?
            NativeButtonSoundResult::Silent : NativeButtonSoundResult::InvalidEvent;
        return;
    }
    // Snapshot before any user/engine service can rebind or destroy a button.
    const NativeButtonSoundServices bound = services;
    DispatchScope scope;
    if (event == BUTTON_SOUND_DISABLED_CLICK) {
        if (!bound.playFile) { lastResult = NativeButtonSoundResult::Unbound; return; }
        lastPlayback = bound.playFile("Sounds\\Disabled Button.wav", 0xffffffffu, 15, 1, 128);
    } else {
        if (!bound.playSample) { lastResult = NativeButtonSoundResult::Unbound; return; }
        const auto& pair = bound.samples[scheme - BUTTON_SOUND_SCHEME_VERYSMALLSWITCH1];
        const UINT32 sample = event == BUTTON_SOUND_CLICKED_ON ? pair.clickedOn : pair.clickedOff;
        if (sample == 0xffffffffu) { lastResult = NativeButtonSoundResult::MissingSample; return; }
        // RATE_11025 is the original0xffffffff native/default-rate sentinel,
        // not a literal11025 Hz override. Volume/loop/pan are exact originals.
        lastPlayback = bound.playSample(sample, 0xffffffffu, 15, 1, 128);
    }
    lastResult = lastPlayback == 0xffffffffu ? NativeButtonSoundResult::PlaybackFailed :
        NativeButtonSoundResult::Submitted;
}
