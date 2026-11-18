"""Extract the production Bobby Ray shipment-region lifecycle."""
# P710 native snapshot update: tests/native-core/extract_bobbyr_shipment_regions.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


import json
from pathlib import Path
import sys


def extract_function(source: Path, text: str, signature: str, output: Path) -> None:
    start = text.rindex("\n" + signature) + 1
    opening = text.index("{", start)
    depth = 0
    end = opening
    for end in range(opening, len(text)):
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
            if depth == 0:
                end += 1
                break
    else:
        raise RuntimeError(f"unterminated function: {signature}")

    line = text.count("\n", 0, start) + 1
    output.write_text(
        f"#line {line} {json.dumps(str(source))}\n" + text[start:end] + "\n",
        encoding="utf-8",
    )


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: extract_bobbyr_shipment_regions.py ENGINE_ROOT OUTPUT_DIR",
              file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    output_dir = Path(sys.argv[2])
    source = root / "Laptop" / "BobbyRShipments.cpp"
    text = source.read_text(encoding="latin-1")
    output_dir.mkdir(parents=True, exist_ok=True)
    extract_function(
        source,
        text,
        "void CreatePreviousShipmentsMouseRegions()",
        output_dir / "create_previous_shipments_regions.inc",
    )
    extract_function(
        source,
        text,
        "void RemovePreviousShipmentsMouseRegions()",
        output_dir / "remove_previous_shipments_regions.inc",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
