# P710 native notice preparation: tests/native-core/verify_video_demo.py
# New candidate test/build/documentation/metadata path.
# Candidate packaging/verification addition recorded for the 2026-11-12 publication candidate; engine native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add standalone independent literal-pixel verification of authored video frames.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

"""Independent literal pixel expectations for the authored two-frame native demo."""
from pathlib import Path
import hashlib
import json
import struct
import sys

W, H, SCALE, PITCH = 32, 20, 8, 72
grid = [[0x2104] * W for _ in range(H)]
for y in range(1, 19):
    for x in range(1, 31):
        grid[y][x] = 0x4208
for y in range(3, 7):
    for x in range(2, 10):
        grid[y][x] = 0x001F
for y in (4, 5):
    for x in range(3, 9):
        grid[y][x] = 0xF800
for x, y in [(14, 3), (13, 4), (15, 4), (13, 5), (15, 5), (16, 4), (17, 4), (16, 5), (16, 6)]:
    grid[y][x] = 0xFFFF
for x, y in [(14, 4), (17, 6)]:
    grid[y][x] = 0x07E0
for x, y in [(20, 11), (21, 11), (20, 12), (20, 13)]:
    grid[y][x] = 0x001F
for x, y in [(24, 10), (23, 11), (25, 11), (23, 12), (25, 12)]:
    grid[y][x] = 0x07E0
for x, y in [(21, 13), (24, 11)]:
    grid[y][x] = 0xF800
grid[12][24] = 0
first = b"".join(struct.pack("<32H", *row) + b"\xa5" * 8 for row in grid)
assert hashlib.sha256(first).hexdigest() == "1d3e681de0c0ae9e19554fe1a954e1559fe5f41bac8b99c5ddf4198cf122cf46"
second = bytearray(first)
for y in range(3, 7):
    for x in range(2, 10):
        struct.pack_into("<H", second, y * PITCH + x * 2, 0xFFE0)
second = bytes(second)
assert hashlib.sha256(second).hexdigest() == "307b8eaa72dbeac862dfb040c4b3efeae00ac5b7218fb676255a29b9c81bd3b7"


def rgb(value):
    red, green, blue = (value >> 11) & 31, (value >> 5) & 63, value & 31
    return ((red << 3) | (red >> 2), (green << 2) | (green >> 4),
            (blue << 3) | (blue >> 2))


def verify(path, expected):
    raw = path.with_suffix(".rgb565").read_bytes()
    assert raw == expected, "whole padded source mismatch: " + str(path)
    bmp = path.read_bytes()
    assert bmp[:2] == b"BM" and len(bmp) >= 54
    offset = struct.unpack_from("<I", bmp, 10)[0]
    size, width, height, planes, bpp, compression = struct.unpack_from("<IiiHHI", bmp, 14)
    assert size >= 40 and width == W * SCALE and abs(height) == H * SCALE
    assert planes == 1 and bpp in (24, 32) and compression == 0
    stride = ((width * bpp + 31) // 32) * 4
    assert offset >= 54 and offset + stride * abs(height) <= len(bmp)
    for y in range(abs(height)):
        by = abs(height) - 1 - y if height > 0 else y
        for x in range(width):
            pos = offset + by * stride + x * (bpp // 8)
            got = tuple(bmp[pos:pos + 3][::-1])
            pixel = struct.unpack_from("<H", expected, (y // SCALE) * PITCH + (x // SCALE) * 2)[0]
            assert got == rgb(pixel), (str(path), x, y, got, rgb(pixel))
    return {"frame": str(path), "raw_bytes_equal": len(raw),
            "window_pixels_equal": width * abs(height),
            "raw_sha256": hashlib.sha256(raw).hexdigest(),
            "bmp_sha256": hashlib.sha256(bmp).hexdigest()}


assert len(sys.argv) >= 2, "usage: verify_video_demo.py output-base [second-output-base]"
results = []
for base in sys.argv[1:]:
    for name, expected in (("first", first), ("second", second)):
        results.append(verify(Path(base + "-" + name + ".bmp"), expected))
for index in range(2, len(results)):
    assert results[index]["bmp_sha256"] == results[index % 2]["bmp_sha256"]
print(json.dumps({"status": "PASS", "changed_source_pixels": 32,
                  "all_corresponding_bmps_byte_equal": True, "frames": results}, indent=2))
