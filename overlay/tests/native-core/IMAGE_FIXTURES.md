> P710 native notice preparation: tests/native-core/IMAGE_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document synthetic STCI/ETRLE fixture layout and literal pixel expectations.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

# Synthetic STCI image fixtures

These fixtures contain only newly authored tiny pixels and byte streams. They do
not read or redistribute game assets. The acceptance result is a real
`FileMan -> LoadSTCIFileToImage -> HVOBJECT -> guarded RGB565 surface` test with
literal expected pixels, including clipping and transparency, without deriving
its oracle from the production loader or blitter.

Run with the existing Python standard library:

```sh
python3 tests/native-core/generate_image_fixtures.py /exact/chosen/output/directory
```

The argument is the actual output directory, with no implicit child directory.
The generator writes only the fixed named fixture/sidecar/manifest leaves there;
it does not clear the directory or delete unrelated leaves. Existing symlink
paths are refused. Running it again replaces its own named outputs. It does not
install anything, execute a production decoder, or invoke upstream scripts.

## Build versus reuse and source provenance

Reuse: Python `struct`, `zlib`, `json`, and `hashlib`; existing FileMan and native
C++ tests consume the fixtures. A small dedicated writer is necessary because
an engine serializer or existing proprietary image would couple the oracle to
the implementation or require game data. There is no general image framework.

Wire facts were checked against original source commit
`3394695a387a14c78e92b158cfdcf50e4068c96b`, using `git show`, independently of the
new native loader and scalar kernels:

- `sgp/imgfmt.h`: STCI flags, 64-byte header limit, RGB mask/depth fields,
  256x3 RGB palette convention, 16-byte subimage record, and 7-bit run limit.
- Original `sgp/STCI.cpp`: section order, palette count 256, subimage records
  before pixels for ETRLE, stored pixel-section size, and app section after
  pixels. Original code reads an encoded section without deriving its length
  from the sum of row widths.
- Original `sgp/vobject_blitters.cpp`,
  `Blt8BPPDataTo16BPPBufferTransparent` at lines 10212–10350: signed high-bit
  dispatch, skip length `control & 0x7f`, zero-control EOL, stop after object
  height rows, and application of signed object offsets. This establishes that
  `0x80` is a transparent zero-length no-op and that bytes after the final EOL
  in a declared subimage region need not be interpreted as additional rows.

No production struct layout, native decoder output, or native palette converter
was used to serialize or derive the oracle. Pixel grids and RGB565 values are
literal independent inputs. Exact black is `0000`; nonzero near-black `(1,1,1)`
quantizes to the explicitly required black substitute `0001`.

## Explicit little-endian wire layout

All scalar integers are little endian, including signed two's-complement object
offsets. Every header is exactly 64 bytes; host packing is irrelevant.

| Header offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | ASCII `STCI` |
| 4 | 4 | original pixel bytes |
| 8 | 4 | stored pixel bytes |
| 12 | 4 | transparent value, zero |
| 16 | 4 | flags: RGB=4, indexed=8, zlib=16, ETRLE=32 |
| 20 | 2 | global height |
| 22 | 2 | global width |
| 24 | 20 | indexed or RGB union |
| 44 | 1 | stored pixel depth |
| 45 | 4 | application-data byte count (deliberately unaligned) |
| 49 | 15 | reserved zeros |

Indexed union: palette count at 24 (4 bytes), subimage count at 28 (2 bytes),
RGB component depth bytes `(8,8,8)` at 30–32, then zeros. RGB union: four 32-bit
masks at 24,28,32,36; four one-byte component depths at 40–43. RGB16 uses masks
`f800/07e0/001f/0000` and depths `5/6/5/0`. RGB24 and RGB32 use masks
`00ff0000/0000ff00/000000ff/00000000`, depths `8/8/8/0`, packed BGR/BGRX bytes.

An indexed file has a 768-byte palette immediately after its header. An ETRLE
file then has one 16-byte record per object:

| Record offset | Bytes | Field |
|---:|---:|---|
| 0 | 4 | pixel-data offset relative to start of pixel section |
| 4 | 4 | encoded object byte count |
| 8 | 2 | signed x offset |
| 10 | 2 | signed y offset |
| 12 | 2 | object height |
| 14 | 2 | object width |

Plain indexed and RGB files have no subimage table. The app section follows the
stored pixel section; every valid fixture has exactly 8 app bytes:
`41 50 50 00 4d 45 54 41` (`APP\0META`). For zlib files the stored size is the
compressed byte count; original size is the uncompressed byte count. The
compressed bytes are retained in the generated `.sti`; no sidecar substitutes
for their representation.

## Palette and independent pixel oracle

Palette entries 7–255 are fixed black. The authored entries are:

| Index | RGB | Expected RGB565 |
|---:|---|---|
| 0 | 0,0,0 | `0000` |
| 1 | 255,0,0 | `f800` |
| 2 | 0,255,0 | `07e0` |
| 3 | 0,0,255 | `001f` |
| 4 | 255,255,255 | `ffff` |
| 5 | 1,1,1 | `0001` (black substitute) |
| 6 | 255,255,0 | `ffe0` |

A dot below is transparent. Literal palette index zero is **opaque black**,
including when it occurs inside an ETRLE literal run. Tests must use the mask
or explicit grids to distinguish the two. Color value zero cannot substitute
for transparency in an ETRLE test.

`sprite.sti` is a collection whose global canvas is **0x0**, with two objects.
Do not reject this collection because its global dimensions are zero. Frame 0
is 7x5 with offset **(-1,+2)**:

