#!/usr/bin/env python3
"""Generate tiny, synthetic bfVFS test inputs; never read game resources.

SLF disk offsets are literal Windows-layout bytes, independent of C++ sizeof.
See VFS_FIXTURES.md for provenance, expected behavior, and limitations.
"""
# P710 native notice preparation: tests/native-core/generate_vfs_fixtures.py
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add synthetic SLF/7z fixtures with independent archive-byte expectations.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/generate_vfs_fixtures.py
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


import argparse
import binascii
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import struct
import subprocess


HEADER_SIZE = 532
ENTRY_SIZE = 280
PIN = "3394695a387a14c78e92b158cfdcf50e4068c96b"


def fixed_string(value, width=256):
    data = value.encode("utf-8")
    if b"\0" in data or len(data) >= width:
        raise ValueError("fixture string must fit with a NUL terminator")
    return data + bytes(width - len(data))


def header(count, used=None, prefix=""):
    data = bytearray(HEADER_SIZE)
    data[0:256] = fixed_string("Synthetic P710 fixture")
    data[256:512] = fixed_string(prefix)
    struct.pack_into("<iiHHB", data, 512, count, count if used is None else used,
                     0, 0x0200, 1)
    # 525..527 are Windows ABI padding; reserved Int32 is at byte 528.
    struct.pack_into("<i", data, 528, 0)
    return data


def entry(name, offset, length, state=0):
    data = bytearray(ENTRY_SIZE)
    data[0:256] = fixed_string(name)
    struct.pack_into("<IIBB", data, 256, offset, length, state, 0)
    # 266..267 padding, FILETIME low/high at 268/272, UInt16 at 276.
    struct.pack_into("<IIH", data, 268, 0, 0, 0)
    return data


def slf(records, prefix=""):
    payload = bytearray()
    directory = bytearray()
    for name, content, state in records:
        directory += entry(name, HEADER_SIZE + len(payload), len(content), state)
        payload += content
    return bytes(header(len(records), sum(r[2] == 0 for r in records), prefix)
                 + payload + directory)


def seven_uint(value):
    """7z's variable-length UINT64 (not LEB128)."""
    for extra in range(8):
        if value < (1 << (7 + 7 * extra)):
            first = ((0xff << (8 - extra)) & 0xff) if extra else 0
            return bytes([first | (value >> (8 * extra))]) + value.to_bytes(8, "little")[:extra]
    return b"\xff" + value.to_bytes(8, "little")


