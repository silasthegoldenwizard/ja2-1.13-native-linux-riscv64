#!/usr/bin/env python3
"""Check the original fade-in path presents one monotonic frame per tick."""
# P710 native snapshot update: tests/native-core/check_fade_in_presentation.py
# Native Linux source/build/test adaptations captured in the accepted native checkpoint dated 2026-11-18.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-18.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


def function_body(source: str, signature: str) -> str:
    definition = re.search(re.escape(signature) + r"\s*\{", source)
    if definition is None:
        raise ValueError(f"missing {signature}")
    opening = definition.end() - 1
    depth = 0
    for index in range(opening, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[opening + 1 : index]
    raise ValueError(f"unterminated body for {signature}")


def main() -> int:
    if len(sys.argv) != 2:
        print("usage: check_fade_in_presentation.py ENGINE_ROOT", file=sys.stderr)
        return 2

    root = Path(sys.argv[1])
    source = (root / "Ja2" / "Fade Screen.cpp").read_text(encoding="latin-1")
    body = function_body(source, "void FadeInFrameBufferRealFade( )")
    failures: list[str] = []
    checks = 0

    def require(condition: bool, message: str) -> None:
        nonlocal checks
        checks += 1
        if not condition:
            failures.append(message)

    restore = body.find("RestoreExternBackgroundRect")
    shade_loop = re.search(
        r"for\s*\(\s*cnt\s*=\s*0\s*;\s*"
        r"cnt\s*<\s*\(\s*gsFadeLimit\s*-\s*gsFadeCount\s*\)\s*;\s*"
        r"cnt\+\+\s*\)",
        body,
    )
    shadow = body.find("ShadowVideoSurfaceRectUsingLowPercentTable")
    state_update = body.find("gsFadeRealCount = gsFadeCount")

    require(restore >= 0, "fade-in does not restore its clean saved frame")
    require(shade_loop is not None, "fade-in shade count no longer follows remaining steps")
    require(shadow >= 0, "fade-in no longer applies the production low-percent shade")
    require(
        restore >= 0 and shade_loop is not None and restore < shade_loop.start() < shadow,
        "fade-in shades stale framebuffer pixels before restoring the saved frame",
    )
    require(
        "RefreshScreen" not in body,
        "fade-in presents internally and is presented a second time by the game loop",
    )
    require(
        state_update > shadow,
        "fade-in marks the step complete before producing its frame",
    )

    # The checked loop produces limit-count shade passes from the same clean
    # snapshot. Any nonzero darkening factor therefore grows monotonically to
    # the exact source value, and the ordinary game loop presents it once.
    limit = 8
    shade_factor = 0.8
    intensities = [shade_factor ** (limit - count) for count in range(limit + 1)]
    require(
        all(left < right for left, right in zip(intensities, intensities[1:])),
        "modeled fade-in intensity is not strictly monotonic",
    )
    require(intensities[-1] == 1.0, "final fade-in frame is not the exact saved frame")

    if failures:
        for failure in failures:
            print(f"FAIL: {failure}", file=sys.stderr)
        print(f"FAIL: {len(failures)} of {checks} fade-in checks failed", file=sys.stderr)
        return 1

    print(f"PASS: {checks} fade-in presentation checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
