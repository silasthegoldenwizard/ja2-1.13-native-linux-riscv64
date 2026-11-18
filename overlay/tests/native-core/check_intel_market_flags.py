#!/usr/bin/env python3
"""Check Intel-market map-flag bounds and unsigned-mask construction."""
# P710 native snapshot update: tests/native-core/check_intel_market_flags.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def section(source: str, start_text: str, end_text: str) -> str:
    start = source.find(start_text)
    end = source.find(end_text, start + len(start_text)) if start >= 0 else -1
    return source[start:end] if start >= 0 and end >= 0 else ""


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_intel_market_flags.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Laptop" / "Intelmarket.cpp").read_text(encoding="latin-1")
    header = (root / "Laptop" / "Intelmarket.h").read_text(encoding="latin-1")
    callback = section(source, "void IntelBuyButtonCallback(", "void EnterIntelmarket()")
    if not callback:
        print("FAIL: IntelBuyButtonCallback was not found", file=sys.stderr)
        return 1

    helper_call = callback.find("Intelmarket::BuildMapIntelFlagMask(")
    balance_check = callback.find("GetIntel()")
    debit = callback.find("AddIntel(")
    checks = (
        (
            "BuildMapIntelFlagMask" in header
            and "region < 0" in header
            and "region >= 16" in header,
            "the shared mask helper rejects negative and out-of-range regions",
        ),
        (
            "std::uint32_t{1}" in header
            and "16u + shift" in header
            and re.search(r"(?<![\w:])1\s*<<", header) is None,
            "both low and high bits are formed from an unsigned 32-bit value",
        ),
        (
            helper_call >= 0
            and balance_check >= 0
            and debit >= 0
            and helper_call < balance_check < debit,
            "the callback validates the selected region before checking or spending Intel",
        ),
        (
            re.search(r"(?<![\w:])1\s*<<", callback) is None
            and "LaptopSaveInfo.usMapIntelFlags |= mapIntelFlags;" in callback,
            "the callback applies only the validated combined mask",
        ),
        (
            "gIntelBuyMapPartNamesStr[i][63] = '\\0';" in source
            and "gIntelBuyMapPartNamesStr[i][63] = '/0';" not in source,
            "map-region names receive a real NUL terminator",
        ),
    )

    failures = 0
    for passed, message in checks:
        print(f"{'PASS' if passed else 'FAIL'}: {message}")
        failures += not passed
    if failures:
        print(f"FAIL: {failures} of {len(checks)} Intel-market flag checks failed")
        return 1
    print(f"PASS: {len(checks)} Intel-market flag checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
