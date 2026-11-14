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

Native change: 2026-11-12. All assets here are newly made synthetic data.
These tests use no proprietary game assets or game installation. They do not start the full engine.
The manager preserves a limited part of the original 1dot13 SGP contract at
`3394695a387a14c78e92b158cfdcf50e4068c96b`. This is not a port of the full button engine.

## Fixture wire format

`generate_button_fixtures.py OUTPUT` reuses `generate_image_fixtures.collection`
and literal ETRLE streams. It does not import a production decoder or renderer.
It writes exactly `buttons.sti`, `font.sti`, and `bad_buttons.sti`.
The last file has a deliberately truncated 63-byte STCI header. Resource loading must fail for this file.
The output root, its ancestors, and its output files must not be symlinks.

`buttons.sti` has five indexed ETRLE subimages. Each subimage is 8 by 6, with X/Y offset zero.
The collection helper supplies a 64-byte header, 768-byte palette, and five 16-byte little-endian descriptors.
Concatenated image streams and eight-byte APP data follow the descriptors.
Each image is 60 bytes: six literal 10-byte rows.

Descriptor data offsets are 0,60,120,180,240. All lengths are 60, heights are 6, and widths are 8.
Canvas width and height are 0. The helper's palette has deliberate changes at these indices:

| Index | RGB | RGB565 |
| --- | --- | --- |
| 0 | 0,0,0 (transparent skip, never an opaque pixel here) | 0000 |
| 1 | 255,0,0 | F800 |
| 2 | 0,255,0 | 07E0 |
| 3 | 0,0,255 | 001F |
| 4 | 255,255,0 | FFE0 |
| 5 | 128,128,128 | 8410 |
| 6 | 255,255,255 | FFFF |

The other 249 palette entries are zero. The literal top row is
`81 07 06 06 06 06 06 06 06 00`.
It has one transparent pixel, then seven white pixels.
Each of four middle rows is `08 06 C C C C C C 06 00`.
Each middle row has white edges and six pixels of the state color.
The bottom row is `08 06 06 06 06 06 06 06 06 00`.

The frame order and C values are:

| Frame | Meaning | C |
| --- | --- | --- |
| 0 | OffNormal | 3 (blue) |
| 1 | OffHilite | 2 (green) |
| 2 | OnNormal | 1 (red) |
| 3 | OnHilite | 4 (yellow) |
| 4 | Grayed | 5 (gray) |

`font.sti` reuses `generate_render_fixtures.GLYPHS` and its **unchanged** helper palette.
It maps A/B/space through the viewer's translation table.
Thus, index 2 is green, index 3 is blue, and its palette index 4 is white.
Font data is separate from the changed button palette.

No fixture contains negative offsets. Existing native-render tests cover signed glyph offsets.
This UI scene uses positive font offsets and the button's transparent corner.

## Independent six-frame scene specification

The viewer CLI is `native_mouse_button_viewer FIXTURE_ROOT OUTPUT_BASE`.
It writes `OUTPUT_BASE-{idle,hover,pressed,released,cancelled,disabled}.rgb565`
and matching `.bmp` files.
It creates output directories only below the supplied base's parent.
Each raw file is exactly 1440 bytes: twenty rows of 72 bytes.
Each row has 32 little-endian RGB565 pixels and eight unchanged A5 padding bytes.

BMPs capture the actual owned 256 by 160 window at scale 8.
Raw files contain primary-buffer readback. BMP channel expansion can round within a 5/6-bit quantization bucket.
Compare `(R>>3,G>>2,B>>3)` with the raw source bits. Do not compare exact 8-bit expansion.
Account for BMP row direction, actual pixel offset, depth, and row stride.

Begin each frame with all 32 by 20 pixels set to 2104.
Place the 8 by 6 button at (2,3).
Use the literal mask above. The corner (2,3) stays 2104.
White covers (3..9,3), (2..9,8), and (2,4..7)/(9,4..7).
The state color fills (3..8,4..7).

Draw the existing monochrome bitmap caption AB at the requested baseline (16,5).
Use foreground FFFF, shadow 07E0, and a transparent background.
The independent grids are:

- A at (17,5): `[skip,FFFF,skip]`, `[FFFF,07E0,FFFF]`, `[FFFF,skip,FFFF]`.
- B at (20,6): `[FFFF,FFFF]`, `[FFFF,skip]`, `[FFFF,07E0]`.

Write exactly one marker pixel at (26,3) last.
Start with F800. Change it to 07E0 after the only accepted inside-release activation.
All other pixels stay 2104. The production renderer uses no expected scene grid.

