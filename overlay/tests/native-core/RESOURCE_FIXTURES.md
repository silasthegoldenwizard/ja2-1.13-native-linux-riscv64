> P710 native notice preparation: tests/native-core/RESOURCE_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document synthetic PCX/TGA/resource chooser cases and limits.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/RESOURCE_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Production image/resource checks

`python3 tests/native-core/generate_resource_fixtures.py OUTPUT_ROOT` writes only independently authored synthetic bytes.
It imports the previous authored STCI fixture helper to keep its explicit wire offsets and literal streams.
It never reads proprietary resources.
It never uses a production decoder/encoder to produce expected pixels.

`p710_native_resource_tests OUTPUT_ROOT` mounts the exact output through actual bfVFS/FileManVFS.
It calls production CreateImage/LoadImageData, PCX's retained public decode/blit API, TGA and GetETRLEImageData.
Expected pixels and descriptors are literal C++ values.

Fixtures include padded indexed PCX rows, escaped/run bytes, clipped transparency, four TGA origins and original raw 5:5:5 16-bit samples.
They also include RGB24 swizzling, strict/fallback PNG chooser behavior, actual STCI ETRLE deep-copy lifetime and malformed wire sections.
Repeated/incremental loading covers all eight content masks.
A present broken PNG must fail.
Fallback applies to an absent replacement.
It does not permit decode fallback.

The generator uses only the Python standard library.
The shared `generate_image_fixtures.py` must stay adjacent to it.
The test links production ImageResource/PCX/impTGA/PngLoader and existing STCI/ImageData/ImagePalette.
It also links actual FileManIO/FileManVFS/bfVFS/Copy7z and the bundled libpng/zlib/Expat dependencies.
It uses no fake readers, global replacements, shadow headers or game assets.

PCX supports 8-bit single-plane RLE with a standard 0x0c palette marker.
BytesPerLine includes row padding.
The loader rejects malformed runs, truncated header/data/palette, invalid stride and trailing compressed data.
The historical PCX exporter treated padding/marker bytes as pixels and made unbounded reads.
These defects are corrected.

TGA support stays at the original implemented type 2, 16/24-bit true color.
Indexed/RLE and 32-bit modes stay unsupported.
The importer interprets row origins from the descriptor.
It converts 24-bit BGR wire values to original RGB memory bytes.
It keeps 16-bit 5:5:5 samples unchanged.
The importer adds no new format family.
