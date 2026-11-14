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

The input tests require the existing pinned SDL3 presentation configuration and an actual X11 display.
They create their own window through the existing native-video code.
They do not initialize another video subsystem reference or load game assets or fonts.
Both new executables and the input object/table use the existing warning lists and optional ASan/UBSan instrumentation lists.
LSan uses the existing host sanitizer environment. All previous targets and CTest registrations stay available.

## Automated fixtures

`input.manager_contracts` tests the original public queue and polling APIs.
Events enter with `SDL_PushEvent`. They go through the production presentation pump and optional sink.
This also applies when quit polling occurs before dequeue.
This supplies SDL-queue integration evidence. It does not establish acceptance of physical keyboard input.

The check helper counts assertions. It reports the count on success or failure.
The test drains only startup traffic before manager initialization.
Production initialization never flushes SDL events, installs filters/watches, or hooks input.

`input.authored_scene` runs `p710_native_input_viewer output-base --scripted`.
No fixture generator is necessary.
The viewer draws through FRAME_BUFFER and the existing video manager.
It saves actual primary bytes and window surfaces:

* `output-base-pressed.rgb565` and `output-base-released.rgb565`: 1440 bytes, 32x20 RGB565, 72-byte pitch, including actual eight-byte row padding.
* Matching `.bmp` files: actual 256x160 X11 window surfaces, nearest scaling 8.
* Background: 0x2104. Half-open left panel [2,2,10,10), right [20,2,28,10).
* Pressed: left 0xf800, right 0x07e0. Released: both 0x001f.
* Both captures contain a one-pixel 0xffff marker at logical (13,15).

Construct expected pixels independently from these literal rules.
Do not use the production drawing routine to construct them.
Keep actual row padding unchanged.
Do a separate check of that padding.
Brief display delays do not determine scripted assertions.

## Directed host acceptance

`p710_native_input_viewer output-base --listen` injects no SDL events.
It prints and flushes `READY SDL_WINDOW_ID=... X11_WINDOW=...`.
It consumes actual routed events for a maximum of eight seconds.
A host test can address only that X11 window.
Do not move or grab the desktop pointer.

Send A and left-button presses with motion to window (104,120).
Release both.
Close that owned window.
Success requires pressed/released captures, cleared held state, and an observed quit.
The viewer prints consumed atoms and final state.

A directed X11 send supplies synthetic host integration evidence.
Record human physical input separately.

## Deliberate compatibility boundaries

Ctrl/Alt keep the historical normal translation bank. Shift selects bank 256.
Specific/peek operate only on the head.
A specific-mask miss copies the head without removal.
DequeueAllKeyBoardEvents drains all event kinds without clearing holds.

This milestone does not cover these functions or environments:

- Text/IME and global hooks.
- Capture/restriction and button timers/double-click synthesis.
- Full engine startup/gameplay, international layouts, and RISC-V.

The implementation does not emulate the full Win32 input-thread/hook protocol.
