> P710 native notice preparation: tests/native-core/RESOURCE_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document synthetic PCX/TGA/resource chooser cases and limits.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

# Production image/resource checks

`python3 tests/native-core/generate_resource_fixtures.py OUTPUT_ROOT` writes independently authored synthetic bytes only. It imports the previous authored STCI fixture helper to preserve its explicit wire offsets/literal streams; it never reads proprietary resources or asks a production decoder/encoder to produce expected pixels.

`p710_native_resource_tests OUTPUT_ROOT` mounts the exact output through actual bfVFS/FileManVFS. It calls production CreateImage/LoadImageData, PCX's retained public decode/blit API, TGA and GetETRLEImageData. Expected pixels and descriptors are literal C++ values. Fixtures include padded indexed PCX rows, escaped/run bytes, clipped transparency, four TGA origins, original raw 5:5:5 16-bit samples, RGB24 swizzling, strict/fallback PNG chooser behavior, real STCI ETRLE deep-copy lifetime, malformed wire sections and repeated/incremental loading for all eight content masks. A present broken PNG must fail; fallback means absent replacement, not decode fallback.

Generator: Python standard library only; shared `generate_image_fixtures.py` must remain adjacent. Test links production ImageResource/PCX/impTGA/PngLoader, existing STCI/ImageData/ImagePalette, actual FileManIO/FileManVFS/bfVFS/Copy7z and the bundled libpng/zlib/Expat dependencies. No fake readers, global replacements, shadow headers or game assets.

PCX supported scope is 8-bit single-plane RLE with a standard 0x0c palette marker, row padding included in BytesPerLine. Malformed runs, truncated header/data/palette, invalid stride and trailing compressed data are rejected. The historical PCX exporter treated padding/marker bytes as pixels and did unbounded reads; these are corrected. TGA scope remains original implemented type 2, 16/24-bit true color; indexed/RLE and 32-bit modes remain unsupported. Row origins are interpreted from the descriptor; 24-bit BGR wire values become original RGB memory bytes; 16-bit 5:5:5 samples remain unchanged. The importer does not add a new format family.