def solid_copy_seven(payloads):
    """One Copy folder, three substreams, one empty-file sentinel.

    The CLI does not make solid Copy archives, even with -ms=on. Encode this
    tiny missing case directly from bundled 7zFormat.txt, then validate with 7z.
    """
    nonempty = [s.encode("utf-8") for s in payloads.values() if s]
    data = b"".join(nonempty)
    packed = b"\x06\x00\x01\x09" + seven_uint(len(data)) + b"\x00"
    unpack = (b"\x07\x0b\x01\x00\x01\x01\x00\x0c"
              + seven_uint(len(data)) + b"\x00")
    substreams = b"\x08\x0d" + seven_uint(len(nonempty)) + b"\x09"
    substreams += b"".join(seven_uint(len(s)) for s in nonempty[:-1])
    substreams += b"\x0a\x01" + b"".join(struct.pack("<I", binascii.crc32(s))
                                           for s in nonempty) + b"\x00"
    names = b"\x00" + b"".join((name + "\0").encode("utf-16le") for name in payloads)
    # Four files, fourth is an empty stream, and that empty stream is a file.
    files = b"\x05\x04\x0e\x01\x10\x0f\x01\x80\x11" + seven_uint(len(names)) + names + b"\x00"
    next_header = b"\x01\x04" + packed + unpack + substreams + b"\x00" + files + b"\x00"
    start = struct.pack("<QQI", len(data), len(next_header), binascii.crc32(next_header))
    return b"7z\xbc\xaf\x27\x1c\x00\x04" + struct.pack("<I", binascii.crc32(start)) + start + data + next_header


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--seven-zip", default="/usr/bin/7z")
    args = parser.parse_args()
    if args.output_dir.is_symlink():
        parser.error("output directory must not be a symlink")
    root = args.output_dir.absolute()
    root.mkdir(parents=True, exist_ok=True)
    root = root.resolve()
    manifest = {
        "schema": 1, "synthetic_only": True,
        "source_pin": PIN,
        "slf_layout": {"header_bytes": HEADER_SIZE, "entry_bytes": ENTRY_SIZE,
                       "integer_order": "little endian", "filename_encoding": "UTF-8"},
        "files": {}, "archives": {},
        "loose_lookup": {
            "base/Shared.txt": "base shared\n", "mod/Shared.txt": "mod shared\n",
            "base/BaseOnly.txt": "base only\n",
            "writable/WriteTarget.txt": "initial writable\n",
            "base/Loose/MiXeD.txt": "loose mixed payload\n",
            "base/Loose/Grüße.txt": "loose bmp payload\n",
            "base/Loose/🌍.txt": "loose astral payload\n"},
        "profile_expectations": {
            "mod_before_base_Shared.txt": "mod shared\n",
            "base_only_BaseOnly.txt": "base only\n",
            "writable_initial_WriteTarget.txt": "initial writable\n"},
    }

    def target(relative):
        part = PurePosixPath(relative)
        if part.is_absolute() or not part.parts or any(p in ("..", ".") for p in part.parts):
            raise ValueError("unsafe fixture output path")
        path = root
        for segment in part.parts:
            path = path / segment
            if path.is_symlink():
                raise ValueError(f"refusing symlink output: {relative}")
        path.parent.mkdir(parents=True, exist_ok=True)
        return path

    def record(relative):
        data = target(relative).read_bytes()
        manifest["files"][relative] = {
            "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()}

    def write(relative, data):
        path = target(relative)
        path.write_bytes(data)
        record(relative)
        return path

    def archive(relative, data, expected, **details):
        write(relative, data)
        manifest["archives"][relative] = {"expected": expected, **details}

    for relative, content in manifest["loose_lookup"].items():
        write(relative, content.encode("utf-8"))

    valid_records = [
        ("Archive\\Mixed.TXT", b"slf mixed payload\n", 0),
        ("Archive\\Grüße.txt", b"slf unicode payload\n", 0),
        ("Archive\\Deleted.txt", b"deleted bytes must stay invisible\n", 0xff),
        ("Archive\\Old.txt", b"old bytes must stay invisible\n", 1),
        ("Archive\\Absent.txt", b"absent bytes must stay invisible\n", 0xfe),
    ]
    archive("archives/valid.slf", slf(valid_records), "accept",
            visible={"Archive/Mixed.TXT": "slf mixed payload\n",
                     "Archive/Grüße.txt": "slf unicode payload\n"},
            absent=["Archive/Deleted.txt", "Archive/Old.txt", "Archive/Absent.txt"],
            equivalent_queries=["archive/mixed.txt", "ARCHIVE\\MIXED.TXT"])
    archive("archives/prefixed.slf", slf([("Inner.TXT", b"prefixed payload\n", 0)],
                                       "Prefix\\"), "accept",
            visible={"Prefix/Inner.TXT": "prefixed payload\n"})
    archive("archives/empty.slf", bytes(header(0)), "accept", visible={})
    archive("archives/case-collision.slf", slf([
        ("Collision.TXT", b"collision first\n", 0),
        ("collision.txt", b"collision second\n", 0)]), "policy_choice",
        policy_choices={"reject": "initialization fails",
                        "first": "collision first\n"},
        entry_order=["Collision.TXT", "collision.txt"])

    # Each malformed input isolates a fixed bad disk property.
    archive("archives/short-header.slf", bytes(header(0)[:531]), "reject",
            reason="header is one byte short")
    archive("archives/negative-count.slf", bytes(header(-1, 0)), "reject",
            reason="signed entry count is -1")
    archive("archives/overflow-count.slf", bytes(header(0x7fffffff, 0)), "reject",
            reason="entry count INT32_MAX; count*280 exceeds signed 32-bit range")
    archive("archives/impossible-count.slf", bytes(header(2, 0)) + bytes(ENTRY_SIZE),
            "reject", reason="two entries claimed with space for only one")
    archive("archives/truncated-directory.slf", bytes(header(1)) + bytes(279),
            "reject", reason="directory record is one byte short")
    ordinary = bytearray(slf([("Range.txt", b"range\n", 0)]))
    directory_start = HEADER_SIZE + len(b"range\n")
    for name, offset, length, reason in [
        ("range-before-payload", 0, 1, "data points inside library header"),
        ("range-past-eof", 0xffffffff, 2, "data starts past EOF and 32-bit end overflows"),
        ("range-in-directory", directory_start, 1, "data points inside directory"),
        ("range-cross-directory", HEADER_SIZE, 7, "data end overlaps directory by one byte"),
    ]:
        damaged = ordinary.copy()
        struct.pack_into("<II", damaged, directory_start + 256, offset, length)
        archive(f"archives/{name}.slf", bytes(damaged), "reject", reason=reason)
    no_nul = ordinary.copy()
    no_nul[directory_start:directory_start + 256] = b"N" * 256
    archive("archives/no-nul-entry.slf", bytes(no_nul), "reject",
            reason="active filename has no NUL within its 256-byte field")
    for name, start in [("no-nul-library-name", 0), ("no-nul-library-path", 256)]:
        no_nul = ordinary.copy()
        no_nul[start:start + 256] = b"N" * 256
        archive(f"archives/{name}.slf", bytes(no_nul), "reject",
                reason="header string has no NUL within its 256-byte field")

    seven_payloads = {"Seven/Mixed.TXT": "7z mixed payload\n",
                      "Seven/Grüße.txt": "7z bmp payload\n",
                      "Seven/🌍.txt": "7z astral payload\n",
                      "Seven/Empty.txt": ""}
    for relative, content in seven_payloads.items():
        path = write("_7z-input/" + relative, content.encode("utf-8"))
        os.utime(path, (946684800, 946684800))
    for filename, method, solid, expected in [
        ("copy.7z", "Copy", "on", "accept"),
        ("copy-nonsolid.7z", "Copy", "off", "accept"),
        ("compressed.7z", "LZMA", "on", "reject"),
    ]:
        relative = "archives/" + filename
        destination = target(relative)
        if destination.exists():
            destination.unlink()  # This exact generated leaf only; no tree deletion.
        command = [args.seven_zip, "a", "-t7z", "-y", "-bd", "-bb0",
                   "-m0=" + method, "-ms=" + solid, "-mhc=off", "-mtc=off",
                   "-mtm=off", "-mta=off", str(destination), *seven_payloads]
        subprocess.run(command, cwd=root / "_7z-input", check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                       text=True, encoding="utf-8", errors="replace")
        listing = subprocess.run([args.seven_zip, "l", "-slt", str(destination)],
                                 check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                 text=True, encoding="utf-8", errors="replace").stdout
        # Retain CLI evidence, especially whether Copy really shares one folder.
        write(relative + ".listing.txt", listing.encode("utf-8"))
        record(relative)
        manifest["archives"][relative] = {
            "expected": expected, "method_requested": method,
            "solid_requested": solid, "visible": seven_payloads,
            "reason": "uncompressed provider must reject LZMA" if expected == "reject" else "valid Copy archive",
            "command": command[:12] + ["<OUTPUT>/" + relative, *seven_payloads],
            "listing": relative + ".listing.txt"}
    archive("archives/solid-copy.7z", solid_copy_seven(seven_payloads), "accept",
            visible=seven_payloads, method="Copy", folders=1, nonempty_substreams=3,
            empty_file="Seven/Empty.txt", writer="explicit 7z header; validated by installed 7z")
    listing = subprocess.run([args.seven_zip, "l", "-slt", str(target("archives/solid-copy.7z"))],
                             check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             text=True, encoding="utf-8", errors="replace").stdout
    if "Solid = +" not in listing or "Blocks = 1" not in listing:
        raise RuntimeError("solid-copy.7z must have one folder shared by nonempty files")
    write("archives/solid-copy.7z.listing.txt", listing.encode("utf-8"))
    for filename in ("copy.7z", "copy-nonsolid.7z", "compressed.7z", "solid-copy.7z"):
        path = target("archives/" + filename)
        subprocess.run([args.seven_zip, "t", "-bd", str(path)], check=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        for name, expected in seven_payloads.items():
            extracted = subprocess.run([args.seven_zip, "x", "-so", "-bd", str(path), name],
                                       check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            if extracted.stdout != expected.encode("utf-8"):
                raise RuntimeError(f"7z extraction differs from fixed oracle: {filename}: {name}")
    copy_bytes = target("archives/copy.7z").read_bytes()
    archive("archives/truncated.7z", copy_bytes[:31], "reject",
            reason="7z start header truncated to 31 bytes")
    bad_signature = bytearray(copy_bytes)
    bad_signature[0] ^= 0xff
    archive("archives/bad-signature.7z", bytes(bad_signature), "reject",
            reason="7z signature first byte corrupted")
    archive("archives/truncated-body.7z", copy_bytes[:-1], "reject",
            reason="last byte of 7z next header missing")
    target("manifest.json").write_text(json.dumps(manifest, ensure_ascii=False,
                                                  indent=2, sort_keys=True) + "\n",
                                       encoding="utf-8")
    print(f"Generated {len(manifest['files'])} synthetic files and "
          f"{len(manifest['archives'])} archive cases in {root}")


if __name__ == "__main__":
    main()
