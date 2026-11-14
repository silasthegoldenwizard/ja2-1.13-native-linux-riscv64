> P710 native notice preparation: tests/native-core/README.md
> New candidate test/build/documentation/metadata path.
> Native adaptation recorded in the 2026-11-12 checkpoint.
> Exact first-edit/creation date is not established; this is a checkpoint-record date.
> Scope: document isolated native component checks and remaining limits.
> Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
> Notice inserted: 2026-11-12.
> Existing source bytes and original notices retained; no additional rights are granted.
> This record does not establish license applicability, compliance or distribution permission.

<!--
P710 native snapshot update: tests/native-core/README.md
Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
Update notice inserted during source-only package preparation: 2026-11-14.
This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
-->

# Original 1.13 native raster, files, images and presentation

This standalone C++17 target compiles the real `../../sgp/line.cpp` once. It
does not configure the full-game root project. The native modules use production
portable headers; the integration targets also compile the actual bundled VFS.
The production `sgp/core_types.h` supplies the existing raster primitives and
signed rectangle; `types.h` retains the wider legacy/VFS interface for callers
outside this slice. INT32 replaces Win32 BOOL at the raster API, preserving a
32-bit zero/nonzero flag on the selected platform. The original raster
algorithms and byte-pitch interface are retained.

Use existing GCC/G++, CMake >= 3.20, Make, Python3, 7z and the installed zlib
development files. SDL3 is optional and uses the explicitly approved local prefix:

```sh
cmake -S tests/native-core -B /path/to/build/debug -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug
cmake --build /path/to/build/debug
ctest --test-dir /path/to/build/debug --output-on-failure

cmake -S tests/native-core -B /path/to/build/sanitized -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DP710_SANITIZERS=ON
cmake --build /path/to/build/sanitized
ctest --test-dir /path/to/build/sanitized --output-on-failure
```

Each fixture compares every byte of a 5x5 framebuffer with 14-byte row pitch,
four padding bytes per row, and 32-byte prefix/suffix guards. Pixel coordinate
lists and the composite preview's 25 colors are hand-authored expected data;
the tests do not call the renderer to generate their oracle. Shallow and steep
ties, all eight line directions, reverse endpoints, clipping, color alteration,
signed rectangle coordinates and deliberately offset byte storage are covered.
Unclipped calls use in-bounds inputs. The module's global clip/pitch state is
used serially, as in its original API.

Artifacts contain actual/expected guarded bytes, an index, summary JSON and a
5x5 RGB565/PPM synthetic preview. Passing the raster test requires byte equality,
including unchanged padding and guards. Compiler diagnostics are intentionally
enabled for the original unit; no blanket warning suppressions are used.

Sanitized builds add two **intentional failure canaries**, separate from the
raster fixtures. CMake checks that the heap overrun produces the specific ASan
diagnostic and signed overflow produces the specific UBSan diagnostic, with
nonzero exits. An arbitrary crash cannot pass these checks. Their diagnostic
logs are retained under `artifacts/`. The canary failures are expected evidence
of instrumentation; the raster test itself must exit cleanly.

This demonstrates one CPU raster module. It does not build the game, validate
all mechanics or resource loaders, or establish Windows/RISC-V/hardware parity.

## FileMan handle slice (P0-03)

The same target also compiles production `sgp/FileManIO.cpp`. This owns the real
FileOpen/Close/Read/ReadLine/Write/Seek/GetPos/GetSize/EOF functions and the opaque
32-bit handle registry. The handle-only executable links a generated-file
provider for fault injection. The actual production bfVFS adapter now lives in
`sgp/FileManVFS.cpp`; its separate integration executable is described below.
Win32 metadata functions remain in `sgp/FileMan.cpp`. The full-game SGP source
list includes the three production translation units.

The test links a generated-file provider at the internal `OpenBackend` seam.
It exercises native pointers above UINT32 range, invalid/stale IDs, native address
reuse, aliases with a shared cursor, last-alias closure, mode checks, short I/O,
zero/null requests, signed seek distances, EOF/lines, position/size overflow,
exception cleanup, shutdown, concurrent callers, and FMOD-style opaque tokens.
Fixture profiles check the request policy and which generated file changes;
they do not establish actual bfVFS profile/archive/path behavior. Named-profile
reads and stop-on-writable-profile writes remain in the source-reviewed adapter.

Each CTest run writes only generated fixtures below its build artifact directory
and a FileMan JSON result. No original data files are needed. Handles are never
recycled: zero/all-ones are invalid and allocation fails closed at exhaustion.
The exhaustive token-exhaustion boundary is source reviewed, not stress tested.
Repeated opens of the same VFS object retain its existing shared cursor; a handle
ID is not a promise of an independent file stream. FileMan serializes backend
operations, but this is not ThreadSanitizer or full VFS concurrency acceptance.

