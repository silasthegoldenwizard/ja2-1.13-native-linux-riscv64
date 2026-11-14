# Native Linux build context

## Source reconstruction details

The overlay supplies new source/build/synthetic-test paths and exact patches
against `1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b`.
Unchanged engine/vendor sources are obtained separately. The
[reconstruction manifest](../RECONSTRUCTION-MANIFEST.json) pins the origins,
original and reconstructed bytes, and reversible notice insertions.
File-local notices retain prior preparation where applicable and record the
actual November 14 snapshot/package date, without inventing first-edit dates
or independent authorship.

Inherited non-UTF8 patch context is transported as base64 in
`patches/native-byte-context.json`, without transcoding. Reversing the recorded
notice insertions restores accepted source after declared CRLF normalization.
Comments can change line-dependent diagnostics, debug locations or `__LINE__`
effects; no runtime equivalence is asserted by this packaging check.

The [reconstruction helper](../reconstruct.py) reads pinned Git objects,
disables lazy fetch and refuses an existing output destination. It does not
install dependencies, compile source, run the game or obtain game assets.
Reconstruction verification covers the selected source closure; the held RakNet
component described below still prevents a full accepted-engine build from
this package alone.

## Development build context

The accepted development checkpoint uses C++17, Clang/Clang++ 19, CMake 3.20+
and Make. Reuse existing Python 3, 7z/7zz, zlib, Expat, pthread and X11 development
dependencies. No installer or dependency binary is included in this overlay.
Exact external source pins are in `SOURCE_PIN.json`.

- SDL 3.4.20, commit `f00dc919ca35b9756316e86c4777392de442f9b2`, shared,
  ordinary linked X11 backend (`SDL_X11_SHARED=OFF`).
- FFmpeg 7.1.5, signed release archive SHA-256
  `de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f`;
  decoder-only shared build, network/autodetection/GPL/nonfree options disabled.
- FreeType 2.15.0, release archive SHA-256
  `93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5`.
  Publisher checksum verified; no signature verification claim.
- bfVFS, bundled libpng, Lua 5.1.5 and libsmacker come from the separately
  obtained pinned engine source and listed patches. bfVFS still supplies the
  live backend; isolated replacement experiments are not integrated here.

Provide reviewed local prefixes explicitly. The following describes the
accepted engine's configuration shape, but is **blocked for this package** by
the held RakNet headers/library; do not interpret it as a verified clean-room
build recipe or authorization to acquire/install dependencies:

```sh
cmake -S platform/linux -B /path/to/new-build -G 'Unix Makefiles' \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/pinned-sdl-prefix \
  -DP710_FFMPEG_ROOT=/path/to/pinned-ffmpeg-prefix \
  -DP710_FREETYPE_ROOT=/path/to/pinned-freetype-prefix \
  -DP710_RAKNET_ROOT=/path/to/reviewed-native-raknet-build
cmake --build /path/to/new-build --target p710_native_game --parallel 2
cmake --build /path/to/new-build --parallel 2
ctest --test-dir /path/to/new-build --output-on-failure
```

For the sanitizer lane, configure a separate new build with
`-DP710_SANITIZERS=ON` and matching instrumented external RakNet archive. The
recorded runs use ASan/UBSan with leak detection and no suppressions. SDL,
FFmpeg and FreeType SDKs were not themselves sanitizer-instrumented.
Display-dependent checks require an authorized X11 environment. This preparation
did not execute these commands. Original game data must be supplied separately
and kept private; no assets, profiles or saves are included.
