# Jagged Alliance 2 v1.13 for native Linux

An experimental native Linux port of [1dot13's](https://github.com/1dot13) [JA2 v1.13](https://github.com/1dot13/source) engine.
The goal is to preserve v1.13 gameplay and support Linux x86-64, followed by RISC-V64.

## For testers

Use this source preview for technical validation, source reconstruction, and native component tests.
It does not provide a complete game installation.

### Current status

- The downloadable release is [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1), a source preview from November 15, 2026.
- The [November 19 source checkpoint](docs/CHANGELOG.md#november-19-source-checkpoints) on `main` handles empty roster slots at battle end and during world teardown.
- The usable-mercenary scan and pre-battle wake-up include the final configured player slot. The scripted SDL/X11 button viewer drains stray input before each step.
- Original game assets, private profiles, saves, and prebuilt game binaries are separate from this source preview.

### What this repository is useful for

You can use this repository to:

- Rebuild the native Linux source from the pinned public base and overlay.
- Verify source reconstruction and package integrity.
- Run native component tests.
- Inspect sanitizer and regression coverage for selected subsystems.
- Evaluate the port before broader gameplay testing.

This preview does not provide an immediate game launch or a complete end-user Linux installation.
Do not treat the current port as proven gameplay-complete.

### What the tests cover

The latest published source checkpoint passed 268 native tests and 270 strict sanitizer tests in development.
The suites ran serially on a real X11 display. The strict build used AddressSanitizer, UndefinedBehaviorSanitizer, and LeakSanitizer.

Focused synthetic tests use controlled inputs and test doubles. Production-linked regressions run selected engine paths with controlled state.
The [hosted checks](docs/CI.md) cover source reconstruction, selected native components, source checks, and the attachment fixture.
Live strategic gameplay, full campaign testing, Windows, and native RISC-V validation remain open. See the [validation status](docs/STATUS.md#validation).

### Prerequisites

For the documented development build, use:

- A recent Linux environment.
- Git, Python 3, and 7z/7zz.
- A C++17 toolchain with Clang/Clang++ 19.
- CMake 3.20+ and Make.
- zlib, Expat, pthread, and X11 development dependencies.
- SDL3, FFmpeg, and FreeType development SDKs with the versions and configurations in the selected snapshot's [source pins](SOURCE_PIN.json).
- The separately published RakNet 3.401 source companion.

The overlay does not include an installer or dependency binaries.
Obtain original JA2 game data separately before runtime or gameplay testing.
Keep game data, private profiles, and saves private.

### Setup and build

Use one engine snapshot throughout reconstruction and compilation.
Replace the example paths below with your local paths.

1. Select the engine overlay from the release or this repository at a fixed commit.
   Download the RakNet 3.401 companion from the [release](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).
   Verify release archives against the release's `SHA256SUMS`.
   Use the reconstruction files and build guide from your selected engine snapshot.

2. Get the public upstream Git objects and reconstruct the engine into a new, absent directory.
   The helper verifies the pinned inputs and reconstructed file hashes.
   It does not download missing objects or game data.

   ```sh
   git clone --bare https://github.com/1dot13/source /path/to/ja2-base.git
   python3 /path/to/engine-overlay/reconstruct.py \
     --base-repo /path/to/ja2-base.git --output /path/to/new-engine
   ```

3. Copy the companion's `game-headers/` contents into the reconstructed engine's `Multiplayer/raknet/` directory.

4. Follow the companion README to verify its source and build `libraknet3401.a`.
   Keep its required `sdk/packaging-notices.json` with `sdk/prepare.py`.
   Use the SDK build directory as `P710_RAKNET_ROOT`.

5. Configure and build from the reconstructed engine directory.
   Specify the matching local dependency prefixes.

   ```sh
   cd /path/to/new-engine
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

Display tests need an X11 environment.
Use a separate test display to avoid interference with a desktop game.
See the [Linux build guide](docs/BUILD-LINUX.md) for dependency pins, sanitizer setup, and exact validation limits.
The build and test commands do not install or configure original game data.

## Project information

- [Release status, known issues, and validation cutoffs](docs/STATUS.md)
- [Changes](docs/CHANGELOG.md)
- [Component terms](docs/PUBLICATION-LIMITS.md) and [notices](notices/)

The engine and third-party components retain their individual terms.
Read the component terms before reuse or redistribution.