Legacy FILE_ACCESS_READWRITE still selects write mode. Creation/truncation flags
and delete-on-close remain as handled by the preexisting VFS FileOpen path (the
creation bits and delete-on-close argument are not separately implemented here).
FileGetPos returns -1 beyond INT32 range; FileGetSize returns 0 beyond UINT32
range. The public API and resource formats have not been widened. Invalid or
wrong-mode zero-byte I/O now fails safely; valid zero-byte I/O permits null
buffers/count pointers. Failed operations initialize optional counts to zero,
and short reads/writes report their actual counts. Read/write exceptions close
the backend once and invalidate all aliases before propagating the exception.

Run the sanitized suite in a native environment that permits leak detection:

```sh
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --test-dir /path/to/build/sanitized --output-on-failure
```

The instrumentation applies to the production objects and fixture drivers,
the diagnostic canary, and (with P710_VFS enabled) the actual bundled VFS and
7z sources. Full-game/FMOD runtime, Windows and RISC-V remain untested.


## Actual VFS integration (P0-04)

`P710_VFS=ON` is the default. It compiles the bundled bfVFS, bundled 7z and the
real FileManVFS provider, and links these to the real FileManIO entrypoints.
`native_vfs_tests.cpp` has no replacement OpenBackend function. Its fixtures are
generated by Python3 plus an already installed 7z CLI. No proprietary game
resources are read. Use `-DP710_VFS=OFF -DP710_IMAGES=OFF` for the older handle/raster-only build.

The fixture generator writes explicit little-endian SLF records and validates
its 7z bytes with the independent CLI. See VFS_FIXTURES.md for literal payloads,
wire widths, inactive records, malformed inputs, Unicode names, solid Copy
substreams and the compressed-method rejection expectation. The generated test
roots are disposable; earlier evidence folders are retained separately.

Integration checks mount real base/mod/writable profiles and actual SLF/7z
libraries, then exercise FileOpen/read/write/seek/size/close/EOF. They cover
ASCII case and separator equivalence, exact Unicode names, profile priority,
writable-root creation, preserved readonly sources, shared aliases, successful
and failed read-to-write upgrade, mixed update-stream directions, directory
patterns, invalid UTF data and malformed archives. The Copy7z provider rejects
compressed folders rather than returning compressed bytes as resource content.
No new Unicode normalization or global case-folding policy is introduced.

Both the VFS library and its bundled C decoder are instrumented by the sanitizer
option, including vptr and alignment checks. Leak detection requires a native
execution environment that permits it; do not disable leaks to claim a clean
integration run. The tests clean up VFS/profile/file objects before clearing the
existing object allocator. Passing these synthetic tests does not establish
full asset compatibility or a native playable game.


## Real image and transparent sprite path

`P710_IMAGES=ON` is the default and requires actual VFS integration. Production
`STCI.cpp`, image ownership/palette helpers, original runtime image/object types,
and the two active transparent ETRLE functions compile in the native target.
The STCI loader reads explicit little-endian wire fields, validates bounded
sections and replaces requested contents only after successful validation.
Zlib-compressed pixel representations remain compressed after validation. The
other image readers and blitter families remain outside this native target.

`IMAGE_FIXTURES.md` describes independent authored resources: twelve accepted
files and 28 malformed files, with literal pixel grids/masks. Tests load those
through FileMan and compare entire guarded/padded/unaligned RGB565 surfaces.
The oracle is independent of the production decoder and drawing functions.
No proprietary resources are required. Actual result counts are emitted into
`artifacts/images/image-result.json`; they are coverage, not game playability.

## Optional native SDL3 presentation

The user-approved SDL3 3.4.20 dependency is found with an exact CMake package
version. Enable the viewer using the approved prefix:

```sh
cmake -S tests/native-core -B /path/to/build/debug \
  -DP710_SDL_PRESENTATION=ON -DCMAKE_PREFIX_PATH=/path/to/approved/SDL3/prefix
cmake --build /path/to/build/debug
ctest --test-dir /path/to/build/debug --output-on-failure
SDL_VIDEODRIVER=x11 /path/to/build/debug/p710_native_image_viewer \
  /path/to/build/debug/artifacts/images /path/to/native-window.bmp
```

The verifier mounts the synthetic fixture directory through actual VFS, loads
`sprite.sti`, creates the production palette, draws both frames and a clipped
copy through the retained HVOBJECT interface, and presents a padded RGB565
surface in its own native window. It saves the actual SDL window surface and
raw frame. The host must permit native desktop access; selecting x11 rules out
a dummy/offscreen fallback. This is an image/presentation integration result,
not the complete engine video manager, input loop, menu or game.
