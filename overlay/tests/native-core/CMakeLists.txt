# P710 native notice preparation: tests/native-core/CMakeLists.txt
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add isolated native C++17 component, fixture, viewer and sanitizer test build.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/CMakeLists.txt
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

cmake_minimum_required(VERSION 3.20)
project(p710_native_core LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(P710_SANITIZERS "Instrument the isolated native tests with ASan and UBSan" OFF)
option(P710_VFS "Build the actual bundled VFS and production FileMan provider" ON)
option(P710_IMAGES "Build original STCI and scalar transparent sprite tests" ON)
option(P710_RESOURCE_MANAGERS "Build production image readers and video-object ownership" ON)
option(P710_SDL_PRESENTATION "Build the native SDL3 image presentation verifier" OFF)

enable_testing()
get_filename_component(P710_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

add_library(p710_raster OBJECT "${P710_SOURCE_ROOT}/sgp/line.cpp")
target_include_directories(p710_raster PUBLIC "${P710_SOURCE_ROOT}/sgp")
add_executable(p710_native_raster_tests raster_tests.cpp)
target_link_libraries(p710_native_raster_tests PRIVATE p710_raster)
add_library(p710_format STATIC "${P710_SOURCE_ROOT}/sgp/NativeFormat.cpp")
target_include_directories(p710_format PUBLIC "${P710_SOURCE_ROOT}/sgp")
add_executable(p710_native_format_tests native_format_tests.cpp)
target_link_libraries(p710_native_format_tests PRIVATE p710_format)
add_test(NAME text.native_format_contracts COMMAND p710_native_format_tests)
add_executable(p710_native_scalar_portability_tests native_scalar_portability_tests.cpp)
target_include_directories(p710_native_scalar_portability_tests PRIVATE "${P710_SOURCE_ROOT}/sgp")
add_test(NAME engine.native_scalar_contracts COMMAND p710_native_scalar_portability_tests)
add_executable(p710_native_legacy_math_tests native_legacy_math_tests.cpp)
target_include_directories(p710_native_legacy_math_tests PRIVATE "${P710_SOURCE_ROOT}/sgp")
add_test(NAME engine.native_fixed_point_contracts COMMAND p710_native_legacy_math_tests)
add_library(p710_clock OBJECT "${P710_SOURCE_ROOT}/sgp/NativeClock.cpp")
target_include_directories(p710_clock PUBLIC "${P710_SOURCE_ROOT}/sgp")
add_executable(p710_native_clock_tests native_clock_tests.cpp)
target_link_libraries(p710_native_clock_tests PRIVATE p710_clock)
add_test(NAME clock.sgp_monotonic COMMAND p710_native_clock_tests)

find_package(Threads REQUIRED)
add_library(p710_fileman OBJECT "${P710_SOURCE_ROOT}/sgp/FileManIO.cpp")
target_include_directories(p710_fileman PUBLIC "${P710_SOURCE_ROOT}/sgp")
target_link_libraries(p710_fileman PUBLIC Threads::Threads)
add_executable(p710_native_fileman_tests fileman_tests.cpp)
target_link_libraries(p710_native_fileman_tests PRIVATE p710_fileman)

set(P710_NATIVE_TARGETS p710_raster p710_native_raster_tests
  p710_fileman p710_native_fileman_tests p710_format p710_native_format_tests
  p710_clock p710_native_clock_tests)
list(APPEND P710_NATIVE_TARGETS p710_native_scalar_portability_tests)
list(APPEND P710_NATIVE_TARGETS p710_native_legacy_math_tests)
set(P710_NATIVE_EXECUTABLES p710_native_raster_tests p710_native_fileman_tests p710_native_format_tests p710_native_clock_tests)
list(APPEND P710_NATIVE_EXECUTABLES p710_native_scalar_portability_tests)
list(APPEND P710_NATIVE_EXECUTABLES p710_native_legacy_math_tests)
add_library(p710_game_timer OBJECT "${P710_SOURCE_ROOT}/Utils/native_game_timer.cpp")
target_include_directories(p710_game_timer PUBLIC "${P710_SOURCE_ROOT}/Utils")
target_link_libraries(p710_game_timer PUBLIC Threads::Threads)
add_executable(p710_native_game_timer_tests native_game_timer_tests.cpp)
target_link_libraries(p710_native_game_timer_tests PRIVATE p710_game_timer Threads::Threads)
list(APPEND P710_NATIVE_TARGETS p710_game_timer p710_native_game_timer_tests)
list(APPEND P710_NATIVE_EXECUTABLES p710_native_game_timer_tests)
add_test(NAME clock.original_gameplay_core COMMAND p710_native_game_timer_tests)
if(P710_VFS)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  find_program(P710_7Z_EXECUTABLE NAMES 7z 7zz REQUIRED)
  add_subdirectory("${P710_SOURCE_ROOT}/ext/VFS" "${CMAKE_CURRENT_BINARY_DIR}/vfs")
  add_library(p710_fileman_vfs OBJECT "${P710_SOURCE_ROOT}/sgp/FileManVFS.cpp")
  target_include_directories(p710_fileman_vfs PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_fileman_vfs PUBLIC bfVFS)
  add_executable(p710_native_vfs_tests native_vfs_tests.cpp)
  target_link_libraries(p710_native_vfs_tests PRIVATE p710_fileman p710_fileman_vfs bfVFS)
  list(APPEND P710_NATIVE_TARGETS p710_fileman_vfs p710_native_vfs_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_vfs_tests)
  add_library(p710_debug OBJECT "${P710_SOURCE_ROOT}/sgp/NativeDebug.cpp")
  target_include_directories(p710_debug PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_debug PUBLIC bfVFS Threads::Threads)
  add_executable(p710_native_debug_tests native_debug_tests.cpp)
  target_link_libraries(p710_native_debug_tests PRIVATE p710_debug bfVFS Threads::Threads)
  list(APPEND P710_NATIVE_TARGETS p710_debug p710_native_debug_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_debug_tests)
  add_test(NAME debug.native_contracts COMMAND p710_native_debug_tests)
  add_library(p710_memory OBJECT "${P710_SOURCE_ROOT}/sgp/MemMan.cpp")
  target_include_directories(p710_memory PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_memory PUBLIC p710_debug bfVFS Threads::Threads)
  add_executable(p710_native_memory_manager_tests native_memory_manager_tests.cpp)
  target_link_libraries(p710_native_memory_manager_tests PRIVATE p710_memory p710_debug bfVFS Threads::Threads)
  list(APPEND P710_NATIVE_TARGETS p710_memory p710_native_memory_manager_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_memory_manager_tests)
  add_test(NAME memory.native_manager_contracts COMMAND p710_native_memory_manager_tests)
  add_library(p710_fileman_services OBJECT "${P710_SOURCE_ROOT}/sgp/NativeFileMan.cpp")
  target_include_directories(p710_fileman_services PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_fileman_services PUBLIC bfVFS)
  add_executable(p710_native_fileman_metadata_tests native_fileman_metadata_tests.cpp)
  target_link_libraries(p710_native_fileman_metadata_tests PRIVATE p710_fileman_services
    p710_fileman p710_fileman_vfs bfVFS)
  list(APPEND P710_NATIVE_TARGETS p710_fileman_services p710_native_fileman_metadata_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_fileman_metadata_tests)
  add_test(NAME fileman.native_metadata COMMAND p710_native_fileman_metadata_tests)
  add_library(p710_text STATIC "${P710_SOURCE_ROOT}/sgp/NativeText.cpp")
  target_include_directories(p710_text PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_text PUBLIC bfVFS)
  target_link_libraries(p710_fileman_services PUBLIC p710_text)
  add_executable(p710_native_text_tests native_text_tests.cpp)
  target_link_libraries(p710_native_text_tests PRIVATE p710_text bfVFS)
  list(APPEND P710_NATIVE_TARGETS p710_text p710_native_text_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_text_tests)
  add_test(NAME text.native_unicode_resources COMMAND p710_native_text_tests)
  add_library(p710_ini OBJECT "${P710_SOURCE_ROOT}/Utils/INIReader.cpp")
  target_include_directories(p710_ini PUBLIC "${P710_SOURCE_ROOT}/Utils" "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_ini PUBLIC bfVFS)
  add_executable(p710_native_ini_tests native_ini_tests.cpp)
  target_link_libraries(p710_native_ini_tests PRIVATE p710_ini bfVFS)
  list(APPEND P710_NATIVE_TARGETS p710_ini p710_native_ini_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_ini_tests)
  add_test(NAME configuration.original_ini COMMAND p710_native_ini_tests)
endif()
if(P710_IMAGES)
  if(NOT P710_VFS)
    message(FATAL_ERROR "Image verification requires the actual VFS provider")
  endif()
  find_package(ZLIB REQUIRED)
  add_library(p710_images OBJECT "${P710_SOURCE_ROOT}/sgp/STCI.cpp"
    "${P710_SOURCE_ROOT}/sgp/ImageData.cpp" "${P710_SOURCE_ROOT}/sgp/ImagePalette.cpp"
    "${P710_SOURCE_ROOT}/sgp/etrle_blitters.cpp" "${P710_SOURCE_ROOT}/sgp/clip_state.cpp")
  target_include_directories(p710_images PUBLIC "${P710_SOURCE_ROOT}/sgp")
  target_link_libraries(p710_images PUBLIC ZLIB::ZLIB)
  add_executable(p710_native_image_tests image_tests.cpp)
  target_link_libraries(p710_native_image_tests PRIVATE p710_images p710_fileman p710_fileman_vfs bfVFS)
  list(APPEND P710_NATIVE_TARGETS p710_images p710_native_image_tests)
  list(APPEND P710_NATIVE_EXECUTABLES p710_native_image_tests)
  if(P710_RESOURCE_MANAGERS)
    find_package(EXPAT REQUIRED)
    add_subdirectory("${P710_SOURCE_ROOT}/ext/libpng" "${CMAKE_CURRENT_BINARY_DIR}/libpng")
    add_library(p710_resources OBJECT
      "${P710_SOURCE_ROOT}/sgp/ImageResource.cpp" "${P710_SOURCE_ROOT}/sgp/PCX.cpp"
      "${P710_SOURCE_ROOT}/sgp/impTGA.cpp" "${P710_SOURCE_ROOT}/sgp/PngLoader.cpp"
      "${P710_SOURCE_ROOT}/sgp/VideoObjectCore.cpp" "${P710_SOURCE_ROOT}/sgp/ShadingTables.cpp")
    target_include_directories(p710_resources PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_resources PUBLIC p710_images bfVFS libpng EXPAT::EXPAT)
    foreach(slice resource png object)
      if(slice STREQUAL "resource")
        set(driver image_resource_tests.cpp)
      elseif(slice STREQUAL "png")
        set(driver png_loader_tests.cpp)
      else()
        set(driver video_object_tests.cpp)
      endif()
      add_executable(p710_native_${slice}_tests "${driver}")
      target_link_libraries(p710_native_${slice}_tests PRIVATE p710_resources p710_images
        p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_${slice}_tests)
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_${slice}_tests)
    endforeach()
    list(APPEND P710_NATIVE_TARGETS p710_resources)
    include("${P710_SOURCE_ROOT}/platform/linux/FreeType.cmake")
    add_library(p710_scalable STATIC "${P710_SOURCE_ROOT}/sgp/NativeScalableFont.cpp")
    target_include_directories(p710_scalable PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_scalable PUBLIC Freetype::Freetype)
    list(APPEND P710_NATIVE_TARGETS p710_scalable)
    add_library(p710_render OBJECT "${P710_SOURCE_ROOT}/sgp/NativeSurface.cpp"
      "${P710_SOURCE_ROOT}/sgp/NativeFont.cpp")
    target_include_directories(p710_render PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_render PUBLIC p710_resources p710_format p710_scalable)
    add_executable(p710_native_render_tests native_render_tests.cpp)
    target_link_libraries(p710_native_render_tests PRIVATE p710_render p710_resources
      p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_render p710_native_render_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_render_tests)
    add_executable(p710_native_surface_resource_tests native_surface_resource_tests.cpp)
    target_link_libraries(p710_native_surface_resource_tests PRIVATE p710_render p710_resources
      p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_surface_resource_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_surface_resource_tests)
    add_executable(p710_native_indexed_surface_tests native_indexed_surface_tests.cpp)
    target_link_libraries(p710_native_indexed_surface_tests PRIVATE p710_render p710_resources
      p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_indexed_surface_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_indexed_surface_tests)
    add_test(NAME rendering.native_indexed_surfaces COMMAND p710_native_indexed_surface_tests)
    add_library(p710_background OBJECT "${P710_SOURCE_ROOT}/TileEngine/native_background.cpp")
    target_include_directories(p710_background PUBLIC "${P710_SOURCE_ROOT}/TileEngine" "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_background PUBLIC p710_render)
    add_executable(p710_native_background_tests native_background_tests.cpp)
    target_link_libraries(p710_native_background_tests PRIVATE p710_background p710_render p710_resources
      p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_background p710_native_background_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_background_tests)
    add_test(NAME rendering.original_background_buffers COMMAND p710_native_background_tests)
    add_executable(p710_native_scalable_font_tests native_scalable_font_tests.cpp)
    target_link_libraries(p710_native_scalable_font_tests PRIVATE p710_scalable p710_render
      p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    add_executable(p710_native_font_replacement_tests native_font_replacement_tests.cpp)
    target_link_libraries(p710_native_font_replacement_tests PRIVATE p710_scalable p710_render
      p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_scalable_font_tests p710_native_font_replacement_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_scalable_font_tests p710_native_font_replacement_tests)
    set(P710_PUBLIC_REGULAR_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" CACHE FILEPATH "Existing public TTF test input")
    set(P710_PUBLIC_BOLD_FONT "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" CACHE FILEPATH "Existing public bold TTF test input")
    add_test(NAME fonts.native_scalable_core COMMAND p710_native_scalable_font_tests
      "${P710_PUBLIC_REGULAR_FONT}" "${P710_PUBLIC_BOLD_FONT}")
    add_test(NAME fonts.original_replacement_selection COMMAND p710_native_font_replacement_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering" "${P710_PUBLIC_REGULAR_FONT}")
    set_tests_properties(fonts.original_replacement_selection PROPERTIES FIXTURES_REQUIRED render_inputs)

    add_library(p710_world_blitters OBJECT "${P710_SOURCE_ROOT}/sgp/NativeWorldBlitters.cpp")
    target_link_libraries(p710_world_blitters PUBLIC p710_background p710_render)
    add_executable(p710_native_world_blitter_tests native_world_blitter_tests.cpp)
    target_link_libraries(p710_native_world_blitter_tests PRIVATE p710_world_blitters p710_background
      p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_world_blitters p710_native_world_blitter_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_world_blitter_tests)
    add_test(NAME rendering.original_world_z_kernels COMMAND p710_native_world_blitter_tests)
    # Real portable replacements for original x86 rendering kernels.
    add_library(p710_raw_blitters OBJECT "${P710_SOURCE_ROOT}/sgp/NativeRawBlitters.cpp")
    add_library(p710_shadow_blitters OBJECT "${P710_SOURCE_ROOT}/sgp/NativeShadowBlitters.cpp")
    add_library(p710_effect_blitters OBJECT "${P710_SOURCE_ROOT}/sgp/NativeEffectBlitters.cpp")
    add_library(p710_tile_blitters OBJECT "${P710_SOURCE_ROOT}/TileEngine/NativeTileBlitters.cpp")
    foreach(provider raw shadow effect tile)
      target_link_libraries(p710_${provider}_blitters PUBLIC p710_background p710_render)
      list(APPEND P710_NATIVE_TARGETS p710_${provider}_blitters)
    endforeach()
    foreach(kind raw shadow effect tile)
      add_executable(p710_native_${kind}_blitters_tests native_${kind}_blitters_tests.cpp)
      target_link_libraries(p710_native_${kind}_blitters_tests PRIVATE
        p710_${kind}_blitters p710_raw_blitters p710_world_blitters p710_background
        p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_${kind}_blitters_tests)
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_${kind}_blitters_tests)
      add_test(NAME rendering.original_${kind}_kernels COMMAND p710_native_${kind}_blitters_tests)
    endforeach()
    add_library(p710_scroll STATIC "${P710_SOURCE_ROOT}/sgp/NativeScroll.cpp")
    target_link_libraries(p710_scroll PUBLIC p710_background)
    add_executable(p710_native_scroll_tests native_scroll_tests.cpp)
    target_link_libraries(p710_native_scroll_tests PRIVATE p710_scroll p710_background
      p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_scroll p710_native_scroll_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_scroll_tests)
    add_test(NAME video.original_scroll_geometry COMMAND p710_native_scroll_tests)
  endif()
  if(P710_SDL_PRESENTATION)
    if(NOT P710_RESOURCE_MANAGERS)
      message(FATAL_ERROR "Native presentation now exercises actual resource managers")
    endif()
    find_package(SDL3 3.4.20 EXACT REQUIRED CONFIG)
    include("${P710_SOURCE_ROOT}/platform/linux/FFmpeg.cmake")
    add_library(p710_audio_decoder STATIC "${P710_SOURCE_ROOT}/sgp/NativeAudioDecoder.cpp")
    target_include_directories(p710_audio_decoder PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_audio_decoder PUBLIC FFmpeg::avformat FFmpeg::avcodec
      FFmpeg::avutil FFmpeg::swresample)
    list(APPEND P710_NATIVE_TARGETS p710_audio_decoder)
    add_library(p710_sound OBJECT "${P710_SOURCE_ROOT}/sgp/NativeSoundMan.cpp")
    target_include_directories(p710_sound PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_sound PUBLIC SDL3::SDL3 bfVFS Threads::Threads p710_audio_decoder)
    add_executable(p710_native_sound_tests native_sound_tests.cpp)
    target_link_libraries(p710_native_sound_tests PRIVATE p710_sound p710_fileman
      p710_fileman_vfs p710_fileman_services bfVFS Threads::Threads)
    list(APPEND P710_NATIVE_TARGETS p710_sound p710_native_sound_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_sound_tests)
    add_test(NAME audio.native_sound_contracts COMMAND p710_native_sound_tests)
    set_tests_properties(audio.native_sound_contracts PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(p710_native_audio_decoder_tests native_audio_decoder_tests.cpp)
    target_link_libraries(p710_native_audio_decoder_tests PRIVATE p710_sound p710_audio_decoder
      p710_fileman p710_fileman_vfs p710_fileman_services bfVFS Threads::Threads)
    list(APPEND P710_NATIVE_TARGETS p710_native_audio_decoder_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_audio_decoder_tests)
    add_test(NAME audio.native_codec_streams COMMAND p710_native_audio_decoder_tests /usr/bin/ffmpeg)
    set_tests_properties(audio.native_codec_streams PROPERTIES
      ENVIRONMENT "SDL_AUDIODRIVER=dummy" TIMEOUT 30)
    add_library(p710_presentation OBJECT "${P710_SOURCE_ROOT}/sgp/SDLPresentation.cpp")
    target_include_directories(p710_presentation PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_presentation PUBLIC SDL3::SDL3 p710_images)
    add_executable(p710_native_image_viewer image_viewer.cpp)
    target_link_libraries(p710_native_image_viewer PRIVATE p710_presentation p710_images
      p710_resources p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_presentation p710_native_image_viewer)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_image_viewer)
    add_executable(p710_native_render_viewer native_render_viewer.cpp)
    target_link_libraries(p710_native_render_viewer PRIVATE p710_presentation p710_render
      p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_render_viewer)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_render_viewer)
    add_library(p710_video OBJECT "${P710_SOURCE_ROOT}/sgp/NativeVideo.cpp")
    target_link_libraries(p710_video PUBLIC p710_presentation p710_render p710_scroll p710_background)
    foreach(kind tests viewer)
      add_executable(p710_native_video_${kind} native_video_${kind}.cpp)
      target_link_libraries(p710_native_video_${kind} PRIVATE p710_video p710_presentation
        p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_video_${kind})
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_video_${kind})
    endforeach()
    list(APPEND P710_NATIVE_TARGETS p710_video)
    add_executable(p710_native_scroll_refresh_tests native_scroll_refresh_tests.cpp)
    target_link_libraries(p710_native_scroll_refresh_tests PRIVATE p710_video p710_presentation
      p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_scroll_refresh_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_scroll_refresh_tests)
    add_test(NAME video.scroll_refresh_rollback COMMAND p710_native_scroll_refresh_tests)
    set_tests_properties(video.scroll_refresh_rollback PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_library(p710_input OBJECT "${P710_SOURCE_ROOT}/sgp/NativeInput.cpp"
      "${P710_SOURCE_ROOT}/sgp/English.cpp")
    # The retained types.h uses the MS spelling; do not modify that header/table.
    target_compile_definitions(p710_input PUBLIC "__int64=long long")
    target_link_libraries(p710_input PUBLIC p710_video bfVFS)
    foreach(kind tests viewer)
      add_executable(p710_native_input_${kind} native_input_${kind}.cpp)
      target_link_libraries(p710_native_input_${kind} PRIVATE p710_input p710_video
        p710_presentation p710_render p710_resources p710_images
        p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_input_${kind})
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_input_${kind})
    endforeach()
    list(APPEND P710_NATIVE_TARGETS p710_input)
    # The original cursor database/controller now uses the real native bitmap
    # and SDL color cursor APIs. Compile it once for fixtures and the engine.
    add_library(p710_cursor OBJECT "${P710_SOURCE_ROOT}/sgp/Cursor Control.cpp")
    target_link_libraries(p710_cursor PUBLIC p710_video p710_render bfVFS)
    add_executable(p710_native_cursor_tests native_cursor_tests.cpp)
    target_link_libraries(p710_native_cursor_tests PRIVATE p710_cursor p710_input
      p710_video p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_cursor p710_native_cursor_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_cursor_tests)
    add_test(NAME input.original_cursor_contracts COMMAND p710_native_cursor_tests)
    set_tests_properties(input.original_cursor_contracts PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_test(NAME input.cursor_offscreen_backend COMMAND p710_native_cursor_tests)
    set_tests_properties(input.cursor_offscreen_backend PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    add_library(p710_ui OBJECT "${P710_SOURCE_ROOT}/sgp/NativeMouseSystem.cpp"
      "${P710_SOURCE_ROOT}/sgp/NativeTooltipText.cpp"
      "${P710_SOURCE_ROOT}/sgp/NativeButtons.cpp"
      "${P710_SOURCE_ROOT}/sgp/NativeButtonSounds.cpp")
    target_link_libraries(p710_ui PUBLIC p710_input p710_video p710_render bfVFS)
    foreach(kind tests viewer)
      add_executable(p710_native_mouse_button_${kind} native_mouse_button_${kind}.cpp)
      target_link_libraries(p710_native_mouse_button_${kind} PRIVATE p710_ui p710_input
        p710_video p710_presentation p710_render p710_resources p710_images
        p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_mouse_button_${kind})
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_mouse_button_${kind})
    endforeach()
    list(APPEND P710_NATIVE_TARGETS p710_ui)
    add_executable(p710_native_ui_payload_tests native_ui_payload_tests.cpp)
    target_link_libraries(p710_native_ui_payload_tests PRIVATE p710_ui p710_input
      p710_video p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_ui_payload_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_ui_payload_tests)
    add_test(NAME ui.pointer_payload_and_cursor_dispatch COMMAND p710_native_ui_payload_tests)
    add_executable(p710_native_button_sound_tests native_button_sound_tests.cpp)
    target_link_libraries(p710_native_button_sound_tests PRIVATE p710_ui p710_sound p710_input p710_video
      p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_button_sound_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_button_sound_tests)
    add_test(NAME ui.button_sound_contracts COMMAND p710_native_button_sound_tests)
    set_tests_properties(ui.button_sound_contracts PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    add_executable(p710_native_font_layout_tests native_font_layout_tests.cpp)
    target_link_libraries(p710_native_font_layout_tests PRIVATE p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_font_layout_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_font_layout_tests)
    add_test(NAME text.original_font_layout COMMAND p710_native_font_layout_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering")
    set_tests_properties(text.original_font_layout PROPERTIES FIXTURES_REQUIRED render_inputs)
    add_executable(p710_native_generic_button_tests native_generic_button_tests.cpp)
    target_link_libraries(p710_native_generic_button_tests PRIVATE p710_ui p710_input p710_video
      p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_generic_button_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_generic_button_tests)
    add_test(NAME ui.generate_generic_fixtures COMMAND "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_generic_button_fixtures.py"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/generic-buttons")
    set_tests_properties(ui.generate_generic_fixtures PROPERTIES FIXTURES_SETUP generic_button_inputs)
    add_test(NAME ui.original_generic_buttons COMMAND p710_native_generic_button_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/generic-buttons")
    set_tests_properties(ui.original_generic_buttons PROPERTIES FIXTURES_REQUIRED generic_button_inputs)
    add_executable(p710_native_game_mouse_tests native_game_mouse_tests.cpp)
    target_link_libraries(p710_native_game_mouse_tests PRIVATE p710_ui p710_input p710_video
      p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_game_mouse_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_game_mouse_tests)
    add_test(NAME input.original_game_mouse_bridge COMMAND p710_native_game_mouse_tests)
    set_tests_properties(input.original_game_mouse_bridge PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_executable(p710_native_button_image_tests native_button_image_tests.cpp)
    target_link_libraries(p710_native_button_image_tests PRIVATE p710_ui p710_input
      p710_video p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_button_image_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_button_image_tests)
    add_test(NAME ui.generate_fixtures COMMAND "${Python3_EXECUTABLE}" -B
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_button_fixtures.py"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons")
    set_tests_properties(ui.generate_fixtures PROPERTIES FIXTURES_SETUP button_inputs)
    add_test(NAME ui.shared_button_images COMMAND p710_native_button_image_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons")
    set_tests_properties(ui.shared_button_images PROPERTIES FIXTURES_REQUIRED button_inputs)
    add_executable(p710_native_menu_api_tests native_menu_api_tests.cpp)
    target_link_libraries(p710_native_menu_api_tests PRIVATE p710_ui p710_input p710_video
      p710_presentation p710_render p710_resources p710_images
      p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_native_menu_api_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_menu_api_tests)
    foreach(kind checkbox fast_help)
      add_executable(p710_native_${kind}_tests native_${kind}_tests.cpp)
      target_link_libraries(p710_native_${kind}_tests PRIVATE p710_ui p710_input p710_video
        p710_presentation p710_render p710_resources p710_images
        p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_${kind}_tests)
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_${kind}_tests)
    endforeach()
    add_test(NAME ui.checkbox_contracts COMMAND p710_native_checkbox_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons")
    set_tests_properties(ui.checkbox_contracts PROPERTIES FIXTURES_REQUIRED button_inputs)
    add_test(NAME ui.fast_help_contracts COMMAND p710_native_fast_help_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering" "${P710_PUBLIC_REGULAR_FONT}" "${P710_PUBLIC_BOLD_FONT}")
    set_tests_properties(ui.fast_help_contracts PROPERTIES FIXTURES_REQUIRED render_inputs)
    add_test(NAME ui.original_menu_apis COMMAND p710_native_menu_api_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons")
    set_tests_properties(ui.original_menu_apis PROPERTIES FIXTURES_REQUIRED button_inputs)
    add_test(NAME ui.mouse_button_contracts COMMAND p710_native_mouse_button_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons")
    add_test(NAME ui.button_scene COMMAND p710_native_mouse_button_viewer
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/buttons"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/button-scene")
    set_tests_properties(ui.mouse_button_contracts ui.button_scene PROPERTIES
      FIXTURES_REQUIRED button_inputs ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_test(NAME input.manager_contracts COMMAND p710_native_input_tests)
    add_test(NAME input.authored_scene COMMAND p710_native_input_viewer
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/input-scene" --scripted)
    set_tests_properties(input.manager_contracts input.authored_scene PROPERTIES
      ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_test(NAME video.manager_contracts COMMAND p710_native_video_tests)
    set_tests_properties(video.manager_contracts PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")
    add_test(NAME video.two_frame_scene COMMAND p710_native_video_viewer
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/video-scene")
    set_tests_properties(video.two_frame_scene PROPERTIES
      FIXTURES_REQUIRED render_inputs ENVIRONMENT "SDL_VIDEODRIVER=x11")
  endif()
endif()
if(P710_RESOURCE_MANAGERS AND NOT P710_IMAGES)
  message(FATAL_ERROR "Production resource managers require image support")
endif()
foreach(target ${P710_NATIVE_TARGETS})
  target_compile_options(${target} PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wformat=2 -Wcast-align)
endforeach()

if(P710_SANITIZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "The isolated sanitizer option requires GCC or Clang")
  endif()
  foreach(target ${P710_NATIVE_TARGETS})
    target_compile_options(${target} PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  endforeach()
  if(P710_VFS)
    foreach(target bfVFS 7z)
      target_compile_options(${target} PRIVATE
        -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    endforeach()
  endif()
  if(P710_RESOURCE_MANAGERS)
    target_compile_options(libpng PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  endif()
  foreach(target ${P710_NATIVE_EXECUTABLES})
    target_link_options(${target} PRIVATE
      -fsanitize=address,undefined -fno-sanitize-recover=all)
  endforeach()

  add_executable(p710_sanitizer_probe sanitizer_probe.cpp)
  target_compile_options(p710_sanitizer_probe PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_sanitizer_probe PRIVATE
    -fsanitize=address,undefined -fno-sanitize-recover=all)
  foreach(kind asan ubsan)
    add_test(NAME sanitizer.${kind}.diagnostic
      COMMAND "${CMAKE_COMMAND}" "-DPROBE=$<TARGET_FILE:p710_sanitizer_probe>"
        "-DKIND=${kind}" "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/artifacts"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/verify_sanitizer.cmake")
  endforeach()
endif()
if(P710_IMAGES)
  add_test(NAME images.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/generate_image_fixtures.py"
    "${CMAKE_CURRENT_BINARY_DIR}/artifacts/images")
  set_tests_properties(images.generate_fixtures PROPERTIES FIXTURES_SETUP image_inputs)
  add_test(NAME images.resource_and_pixels COMMAND p710_native_image_tests
    "${CMAKE_CURRENT_BINARY_DIR}/artifacts/images")
  set_tests_properties(images.resource_and_pixels PROPERTIES FIXTURES_REQUIRED image_inputs)
  if(P710_RESOURCE_MANAGERS)
    add_test(NAME rendering.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_render_fixtures.py"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering")
    set_tests_properties(rendering.generate_fixtures PROPERTIES FIXTURES_SETUP render_inputs)
    add_test(NAME rendering.production_interfaces COMMAND p710_native_render_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/rendering")
    set_tests_properties(rendering.production_interfaces PROPERTIES FIXTURES_REQUIRED render_inputs)
    add_test(NAME resources.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_resource_fixtures.py"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/resources")
    set_tests_properties(resources.generate_fixtures PROPERTIES FIXTURES_SETUP resource_inputs)
    add_test(NAME resources.image_api COMMAND p710_native_resource_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/resources")
    set_tests_properties(resources.image_api PROPERTIES FIXTURES_REQUIRED resource_inputs)
    add_test(NAME rendering.menu_surface_resources COMMAND p710_native_surface_resource_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/resources"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/png"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/images")
    set_tests_properties(rendering.menu_surface_resources PROPERTIES
      FIXTURES_REQUIRED "resource_inputs;png_inputs;image_inputs")
    add_test(NAME png.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/generate_png_fixtures.py"
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/png" --seven-zip "${P710_7Z_EXECUTABLE}")
    set_tests_properties(png.generate_fixtures PROPERTIES FIXTURES_SETUP png_inputs)
    add_test(NAME png.production_loaders COMMAND p710_native_png_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/png")
    set_tests_properties(png.production_loaders PROPERTIES FIXTURES_REQUIRED png_inputs)
    add_test(NAME objects.production_ownership COMMAND p710_native_object_tests
      "${CMAKE_CURRENT_BINARY_DIR}/artifacts/images")
    set_tests_properties(objects.production_ownership PROPERTIES FIXTURES_REQUIRED image_inputs)
  endif()
endif()

add_test(NAME raster.byte_fixtures COMMAND p710_native_raster_tests
  --output "${CMAKE_CURRENT_BINARY_DIR}/artifacts")
add_test(NAME fileman.handle_contracts COMMAND p710_native_fileman_tests
  "${CMAKE_CURRENT_BINARY_DIR}/artifacts/fileman")
if(P710_VFS)
  add_test(NAME vfs.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/generate_vfs_fixtures.py"
    "${CMAKE_CURRENT_BINARY_DIR}/artifacts/vfs" --seven-zip "${P710_7Z_EXECUTABLE}")
  set_tests_properties(vfs.generate_fixtures PROPERTIES FIXTURES_SETUP vfs_inputs)
  add_test(NAME vfs.production_provider COMMAND p710_native_vfs_tests
    "${CMAKE_CURRENT_BINARY_DIR}/artifacts/vfs")
  set_tests_properties(vfs.production_provider PROPERTIES FIXTURES_REQUIRED vfs_inputs)
endif()
