<!--
P710 native snapshot update: tests/native-core/BUTTON_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Native quick-button fixtures and acceptance boundary

Native change: 2026-11-12. All assets here are authored synthetic data; no
proprietary game asset, game installation, or full-engine startup is involved.
The manager preserves the bounded original 1dot13 SGP contract at
`3394695a387a14c78e92b158cfdcf50e4068c96b`; this is not a full button-engine port.

## Fixture wire format

`generate_button_fixtures.py OUTPUT` reuses `generate_image_fixtures.collection`
and literal ETRLE streams, without importing any production decoder/renderer.
It writes exactly `buttons.sti`, `font.sti`, and `bad_buttons.sti`. The latter is
a deliberately truncated 63-byte STCI header and must fail resource loading.
The output root and ancestors/leaves must not be symlinks.

`buttons.sti` has five indexed ETRLE subimages, each 8 by 6, X/Y offset zero,
with the collection helper's 64-byte header, 768-byte palette, five 16-byte
little-endian descriptors, concatenated image streams, and eight-byte APP data.
Each image is 60 bytes: six literal 10-byte rows. Descriptor data offsets are
0,60,120,180,240; lengths are all60; height6/width8. Canvas width/height are0.
The helper's palette is deliberately patched at these indices:

| Index | RGB | RGB565 |
| --- | --- | --- |
| 0 | 0,0,0 (transparent skip, never an opaque pixel here) | 0000 |
| 1 | 255,0,0 | F800 |
| 2 | 0,255,0 | 07E0 |
| 3 | 0,0,255 | 001F |
| 4 | 255,255,0 | FFE0 |
| 5 | 128,128,128 | 8410 |
| 6 | 255,255,255 | FFFF |

The other249palette entries are zero. The literal top row is
`81 07 06 06 06 06 06 06 06 00`: one transparent pixel followed by seven white
pixels. Each of four middle rows is `08 06 C C C C C C 06 00`, with white edges
and six state-color pixels. The bottom row is
`08 06 06 06 06 06 06 06 06 00`. Frame ordering and C values:

| Frame | Meaning | C |
| --- | --- | --- |
| 0 | OffNormal | 3 (blue) |
| 1 | OffHilite | 2 (green) |
| 2 | OnNormal | 1 (red) |
| 3 | OnHilite | 4 (yellow) |
| 4 | Grayed | 5 (gray) |

`font.sti` reuses `generate_render_fixtures.GLYPHS` and its **unchanged** helper
palette. It maps A/B/space through the viewer's translation table. Thus its
index2is green and index3is blue; its own palette index4is white. Font data is
separate from the patched button palette. No fixture contains negative offsets;
existing native-render tests cover signed glyph offsets, whereas the UI scene
here exercises the positive font offsets and the button's transparent corner.

## Independent six-frame scene specification

The viewer CLI is `native_mouse_button_viewer FIXTURE_ROOT OUTPUT_BASE`.
It writes `OUTPUT_BASE-{idle,hover,pressed,released,cancelled,disabled}.rgb565`
and matching `.bmp` files. Output directories are created only below the supplied
base's parent. Each raw file is exactly1440bytes: twenty rows of72bytes, with
32little-endian RGB565 pixels per row and eight unchanged A5 padding bytes.
BMPs capture the actual256by160owned window (scale8); raw is primary-buffer
readback. BMP channel expansion may round within a 5/6-bit quantization bucket:
compare `(R>>3,G>>2,B>>3)` with the raw source bits, not exact8-bit expansion.
Account for BMP row direction, actual pixel offset, depth and row stride.

Begin every frame with all32by20pixels set to2104. Place the8by6button at(2,3),
using the literal mask above. The corner(2,3) remains2104; white covers(3..9,3),
(2..9,8), and(2,4..7)/(9,4..7). The state color fills(3..8,4..7).
Draw the existing monochrome bitmap caption AB at the requested baseline(16,5),
foregroundFFFF, shadow07E0, transparent background. Its independent grids are:

- A placed at(17,5): `[skip,FFFF,skip]`, `[FFFF,07E0,FFFF]`,
  `[FFFF,skip,FFFF]`.
- B placed at(20,6): `[FFFF,FFFF]`, `[FFFF,skip]`, `[FFFF,07E0]`.

Finally write exactly one marker pixel at(26,3), initiallyF800, then07E0after
the sole accepted inside-release activation. Everything else remains2104.
No expected scene grid is used by the production renderer.

