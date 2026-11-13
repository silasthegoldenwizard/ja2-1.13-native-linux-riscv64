#!/usr/bin/env python3
# P710 native notice preparation: tests/native-core/generate_image_fixtures.py
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add authored STCI/ETRLE fixture and literal-pixel oracle generation.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

"""Write tiny synthetic STCI fixtures and independent literal-pixel oracles.

Python standard library only. Never reads game assets or executes a decoder.
Only the named generated leaves in the exact caller-supplied directory change.
See IMAGE_FIXTURES.md for source provenance and the expected-load contract.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib

PIN = "3394695a387a14c78e92b158cfdcf50e4068c96b"
RGB, INDEXED, ZLIB, ETRLE = 4, 8, 16, 32
APP = b"APP\0META"
PALETTE = bytes([0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
                 255, 255, 255, 1, 1, 1, 255, 255, 0]) + bytes(249 * 3)
# Hand-specified RGB565 values, not obtained from a production converter.
COLORS565 = [0x0000, 0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0001, 0xFFE0]
FRAME0 = [[None, 1, 1, 2, None, 0, 3], [4, None, None, 2, 3, 4, None],
          [None] * 7, [0, 1, 2, 3, 4, 5, 6], [1, 1, 1, None, None, 2, 2]]
FRAME1 = [[1, None, 2], [0, 3, 4]]
# Literal control bytes: 0x80 consumes no pixels and is not an end-of-line.
STREAM0 = bytes.fromhex("80 81 03 01 01 02 81 02 00 03 00 "
                        "01 04 82 03 02 03 04 81 00 "
                        "87 00 "
                        "07 00 01 02 03 04 05 06 00 "
                        "03 01 01 01 82 02 02 02 00")
STREAM1 = bytes.fromhex("01 01 81 01 02 00 03 00 03 04 00")


def header(*, flags, width, height, depth, stored, original=None, count=0,
           app=APP, colors=256):
    """Encode explicit wire offsets, independent of any C/C++ ABI or sizeof."""
    h = bytearray(64)
    h[:4] = b"STCI"
    struct.pack_into("<IIIIHH", h, 4, stored if original is None else original,
                     stored, 0, flags, height, width)
    if flags & INDEXED:
        struct.pack_into("<IHBBB", h, 24, colors, count, 8, 8, 8)
    else:
        masks = ((0xF800, 0x07E0, 0x001F, 0, 5, 6, 5, 0) if depth == 16 else
                 (0xFF0000, 0x00FF00, 0x0000FF, 0, 8, 8, 8, 0))
        struct.pack_into("<IIIIBBBB", h, 24, *masks)
    h[44] = depth
    struct.pack_into("<I", h, 45, len(app))
    return bytes(h)


def indexed(pixels, width, height, *, compressed=False, app=APP):
    stored = zlib.compress(pixels, level=9) if compressed else pixels
    return (header(flags=INDEXED | (ZLIB if compressed else 0), width=width,
                   height=height, depth=8, stored=len(stored), original=len(pixels),
                   app=app) + PALETTE + stored + app)


def rgb(pixels, depth, *, compressed=False, app=APP):
    stored = zlib.compress(pixels, level=9) if compressed else pixels
    return (header(flags=RGB | (ZLIB if compressed else 0), width=3, height=2,
                   depth=depth, stored=len(stored), original=len(pixels), app=app)
            + stored + app)


def collection(objects, *, canvas=(0, 0), app=APP):
    table, pixels = bytearray(), bytearray()
    original = 0
    for width, height, x, y, stream in objects:
        table += struct.pack("<IIhhHH", len(pixels), len(stream), x, y, height, width)
        pixels += stream
        original += width * height
    return (header(flags=INDEXED | ETRLE, width=canvas[0], height=canvas[1], depth=8,
                   stored=len(pixels), original=original, count=len(objects), app=app)
            + PALETTE + table + pixels + app)


def patch(data, offset, fmt, *values):
    changed = bytearray(data)
    struct.pack_into(fmt, changed, offset, *values)
    return bytes(changed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    root = args.output_dir.absolute()
    # Refuse path redirection through an existing symlink, including an ancestor.
    if any(p.is_symlink() for p in (root, *root.parents)):
        parser.error("output directory and its ancestors must not be symlinks")
    root.mkdir(parents=True, exist_ok=True)
    manifest = {"schema": 1, "synthetic_only": True, "source_pin": PIN,
                "header_bytes": 64, "app_size_offset": 45, "subimage_bytes": 16,
                "palette_bytes": 768, "app_hex": APP.hex(), "files": {}, "oracles": {}}

    def write(name, data, *, load=None, reason=None, details=None):
        path = root / name
        if path.is_symlink():
            raise ValueError(f"refusing symlink output leaf: {name}")
        path.write_bytes(data)
        record = {"bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}
        if load is not None:
            record["expected_load_all"] = load
        if reason:
            record["reason"] = reason
        if details:
            record.update(details)
        manifest["files"][name] = record

    def oracle(name, grid, offset=(0, 0)):
        flat = [pixel for row in grid for pixel in row]
        values = [0 if p is None else COLORS565[p] for p in flat]
        write(name + ".rgb565le", struct.pack("<" + "H" * len(values), *values))
        write(name + ".mask", bytes(p is not None for p in flat))
        write(name + ".indices", bytes(255 if p is None else p for p in flat))
        manifest["oracles"][name] = {"width": len(grid[0]), "height": len(grid),
                                     "offset_x": offset[0], "offset_y": offset[1],
                                     "indices": grid, "rgb565": values}

    sprite = collection([(7, 5, -1, 2, STREAM0), (3, 2, 2, -1, STREAM1)])
    write("sprite.sti", sprite, load=True, details={"width": 0, "height": 0,
          "depth": 8, "objects": 2, "stored_size": len(STREAM0) + len(STREAM1),
          "original_size": 41, "stream_hex": [STREAM0.hex(), STREAM1.hex()]})
    oracle("sprite_frame0", FRAME0, (-1, 2))
    oracle("sprite_frame1", FRAME1, (2, -1))
    write("sprite_canvas.sti", collection([(7, 5, -1, 2, STREAM0),
          (3, 2, 2, -1, STREAM1)], canvas=(7, 5)), load=True)
    # Original blitters stop at the requested row count, not uiDataLength.
    write("sprite_trailing.sti", collection([(7, 5, -1, 2, STREAM0 + b"\xA5\x5A"),
          (3, 2, 2, -1, STREAM1)]), load=True,
          reason="two benign trailing bytes inside frame 0 data region")
    long_stream = b"\x7f" + bytes([1]) * 127 + b"\x03\x02\x02\x02\x00\xff\x83\x00"
    write("long_runs.sti", collection([(130, 2, 0, 0, long_stream)]), load=True)
    oracle("long_runs_frame0", [[1] * 127 + [2] * 3, [None] * 130])

    plain_indices = bytes([0, 1, 2, 3, 4, 5])
    plain_grid = [[0, 1, 2], [3, 4, 5]]
    write("indexed8.sti", indexed(plain_indices, 3, 2), load=True)
    write("indexed8_zlib.sti", indexed(plain_indices, 3, 2, compressed=True), load=True,
          reason="ordinary zlib-wrapped DEFLATE; compressed bytes retained in file")
    oracle("indexed8", plain_grid)
    rgb565 = bytes.fromhex("00 00 00 f8 e0 07 1f 00 ff ff e0 ff")
    # Packed little-endian RGB masks: byte order B,G,R[,unused].
    rgb24 = bytes.fromhex("00 00 00 00 00 ff 00 ff 00 ff 00 00 ff ff ff 00 ff ff")
    rgb32 = bytes.fromhex("00 00 00 00 00 00 ff 00 00 ff 00 00 "
                         "ff 00 00 00 ff ff ff 00 00 ff ff 00")
    for depth, raw in ((16, rgb565), (24, rgb24), (32, rgb32)):
        write(f"rgb{depth}.sti", rgb(raw, depth), load=True,
              details={"width": 3, "height": 2, "depth": depth, "raw_hex": raw.hex()})
        write(f"rgb{depth}_zlib.sti", rgb(raw, depth, compressed=True), load=True,
              details={"raw_hex": raw.hex()})
    oracle("rgb", [[0, 1, 2], [3, 4, 6]])

    def bad(name, data, reason):
        write("bad_" + name + ".sti", data, load=False, reason=reason)

    plain = indexed(plain_indices, 3, 2)
    bad("short_header", sprite[:63], "63-byte header")
    bad("short_palette", sprite[:64 + 767], "palette is one byte short")
    bad("short_table", sprite[:64 + 768 + 31], "two-object table is one byte short")
    bad("short_pixels", sprite[:64 + 768 + 32 + len(STREAM0) + len(STREAM1) - 1],
        "pixel section is one byte short")
    bad("short_app", sprite[:-1], "declared eight-byte app section is one byte short")
    bad("magic", b"NOPE" + sprite[4:], "wrong signature")
    bad("flags_none", patch(sprite, 16, "<I", 0), "neither indexed nor RGB")
    bad("flags_both", patch(sprite, 16, "<I", RGB | INDEXED | ETRLE), "contradictory organizations")
    bad("depth", patch(sprite, 44, "<B", 16), "indexed pixels must have depth 8")
    bad("palette_count", patch(sprite, 24, "<I", 255), "indexed palette count is not 256")
    bad("object_zero_width", patch(sprite, 64 + 768 + 14, "<H", 0), "zero object width")
    bad("object_zero_height", patch(sprite, 64 + 768 + 12, "<H", 0), "zero object height")
    bad("object_offset", patch(sprite, 64 + 768, "<I", len(STREAM0) + len(STREAM1) + 1),
        "object data offset exceeds pixel section")
    bad("object_length", patch(sprite, 64 + 768 + 4, "<I", len(STREAM0) + len(STREAM1) + 1),
        "object data length exceeds pixel section")
    bad("object_count_zero", patch(sprite, 28, "<H", 0), "ETRLE collection without objects")
    bad("object_count_extra", patch(sprite, 28, "<H", 3), "table count exceeds supplied records")
    for name, stream, reason in (
            ("run_overflow", bytes.fromhex("84 00"), "skip exceeds 3-pixel row"),
            ("run_underflow", bytes.fromhex("82 00"), "EOL before 3-pixel row complete"),
            ("literal_overflow", bytes.fromhex("04 01 02 03 04 00"), "literal exceeds row"),
            ("literal_truncated", bytes.fromhex("03 01 02"), "literal lacks third index"),
            ("missing_eol", bytes.fromhex("03 01 02 03"), "complete row lacks EOL")):
        bad(name, collection([(3, 1, 0, 0, stream)]), reason)
    bad("etrle_zlib", patch(sprite, 16, "<I", INDEXED | ETRLE | ZLIB),
        "ETRLE and whole-image zlib conflict")
    bad("plain_zero_width", patch(plain, 22, "<H", 0), "plain bitmap width is zero")
    bad("plain_size", patch(plain, 4, "<I", 5), "uncompressed declared size mismatches dimensions")
    compressed = indexed(plain_indices, 3, 2, compressed=True)
    bad("deflate_header", compressed[:832] + b"\x00\x00" + compressed[834:],
        "invalid zlib wrapper")
    corrupt = bytearray(compressed)
    corrupt[-len(APP) - 1] ^= 1
    bad("deflate_checksum", bytes(corrupt), "Adler32 mismatch")
    bad("deflate_size", patch(compressed, 4, "<I", 7), "inflated size differs from declared original")
    # A valid wrapper with an incomplete DEFLATE stream, independent of trailer checks.
    short_zip = zlib.compress(plain_indices, level=9)[:-3]
    bad("deflate_truncated", header(flags=INDEXED | ZLIB, width=3, height=2, depth=8,
        stored=len(short_zip), original=6) + PALETTE + short_zip + APP,
        "zlib stream ends before complete checksum")

    lines = ["P710 synthetic image oracle; independent literal pixel grids.",
             "Each '.' is transparent; literal 0 is opaque black.",
             "Palette RGB565: 0=0000 1=f800 2=07e0 3=001f 4=ffff 5=0001 6=ffe0.",
             "RGB565 is row-major little endian; .mask is 0 transparent / 1 opaque.",
             ".indices stores transparent as 255; fixtures never use palette index 255.",
             "App bytes: 41 50 50 00 4d 45 54 41 (APP\\0META).", ""]
    for name, info in manifest["oracles"].items():
        lines.append(f"{name}: {info['width']}x{info['height']} offsets "
                     f"({info['offset_x']},{info['offset_y']})")
        lines.extend(" ".join("." if p is None else str(p) for p in row)
                     for row in info["indices"])
        lines.append("")
    lines.append("Load-all expectations:")
    lines.extend(f"{name}: {'PASS' if info['expected_load_all'] else 'REJECT'}"
                 for name, info in manifest["files"].items() if "expected_load_all" in info)
    write("image_oracle.txt", ("\n".join(lines) + "\n").encode("ascii"))
    path = root / "image_manifest.json"
    if path.is_symlink():
        raise ValueError("refusing symlink manifest output")
    path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote {len(manifest['files'])} synthetic files and image_manifest.json in {root}")


if __name__ == "__main__":
    main()
