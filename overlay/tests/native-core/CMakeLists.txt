# P710 native notice preparation: tests/native-core/CMakeLists.txt
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: add isolated native C++17 component, fixture, viewer and sanitizer test build.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

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

find_package(Threads REQUIRED)
add_library(p710_fileman OBJECT "${P710_SOURCE_ROOT}/sgp/FileManIO.cpp")
target_include_directories(p710_fileman PUBLIC "${P710_SOURCE_ROOT}/sgp")
target_link_libraries(p710_fileman PUBLIC Threads::Threads)
add_executable(p710_native_fileman_tests fileman_tests.cpp)
target_link_libraries(p710_native_fileman_tests PRIVATE p710_fileman)

set(P710_NATIVE_TARGETS p710_raster p710_native_raster_tests
  p710_fileman p710_native_fileman_tests)
set(P710_NATIVE_EXECUTABLES p710_native_raster_tests p710_native_fileman_tests)
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
    add_library(p710_render OBJECT "${P710_SOURCE_ROOT}/sgp/NativeSurface.cpp"
      "${P710_SOURCE_ROOT}/sgp/NativeFont.cpp")
    target_include_directories(p710_render PUBLIC "${P710_SOURCE_ROOT}/sgp")
    target_link_libraries(p710_render PUBLIC p710_resources)
    add_executable(p710_native_render_tests native_render_tests.cpp)
    target_link_libraries(p710_native_render_tests PRIVATE p710_render p710_resources
      p710_images p710_fileman p710_fileman_vfs bfVFS)
    list(APPEND P710_NATIVE_TARGETS p710_render p710_native_render_tests)
    list(APPEND P710_NATIVE_EXECUTABLES p710_native_render_tests)
  endif()
  if(P710_SDL_PRESENTATION)
    if(NOT P710_RESOURCE_MANAGERS)
      message(FATAL_ERROR "Native presentation now exercises actual resource managers")
    endif()
    find_package(SDL3 3.4.20 EXACT REQUIRED CONFIG)
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
    target_link_libraries(p710_video PUBLIC p710_presentation p710_render)
    foreach(kind tests viewer)
      add_executable(p710_native_video_${kind} native_video_${kind}.cpp)
      target_link_libraries(p710_native_video_${kind} PRIVATE p710_video p710_presentation
        p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
      list(APPEND P710_NATIVE_TARGETS p710_native_video_${kind})
      list(APPEND P710_NATIVE_EXECUTABLES p710_native_video_${kind})
    endforeach()
    list(APPEND P710_NATIVE_TARGETS p710_video)
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
