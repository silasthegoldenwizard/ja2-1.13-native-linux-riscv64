> P710 native notice preparation: tests/native-core/RENDER_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document authored surface/font scene and expected rendering.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/RENDER_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Native rendering slice

All assets are authored synthetic data. No proprietary files are read or copied.
Wire framing alone reuses `generate_image_fixtures.collection`; ETRLE streams
and coordinate-grid oracles are literal, independently specified here/in tests.
Engine API/behavior reference: supplied v1.13 sources at
`3394695a387a14c78e92b158cfdcf50e4068c96b`; retain the engine source license.
No external renderer implementation or asset is incorporated.

Translation: `[65,66,32]`. Palette: 0 black, 1 red, 2 green, 3 blue.
RGB565 constants: `0000 f800 07e0 001f`.

* A: 3x3, offset (1,0): `. 2 . / 2 1 2 / 2 0 2`.
* B: 2x3, offset (0,1): `3 3 / 3 . / 3 1`.
* Space: 2x3, offset (0,0), transparent throughout.
* Alternate font maps A to B's descriptor and stream for explicit-font tests.
* Bad descriptor has offset ffffffff; bad run skips four in a three-pixel row.

`rendering.production_interfaces` mounts the actual VFS, owns manager cleanup,
counts executed assertions and compares entire padded allocations. No test
provider replaces FileMan, resources, objects, palettes or rendering kernels.
The optional SDL viewer accepts fixture-root and output-BMP arguments. It owns
one 32x20 frame and one 8x4 tile; it presents only an explicitly locked CPU view.
It exports the complete 72-byte-pitch allocation and scale-eight SDL window BMP.
Run the fixture script with the BMP path and `--verify` for independent host
checks of every raw pixel, every padding row and every displayed BMP pixel.
The verifier compares BMP channels at RGB565 precision to allow SDL expansion
rounding. Normal and sanitizer CTest registration share the same test source.
Window creation/display is an attended host check, not a dummy-video CTest.
