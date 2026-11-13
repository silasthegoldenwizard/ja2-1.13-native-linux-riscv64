> P710 native notice preparation: tests/native-core/PNG_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document independent PNG/JPC fixture bytes and expected decoding.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

# Independent PNG and JPC fixture oracle

`generate_png_fixtures.py` authors PNG signature/IHDR/PLTE/oFFs/IDAT/IEND chunks using Python's standard `struct` and `zlib`; it does not call the production decoder/encoder or derive expectations from loader output. Existing `7z` writes Copy-method archives (`-mx=0 -ms=off`), matching the retained bfVFS library contract. All fixtures are synthetic and disposable.

Run `python3 tests/native-core/generate_png_fixtures.py <artifact-dir> --seven-zip /usr/bin/7z`, then the parent CMake target `p710_native_png_tests <artifact-dir>` (or the focused agent probe `build/png-resource-agent/png_tests build/png-resource-agent/fixtures`).

Literal oracles in `png_loader_tests.cpp`:

- Indexed 4x2 rows: `[0,1,1,0]`, `[2,0,3,3]`. Palette entries0–3: black, red, green, blue; entries4–255 are zero. Native ETRLE bytes are `81 02 01 01 81 00 01 02 81 02 03 03 00`. Index0 remains the original transparent index.
- RGB24 3x1 pixels red, green, blue become configured native RGB565 words `f800 07e0 001f`. RGBA32 2x1 remains byte sequence `0c 22 38 4e 5a 7b d2 ff`. Descriptor dimensions/count, byte size and offsets are checked independently.
- A single transparent pixel encodes as `81 00`. A130-pixel row with128 opaque index1 values followed by two transparent pixels encodes as127 opaque values, one opaque value, `82 00`, testing the127-byte run boundary.
- JPC archive creation order10/0/2 differs from numeric frame order0/2/10. The additional1x1 frame indices are2 and3, so exact concatenated bytes prove ordering. Multi-frame canvas remains the historical640x480.
- XML0 overrides offsets to(-7,8), emits wall9/tile count2/location4660/current frame1/frame count2/flags0x23. The16 app bytes are `09 02 34 12 00 00 00 01 02 23 00 00 00 00 00 00`. Unmentioned app records remain zero. An x-only offset override on frame2 resets y to0, preserving original parser semantics. A single RGBA member plus XML is also exercised.
- Every combination of requested palette/bitmap/app flags, incremental replacement and repeated release is checked. Each malformed case is tested with metadata-only, individual-section and full-section requests, both fresh and after a prior valid load; failure must preserve every prior HIMAGE byte and still-valid pixel storage.

Seven valid PNGs and four valid JPCs are accompanied by eight malformed PNGs (signature, truncation, small palette, offset range, CRC, dimension overflow, unsupported depth, unsupported grayscale) and15 malformed JPCs (negative/past-end/duplicate XML frame indices, integer overflow/junk, excess animation frame sum, offset range, DTD/entity input, broken XML, invalid/duplicate numeric filenames, multi-frame RGB, no frames, malformed PNG member, invalid archive). A micrometer-unit oFFs fixture preserves the old loader's direct numeric offset behavior. A valid trailing-data PNG appends a fake oFFs chunk and arbitrary bytes after IEND; offsets must remain those of the parsed PNG. No proprietary resource or game installation is read.
