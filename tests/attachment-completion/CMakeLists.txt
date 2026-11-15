cmake_minimum_required(VERSION 3.20)
project(p710_attachment_completion LANGUAGES CXX)
enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(P710_ENGINE_SOURCE "" CACHE PATH "Hash-verified reconstructed engine source")
option(P710_SANITIZERS "Use strict ASan and UBSan with leak detection" OFF)
option(P710_ATTACHMENT_UNGUARDED "Compile the historical failing control" OFF)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "The source include-case overlay requires Clang")
endif()
get_filename_component(P710_PAYLOAD "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
  --engine "${P710_ENGINE_SOURCE}" --payload "${P710_PAYLOAD}"
  --output "${CMAKE_CURRENT_BINARY_DIR}/production" COMMAND_ERROR_IS_FATAL ANY)
add_executable(p710_attachment_completion_tests attachment_cursor_completion_tests.cpp)
target_compile_features(p710_attachment_completion_tests PRIVATE cxx_std_17)
set_target_properties(p710_attachment_completion_tests PROPERTIES CXX_EXTENSIONS OFF)
target_compile_options(p710_attachment_completion_tests PRIVATE
  -ivfsoverlay "${CMAKE_CURRENT_BINARY_DIR}/production/include-overlay.json"
  -Wall -Wextra -Wno-unused-parameter -Wno-writable-strings)
target_compile_definitions(p710_attachment_completion_tests PRIVATE
  P710_NATIVE_HOST CINTERFACE XML_STATIC VFS_STATIC SZIP_STATIC VFS_WITH_SLF VFS_WITH_7ZIP
  ENABLE_BRIEFINGROOM ROBOT_ALWAYS_READY FORCE_ASSERTS_ON BMP_RANDOM CALLBACKTIMER
  DISABLE_MP_INTERRUPTS_IN_COOP INTERRUPT_MP_DEADLOCK_FIX
  ENABLE_MP_FRIENDLY_PLAYERS_SHARE_SAME_FOV "__int64=long long")
foreach(dir Ja2 ext/VFS/include Utils TileEngine TacticalAI ModularizedTacticalAI/include
    Tactical Strategic sgp Ja2/Res lua Laptop Multiplayer Editor i18n/include)
  target_include_directories(p710_attachment_completion_tests PRIVATE "${P710_ENGINE_SOURCE}/${dir}")
endforeach()
if(P710_ATTACHMENT_UNGUARDED)
  target_compile_definitions(p710_attachment_completion_tests PRIVATE BUILD_BEFORE)
endif()
if(P710_SANITIZERS)
  target_compile_options(p710_attachment_completion_tests PRIVATE
    -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_attachment_completion_tests PRIVATE
    -fsanitize=address,undefined -fno-sanitize-recover=all)
endif()
add_test(NAME attachment.production_cursor_completion COMMAND p710_attachment_completion_tests)
