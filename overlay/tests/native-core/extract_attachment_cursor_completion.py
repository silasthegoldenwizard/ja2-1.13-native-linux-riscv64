"""Extract the current production attachment-cursor completion path."""
# P710 native snapshot update: tests/native-core/extract_attachment_cursor_completion.py
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.


from pathlib import Path
import re
import sys


interface_source, item_source, output_dir = map(Path, sys.argv[1:])
interface_text = interface_source.read_text()
item_text = item_source.read_text()
output_dir.mkdir(parents=True, exist_ok=True)


def masked(text: str) -> str:
    return re.sub(
        r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        lambda match: "".join("\n" if char == "\n" else " " for char in match.group()),
        text,
    )


def balanced_function(text: str, signature: str) -> tuple[int, str]:
    assert text.count(signature) == 1, (signature, text.count(signature))
    start = text.index(signature)
    clean = masked(text)
    opening = clean.index("{", start)
    depth = 0
    for index in range(opening, len(clean)):
        if clean[index] == "{":
            depth += 1
        elif clean[index] == "}":
            depth -= 1
            if depth == 0:
                return start, text[start : index + 1]
    raise AssertionError(f"unterminated function: {signature}")


def balanced_block_end(text: str, start: int) -> int:
    clean = masked(text)
    opening = clean.index("{", start)
    depth = 0
    for index in range(opening, len(clean)):
        if clean[index] == "{":
            depth += 1
        elif clean[index] == "}":
            depth -= 1
            if depth == 0:
                return index + 1
    raise AssertionError("unterminated completion block")


def line_number(text: str, offset: int) -> int:
    return text[:offset].count("\n") + 1


def line_directive(source: Path, text: str, offset: int) -> str:
    return f'#line {line_number(text, offset)} "{source}"\n'


do_start, do_attachment = balanced_function(
    interface_text, "void DoAttachment( UINT8 subObject, INT32 iItemPos )"
)
(output_dir / "attachment_do_attachment.inc").write_text(
    line_directive(interface_source, interface_text, do_start) + do_attachment + "\n"
)

call = "DoAttachment((UINT8)ubStatusIndex, uiItemPos);"
guard = "if(gpItemPointer && gpItemPointer->exists()){"
target_refresh = "if(gpItemDescObject != NULL)"
assert interface_text.count(call) == 1
assert interface_text.count(guard) == 1
assert interface_text.count(target_refresh) >= 1
call_offset = interface_text.index(call)
completion_start = interface_text.rfind("\n", 0, call_offset) + 1
guard_offset = interface_text.index(guard, call_offset + len(call))
guard_end = balanced_block_end(interface_text, guard_offset)
target_offset = interface_text.index(target_refresh, guard_end)
assert not interface_text[guard_end:target_offset].strip()
completion_end = balanced_block_end(interface_text, target_offset)
completion = interface_text[completion_start:completion_end]
assert completion.count(call) == 1
assert completion.count(guard) == 1
(output_dir / "attachment_completion.inc").write_text(
    line_directive(interface_source, interface_text, completion_start)
    + completion
    + "\n"
)

method_signatures = (
    "bool OBJECTTYPE::exists() const",
    "OBJECTTYPE::OBJECTTYPE()",
    "void OBJECTTYPE::initialize()",
    "OBJECTTYPE::~OBJECTTYPE()",
    "StackedObjectData::StackedObjectData()",
    "StackedObjectData::~StackedObjectData()",
    "ObjectData::~ObjectData()",
    "void ObjectData::DeleteLBE()",
)
methods = []
for signature in method_signatures:
    start, method = balanced_function(item_text, signature)
    methods.append(line_directive(item_source, item_text, start) + method + "\n")
(output_dir / "attachment_object_methods.inc").write_text("\n".join(methods))
