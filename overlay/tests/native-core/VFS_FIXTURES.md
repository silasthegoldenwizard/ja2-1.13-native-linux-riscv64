> P710 native notice preparation: tests/native-core/VFS_FIXTURES.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document synthetic SLF/7z construction, original format contract and independent expectations.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/VFS_FIXTURES.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Synthetic VFS archive fixtures

Run with Python 3 and the already installed 7z CLI; no Python packages are needed:

```sh
python3 tests/native-core/generate_vfs_fixtures.py /absolute/output/directory
```

An optional `--seven-zip /path/to/7z` selects the executable. The generator creates
only named leaves below the caller's output directory. It rejects symlink output
leaves/parents, never recursively deletes a directory, and unlinks only its three
known CLI-generated archive leaves before recreating them. Existing unrelated
files remain untouched. The generator reads no game assets and runs no VFS code.

The output is `base/`, `mod/`, `writable/`, `archives/`, `_7z-input/`, and
`manifest.json`. The manifest records exact file sizes and SHA-256 digests,
literal payload expectations, archive acceptance expectations, malformed-input
reasons, and the CLI commands/listings. It does not infer correct VFS results
from production-reader output. The generated writable file is reset to its
initial bytes on every invocation, so regenerate before a fresh integration run.

## Build versus reuse

The existing installed 7z CLI supplies ordinary Copy and LZMA archives and checks
their integrity and extracted bytes. Python's standard library is sufficient
for small fixed SLF disk records. A tiny explicit 7z header is needed only for
the shared Copy-folder case: installed 7-Zip 25.01 produces three separate Copy
folders even when passed `-ms=on`. No generic archive framework is introduced.

Acceptance for these inputs is that the independent 7z CLI accepts every valid
7z container and extracts the fixed expected bytes, and that the parent VFS
tests consume the independent SLF bytes and fixed logical oracle. Generating
fixtures alone does not establish VFS correctness or native game behavior.

## SLF source provenance and disk layout

The source contract inspected is `ext/VFS/src/Ext/slf/vfs_slf_library.cpp` at
engine revision `3394695a387a14c78e92b158cfdcf50e4068c96b`, specifically the
`slf::LIBHEADER` and `slf::DIRENTRY` declarations, state constants, directory-at-EOF
seek, and active-entry filtering. This file is part of bfVFS, copyright
2008–2010 (BF), LGPL 2.1 or later. Fixture code is new test code, not copied
production serialization. No second independent SLF specification was bundled
at the inspected paths, and no proprietary SLF was opened for this task.

The writer uses literal little-endian offsets for the historical Windows layout,
including its padding. It does not serialize C++ structures or use native
`sizeof`, alignment, or host byte order:

| Header byte offset | Field |
| --- | --- |
| 0 | library name, 256 bytes |
| 256 | library path, 256 bytes |
| 512 | signed 32-bit entry count |
| 516 | signed 32-bit active count |
| 520 | unsigned 16-bit sort flag |
| 522 | unsigned 16-bit version |
| 524 | one-byte subdirectory flag |
| 525 | three padding bytes |
| 528 | signed 32-bit reserved field; header ends at 532 |

| Directory-record byte offset | Field |
| --- | --- |
| 0 | filename, 256 bytes |
| 256 | unsigned 32-bit payload offset from archive start |
| 260 | unsigned 32-bit payload length |
| 264 | state: active=0, deleted=255, old=1, absent=254 |
| 265 | one reserved byte |
| 266 | two padding bytes |
| 268 | FILETIME low unsigned 32-bit word |
| 272 | FILETIME high unsigned 32-bit word |
| 276 | reserved unsigned 16-bit field |
| 278 | two tail-padding bytes; record ends at 280 |

Directory records follow all payload bytes, at
`archive_size - entry_count * 280`. The UTF-8 SLF filename fixture exercises
bfVFS's explicit `vfs::String::as_utf16` path conversion; it does not claim that
every historical external SLF uses UTF-8.

## Fixed logical oracle