| Export | Injected logical position/action | State color | Enabled / clicked | Presses / activations | Marker |
| --- | --- | --- | --- | --- | --- |
| idle | move(20,15) | 001F | yes / no | 0 / 0 | F800 |
| hover | move(4,5) | 07E0 | yes / no | 0 / 0 | F800 |
| pressed | left DOWN(4,5) | FFE0 | yes / yes | 1 / 0 | F800 |
| released | left UP(4,5) | 07E0 | yes / no | 1 / 1 | 07E0 |
| cancelled | DOWN inside, move(20,15), UP outside | 001F | yes / no | 2 / 1 | 07E0 |
| disabled | focus cancel recovery, disable, inside DOWN/UP | 8410 | no / no | 3 / 1 | 07E0 |

Before the disabled export, one more inside press occurs, then focus is lost.
Focus loss must restore the off state from before the press. Activations must stay at 1.
There is no seventh exported frame.
Disabled clicks do not increase presses because this viewer does not enable disabled callbacks.
`BUTTON_NO_TOGGLE` and the original-named `DEFAULT_MOVE_CALLBACK` control the push button.

MSYS geometry is **inclusive**.
The original quick-button convention defines the region from (2,3) through (10,9), inclusive.
It uses image width 8 and height 6. Actual drawn pixels occupy half-open [2,10)x[3,9).
Separate tests cover region edge behavior. Drawing rectangles and clip bounds stay half-open.

## Manager contract coverage and lifetime policy

`native_mouse_button_tests FIXTURE_ROOT` uses the same small resource files. It writes no artifacts.
Mouse-region tests cover:

- Priority, ties, and inclusive borders.
- GAIN with old coordinates compared with MOVE with updated coordinates.
- Disabled raw passthrough and help occlusion.
- Filtering when position does not change.
- Priority changes without a change of order.
- Integer userdata, removal, and movement geometry.
- The clicked-ID gate compared with continued hover.
- Shared L/R release and rejection of incorrect event types.

Button tests cover:

- Default anchor cancellation and reentry.
- Stray UP, disabled occlusion, and high-bit callback reasons.
- Hide/show, position/range/null handling, and unsupported generic flags.
- Null-click fallback, IGNORE_CLICKS, and custom callback ownership.
- NEWTOGGLE restoration of lost state.
- Checkbox release before the callback.

Click, movement, and disabled callbacks remove themselves and another button.
They create a replacement and try nested dispatch. Removal unlinks immediately.
Physical free and slot reuse wait until the outer event ends.
Any region-list change stops the current event and schedules a later refresh.
This limited rule prevents stale callbacks and address reuse.

The manager rejects nested mouse dispatch. Callbacks must not throw.
Region owners must remove regions before their storage ends.

Image tests reject malformed loads and state indices. They reject image unload while buttons are live.
They make sure borrowed objects survive unload and UI shutdown.
They make sure the destination stays unchanged after rejected selection or drawing.
They inspect actual blit transparency, color, and padding.

Borrowed objects stay caller-owned. Keep them alive and unchanged until image unload or UI shutdown.
Loaded image objects are owned. The manager deletes them after button removal and image unload.
These tests do not deliberately use dangling pointers, forged pointers, or invalid enums.

Actual owned X11-window tests inject SDL events through the accepted event pump and original FIFO.
Batch DOWN+UP must work when the final polling state is already released.
Focus loss and video destruction restore press state without a release activation.
The manager suppresses DOWN queued before close, even when it drains DOWN after close.

Video reopen requires explicit input shutdown and reinitialization.
UI shutdown and reinitialization clear registrations and slots.
Input reset generation is process-local and monotonic. It saturates at the uint64 maximum.
It is not a saved-game field.

MSYS IDs allocate without collisions in their actual 16-bit domain.
The native manager deliberately corrects the original overflow and infinite-search defect.

## Execution and remaining boundaries

Use the existing configured native build. No new dependency is necessary.

```sh
python3 -B tests/native-core/generate_button_fixtures.py BUILD/ui-fixtures
SDL_VIDEODRIVER=x11 BUILD/native_mouse_button_tests BUILD/ui-fixtures
SDL_VIDEODRIVER=x11 BUILD/native_mouse_button_viewer BUILD/ui-fixtures BUILD/ui-scene
ctest --test-dir BUILD -R '^ui\.' --output-on-failure -j1
```

The paths above are relative to the isolated native-core source/build that the owner selects.
CTest entries are `ui.generate_fixtures`, `ui.mouse_button_contracts`, and `ui.button_scene`.
The generator is a fixture setup dependency for both other entries.
Run the existing full normal suite sequentially with the sanitized suite.

Actual run results and independent full-pixel/BMP review are reported separately.
These tests and documents alone do not establish success or human acceptance.
When run, this tests a synthetic path from queue through button and image to presentation.

This milestone does not prove these functions or environments:

- Physical-human/default-XInput2 input, original game assets, or full menu/game startup.
- Windows/RISC-V behavior, actual pointer grab/capture, or thread safety.
- Generic/text/icon button layout, fast help rendering, sounds, or cursors.
- Dirty-background restoration, double clicks, or wheel/extra-button synthesis.

Logical grab and original macros are compatibility APIs. They do not control the desktop pointer.
The change adds no global filter/watch, external pointer warp, or new event pump.