| Export | Injected logical position/action | State color | Enabled / clicked | Presses / activations | Marker |
| --- | --- | --- | --- | --- | --- |
| idle | move(20,15) | 001F | yes / no | 0 / 0 | F800 |
| hover | move(4,5) | 07E0 | yes / no | 0 / 0 | F800 |
| pressed | left DOWN(4,5) | FFE0 | yes / yes | 1 / 0 | F800 |
| released | left UP(4,5) | 07E0 | yes / no | 1 / 1 | 07E0 |
| cancelled | DOWN inside, move(20,15), UP outside | 001F | yes / no | 2 / 1 | 07E0 |
| disabled | focus cancel recovery, disable, inside DOWN/UP | 8410 | no / no | 3 / 1 | 07E0 |

Before the disabled export, one additional inside press is followed by focus
loss. It must restore the prepress off state and leave activations1. There is
no seventh exported frame. Disabled clicks do not increment presses because
this viewer does not enable disabled callbacks. `BUTTON_NO_TOGGLE` and the
original-named `DEFAULT_MOVE_CALLBACK` control the push button.

MSYS geometry is **inclusive**. This original quick-button convention defines
its region from(2,3)through(10,9), inclusive, from image width8and height6;
the actual drawn pixels occupy half-open[2,10)x[3,9). Region edge behavior is
separately tested. Drawing rectangles and clip bounds remain half-open.

## Manager contract coverage and lifetime policy

`native_mouse_button_tests FIXTURE_ROOT` uses the same tiny resource files and
writes no artifacts. It checks priority/ties/inclusive borders, old-coordinate
GAIN versus updated MOVE, disabled raw passthrough/help occlusion, unchanged
position filtering, priority-change-without-reorder, integer userdata, removal,
move geometry, clicked-ID gate versus continuing hover, shared L/R release,
and exact event-type rejection. Button checks cover default anchor cancel and
reentry, stray UP, disabled occlusion/high-bit callback reasons, hide/show,
position/range/null handling, unsupported generic flags, null-click fallback,
IGNORE_CLICKS, custom callback ownership, NEWTOGGLE lost-state restoration,
and checkbox release-before-callback ordering.

Click, movement and disabled callbacks remove themselves and another button,
create a replacement, and attempt nested dispatch. Removal unlinks immediately;
physical free/slot reuse waits for the outer event. Any region-list mutation
aborts the current event and schedules later refresh; this bounded rule avoids
stale callbacks/address reuse. Nested mouse dispatch is rejected. Callbacks must
not throw. Region owners must remove regions before their storage dies.

Image tests reject malformed loads/state indices, reject unloading an image
with live buttons, check borrowed-object survival through unload/UI shutdown,
validate unchanged destination after rejected selection/draw, and inspect real
blit transparency/color/padding. Borrowed objects remain caller-owned and must
stay alive and unmodified until image unload or UI shutdown. Loaded image
objects are owned and deleted after buttons are removed and image unloaded.
These tests do not intentionally use dangling/forged pointers or invalid enums.

Owned real X11-window tests inject SDL events through the accepted event pump
and original FIFO. Batch DOWN+UP must work despite final polling state already
being released. Focus loss/video destruction restore press state without a
release activation. DOWN queued before close is suppressed even when drained
after close. Video reopen needs explicit input shutdown/reinitialization; UI
shutdown/reinit clears registrations and slots. Input reset generation is
process-local, monotonic (saturating at uint64 maximum), not a saved-game field.
MSYS IDs allocate without collisions within their actual16-bit domain; the
original overflow/infinite-search defect is intentionally corrected.

## Execution and remaining boundaries

Use the existing configured native build (no new dependency):

```sh
python3 -B tests/native-core/generate_button_fixtures.py BUILD/ui-fixtures
SDL_VIDEODRIVER=x11 BUILD/native_mouse_button_tests BUILD/ui-fixtures
SDL_VIDEODRIVER=x11 BUILD/native_mouse_button_viewer BUILD/ui-fixtures BUILD/ui-scene
ctest --test-dir BUILD -R '^ui\.' --output-on-failure -j1
```

Paths above are relative to the isolated native-core source/build chosen by the
owner. CTest entries are `ui.generate_fixtures`, `ui.mouse_button_contracts`, and
`ui.button_scene`; the generator is a fixture setup dependency for both others.
Run the existing full normal/sanitized suites sequentially as well. Actual run
results and independent full-pixel/BMP review are reported separately; writing
these tests/documents does not establish their success or human acceptance.

This proves a synthetic queue-to-button-to-image presentation path when run.
It does not prove physical-human/default-XInput2 input, original game assets,
full menu/game startup, Windows/RISC-V behavior, real pointer grab/capture,
thread safety, generic/text/icon button layout, fast help rendering, sounds,
cursors, dirty-background restoration, double clicks or wheel/extra-button
synthesis. Logical grab and original macros are compatibility APIs, not desktop
pointer control. No global filter/watch, external pointer warp or new event
pump is added.
