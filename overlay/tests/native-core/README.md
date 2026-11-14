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

This separate C++17 target compiles the actual `../../sgp/line.cpp` once.
It does not configure the root project for the full game.
Native modules use production portable headers.
Integration targets also compile the actual bundled VFS.

The production `sgp/core_types.h` supplies the existing raster primitives and signed rectangle.
The `types.h` header keeps the wider legacy/VFS interface for callers outside this component.
At the raster API, INT32 replaces Win32 BOOL.
This change keeps a 32-bit zero/nonzero flag on the selected platform.
The original raster algorithms and byte-pitch interface stay the same.

Use the existing GCC/G++, CMake >= 3.20, Make, Python3, 7z and installed zlib development files.
SDL3 is optional.
For SDL3, use the explicitly approved local prefix:

```sh
cmake -S tests/native-core -B /path/to/build/debug -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug
cmake --build /path/to/build/debug
ctest --test-dir /path/to/build/debug --output-on-failure

cmake -S tests/native-core -B /path/to/build/sanitized -G 'Unix Makefiles' -DCMAKE_BUILD_TYPE=Debug -DP710_SANITIZERS=ON
cmake --build /path/to/build/sanitized
ctest --test-dir /path/to/build/sanitized --output-on-failure
```

Each fixture compares every byte of a 5x5 framebuffer.
The framebuffer has a 14-byte row pitch, four padding bytes per row, and 32-byte prefix/suffix guards.
The pixel coordinate lists and the composite preview's 25 colors are manually specified expected data.
The tests do not use the renderer to generate their oracle.

The tests cover shallow and steep ties, all eight line directions, reverse endpoints, clipping and color alteration.
They also cover signed rectangle coordinates and deliberately offset byte storage.
Unclipped calls use in-bounds inputs.
The tests use the module's global clip/pitch state serially, as in the original API.

Artifacts contain actual/expected guarded bytes, an index, summary JSON and a 5x5 RGB565/PPM synthetic preview.
The raster test passes only if all bytes are equal, including unchanged padding and guards.
The build enables compiler diagnostics for the original unit.
The build does not use blanket warning suppressions.

Sanitized builds add two **intentional failure canaries**, separate from the raster fixtures.
CMake makes sure that the heap overrun produces the specific ASan diagnostic and a nonzero exit.
It also makes sure that signed overflow produces the specific UBSan diagnostic and a nonzero exit.
An arbitrary crash cannot pass these checks.
The build keeps their diagnostic logs under `artifacts/`.

The canary failures give expected evidence of instrumentation.
The raster test itself must exit cleanly.
This result demonstrates one CPU raster module.
It does not build the game or validate all mechanics or resource loaders.
It does not establish Windows/RISC-V/hardware parity.

## FileMan handle slice (P0-03)

The same target also compiles production `sgp/FileManIO.cpp`.
This module owns the actual FileOpen/Close/Read/ReadLine/Write/Seek/GetPos/GetSize/EOF functions and the opaque 32-bit handle registry.
The handle-only executable links a generated-file provider for fault injection.
The actual production bfVFS adapter is now in `sgp/FileManVFS.cpp`.
The next section describes its separate integration executable.

Win32 metadata functions stay in `sgp/FileMan.cpp`.
The full-game SGP source list includes the three production translation units.

The test links a generated-file provider at the internal `OpenBackend` seam.
It covers native pointers above UINT32 range, invalid/stale IDs, native address reuse, aliases with a shared cursor and last-alias closure.
It also covers mode checks, short I/O, zero/null requests, signed seek distances, EOF/lines and position/size overflow.
Other cases cover exception cleanup, shutdown, concurrent callers and FMOD-style opaque tokens.

Fixture profiles test the request policy and identify which generated file changes.
They do not establish actual bfVFS profile/archive/path behavior.
The adapter retains named-profile reads and stop-on-writable-profile writes.
These adapter behaviors have a source review.

Each CTest run writes only generated fixtures below its build artifact directory and a FileMan JSON result.
It does not need original data files.
The registry never recycles handles.
Zero and all-ones are invalid.
Allocation fails closed at exhaustion.
The exhaustive token-exhaustion boundary has a source review, but no stress test.

Repeated opens of the same VFS object keep its existing shared cursor.
A handle ID does not promise an independent file stream.
FileMan serializes backend operations.
This result does not establish ThreadSanitizer or full VFS concurrency acceptance.

Legacy FILE_ACCESS_READWRITE still selects write mode.
The existing VFS FileOpen path continues to handle creation/truncation flags and delete-on-close.
This component does not separately implement the creation bits or delete-on-close argument.
FileGetPos returns -1 beyond INT32 range.
FileGetSize returns 0 beyond UINT32 range.
The public API and resource formats have not become wider.

