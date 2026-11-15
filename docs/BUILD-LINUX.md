# Native Linux build context

## Source reconstruction details

The overlay contains new source, build, and synthetic-test files.
It also contains exact patches for `1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b`.
Unchanged engine and vendor source files come from a separate copy.

The [reconstruction manifest](../RECONSTRUCTION-MANIFEST.json) records the source of each file.
It records original bytes, reconstructed bytes, and reversible notice insertions.
Some files keep notices from earlier preparation.
New file notices record the November 15 snapshot and package date.
Unchanged previously edited guides retain their exact accepted notice and editorial records.
They do not invent first-edit dates or independent authorship.

The package stores inherited non-UTF8 patch context as base64 in `patches/native-byte-context.json`.
It does not transcode these bytes.
The original package used reversible notice insertions.
Removal of those insertions restored the accepted source after the stated CRLF normalization.
The documentation rewrite has separate changed bytes.
Its new document hashes need separate records.

Comments can change diagnostic line numbers, debug locations, or `__LINE__` effects.
A source-reconstruction check does not prove equal runtime behavior after these changes.

The [reconstruction helper](../reconstruct.py) reads pinned Git objects.
It disables lazy fetch.
It refuses an output destination that already exists.
It does not install dependencies, compile source, run the game, or get game assets.

Reconstruction checks cover the selected source files.
The engine overlay needs the separate RakNet source companion for its complete source-input route.

The companion supplies these reviewed inputs:

- Engine `Multiplayer/raknet/BitStream.h` and `Multiplayer/raknet/RakMemoryOverride.h` adaptations.
- RakNet 3.401 source at `catacs/gameprojectx@a4c67c6c89b4e83b6d8455ac5e84719e288546f7`, component `Libs/RakNet/Source`.
- Its 79-source static-library recipe and eight native SDK adaptations.
- Component notices for Jenkins material, inherited AEScrypt material, Rand, and SHA1.

The eight SDK paths are `BitStream.h`, `DS_BinarySearchTree.h`, `FCMHost.cpp`, `PacketLogger.cpp`,
`RakMemoryOverride.h`, `RakPeer.cpp`, `Rand.cpp`, and `ThreadPool.h`.
The recipe includes AES, RSA, Rand, SHA1, and DataBlockEncryptor sources.
Source-package review must account for those actual inputs.
The corrected source companion has passed its targeted package review.
The [build record](BUILD-RESULTS.json) identifies the later clean-build result and remaining limits.

## Development build context

The accepted development checkpoint uses these tools:

- C++17
- Clang/Clang++ 19
- CMake 3.20+
- Make

The build also needs Python 3, 7z/7zz, zlib, Expat, pthread, and X11 development dependencies.
This overlay does not include an installer or dependency binary.
`SOURCE_PIN.json` gives the exact external source versions.

- SDL 3.4.20 uses commit `f00dc919ca35b9756316e86c4777392de442f9b2`.
  It is a shared build with the ordinary linked X11 backend.
  Its setting is `SDL_X11_SHARED=OFF`.
- FFmpeg 7.1.5 uses a signed release archive.
  Its SHA-256 is `de668509caf9e35e3cd162473441fdb29538c6d96ed080292b3cf9e6fc5d558f`.
  The shared build has decoders only.
  Network, autodetection, GPL, and nonfree options are disabled.
- FreeType 2.15.0 uses a release archive.
  Its SHA-256 is `93cd47eb719c325107f2d3cf087d06299e944c3b48a40b55537302304dea1dd5`.
  The publisher checksum matched.
  The record has no signature check.
- bfVFS, bundled libpng, Lua 5.1.5, and libsmacker come from the separate pinned engine source and listed patches.
  bfVFS still supplies the live backend.
  This snapshot does not include the isolated replacement experiments.

## Build the paired source

Download and extract both source archives from the release.
Reconstruct the engine into a new directory with the helper described above.
Copy the companion's `game-headers/` contents into the reconstructed engine's `Multiplayer/raknet/` directory.
Follow the companion README to verify its source and build `libraknet3401.a`.
Keep its required `sdk/packaging-notices.json` with `sdk/prepare.py`.
Use the new SDK build directory as `P710_RAKNET_ROOT`.
Run the following commands from the reconstructed engine directory.
Specify the matching local dependency prefixes.

```sh
cmake -S platform/linux -B /path/to/new-build -G 'Unix Makefiles' \
  -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
  -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/pinned-sdl-prefix \
  -DP710_FFMPEG_ROOT=/path/to/pinned-ffmpeg-prefix \
  -DP710_FREETYPE_ROOT=/path/to/pinned-freetype-prefix \
  -DP710_RAKNET_ROOT=/path/to/reviewed-native-raknet-build
cmake --build /path/to/new-build --target p710_native_game --parallel 2
cmake --build /path/to/new-build --parallel 2
cmake --build /path/to/new-build --target p710_native_profile101_tests --parallel 2
ctest --test-dir /path/to/new-build --output-on-failure
```

For the sanitizer lane, configure a separate new build with `-DP710_SANITIZERS=ON`.
Use the matching instrumented external RakNet archive.
The profile-test target is excluded from the default all-target build and needs the explicit command above.

The recorded runs use ASan/UBSan with leak detection and no suppressions.
The SDL, FFmpeg, and FreeType SDKs do not have sanitizer instrumentation.

Display tests need an X11 environment.
Use a separate test display to avoid interference with a desktop game.
The snapshot-preparation records describe the earlier packaging stage.
Use [BUILD-RESULTS.json](BUILD-RESULTS.json) for exact evidence scopes.
The successful prior package build predates the attachment guard.
The refreshed source has one guarded pointer condition and no new package build claim.
Owner-reported live results and pending acceptance remain separate.
The initial native attempt found a missing unchanged `i18n/text.def` reconstruction input.
The corrected package declares that exact pinned base file.
Native completion reused valid objects from its initial empty isolated build; the sanitizer lane started fresh.
The record preserves that distinction and the initial failure.
Original game data comes from a separate source.
Keep that data private.
This package does not include game assets, profiles, or saves.
