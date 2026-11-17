#!/usr/bin/env python3
"""Check postal-delivery bounds and temporary ownership."""
# P710 native snapshot update: tests/native-core/check_postal_delivery_ownership.py
# Native Linux source/build/test adaptations captured in the accepted ownership and bounds checkpoint dated 2026-11-17.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-17.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def extract(source: str, start_marker: str, end_marker: str) -> str:
    start = source.find(start_marker)
    end = source.find(end_marker, start + len(start_marker))
    return source[start:end] if start >= 0 and end >= 0 else ""


def has_bounded_lookup(function: str) -> bool:
    return bool(
        re.search(r"usShipmentID\s*>=\s*_UsedShipmentIDList\.size\s*\(\s*\)", function)
        and re.search(
            r"while\s*\(\s*sli\s*!=\s*_Shipments\.end\s*\(\s*\)\s*&&\s*"
            r"SHIPMENT\s*\(\s*sli\s*\)\.usID\s*!=\s*usShipmentID\s*\)",
            function,
        )
        and re.search(r"if\s*\(\s*sli\s*==\s*_Shipments\.end\s*\(\s*\)\s*\)", function)
    )


def has_stack_manipulator(function: str) -> bool:
    return bool(
        re.search(r"\bCShipmentManipulator\s+ShipmentManipulator\s*\(", function)
        and "new CShipmentManipulator" not in function
    )


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_postal_delivery_ownership.py ENGINE_ROOT", file=sys.stderr)
        return 2

    source = (Path(sys.argv[1]) / "Laptop" / "PostalService.cpp").read_text(
        encoding="latin-1"
    )
    delivery = extract(
        source,
        "BOOLEAN CPostalService::DeliverShipment(UINT16 usShipmentID)",
        "BOOLEAN CPostalService::DeliverShipmentForMultiplayer(",
    )
    multiplayer = extract(
        source,
        "BOOLEAN CPostalService::DeliverShipmentForMultiplayer(UINT16 usShipmentID)",
        "BOOLEAN CPostalService::RegisterDeliveryCallback(",
    )
    if not delivery or not multiplayer:
        print("FAIL: one or more postal delivery functions were not found", file=sys.stderr)
        return 1

    checks = [
        ("delivery lookup is range- and end-bounded", has_bounded_lookup(delivery)),
        (
            "multiplayer delivery lookup is range- and end-bounded",
            has_bounded_lookup(multiplayer),
        ),
        ("delivery callback manipulator has automatic ownership", has_stack_manipulator(delivery)),
        (
            "multiplayer callback manipulator has automatic ownership",
            has_stack_manipulator(multiplayer),
        ),
        (
            "delivery item array has automatic ownership",
            bool(re.search(r"std::unique_ptr\s*<\s*OBJECTTYPE\s*\[\s*\]\s*>\s+pObject", delivery)),
        ),
        (
            "delivery stolen-item array has automatic ownership",
            bool(re.search(r"std::unique_ptr\s*<\s*OBJECTTYPE\s*\[\s*\]\s*>\s+pStolenObject", delivery)),
        ),
        (
            "multiplayer item array has automatic ownership",
            bool(re.search(r"std::unique_ptr\s*<\s*OBJECTTYPE\s*\[\s*\]\s*>\s+pObject", multiplayer)),
        ),
    ]

    failures = 0
    for label, passed in checks:
        if passed:
            print(f"PASS: {label}")
        else:
            failures += 1
            print(f"FAIL: {label}", file=sys.stderr)

    if failures:
        print(f"FAIL: {failures} postal delivery checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {len(checks)} postal delivery checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
