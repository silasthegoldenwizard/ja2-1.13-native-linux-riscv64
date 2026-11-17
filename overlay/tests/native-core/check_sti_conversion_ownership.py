#!/usr/bin/env python3
"""Check production ETRLE conversion-buffer ownership."""
# P710 native snapshot update: tests/native-core/check_sti_conversion_ownership.py
# Native Linux source/build/test adaptations captured in the accepted native crash-hardening checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, name: str) -> str:
    match = re.search(
        rf"(?:void|BOOLEAN)\s+{name}\s*\([^)]*\)\s*\{{(?P<body>.*?)\n\}}",
        source,
        re.DOTALL,
    )
    return match.group("body") if match else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_sti_conversion_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Utils" / "STIConvert.cpp").read_text(
        encoding="latin-1"
    )
    writer = function_body(source, "WriteSTIFile")
    converter = function_body(source, "ConvertToETRLE")
    helper = function_body(source, "FreeETRLEBuffers")
    if not writer or not converter:
        print("FAIL: STI conversion functions were not found", file=sys.stderr)
        return 1

    checks = (
        (
            bool(helper)
            and helper.count("MemFree") == 2
            and "*ppDest = NULL;" in helper
            and "*ppSubImageBuffer = NULL;" in helper,
            "shared ETRLE cleanup does not release and clear both outputs",
        ),
        (
            converter.count("FreeETRLEBuffers(ppDest, ppSubImageBuffer);") == 5,
            "not every ETRLE conversion failure uses shared cleanup",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*!ConvertToETRLE.*?\)\s*\{\s*"
                r"FreeETRLEBuffers\s*\(\s*&pOutputBuffer\s*,\s*"
                r"\(UINT8\s*\*\*\)\s*&pSubImageBuffer\s*\)\s*;\s*return\s*;",
                writer,
                re.DOTALL,
            )),
            "failed ETRLE conversion can continue into file writes",
        ),
        (
            bool(re.search(
                r"if\s*\(\s*!hFile\s*\)\s*\{\s*"
                r"FreeETRLEBuffers\s*\(\s*&pOutputBuffer\s*,\s*"
                r"\(UINT8\s*\*\*\)\s*&pSubImageBuffer\s*\)\s*;\s*return\s*;",
                writer,
                re.DOTALL,
            )),
            "file-open failure leaks ETRLE conversion buffers",
        ),
        (
            writer.count(
                "FreeETRLEBuffers(&pOutputBuffer, (UINT8 **)&pSubImageBuffer);"
            ) == 3,
            "STI writer does not release both buffers on every exit",
        ),
    )

    failures = [message for passed, message in checks if not passed]
    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {len(checks)} STI ownership checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} STI conversion ownership checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