Invalid or wrong-mode zero-byte I/O now fails safely.
Valid zero-byte I/O permits null buffers/count pointers.
Failed operations set optional counts to zero.
Short reads/writes report their actual counts.
Read/write exceptions close the backend once and invalidate all aliases before they propagate.

Run the sanitized suite in a native environment that permits leak detection:

```sh
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
ctest --test-dir /path/to/build/sanitized --output-on-failure
```

Instrumentation applies to the production objects, fixture drivers and diagnostic canary.
With P710_VFS enabled, it also applies to the actual bundled VFS and 7z sources.
Full-game/FMOD runtime, Windows and RISC-V have not been tested.

## Actual VFS integration (P0-04)

`P710_VFS=ON` is the default.
It compiles the bundled bfVFS, bundled 7z and actual FileManVFS provider.
It links these components to the actual FileManIO entrypoints.
The `native_vfs_tests.cpp` file has no replacement OpenBackend function.
Python3 and an already installed 7z CLI generate its fixtures.
The tests read no proprietary game resources.

For the older handle/raster-only build, use `-DP710_VFS=OFF -DP710_IMAGES=OFF`.

The fixture generator writes explicit little-endian SLF records.
It uses the independent CLI to validate its 7z bytes.
See VFS_FIXTURES.md for literal payloads, wire widths, inactive records and malformed inputs.
It also describes Unicode names, solid Copy substreams and the compressed-method rejection expectation.
The generated test roots are disposable.
Earlier evidence folders are kept separately.

Integration checks mount actual base/mod/writable profiles and actual SLF/7z libraries.
They then test FileOpen/read/write/seek/size/close/EOF.
Cases cover ASCII case and separator equivalence, exact Unicode names, profile priority, writable-root creation, preserved readonly sources and shared aliases.
They also cover successful and failed read-to-write upgrade, mixed update-stream directions, directory patterns, invalid UTF data and malformed archives.

The Copy7z provider rejects compressed folders.
It does not return compressed bytes as resource content.
This component adds no new Unicode normalization or global case-folding policy.

The sanitizer option instruments the VFS library and its bundled C decoder, including vptr and alignment checks.
Leak detection needs a native execution environment that permits it.
Do not disable leak detection to claim a clean integration run.
The tests clean up VFS/profile/file objects before they clear the existing object allocator.
Passing these synthetic tests does not establish full asset compatibility or a native playable game.

## Real image and transparent sprite path

`P710_IMAGES=ON` is the default.
It requires actual VFS integration.
The native target compiles production `STCI.cpp`, image ownership/palette helpers and original runtime image/object types.
It also compiles the two active transparent ETRLE functions.

The STCI loader reads explicit little-endian wire fields and validates bounded sections.
It replaces requested contents only after successful validation.
Zlib-compressed pixel representations stay compressed after validation.
The other image readers and blitter families stay outside this native target.

`IMAGE_FIXTURES.md` describes independently authored resources: twelve accepted files and 28 malformed files, with literal pixel grids/masks.
Tests load these resources through FileMan.
They compare entire guarded/padded/unaligned RGB565 surfaces.
The oracle is independent of the production decoder and drawing functions.
The tests need no proprietary resources.

The tests write actual result counts to `artifacts/images/image-result.json`.
These counts describe coverage.
They do not establish game playability.

## Optional native SDL3 presentation

The build finds the user-approved SDL3 3.4.20 dependency with an exact CMake package version.
Enable the viewer with the approved prefix:

```sh
cmake -S tests/native-core -B /path/to/build/debug \
  -DP710_SDL_PRESENTATION=ON -DCMAKE_PREFIX_PATH=/path/to/approved/SDL3/prefix
cmake --build /path/to/build/debug
ctest --test-dir /path/to/build/debug --output-on-failure
SDL_VIDEODRIVER=x11 /path/to/build/debug/p710_native_image_viewer \
  /path/to/build/debug/artifacts/images /path/to/native-window.bmp
```

The verifier mounts the synthetic fixture directory through the actual VFS.
It loads `sprite.sti` and creates the production palette.
It draws both frames and a clipped copy through the retained HVOBJECT interface.
It presents a padded RGB565 surface in its own native window.
It saves the actual SDL window surface and raw frame.

The host must permit native desktop access.
The x11 selection prevents a dummy/offscreen fallback.
This result demonstrates image/presentation integration.
It does not demonstrate the complete engine video manager, input loop, menu or game.
