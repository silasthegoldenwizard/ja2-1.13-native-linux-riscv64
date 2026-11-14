> P710 native notice preparation: tests/native-core/PNG_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document independent PNG/JPC fixture bytes and expected decoding.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/PNG_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Independent PNG and JPC fixture oracle

`generate_png_fixtures.py` makes PNG signature/IHDR/PLTE/oFFs/IDAT/IEND chunks with Python's standard `struct` and `zlib`.
It does not call the production decoder or encoder. It does not derive expectations from loader output.
Existing `7z` writes Copy-method archives (`-mx=0 -ms=off`).
These archives match the retained bfVFS library contract. All fixtures are synthetic and disposable.

Run `python3 tests/native-core/generate_png_fixtures.py <artifact-dir> --seven-zip /usr/bin/7z`.
Then run the parent CMake target `p710_native_png_tests <artifact-dir>`.
Alternatively, run the focused agent probe `build/png-resource-agent/png_tests build/png-resource-agent/fixtures` after fixture generation.

The literal oracles in `png_loader_tests.cpp` define these expected results:

- Indexed 4x2 rows are `[0,1,1,0]`, `[2,0,3,3]`.
  Palette entries 0–3 are black, red, green, and blue. Entries 4–255 are zero.
  Native ETRLE bytes are `81 02 01 01 81 00 01 02 81 02 03 03 00`.
  Index 0 stays the original transparent index.
- RGB24 3x1 pixels red, green, and blue become configured native RGB565 words `f800 07e0 001f`.
  RGBA32 2x1 keeps byte sequence `0c 22 38 4e 5a 7b d2 ff`.
  Tests independently examine descriptor dimensions/count, byte size, and offsets.
- A single transparent pixel encodes as `81 00`.
  A 130-pixel row has 128 opaque index 1 values, then two transparent pixels.
  It encodes as 127 opaque values, one opaque value, and `82 00`.
  This tests the 127-byte run boundary.
- JPC archive creation order 10/0/2 differs from numeric frame order 0/2/10.
  The additional 1x1 frames use indices 2 and 3. Exact concatenated bytes thus prove ordering.
  The multi-frame canvas keeps the historical 640x480 dimensions.
- XML 0 overrides offsets to (-7,8).
  It emits wall 9, tile count 2, location 4660, current frame 1, frame count 2, and flags 0x23.
  The 16 app bytes are `09 02 34 12 00 00 00 01 02 23 00 00 00 00 00 00`.
  Unmentioned app records stay zero.
  An x-only offset override on frame 2 resets y to 0. This preserves original parser behavior.

Tests also use a single RGBA member plus XML.
They examine every combination of requested palette/bitmap/app flags, incremental replacement, and repeated release.
They use metadata-only, individual-section, and full-section requests for each malformed case.
They test each request with a fresh load and after a valid load.
Failure must preserve every prior HIMAGE byte and still-valid pixel storage.

The fixtures include seven valid PNGs and four valid JPCs.
They also include eight malformed PNGs with these defects:

- Signature, truncation, and small palette.
- Offset range, CRC, and dimension overflow.
- Unsupported depth and unsupported grayscale.

The 15 malformed JPCs cover these defects:

- Negative, past-end, and duplicate XML frame indices.
- Integer overflow and junk.
- Excess animation frame sum and offset range.
- DTD/entity input and broken XML.
- Invalid and duplicate numeric filenames.
- Multi-frame RGB, no frames, malformed PNG member, and invalid archive.

A micrometer-unit oFFs fixture preserves the old loader's direct numeric offset behavior.
A valid trailing-data PNG adds a fake oFFs chunk and arbitrary bytes after IEND.
Offsets must stay those of the parsed PNG.
The tests read no proprietary resource or game installation.
