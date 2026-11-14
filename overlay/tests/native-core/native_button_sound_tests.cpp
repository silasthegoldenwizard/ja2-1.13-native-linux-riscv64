/*
 * P710 native snapshot update: tests/native-core/native_button_sound_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored sound-routing expectations and PCM; no game assets or hardware
// audibility claim. Recording callbacks prove dispatch; the final test sends
// the dispatch to the actual native sound manager and checks rendered PCM.
#include "native_button_sounds.h"
#include "native_buttons.h"
#include "native_soundman.h"
#include "input.h"
#include "../../Ja2/screenids.h"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
unsigned checks = 0;
void check(bool ok, const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
struct Request { UINT32 sample, rate, volume, loops, pan; std::string file; };
std::vector<Request> requests;
UINT32 screen = MAINMENU_SCREEN;
UINT32 answer = 123;
UINT32 sample(UINT32 id, UINT32 rate, UINT32 volume, UINT32 loops, UINT32 pan) {
    requests.push_back({id, rate, volume, loops, pan, {}});
    return answer;
}
UINT32 file(const CHAR8* name, UINT32 rate, UINT32 volume, UINT32 loops, UINT32 pan) {
    requests.push_back({0, rate, volume, loops, pan, name});
    return answer;
}
NativeButtonSoundServices bindings() {
    NativeButtonSoundServices s;
    s.currentScreen = &screen; s.playSample = sample; s.playFile = file;
    for (UINT32 i = 0; i != 8; ++i) s.samples[i] = {100 + i * 2, 101 + i * 2};
    return s;
}
INT32 button(GUI_CALLBACK movement = nullptr, GUI_CALLBACK click = nullptr) {
    const INT32 id = CreateHotSpot(2, 3, 8, 6, 31, movement, click);
    check(id >= 0, "real native hotspot creates without images or game assets");
    return id;
}
void fresh() {
    ShutdownButtonSystem(); MSYS_Shutdown(); MSYS_Init();
    check(InitButtonSystem(), "real native button manager initializes");
    screen = MAINMENU_SCREEN; answer = 123; requests.clear(); NativeBindButtonSoundServices(bindings());
}
void parameters(const Request& r) {
    check(r.rate == 0xffffffffu && r.volume == 15 && r.loops == 1 && r.pan == 128,
          "original default-rate sentinel, volume15, one loop and centered128 pan");
}
void schemes() {
    fresh(); const INT32 id = button();
    check(GetButtonPtr(id)->ubSoundSchemeID == BUTTON_SOUND_SCHEME_BIGSWITCH3,
          "creation applies original generic screen default");
    const std::array<UINT32, 11> screens{{MAINMENU_SCREEN, OPTIONS_SCREEN, LOADSAVE_SCREEN,
        SAVE_LOAD_SCREEN, INIT_SCREEN, LAPTOP_SCREEN, AUTORESOLVE_SCREEN, MAP_SCREEN,
        GAME_SCREEN, SHOPKEEPER_SCREEN, GAME_INIT_OPTIONS_SCREEN}};
    const std::array<UINT8, 11> expected{{7,7,7,7,7,8,5,5,5,5,3}};
    for (std::size_t i = 0; i != screens.size(); ++i) {
        screen = screens[i]; SpecifyButtonSoundScheme(id, BUTTON_SOUND_SCHEME_GENERIC);
        check(GetButtonPtr(id)->ubSoundSchemeID == expected[i], "exact original screen-to-generic mapping");
    }
    for (UINT32 unknown : {static_cast<UINT32>(FEATURES_SCREEN), static_cast<UINT32>(MSG_BOX_SCREEN), 0xffffffffu}) {
        screen = unknown; SpecifyButtonSoundScheme(id, BUTTON_SOUND_SCHEME_GENERIC);
        check(GetButtonPtr(id)->ubSoundSchemeID == BUTTON_SOUND_SCHEME_GENERIC,
              "unhandled screen preserves original stored GENERIC fallback quirk");
        PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
        check(requests.empty() && NativeButtonSoundLastResult() == NativeButtonSoundResult::Silent,
              "original unhandled generic screen remains silent");
    }
    screen = MAINMENU_SCREEN; SpecifyButtonSoundScheme(id, BUTTON_SOUND_SCHEME_GENERIC);
    screen = LAPTOP_SCREEN; PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
    check(requests.back().sample == 110, "generic screen is resolved when specified, not every playback");
    SpecifyButtonSoundScheme(id, -1); SpecifyButtonSoundScheme(id, 10);
    check(GetButtonPtr(id)->ubSoundSchemeID == BUTTON_SOUND_SCHEME_BIGSWITCH3, "invalid schemes preserve existing selection");
}
void mapping() {
    fresh(); const INT32 id = button();
    for (INT8 scheme = 2; scheme != 10; ++scheme) {
        SpecifyButtonSoundScheme(id, scheme);
        for (INT32 event : {BUTTON_SOUND_CLICKED_ON, BUTTON_SOUND_CLICKED_OFF, BUTTON_SOUND_DISABLED_CLICK}) {
            requests.clear(); PlayButtonSound(id, event);
            check(requests.size() == 1 && NativeButtonSoundLastResult() == NativeButtonSoundResult::Submitted &&
                  NativeButtonSoundLastPlaybackID() == 123, "each original audible scheme/event submits exactly once");
            const auto& r = requests.back(); parameters(r);
            if (event == BUTTON_SOUND_DISABLED_CLICK)
                check(r.file == "Sounds\\Disabled Button.wav", "disabled click uses exact original file path");
            else check(r.file.empty() && r.sample == static_cast<UINT32>(100 + (scheme - 2) * 2 +
                (event == BUTTON_SOUND_CLICKED_OFF ? 1 : 0)), "exact original scheme pair and event direction");
        }
        for (INT32 event : {BUTTON_SOUND_NONE, BUTTON_SOUND_MOVED_ONTO, BUTTON_SOUND_MOVED_OFF_OF,
            BUTTON_SOUND_DISABLED_MOVED_ONTO, BUTTON_SOUND_DISABLED_MOVED_OFF_OF, BUTTON_SOUND_ALREADY_PLAYED,
            BUTTON_SOUND_CLICKED_ON | BUTTON_SOUND_CLICKED_OFF, -1}) {
            requests.clear(); PlayButtonSound(id, event); check(requests.empty(), "hover/combined/invalid events do not play");
        }
    }
    for (INT8 scheme : {static_cast<INT8>(0), static_cast<INT8>(1)}) {
        screen = MSG_BOX_SCREEN; SpecifyButtonSoundScheme(id, scheme); requests.clear();
        PlayButtonSound(id, BUTTON_SOUND_DISABLED_CLICK); check(requests.empty(), "NONE and unresolved GENERIC silence disabled click too");
    }
    SpecifyButtonSoundScheme(id, BUTTON_SOUND_SCHEME_SMALLSWITCH1);
    NativeBindButtonSoundServices({}); PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
    check(NativeButtonSoundLastResult() == NativeButtonSoundResult::Unbound && NativeButtonSoundLastPlaybackID() == NO_SAMPLE,
          "unbound original service reports no submitted playback");
    NativeButtonSoundServices s; s.playSample = sample; NativeBindButtonSoundServices(s);
    PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
    check(NativeButtonSoundLastResult() == NativeButtonSoundResult::MissingSample, "unmapped sample cannot masquerade as sample zero");
    NativeBindButtonSoundServices(bindings()); answer = SOUND_ERROR; PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
    check(NativeButtonSoundLastResult() == NativeButtonSoundResult::PlaybackFailed, "service failure remains distinguishable from submission");
    requests.clear();
    for (INT32 invalid : {-1, MAX_BUTTONS, 0x7fffffff}) {
        SpecifyButtonSoundScheme(invalid, 3); PlayButtonSound(invalid, BUTTON_SOUND_CLICKED_ON);
        check(NativeButtonSoundLastResult() == NativeButtonSoundResult::InvalidButton, "out-of-range IDs are safe");
    }
    RemoveButton(id); PlayButtonSound(id, BUTTON_SOUND_DISABLED_CLICK);
    check(requests.empty() && NativeButtonSoundLastResult() == NativeButtonSoundResult::InvalidButton,
          "removed button cannot route sound");
}
InputAtom atom(UINT16 kind, unsigned x = 4, unsigned y = 5) {
    InputAtom a{}; a.usEvent = kind; a.uiParam = x | (y << 16); return a;
}
void send(UINT16 kind, unsigned x = 4, unsigned y = 5) {
    check(sgp::DispatchNativeMouseEvent(atom(kind, x, y)), "real native input dispatch accepts mouse event");
}
unsigned clicks = 0;
void click(GUI_BUTTON* b, INT32 reason) {
    ++clicks;
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) b->uiFlags |= BUTTON_CLICKED_ON;
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) b->uiFlags &= ~static_cast<UINT32>(BUTTON_CLICKED_ON);
}
void lifecycle() {
    fresh(); clicks = 0; const INT32 id = button(DEFAULT_MOVE_CALLBACK, click);
    send(MOUSE_POS); check(requests.empty(), "actual hover remains silent for original sample schemes");
    send(LEFT_BUTTON_DOWN); send(LEFT_BUTTON_UP);
    check(clicks == 2 && requests.size() == 2 && requests[0].sample == 110 && requests[1].sample == 111,
          "actual press/release routes original ON/OFF after button callbacks");
    requests.clear(); DisableButton(id);
    send(LEFT_BUTTON_DOWN); send(LEFT_BUTTON_UP); send(RIGHT_BUTTON_DOWN); send(RIGHT_BUTTON_UP);
    check(requests.size() == 2 && requests[0].file == "Sounds\\Disabled Button.wav" &&
          requests[1].file == requests[0].file && clicks == 2,
          "disabled left/right DOWN sounds once each while release and blocked callbacks stay silent");
    EnableButton(id); requests.clear(); send(LEFT_BUTTON_DOWN); send(MOUSE_POS, 25, 20); send(LEFT_BUTTON_UP, 25, 20);
    check(requests.size() == 2 && requests[0].sample == 110 && requests[1].sample == 111 &&
          !(GetButtonPtr(id)->uiFlags & BUTTON_CLICKED_ON),
          "anchored leave emits original OFF; outside release adds no further sound");
}
INT32 victim = -1;
unsigned reentries = 0;
UINT32 removing(UINT32, UINT32, UINT32, UINT32, UINT32) {
    ++reentries; PlayButtonSound(victim, BUTTON_SOUND_CLICKED_ON);
    check(NativeButtonSoundLastResult() == NativeButtonSoundResult::Reentrant, "recursive sound callback is bounded");
    RemoveButton(victim); NativeBindButtonSoundServices({}); return 321;
}
void lifetime() {
    fresh(); victim = button(); auto s = bindings(); s.playSample = removing; NativeBindButtonSoundServices(s);
    reentries = 0; PlayButtonSound(victim, BUTTON_SOUND_CLICKED_ON);
    check(reentries == 1 && !GetButtonPtr(victim) && NativeButtonSoundLastPlaybackID() == 321 &&
          NativeButtonSoundLastResult() == NativeButtonSoundResult::Submitted,
          "direct service callback safely removes button and rebinds service snapshot");
    fresh(); victim = button(DEFAULT_MOVE_CALLBACK, click); s = bindings(); s.playSample = removing;
    NativeBindButtonSoundServices(s); reentries = 0; send(LEFT_BUTTON_DOWN); send(LEFT_BUTTON_UP);
    check(reentries == 1 && !GetButtonPtr(victim), "input sound callback removal invalidates remaining input work");
}
void put16(std::vector<unsigned char>& b, unsigned x) { b.push_back(static_cast<unsigned char>(x)); b.push_back(static_cast<unsigned char>(x >> 8)); }
void put32(std::vector<unsigned char>& b, unsigned x) { put16(b, x); put16(b, x >> 16); }
void tag(std::vector<unsigned char>& b, const char* x) { b.insert(b.end(), x, x + 4); }
std::vector<unsigned char> pcm;
UINT32 nativePlayback(UINT32 id, UINT32 rate, UINT32 volume, UINT32 loops, UINT32 pan) {
    sample(id, rate, volume, loops, pan);
    SOUNDPARMS p; std::memset(&p, 0xff, sizeof(p));
    p.uiSpeed = rate; p.uiVolume = volume; p.uiLoop = loops; p.uiPan = pan;
    return SoundPlayFromBuffer("authored-button.wav", pcm.data(), static_cast<UINT32>(pcm.size()), &p);
}
void actualPCM() {
    fresh(); const INT32 id = button();
    tag(pcm, "RIFF"); put32(pcm, 68); tag(pcm, "WAVE"); tag(pcm, "fmt "); put32(pcm, 16);
    put16(pcm, 1); put16(pcm, 2); put32(pcm, 48000); put32(pcm, 192000); put16(pcm, 4); put16(pcm, 16);
    tag(pcm, "data"); put32(pcm, 32);
    for (unsigned i = 0; i != 8; ++i) { put16(pcm, 8192); put16(pcm, 16384); }
    NativeSoundSetManualRendering(TRUE); check(InitializeSoundManager(), "actual paused SDL dummy sound device initializes");
    auto s = bindings(); s.playSample = nativePlayback; NativeBindButtonSoundServices(s);
    PlayButtonSound(id, BUTTON_SOUND_CLICKED_ON);
    check(NativeButtonSoundLastResult() == NativeButtonSoundResult::Submitted && NativeSoundActiveVoices() == 1,
          "routed sample callback enters real NativeSoundMan rather than only recording a request");
    std::array<float, 16> rendered{}; check(NativeSoundRender(rendered.data(), 8), "production mixer renders authored button PCM");
    const float gain = 30.0f / 255.0f;
    for (unsigned i = 0; i != 8; ++i)
        check(std::abs(rendered[i * 2] - 0.25f * gain) < 0.00001f &&
              std::abs(rendered[i * 2 + 1] - 0.5f * gain) < 0.00001f, "actual PCM retains original volume15 and centered pan");
    SoundServiceStreams(); check(NativeSoundActiveVoices() == 0, "one-loop button sample ends and is reclaimed");
    ShutdownSoundManager(); NativeSoundSetManualRendering(FALSE);
}
void cleanup() { NativeBindButtonSoundServices({}); ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownSoundManager(); }
}
int main() {
    try {
        schemes(); mapping(); lifecycle(); lifetime(); actualPCM(); cleanup();
        std::cout << "PASS: " << checks << " native button sound routing/lifecycle/PCM assertions; authored assets, dummy device only\n";
        return 0;
    } catch (const std::exception& e) {
        cleanup(); std::cerr << "FAIL after " << checks << " assertions: " << e.what() << '\n'; return 1;
    }
}