```text
. 1 1 2 . 0 3
4 . . 2 3 4 .
. . . . . . .
0 1 2 3 4 5 6
1 1 1 . . 2 2
```

Frame 1 is 3x2 with offset **(+2,-1)**:

```text
1 . 2
0 3 4
```

Frame 0's stream starts with the bounded no-op `80`. Its five row streams,
including EOL `00`, are literal constants:

```text
80 81 03 01 01 02 81 02 00 03 00
01 04 82 03 02 03 04 81 00
87 00
07 00 01 02 03 04 05 06 00
03 01 01 01 82 02 02 02 00
```

Frame 1's row streams are:

```text
01 01 81 01 02 00
03 00 03 04 00
```

The sidecars `sprite_frame0.rgb565le`, `.mask`, `.indices` and corresponding
`sprite_frame1` leaves are row-major, without headers. RGB565 has two bytes per
pixel in little-endian order; transparent positions contain zero. Mask has one
byte per pixel: 0=transparent, 1=opaque. Indices has one byte per pixel, with
255 used solely as the transparent oracle sentinel. No authored opaque pixel
uses index 255. Frame offsets are in the manifest/text and STCI records, not in
the raw sidecars.

The same sidecar convention covers `indexed8`, `rgb`, and `long_runs_frame0`.
`image_oracle.txt` contains the complete grids and every load expectation;
`image_manifest.json` adds fixed dimensions/offsets, literal integer RGB565
values, exact stream bytes, file sizes and SHA-256 hashes. C++ tests may hardcode
the documented small grids and filenames; no JSON dependency is necessary.

## Valid fixture names and expectations

All the following should load when requesting palette/bitmap/app as appropriate
(`IMAGE_ALLDATA` for tests that also request application data). RGB palette-only
requests are not supported. The scalar RGB565 sprite path applies to ETRLE;
plain RGB pixel bytes are separately checked by the loader test.

| Filename | Content / pixel expectation |
|---|---|
| `sprite.sti` | 0x0 collection, two frames above, ETRLE no-op and literal index 0 |
| `sprite_canvas.sti` | identical two frames, global dimensions 7x5 |
| `sprite_trailing.sti` | same frames; `a5 5a` inside frame 0 region after final EOL |
| `long_runs.sti` | 130x2 object; row 0 has 127 red then 3 green; row 1 all transparent |
| `indexed8.sti` | 3x2 indices `0 1 2 / 3 4 5`, uncompressed |
| `indexed8_zlib.sti` | identical indices using ordinary zlib-wrapped DEFLATE |
| `rgb16.sti`, `rgb16_zlib.sti` | 3x2 RGB565: black/red/green then blue/white/yellow |
| `rgb24.sti`, `rgb24_zlib.sti` | same six exact colors as packed BGR24 |
| `rgb32.sti`, `rgb32_zlib.sti` | same colors as packed BGRX32, unused bytes zero |

Long-run literal stream: `7f`, 127 bytes of index 1, `03 02 02 02 00`;
transparent row: `ff 83 00`. This exercises maximum 127 literal and skip runs,
followed by a separate three-pixel run. All dimensions and section sizes are
tiny; these fixtures do not perform allocation stress tests.

## Malformed fixtures (expected full-load rejection)

| Exact filename | Reason |
|---|---|
| `bad_short_header.sti` | only 63 header bytes |
| `bad_short_palette.sti` | palette is one byte short |
| `bad_short_table.sti` | two-record table is one byte short |
| `bad_short_pixels.sti` | stored pixel section is one byte short |
| `bad_short_app.sti` | eight-byte app section is one byte short |
| `bad_magic.sti` | signature `NOPE` |
| `bad_flags_none.sti` | no RGB/indexed organization |
| `bad_flags_both.sti` | contradictory RGB/indexed organizations |
| `bad_depth.sti` | indexed file advertises 16-bit pixels |
| `bad_palette_count.sti` | 255 entries instead of 256 |
| `bad_object_zero_width.sti` | first object width zero |
| `bad_object_zero_height.sti` | first object height zero |
| `bad_object_offset.sti` | first object offset beyond pixel section |
| `bad_object_length.sti` | first object length exceeds pixel section |
| `bad_object_count_zero.sti` | ETRLE collection without objects |
| `bad_object_count_extra.sti` | advertises three records but supplies two |
| `bad_run_overflow.sti` | 4-pixel skip in a 3-pixel row |
| `bad_run_underflow.sti` | EOL after only 2 of 3 pixels |
| `bad_literal_overflow.sti` | 4 literal pixels in a 3-pixel row |
| `bad_literal_truncated.sti` | 3-pixel literal with only 2 indices |
| `bad_missing_eol.sti` | complete 3-pixel row lacks EOL |
| `bad_etrle_zlib.sti` | ETRLE and zlib compression flags both set |
| `bad_plain_zero_width.sti` | plain bitmap width zero |
| `bad_plain_size.sti` | uncompressed original size disagrees with dimensions |
| `bad_deflate_header.sti` | invalid zlib wrapper |
| `bad_deflate_checksum.sti` | corrupt Adler32 checksum |
| `bad_deflate_size.sti` | original size disagrees with inflate result |
| `bad_deflate_truncated.sti` | compressed stream missing completion/checksum |

These are desired native validation results for a full load, not a claim that
the old Windows loader validated every malformed record. A partial load can
have different section-validation semantics; test those separately and label
the requested contents. In particular neither `0x80` nor an ETRLE 0x0 global
canvas belongs in the malformed set.
