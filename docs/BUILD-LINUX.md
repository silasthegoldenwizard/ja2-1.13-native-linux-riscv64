# Linux build and authored demo

Run from the source root. Required existing tools are a C/C++17 compiler,
CMake 3.20 or later, Make, Python 3, a `7z` or `7zz` executable, zlib development
files, Expat development files, and **SDL3 3.4.20** with its CMake package and X11
backend. After reconstruction, bfVFS, UTF8-CPP, 7z C sources and libpng come from
the separately obtained pinned upstream; this overlay uploads only the listed
changes and retained notices. A system libpng is not the selected target.

The adopted SDL3 source is `libsdl-org/SDL` tag `release-3.4.20`, commit
`f00dc919ca35b9756316e86c4777392de442f9b2`. The verified shared build enables X11
and uses `SDL_X11_SHARED=OFF`, linking Xlib normally. Its source is unmodified.
Point `SDL3_PREFIX` at an existing installation of that pin. This candidate does
not download, install or rebuild dependencies automatically.

Use an empty build directory outside the source export:

```sh
export SDL3_PREFIX=/path/to/existing/SDL3-3.4.20/prefix
cmake -S tests/native-core -B ../native-debug -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Debug -DP710_SDL_PRESENTATION=ON \
  -DCMAKE_PREFIX_PATH="$SDL3_PREFIX"
cmake --build ../native-debug --parallel 4
PYTHONDONTWRITEBYTECODE=1 SDL_VIDEODRIVER=x11 \
  ctest --test-dir ../native-debug --output-on-failure
python3 -I -B tests/native-core/verify_video_demo.py \
  ../native-debug/artifacts/video-scene
```

The full suite runs the manager regression tests and the actual two-frame window
demo. Expected normal total: **15/15**. Exported `video-scene-first.bmp` and
`video-scene-second.bmp` are actual SDL window surfaces; corresponding `.rgb565`
files contain the padded source allocations. Each frame is 32x20 RGB565 with
72-byte rows, displayed at scale 8 in a 256x160 window. Only the authored yellow
8x4 rectangle changes in frame two. The independent script checks every source
byte, padding byte and displayed RGB pixel. It requires no fixture decoder or
external checkpoint file.

For a manual visible rerun after generating fixtures:

```sh
SDL_VIDEODRIVER=x11 ../native-debug/p710_native_video_viewer \
  ../native-debug/artifacts/rendering ../native-debug/artifacts/manual-video
python3 -I -B tests/native-core/verify_video_demo.py \
  ../native-debug/artifacts/manual-video
```

Optional sanitizer reproduction, using a second empty directory:

```sh
cmake -S tests/native-core -B ../native-sanitized -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE=Debug -DP710_SDL_PRESENTATION=ON -DP710_SANITIZERS=ON \
  -DCMAKE_PREFIX_PATH="$SDL3_PREFIX"
cmake --build ../native-sanitized --parallel 4
PYTHONDONTWRITEBYTECODE=1 SDL_VIDEODRIVER=x11 \
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
  ctest --test-dir ../native-sanitized --output-on-failure
python3 -I -B tests/native-core/verify_video_demo.py \
  ../native-debug/artifacts/video-scene ../native-sanitized/artifacts/video-scene
```

Expected sanitizer total: **17/17**, including two intentional ASan/UBSan
diagnostic canaries. Run in a host environment that permits leak detection,
with `LSAN_OPTIONS` unset and no suppressions. This does not claim that the
external SDL3 library itself was sanitizer-instrumented.

A reachable real X11 display is required. Dummy/offscreen drivers do not meet
demo acceptance. Python bytecode writes are disabled to keep the source export
unchanged. Fixtures, captures and logs are generated only below the build tree.
No CI service or proprietary assets are required. Existing compiler warnings
are visible; there is no blanket warning suppression.
