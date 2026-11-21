#!/usr/bin/env python3
# P710 native snapshot update: tests/native-core/check_native_crash_symbolizer.py
# Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-21.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

from __future__ import annotations

import pathlib
import re
import shutil
import signal
import subprocess
import sys
import tempfile


def elf_build_id(binary: pathlib.Path) -> str:
    notes = subprocess.run(
        ["readelf", "--notes", str(binary)], check=True, text=True, capture_output=True)
    match = re.search(r"Build ID:\s*([0-9a-fA-F]+)", notes.stdout)
    if not match:
        raise RuntimeError("test binary has no GNU ELF build ID")
    return match.group(1).lower()


def main_symbol(binary: pathlib.Path) -> int:
    symbols = subprocess.run(
        ["nm", "-an", "--defined-only", str(binary)],
        check=True, text=True, capture_output=True)
    match = re.search(r"^([0-9a-fA-F]+)\s+[Tt]\s+main$", symbols.stdout, re.MULTILINE)
    if not match:
        raise RuntimeError("test binary has no visible main symbol")
    return int(match.group(1), 16)


def write_synthetic_report(report: pathlib.Path, binary: pathlib.Path) -> None:
    build_id = elf_build_id(binary)
    address = main_symbol(binary)
    report.write_text(
        "*** P710 NATIVE CRASH REPORT ***\n"
        f"elf_build_id {build_id}\n"
        f"main_image {binary.name}\n"
        f"instruction_pointer {address:#x}\n"
        "frames_raw\n"
        f"  [0] {address:#x}\n"
        "frames_symbols\n"
        "modules_base_end_name\n"
        f"0x0 0xffffffffffffffff {binary.name}\n",
        encoding="utf-8")


def main() -> int:
    script = pathlib.Path(sys.argv[1]).resolve(strict=True)
    test_binary = pathlib.Path(sys.argv[2]).resolve(strict=True)
    fault_fixture = pathlib.Path(sys.argv[3]).resolve(strict=True)
    sanitized = sys.argv[4] == "1"
    with tempfile.TemporaryDirectory(prefix="p710-crash-symbolizer-") as directory:
        report = pathlib.Path(directory) / "CrashReports" / "crash_report_001.txt"
        if sanitized:
            binary = test_binary
            report.parent.mkdir()
            write_synthetic_report(report, binary)
        else:
            binary = fault_fixture
            crashed = subprocess.run(
                [str(fault_fixture), directory], check=False,
                text=True, capture_output=True)
            if crashed.returncode != -signal.SIGSEGV:
                raise RuntimeError(
                    f"fault fixture did not preserve SIGSEGV: {crashed.returncode}")
            if not report.is_file():
                raise RuntimeError("fault fixture did not create a crash report")
        result = subprocess.run(
            [sys.executable, str(script), str(report), "--binary", str(binary)],
            check=True, text=True, capture_output=True)
        expected_source = ("native_crash_report_tests.cpp" if sanitized
                           else "native_crash_signal_fixture.cpp")
        if ("Instruction pointer:" not in result.stdout or
                "Resolved game frames:" not in result.stdout or
                expected_source not in result.stdout):
            raise RuntimeError(f"unexpected symbolizer output: {result.stdout!r}")
        renamed_binary = pathlib.Path(directory) / "renamed-exact-binary"
        shutil.copy2(binary, renamed_binary)
        renamed = subprocess.run(
            [sys.executable, str(script), str(report), "--binary", str(renamed_binary)],
            check=True, text=True, capture_output=True)
        if expected_source not in renamed.stdout:
            raise RuntimeError("symbolizer rejected an exact binary copy with a new name")
        build_id = elf_build_id(binary)
        mismatch = report.read_text(encoding="utf-8").replace(build_id, "00", 1)
        report.write_text(mismatch, encoding="utf-8")
        rejected = subprocess.run(
            [sys.executable, str(script), str(report), "--binary", str(binary)],
            check=False, text=True, capture_output=True)
        if rejected.returncode != 2 or "ELF build ID mismatch" not in rejected.stderr:
            raise RuntimeError("symbolizer accepted a report from a different executable")
    print("native_crash_symbolizer: parser and addr2line invocation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
