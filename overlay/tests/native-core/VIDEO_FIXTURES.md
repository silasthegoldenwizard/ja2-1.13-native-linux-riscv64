> P710 native notice preparation: tests/native-core/VIDEO_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document reserved-framebuffer scene, dirty update and reproduction.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/VIDEO_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Native reserved-framebuffer acceptance

This component uses the existing authored `generate_render_fixtures.py` output.
It specifically uses `rendering/font.sti`, mounted through the existing VFS provider.
The copied font translation is A, B, space.
No proprietary inputs are necessary.
There is no new fixture generator or renderer-derived first-frame oracle.

The viewer takes exactly `fixture-root output-base`.
It writes these files:

* `output-base-first.bmp` and `output-base-first.rgb565`
* `output-base-second.bmp` and `output-base-second.rgb565`

Each raw file contains the complete padded PRIMARY_SURFACE allocation: 32x20 RGB565, 72-byte pitch, 1440 bytes.
Each row has eight unchanged 0xa5 padding bytes.
The BMP comes from the actual 256x160 SDL3 X11 window surface.

Frame one uses the retained literal scene.
The background is 0x2104.
The half-open panel [1,1,31,19) is 0x4208.
An 8x4 blue 0x001f tile has a red 0xf800 inset [1,1,7,3) and sits at (2,3).
White mono AB is at (12,3), with a green 0x07e0 shadow and background zero.
Colored BA is at (20,10).

All scene drawing targets FRAME_BUFFER, directly or by blit.
A full refresh transfers FRAME_BUFFER -> BACKBUFFER -> PRIMARY_SURFACE.

Frame two fills only [2,3,10,7) in FRAME_BUFFER with yellow 0xffe0.
It invalidates only that rectangle.
Exactly 32 source pixels change.
The viewer is not the oracle.

Compare the first raw/BMP against the sealed existing literal scene oracle.
Compare the second against that same oracle, with only these coordinates replaced.
Do a check of every pixel and padding byte.
Do a check of all nearest-neighbor scaled BMP pixels.

The example commands use the already approved SDL3 prefix.
Run them from the source root:

```sh
cmake -S tests/native-core -B build/native-video-manager-20261112/debug -DCMAKE_BUILD_TYPE=Debug -DP710_SDL_PRESENTATION=ON -DCMAKE_PREFIX_PATH="$SDL3_PREFIX"
cmake --build build/native-video-manager-20261112/debug -j2
ctest --test-dir build/native-video-manager-20261112/debug --output-on-failure
SDL_VIDEODRIVER=x11 build/native-video-manager-20261112/debug/p710_native_video_viewer build/native-video-manager-20261112/debug/artifacts/rendering build/native-video-manager-20261112/debug/artifacts/manual-video
cmake -S tests/native-core -B build/native-video-manager-20261112/sanitized -DCMAKE_BUILD_TYPE=Debug -DP710_SDL_PRESENTATION=ON -DP710_SANITIZERS=ON -DCMAKE_PREFIX_PATH="$SDL3_PREFIX"
cmake --build build/native-video-manager-20261112/sanitized -j2
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build/native-video-manager-20261112/sanitized --output-on-failure
ASAN_OPTIONS=detect_leaks=1 SDL_VIDEODRIVER=x11 build/native-video-manager-20261112/sanitized/p710_native_video_viewer build/native-video-manager-20261112/sanitized/artifacts/rendering build/native-video-manager-20261112/sanitized/artifacts/manual-video
```

The host must provide an accessible real X11 DISPLAY.
Tests explicitly require the x11 driver.
They accept no dummy/offscreen fallback.
Each viewer frame stays visible briefly and polls quit events.

Only the existing presentation owner balances SDL lifetime.
Neither new driver calls global SDL_Quit.
