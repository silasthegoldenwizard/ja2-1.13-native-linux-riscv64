<!--
P710 native snapshot update: tests/native-core/INPUT_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Native input milestone

The input tests require the existing pinned SDL3 presentation configuration and
an actual X11 display. They create their own existing native-video window; they
do not initialize another video subsystem reference or load game assets/fonts.
Both new executables and the input object/table participate in the existing
warning and optional ASan/UBSan instrumentation lists. LSan uses the existing
host sanitizer environment. All previous targets and CTest registrations remain.

## Automated fixtures

`input.manager_contracts` exercises the original public queue and polling APIs.
Events enter with `SDL_PushEvent`, then traverse the production presentation
pump and optional sink, including when quit polling happens before dequeue.
This is SDL-queue integration evidence, not physical keyboard acceptance.
The check helper counts assertions and reports the count on success/failure.
Only test startup traffic is drained before manager initialization. Production
initialization never flushes SDL events, installs filters/watches, or hooks input.

`input.authored_scene` invokes `p710_native_input_viewer output-base --scripted`.
No fixture generator is necessary. The viewer draws through FRAME_BUFFER and
the existing video manager, and saves actual primary bytes and window surfaces:

* `output-base-pressed.rgb565` and `output-base-released.rgb565`: 1440 bytes,
  32x20 RGB565, 72-byte pitch, including actual eight-byte row padding.
* Matching `.bmp` files: actual 256x160 X11 window surfaces, nearest scaling 8.
* Background: 0x2104. Half-open left panel [2,2,10,10), right [20,2,28,10).
* Pressed: left 0xf800, right 0x07e0. Released: both 0x001f.
* Both captures contain a one-pixel 0xffff marker at logical (13,15).

Expected pixels must be constructed independently from these literal rules,
not from the production drawing routine. Preserve/check actual row padding
separately. Brief display delays do not determine any scripted assertion.

## Directed host acceptance

`p710_native_input_viewer output-base --listen` injects no SDL events. It prints
and flushes `READY SDL_WINDOW_ID=... X11_WINDOW=...`, consuming real routed
events for at most eight seconds. A host test may address only that X11 window,
without moving/grabbing the desktop pointer. Send A and left-button presses
with motion to window (104,120), release both, then close that owned window.
Success requires pressed/released captures, cleared held state, and observed
quit. Consumed atoms and final state are printed. A directed X11 send remains
synthetic host integration evidence; record human physical input separately.

## Deliberate compatibility boundaries

Ctrl/Alt retain the historical normal translation bank; Shift selects bank 256.
Specific/peek are head-only; a specific-mask miss copies but does not remove the
head. DequeueAllKeyBoardEvents drains all event kinds without clearing holds.
Text/IME, global hooks, capture/restriction, button timers/double-click synthesis,
full engine startup/gameplay, international layouts, and RISC-V remain outside
this milestone. No full Win32 input-thread/hook protocol is emulated.
