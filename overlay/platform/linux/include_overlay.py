"""Use Clang's real case-insensitive VFS for the retained Windows include case.

Every virtual file is the actual local source. There are no compatibility header
contents here, and absent Windows SDK headers remain absent. Ambiguous names
within a directory are rejected rather than choosing a different engine header.
"""
# P710 native snapshot update: platform/linux/include_overlay.py
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import json
from pathlib import Path
import sys


def directory(path):
    children = []
    names = set()
    for child in sorted(path.iterdir()):
        if child.name.startswith(".") or child.name in {"__pycache__", "build"}:
            continue
        if child.is_symlink():
            continue
        if child.is_dir():
            entry = directory(child)
            if not entry["contents"]:
                continue
        elif child.suffix.lower() in {".h", ".hpp", ".hxx", ".inc", ".inl", ".cpp", ".c"}:
            entry = {"type": "file", "name": child.name, "external-contents": str(child)}
        else:
            continue
        key = child.name.casefold()
        if key in names:
            raise ValueError(f"ambiguous include case in {path}: {child.name}")
        names.add(key)
        children.append(entry)
    return {"type": "directory", "name": path.name, "contents": children}


if __name__ == "__main__":
    root = Path(sys.argv[1]).resolve(strict=True)
    entry = directory(root)
    entry["name"] = str(root)
    overlay = {"version": 0, "case-sensitive": False, "use-external-names": True, "roots": [entry]}
    Path(sys.argv[2]).write_text(json.dumps(overlay, indent=2) + "\n")
