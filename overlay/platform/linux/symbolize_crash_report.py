#!/usr/bin/env python3
"""Resolve a local P710 native crash report against its exact unstripped binary."""
# P710 native snapshot update: platform/linux/symbolize_crash_report.py
# Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-21.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from __future__ import annotations

import argparse
import pathlib
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass


@dataclass(frozen=True)
class Module:
    base: int
    end: int
    name: str


def parse_address(value: str) -> int:
    return int(value, 16)


def parse_report(path: pathlib.Path) -> tuple[str | None, str | None, int | None, list[int], list[Module]]:
    build_id: str | None = None
    main_image: str | None = None
    instruction: int | None = None
    frames: list[int] = []
    modules: list[Module] = []
    section = ""
    for raw in path.read_text(encoding="utf-8", errors="replace").splitlines():
        line = raw.strip()
        if line.startswith("elf_build_id "):
            build_id = line.split(maxsplit=1)[1].lower()
            continue
        if line.startswith("main_image "):
            main_image = line.split(maxsplit=1)[1]
            continue
        if line.startswith("instruction_pointer "):
            instruction = parse_address(line.split(maxsplit=1)[1])
            continue
        if line == "frames_raw":
            section = "frames"
            continue
        if line == "frames_symbols":
            section = "symbols"
            continue
        if line == "modules_base_end_name":
            section = "modules"
            continue
        if section == "frames" and line.startswith("["):
            frames.append(parse_address(line.rsplit(maxsplit=1)[1]))
        elif section == "modules" and line:
            parts = line.split(maxsplit=2)
            if len(parts) == 3:
                modules.append(Module(parse_address(parts[0]), parse_address(parts[1]), parts[2]))
    return build_id, main_image, instruction, frames, modules


def owning_module(address: int, modules: list[Module]) -> Module | None:
    return next((module for module in modules if module.base <= address < module.end), None)


def resolve(binary: pathlib.Path, offsets: list[int]) -> list[str]:
    if not offsets:
        return []
    addr2line = shutil.which("addr2line")
    if not addr2line:
        raise RuntimeError("addr2line is required to symbolize a report")
    command = [addr2line, "-C", "-f", "-p", "-e", str(binary)]
    command.extend(hex(offset) for offset in offsets)
    result = subprocess.run(command, check=True, text=True, capture_output=True)
    return result.stdout.splitlines()


def read_binary_build_id(binary: pathlib.Path) -> str:
    readelf = shutil.which("readelf")
    if not readelf:
        raise RuntimeError("readelf is required to verify the exact crashing binary")
    result = subprocess.run(
        [readelf, "--notes", str(binary)], check=True, text=True, capture_output=True)
    match = re.search(r"Build ID:\s*([0-9a-fA-F]+)", result.stdout)
    if not match:
        raise RuntimeError(f"{binary} has no GNU ELF build ID")
    return match.group(1).lower()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Resolve a P710 CrashReports/crash_report_*.txt file locally")
    parser.add_argument("report", type=pathlib.Path)
    parser.add_argument("--binary", required=True, type=pathlib.Path,
                        help="the exact unstripped p710_native_game that produced the report")
    args = parser.parse_args()

    report = args.report.resolve(strict=True)
    binary = args.binary.resolve(strict=True)
    report_build_id, main_image, instruction, frames, modules = parse_report(report)
    if not report_build_id or report_build_id == "unavailable":
        raise RuntimeError("report has no usable ELF build ID")
    binary_build_id = read_binary_build_id(binary)
    if binary_build_id != report_build_id:
        raise RuntimeError(
            f"ELF build ID mismatch: report {report_build_id}, binary {binary_build_id}")
    if not main_image:
        raise RuntimeError("report has no main_image field")
    main_module = next((module for module in modules if module.name == main_image), None)
    if main_module is None:
        raise RuntimeError(f"report has no main module named {main_image}")

    requested: list[tuple[str, int]] = []
    if instruction is not None and main_module.base <= instruction < main_module.end:
        requested.append(("fault", instruction - main_module.base))
    for index, address in enumerate(frames):
        module = owning_module(address, modules)
        if module and module.name == main_image:
            requested.append((f"frame {index}", address - module.base))

    resolved = resolve(binary, [offset for _, offset in requested])
    print(f"Report: {report}")
    print(f"Binary: {binary}")
    if instruction is not None:
        module = owning_module(instruction, modules)
        owner = module.name if module else "unknown module"
        print(f"Instruction pointer: {instruction:#x} ({owner})")
    if not requested:
        print("No addresses from the selected binary were present in the report.")
        return 0
    print("Resolved game frames:")
    for (label, offset), text in zip(requested, resolved):
        print(f"  {label}: {text} [binary+{offset:#x}]")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, subprocess.CalledProcessError, ValueError) as error:
        print(f"symbolize_crash_report: {error}", file=sys.stderr)
        raise SystemExit(2)