| Input | Expected result |
| --- | --- |
| `base/Shared.txt` | `base shared\n` |
| `mod/Shared.txt` | `mod shared\n`; wins when mod profile has priority |
| `base/BaseOnly.txt` | `base only\n`; fallback when mod lacks file |
| `writable/WriteTarget.txt` | initial `initial writable\n` |
| `base/Loose/MiXeD.txt` | `loose mixed payload\n` |
| `base/Loose/Grüße.txt` | `loose bmp payload\n` |
| `base/Loose/🌍.txt` | `loose astral payload\n` |
| `archives/valid.slf` → `Archive/Mixed.TXT` | `slf mixed payload\n` |
| `archives/valid.slf` → `Archive/Grüße.txt` | `slf unicode payload\n` |
| `archives/valid.slf` → Deleted/Old/Absent under `Archive/` | absent, although their bytes physically exist |
| `archives/prefixed.slf` → `Prefix/Inner.TXT` | `prefixed payload\n`; tests header prefix |
| `archives/empty.slf` | valid empty library |
| `archives/case-collision.slf` | fixed first entry `Collision.TXT`=`collision first\n`, then `collision.txt`=`collision second\n`; integration owner selects rejection or documented first-wins policy |

Case/separator-equivalent queries for the mixed SLF entry include
`archive/mixed.txt` and `ARCHIVE\MIXED.TXT`. All strings above denote UTF-8 bytes;
`\n` denotes a single byte 0x0a. The manifest repeats these literal expectations.

These SLF archives must be rejected: `short-header.slf`, `negative-count.slf`,
`overflow-count.slf` (INT32_MAX), `impossible-count.slf`,
`truncated-directory.slf`, `range-before-payload.slf`, `range-past-eof.slf`,
`range-in-directory.slf`, `range-cross-directory.slf`, `no-nul-entry.slf`,
`no-nul-library-name.slf`, and `no-nul-library-path.slf`. The malformed range
cases exercise payload offsets into the header/directory, past EOF, and a
32-bit endpoint overflow. Their manifest entries state each isolated defect.

## 7z vectors and independent validation

`archives/copy.7z` and `archives/copy-nonsolid.7z` are CLI-created Copy-method
archives with uncompressed headers. The former requests solid mode, but the
observed CLI listing shows three blocks and `Solid = -`. The latter explicitly
requests separate folders. Both include:

| Filename | Exact payload |
| --- | --- |
| `Seven/Mixed.TXT` | `7z mixed payload\n` (17 bytes) |
| `Seven/Grüße.txt` | `7z bmp payload\n` (15 bytes) |
| `Seven/🌍.txt` | `7z astral payload\n` (18 bytes) |
| `Seven/Empty.txt` | zero bytes, no folder |

`archives/solid-copy.7z` uses the same fixed oracle with three nonempty files
sharing one Copy folder and the empty file's folder sentinel. Its small explicit
header follows bundled `ext/VFS/ext/7z/doc/7zFormat.txt`: one Copy coder, one
packed stream, three substreams with explicit lengths/CRCs, literal UTF-16LE
names including a surrogate pair, and header CRCs. Installed 7z independently
reports `Solid = +`, `Blocks = 1`; the fixture is 257 bytes. This catches
incorrect per-file reads that use the entire folder's offset and unpacked size.

`archives/compressed.7z` is a valid CLI-created LZMA archive with the same
payloads; the intentionally uncompressed bfVFS provider must reject it instead
of treating compressed stream bytes as file data. Its rejection expectation is
a provider limitation, not a claim that this archive is malformed.

Each generation runs `7z t` and extracts each of the four exact named files to
stdout for all four valid 7z containers. Their bytes must equal the fixed
literal oracle. The manifest retains technical listings. The three malformed
7z cases are `truncated.7z` (31-byte start header), `bad-signature.7z`
(corrupted first signature byte), and `truncated-body.7z` (missing final byte).

These are small deterministic test inputs, not compatibility coverage of all
SLF/7z variants or validation of proprietary resource archives. Payload/folder
offsets, Unicode conversion, overlay ordering, writable file operations and
malformed-input handling still require execution through the actual VFS tests.
