# P710 native snapshot update: platform/linux/CMakeLists.txt
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# P710 native snapshot update: platform/linux/CMakeLists.txt
# Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-16.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

cmake_minimum_required(VERSION 3.20)
project(p710_native_engine LANGUAGES C CXX)
enable_testing()

# This is the original engine's compile graph. It deliberately has no substitute
# main/menu and cannot claim a runnable game until the real platform entry links.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR "Use the installed clang++-19: its VFS handles original include case without changing engine headers")
endif()
get_filename_component(P710_ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" -B
  "${CMAKE_CURRENT_SOURCE_DIR}/include_overlay.py" "${P710_ENGINE_ROOT}"
  "${CMAKE_CURRENT_BINARY_DIR}/include-overlay.json" COMMAND_ERROR_IS_FATAL ANY)

set(P710_SDL_PRESENTATION ON CACHE BOOL "Reuse accepted native platform components")
add_subdirectory("${P710_ENGINE_ROOT}/tests/native-core" foundation)
add_subdirectory("${P710_ENGINE_ROOT}/ext/lua-5.1.5" lua51)
add_subdirectory("${P710_ENGINE_ROOT}/ext/libsmacker" smacker)
find_package(EXPAT REQUIRED)
find_package(Threads REQUIRED)
set(P710_RAKNET_ROOT "" CACHE PATH "Approved pinned RakNet3.401 native archive directory")
if(NOT P710_RAKNET_ROOT)
  get_filename_component(_p710_project "${P710_ENGINE_ROOT}/../.." ABSOLUTE)
  if(P710_SANITIZERS)
    set(_p710_raknet_mode sanitized)
  else()
    set(_p710_raknet_mode debug)
  endif()
  set(P710_RAKNET_ROOT "${_p710_project}/build/raknet-3.401-a4c67c6/${_p710_raknet_mode}")
endif()
find_library(P710_RAKNET_LIBRARY NAMES raknet3401 PATHS "${P710_RAKNET_ROOT}" NO_DEFAULT_PATH REQUIRED)
add_library(RakNet::RakNet STATIC IMPORTED GLOBAL)
set_target_properties(RakNet::RakNet PROPERTIES IMPORTED_LOCATION "${P710_RAKNET_LIBRARY}"
  INTERFACE_LINK_LIBRARIES Threads::Threads)


add_library(p710_engine_contract INTERFACE)
target_compile_features(p710_engine_contract INTERFACE cxx_std_17)
target_compile_definitions(p710_engine_contract INTERFACE
  P710_NATIVE_HOST CINTERFACE XML_STATIC VFS_STATIC VFS_WITH_SLF VFS_WITH_7ZIP
  ENABLE_BRIEFINGROOM ROBOT_ALWAYS_READY FORCE_ASSERTS_ON BMP_RANDOM CALLBACKTIMER
  DISABLE_MP_INTERRUPTS_IN_COOP INTERRUPT_MP_DEADLOCK_FIX
  ENABLE_MP_FRIENDLY_PLAYERS_SHARE_SAME_FOV "__int64=long long")
target_compile_options(p710_engine_contract INTERFACE
  -ivfsoverlay "${CMAKE_CURRENT_BINARY_DIR}/include-overlay.json"
  -Wall -Wextra -Wno-unused-parameter -Wno-writable-strings -ferror-limit=12)
foreach(dir Ja2 ext/VFS/include ext/libsmacker ext/lua-5.1.5/src Utils TileEngine
    TacticalAI ModularizedTacticalAI/include Tactical Strategic sgp Ja2/Res lua
    Laptop Multiplayer Editor i18n/include)
  target_include_directories(p710_engine_contract INTERFACE "${P710_ENGINE_ROOT}/${dir}")
endforeach()
target_link_libraries(p710_engine_contract INTERFACE EXPAT::EXPAT lua51 smacker
  p710_ui p710_input p710_cursor p710_video p710_presentation p710_render p710_resources
  p710_images p710_raster p710_fileman p710_fileman_vfs p710_fileman_services p710_debug
  p710_sound p710_format p710_text p710_clock p710_ini p710_memory p710_game_timer p710_background
  p710_scroll p710_world_blitters p710_raw_blitters p710_shadow_blitters
  p710_effect_blitters p710_tile_blitters bfVFS)
if(P710_SANITIZERS)
  target_compile_options(p710_engine_contract INTERFACE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_engine_contract INTERFACE
    -fsanitize=address,undefined -fno-sanitize-recover=all)
  foreach(target lua51 smacker)
    target_compile_options(${target} PRIVATE
      -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  endforeach()
endif()

# The genuine native WinFont implementation is a separate provider. It uses
# the original engine compile contract without a recursive contract dependency.
# The eventual original entry links this provider along with the platform set.
add_library(p710_winfont STATIC "${P710_ENGINE_ROOT}/sgp/NativeWinFont.cpp")
target_link_libraries(p710_winfont PRIVATE p710_engine_contract p710_scalable p710_text p710_format bfVFS)
target_compile_options(p710_winfont PRIVATE -ffunction-sections -fdata-sections)

# Genuine native movie and outer-runtime providers. The original game modules
# are linked below; no substitute main/menu or successful platform stubs.
add_library(p710_movies STATIC
  "${P710_ENGINE_ROOT}/sgp/NativeVideoDecoder.cpp"
  "${P710_ENGINE_ROOT}/Utils/NativeCinematicsBink.cpp")
target_link_libraries(p710_movies PRIVATE p710_engine_contract FFmpeg::avformat
  FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale)
add_library(p710_native_runtime STATIC
  "${P710_ENGINE_ROOT}/platform/linux/NativeRuntime.cpp"
  "${P710_ENGINE_ROOT}/Utils/NativeEncryptedText.cpp"
  "${P710_ENGINE_ROOT}/sgp/NativeFontTranslation.cpp")
target_link_libraries(p710_native_runtime PRIVATE p710_engine_contract)
target_compile_options(p710_native_runtime PRIVATE -ffunction-sections -fdata-sections)
add_library(p710_vobject_bridge STATIC "${P710_ENGINE_ROOT}/sgp/NativeVideoObjectBridge.cpp")
target_link_libraries(p710_vobject_bridge PRIVATE p710_engine_contract)


# Preserve the upstream version generation and every gameplay module's source
# list. Existing native backends replace their original SGP translation units;
# missing interfaces still fail compilation/linkage rather than acquiring stubs.
set(GIT_SHA "3394695-native")
set(GAME_BUILD_INFORMATION "P710 native integration, uncommitted at 3394695")
set(P710_REPLACED_SGP
  "Button System.cpp" "Button Sound Control.cpp" DEBUG.cpp "DirectDraw Calls.cpp" "DirectX Common.cpp"
  "Cursor Control.cpp"
  English.cpp FileMan.cpp FileManIO.cpp FileManVFS.cpp Font.cpp himage.cpp
  ImageData.cpp ImageResource.cpp VideoObjectCore.cpp ShadingTables.cpp
  ImagePalette.cpp clip_state.cpp etrle_blitters.cpp impTGA.cpp input.cpp line.cpp
  mousesystem.cpp vobject_blitters.cpp PCX.cpp PngLoader.cpp STCI.cpp video.cpp vobject.cpp vsurface.cpp soundman.cpp timer.cpp WinFont.cpp)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/source-graph.tsv" "module\tsource\tselection\n")
set(P710_ENGINE_TARGETS)
foreach(module Editor Ja2 Laptop ModularizedTacticalAI sgp Strategic Tactical TacticalAI TileEngine Utils i18n)
  add_subdirectory("${P710_ENGINE_ROOT}/${module}" "original/${module}")
  set(selected)
  foreach(source IN LISTS ${module}Src)
    get_filename_component(name "${source}" NAME)
    if(module STREQUAL "sgp" AND name IN_LIST P710_REPLACED_SGP)
      set(selection "native-backend-interface-integration")
    elseif(module STREQUAL "Utils" AND
           (name STREQUAL "Cinematics Bink.cpp" OR name STREQUAL "Encrypted File.cpp"))
      set(selection "native-backend-interface-integration")
    elseif(module STREQUAL "Utils" AND name STREQUAL "dsutil.cpp")
      # Only the unselected Windows sample Win Util.cpp calls these DS helpers.
      # Native sound is supplied by the genuine SDL/FFmpeg sound provider.
      set(selection "unused-windows-directsound-helper")
    elseif((module STREQUAL "Utils" AND name STREQUAL "INIReader.cpp") OR
           (module STREQUAL "sgp" AND name STREQUAL "MemMan.cpp"))
      # Compile this genuine original source once through the foundation target
      # shared by focused fixtures and the engine, avoiding duplicate objects.
      set(selection "foundation-original-interface-integration")
    else()
      list(APPEND selected "${source}")
      set(selection "original-engine-object")
    endif()
    file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/source-graph.tsv" "${module}\t${source}\t${selection}\n")
  endforeach()
  add_library(p710_engine_${module} OBJECT EXCLUDE_FROM_ALL ${selected})
  target_link_libraries(p710_engine_${module} PRIVATE p710_engine_contract)
  if(module STREQUAL "sgp")
    target_compile_definitions(p710_engine_${module} PRIVATE NO_ZLIB_COMPRESSION)
  endif()
  list(APPEND P710_ENGINE_TARGETS p710_engine_${module})
endforeach()
# Original multiplayer callers can be compiled against the bundled headers
# before admitting their missing external implementation. Never link x86 .lib.
add_library(p710_engine_Multiplayer OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Multiplayer/client.cpp"
  "${P710_ENGINE_ROOT}/Multiplayer/server.cpp"
  "${P710_ENGINE_ROOT}/Multiplayer/transfer_rules.cpp")
target_include_directories(p710_engine_Multiplayer PRIVATE "${P710_ENGINE_ROOT}/Multiplayer/raknet")
target_link_libraries(p710_engine_Multiplayer PRIVATE p710_engine_contract)
list(APPEND P710_ENGINE_TARGETS p710_engine_Multiplayer)
foreach(name client.cpp server.cpp transfer_rules.cpp)
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/source-graph.tsv"
    "Multiplayer\t${P710_ENGINE_ROOT}/Multiplayer/${name}\toriginal-engine-object\n")
endforeach()
add_subdirectory("${P710_ENGINE_ROOT}/lua" engine-lua EXCLUDE_FROM_ALL)
target_link_libraries(Lua PRIVATE p710_engine_contract)
add_custom_target(p710_original_engine_objects DEPENDS ${P710_ENGINE_TARGETS} Lua p710_winfont)
set(P710_NATIVE_LINK_PROVIDERS
  p710_native_runtime p710_movies p710_winfont p710_vobject_bridge p710_engine_contract
  p710_ui p710_input p710_cursor p710_video p710_presentation p710_render p710_resources
  p710_images p710_raster p710_fileman p710_fileman_vfs p710_fileman_services p710_debug
  p710_sound p710_audio_decoder p710_format p710_text p710_clock p710_ini p710_memory
  p710_game_timer p710_background p710_scroll p710_world_blitters p710_raw_blitters
  p710_shadow_blitters p710_effect_blitters p710_tile_blitters bfVFS)
add_executable(p710_native_game EXCLUDE_FROM_ALL "${P710_ENGINE_ROOT}/platform/linux/NativeMain.cpp")
target_link_libraries(p710_native_game PRIVATE ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
# Genuine original engine graph, with a synthetic wire-format test main only.
add_executable(p710_native_profile101_tests EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/tests/native-core/native_profile101_tests.cpp")
target_link_libraries(p710_native_profile101_tests PRIVATE ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_executable(p710_native_explosion_queue_save_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_explosion_queue_save_tests.cpp")
target_link_libraries(p710_native_explosion_queue_save_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME saves.original_explosion_queue_count COMMAND p710_native_explosion_queue_save_tests)
foreach(_p710_save_test email soldier map_message)
  add_executable(p710_native_${_p710_save_test}_save_tests
    "${P710_ENGINE_ROOT}/tests/native-core/native_${_p710_save_test}_save_tests.cpp")
  target_link_libraries(p710_native_${_p710_save_test}_save_tests PRIVATE
    ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
endforeach()
add_test(NAME saves.original_email_utf16_subject COMMAND p710_native_email_save_tests)
add_test(NAME saves.original_soldier_native_layout COMMAND p710_native_soldier_save_tests)
add_test(NAME saves.original_map_message_utf16 COMMAND p710_native_map_message_save_tests)
add_executable(p710_native_map_placement_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_map_placement_tests.cpp")
target_link_libraries(p710_native_map_placement_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME maps.original_static_placement_wire COMMAND p710_native_map_placement_tests)
add_executable(p710_native_translation_table_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_translation_table_tests.cpp")
target_link_libraries(p710_native_translation_table_tests PRIVATE p710_engine_contract)
add_test(NAME text.original_translation_tables
  COMMAND p710_native_translation_table_tests)
add_test(NAME ui.original_screen_toggle_guards
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/check_screen_toggle_guards.py"
    "${P710_ENGINE_ROOT}")
add_test(NAME ui.original_creation_id_guards
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/check_ui_creation_guards.py"
    "${P710_ENGINE_ROOT}")
add_executable(p710_native_recoil_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_recoil_tests.cpp")
target_link_libraries(p710_native_recoil_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME combat.original_ncth_counter_force COMMAND p710_native_recoil_tests)
add_executable(p710_native_world_node_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_world_node_tests.cpp")
target_link_libraries(p710_native_world_node_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME world.original_struct_node_lifetime COMMAND p710_native_world_node_tests)
add_executable(p710_native_dynamic_dialogue_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_dynamic_dialogue_tests.cpp")
target_link_libraries(p710_native_dynamic_dialogue_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME dialogue.original_dynamic_box_lifetime COMMAND p710_native_dynamic_dialogue_tests)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/dynamic_dialogue_display.inc"
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/extract_dynamic_dialogue_display.py"
    "${P710_ENGINE_ROOT}/Laptop/DynamicDialogueWidget.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/dynamic_dialogue_display.inc"
  DEPENDS "${P710_ENGINE_ROOT}/Laptop/DynamicDialogueWidget.cpp"
    "${P710_ENGINE_ROOT}/tests/native-core/extract_dynamic_dialogue_display.py")
add_executable(p710_native_dynamic_dialogue_display_tests
  "${P710_ENGINE_ROOT}/tests/native-core/dynamic_dialogue_display_palette.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/dynamic_dialogue_display.inc")
target_include_directories(p710_native_dynamic_dialogue_display_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
if(P710_SANITIZERS)
  target_compile_options(p710_native_dynamic_dialogue_display_tests PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_native_dynamic_dialogue_display_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME dialogue.original_dynamic_box_display_palette COMMAND p710_native_dynamic_dialogue_display_tests)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/face_palette_lifetime.inc"
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/extract_face_palette_lifetime.py"
    "${P710_ENGINE_ROOT}/Tactical/Faces.cpp"
    "${P710_ENGINE_ROOT}/Tactical/Interface.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/face_palette_lifetime.inc"
  DEPENDS "${P710_ENGINE_ROOT}/Tactical/Faces.cpp"
    "${P710_ENGINE_ROOT}/Tactical/Interface.cpp"
    "${P710_ENGINE_ROOT}/tests/native-core/extract_face_palette_lifetime.py")
add_executable(p710_native_face_palette_lifetime_tests
  "${P710_ENGINE_ROOT}/tests/native-core/face_palette_lifetime.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/face_palette_lifetime.inc")
target_include_directories(p710_native_face_palette_lifetime_tests PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")
if(P710_SANITIZERS)
  target_compile_options(p710_native_face_palette_lifetime_tests PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_native_face_palette_lifetime_tests PRIVATE
    -fsanitize=address,undefined)
endif()
add_test(NAME ui.original_face_palette_lifetime
  COMMAND p710_native_face_palette_lifetime_tests)
set(P710_ATTACHMENT_CURSOR_INCLUDES
  "${CMAKE_CURRENT_BINARY_DIR}/attachment_object_methods.inc"
  "${CMAKE_CURRENT_BINARY_DIR}/attachment_do_attachment.inc"
  "${CMAKE_CURRENT_BINARY_DIR}/attachment_completion.inc")
add_custom_command(OUTPUT ${P710_ATTACHMENT_CURSOR_INCLUDES}
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/extract_attachment_cursor_completion.py"
    "${P710_ENGINE_ROOT}/Tactical/Interface Items.cpp"
    "${P710_ENGINE_ROOT}/Tactical/Item Types.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS "${P710_ENGINE_ROOT}/Tactical/Interface Items.cpp"
    "${P710_ENGINE_ROOT}/Tactical/Item Types.cpp"
    "${P710_ENGINE_ROOT}/tests/native-core/extract_attachment_cursor_completion.py"
  VERBATIM)
add_executable(p710_native_attachment_cursor_completion_tests
  "${P710_ENGINE_ROOT}/tests/native-core/attachment_cursor_completion_tests.cpp"
  ${P710_ATTACHMENT_CURSOR_INCLUDES})
find_package(SDL3 3.4.20 EXACT REQUIRED CONFIG)
target_link_libraries(p710_native_attachment_cursor_completion_tests PRIVATE SDL3::Headers)
target_compile_definitions(p710_native_attachment_cursor_completion_tests PRIVATE
  P710_NATIVE_HOST CINTERFACE XML_STATIC VFS_STATIC VFS_WITH_SLF VFS_WITH_7ZIP
  ENABLE_BRIEFINGROOM ROBOT_ALWAYS_READY FORCE_ASSERTS_ON BMP_RANDOM CALLBACKTIMER
  DISABLE_MP_INTERRUPTS_IN_COOP INTERRUPT_MP_DEADLOCK_FIX
  ENABLE_MP_FRIENDLY_PLAYERS_SHARE_SAME_FOV "__int64=long long")
target_compile_options(p710_native_attachment_cursor_completion_tests PRIVATE
  -ivfsoverlay "${CMAKE_CURRENT_BINARY_DIR}/include-overlay.json"
  -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-writable-strings
  -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-mismatched-tags
  -Wno-comment -Wno-undefined-bool-conversion -Wno-invalid-offsetof
  -ferror-limit=30)
target_include_directories(p710_native_attachment_cursor_completion_tests PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")
foreach(dir Ja2 ext/VFS/include ext/libsmacker ext/lua-5.1.5/src Utils TileEngine
    TacticalAI ModularizedTacticalAI/include Tactical Strategic sgp Ja2/Res lua
    Laptop Multiplayer Editor i18n/include)
  target_include_directories(p710_native_attachment_cursor_completion_tests PRIVATE
    "${P710_ENGINE_ROOT}/${dir}")
endforeach()
if(P710_SANITIZERS)
  target_compile_options(p710_native_attachment_cursor_completion_tests PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_native_attachment_cursor_completion_tests PRIVATE
    -fsanitize=address,undefined -fno-sanitize-recover=all)
endif()
add_test(NAME inventory.original_attachment_cursor_completion
  COMMAND p710_native_attachment_cursor_completion_tests)
add_executable(p710_native_battle_sound_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_battle_sound_tests.cpp")
target_link_libraries(p710_native_battle_sound_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME audio.original_npc_battle_sound_cache COMMAND p710_native_battle_sound_tests)
add_executable(p710_native_same_building_bounds_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_same_building_bounds_tests.cpp")
target_link_libraries(p710_native_same_building_bounds_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME ai.original_same_building_bounds COMMAND p710_native_same_building_bounds_tests)
add_executable(p710_native_sector_inventory_sort_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_sector_inventory_sort_tests.cpp")
target_link_libraries(p710_native_sector_inventory_sort_tests PRIVATE
  ${P710_ENGINE_TARGETS} Lua ${P710_NATIVE_LINK_PROVIDERS} RakNet::RakNet)
add_test(NAME inventory.original_sector_sort_lifetime COMMAND p710_native_sector_inventory_sort_tests)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/door_swap_function.inc"
  COMMAND "${Python3_EXECUTABLE}" -B "${P710_ENGINE_ROOT}/tests/native-core/extract_door_swap.py"
    "${P710_ENGINE_ROOT}/TileEngine/structure.cpp" "${CMAKE_CURRENT_BINARY_DIR}/door_swap_function.inc"
  DEPENDS "${P710_ENGINE_ROOT}/TileEngine/structure.cpp"
    "${P710_ENGINE_ROOT}/tests/native-core/extract_door_swap.py")
add_executable(p710_native_door_swap_tests
  "${P710_ENGINE_ROOT}/tests/native-core/door_swap_lifetime.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/door_swap_function.inc")
target_include_directories(p710_native_door_swap_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
if(P710_SANITIZERS)
  target_compile_options(p710_native_door_swap_tests PRIVATE -fsanitize=address,undefined
    -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_native_door_swap_tests PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME world.original_door_swap_lifetime COMMAND p710_native_door_swap_tests)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/tactical_placement_logic.inc"
  COMMAND "${Python3_EXECUTABLE}" -B
    "${P710_ENGINE_ROOT}/tests/native-core/extract_tactical_placement.py"
    "${P710_ENGINE_ROOT}/TileEngine/Tactical Placement GUI.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/tactical_placement_logic.inc"
  DEPENDS "${P710_ENGINE_ROOT}/TileEngine/Tactical Placement GUI.cpp"
    "${P710_ENGINE_ROOT}/tests/native-core/extract_tactical_placement.py")
add_executable(p710_native_tactical_placement_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_tactical_placement_tests.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/tactical_placement_logic.inc")
target_include_directories(p710_native_tactical_placement_tests PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}")
if(P710_SANITIZERS)
  target_compile_options(p710_native_tactical_placement_tests PRIVATE
    -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  target_link_options(p710_native_tactical_placement_tests PRIVATE
    -fsanitize=address,undefined)
endif()
add_test(NAME tactical.original_edge_direction_sync
  COMMAND p710_native_tactical_placement_tests)
add_test(NAME profiles.original_windows101_wire COMMAND p710_native_profile101_tests)
add_executable(p710_native_vobject_bridge_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_vobject_bridge_tests.cpp")
target_link_libraries(p710_native_vobject_bridge_tests PRIVATE ${P710_NATIVE_LINK_PROVIDERS})
add_test(NAME rendering.original_vobject_bridges COMMAND p710_native_vobject_bridge_tests)
add_executable(p710_native_movie_tests "${P710_ENGINE_ROOT}/tests/native-core/native_movie_tests.cpp")
target_link_libraries(p710_native_movie_tests PRIVATE ${P710_NATIVE_LINK_PROVIDERS} FFmpeg::avutil)
add_test(NAME movies.generate_fixtures COMMAND "${Python3_EXECUTABLE}"
  "${P710_ENGINE_ROOT}/tests/native-core/generate_movie_fixtures.py" "${CMAKE_CURRENT_BINARY_DIR}/movie-fixtures")
set_tests_properties(movies.generate_fixtures PROPERTIES FIXTURES_SETUP movie_inputs)
add_test(NAME movies.original_bink_native_interfaces COMMAND p710_native_movie_tests
  "${CMAKE_CURRENT_BINARY_DIR}/movie-fixtures")
set_tests_properties(movies.original_bink_native_interfaces PROPERTIES
  FIXTURES_REQUIRED movie_inputs ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy")
add_executable(p710_native_runtime_tests "${P710_ENGINE_ROOT}/tests/native-core/native_runtime_tests.cpp")
target_compile_options(p710_native_runtime_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(p710_native_runtime_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(p710_native_runtime_tests PRIVATE ${P710_NATIVE_LINK_PROVIDERS})
add_test(NAME runtime.original_profile_boundaries COMMAND p710_native_runtime_tests
  "${CMAKE_CURRENT_BINARY_DIR}/runtime-profile-fixtures")

add_library(p710_engine_renderer_completion_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/sgp/shading.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Rain.cpp"
  "${P710_ENGINE_ROOT}/TacticalAI/Attacks.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/renderworld.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/structure.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/Shade Table Util.cpp"
  "${P710_ENGINE_ROOT}/Utils/Cinematics.cpp"
  "${P710_ENGINE_ROOT}/Utils/Utilities.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Assignments.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Interface Items.cpp")
target_link_libraries(p710_engine_renderer_completion_compile PRIVATE p710_engine_contract)
add_executable(p710_native_wrap_string_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_wrap_string_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/Utilities.cpp")
target_compile_options(p710_native_wrap_string_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(p710_native_wrap_string_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(p710_native_wrap_string_tests PRIVATE p710_engine_contract
  p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
add_test(NAME text.original_wrap_string COMMAND p710_native_wrap_string_tests
  "${CMAKE_CURRENT_BINARY_DIR}/foundation/artifacts/rendering")
set_tests_properties(text.original_wrap_string PROPERTIES FIXTURES_REQUIRED render_inputs)
add_executable(p710_native_popup_callback_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_popup_callback_tests.cpp")
target_link_libraries(p710_native_popup_callback_tests PRIVATE p710_engine_contract)
add_test(NAME engine.popup_callback_contracts COMMAND p710_native_popup_callback_tests)
add_executable(p710_native_format_caller_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_format_caller_tests.cpp")
target_link_libraries(p710_native_format_caller_tests PRIVATE p710_engine_contract)
add_test(NAME text.original_caller_capacities COMMAND p710_native_format_caller_tests)
add_executable(p710_native_encrypted_text_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_encrypted_text_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/NativeEncryptedText.cpp")
target_link_libraries(p710_native_encrypted_text_tests PRIVATE p710_engine_contract
  p710_fileman p710_fileman_vfs bfVFS)
add_test(NAME text.legacy_encrypted_utf16 COMMAND p710_native_encrypted_text_tests
  "${CMAKE_CURRENT_BINARY_DIR}/artifacts/encrypted-text")
add_executable(p710_native_item_file_services_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_item_file_services_tests.cpp")
target_link_libraries(p710_native_item_file_services_tests PRIVATE p710_engine_contract
  p710_fileman_services p710_fileman p710_fileman_vfs p710_text bfVFS)
add_test(NAME fileman.item_native_boundaries COMMAND p710_native_item_file_services_tests)
add_executable(p710_native_file_time_tests "${P710_ENGINE_ROOT}/tests/native-core/native_file_time_tests.cpp")
target_link_libraries(p710_native_file_time_tests PRIVATE p710_engine_contract
  p710_fileman_services p710_fileman p710_fileman_vfs p710_text bfVFS)
add_test(NAME fileman.native_selected_profile_times COMMAND p710_native_file_time_tests)
add_executable(p710_native_clipboard_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_clipboard_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/Text Input.cpp")
target_compile_options(p710_native_clipboard_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(p710_native_clipboard_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(p710_native_clipboard_tests PRIVATE p710_engine_contract
  p710_text p710_memory p710_debug bfVFS)
add_test(NAME input.original_clipboard COMMAND p710_native_clipboard_tests)
set_tests_properties(input.original_clipboard PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")

# Exercise genuine original GAP entry points, the genuine original speech-volume
# policy and the production native audio/FileMan providers. Discard unrelated
# uncalled Sound Control sections; no substitute engine functions are supplied.
add_executable(p710_native_dialogue_audio_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_dialogue_audio_tests.cpp"
  "${P710_ENGINE_ROOT}/Tactical/GAP.cpp"
  "${P710_ENGINE_ROOT}/Utils/Sound Control.cpp")
target_compile_options(p710_native_dialogue_audio_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(p710_native_dialogue_audio_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(p710_native_dialogue_audio_tests PRIVATE p710_engine_contract
  p710_sound p710_fileman p710_fileman_vfs p710_fileman_services p710_text bfVFS)
add_test(NAME audio.original_dialogue_gap_contracts COMMAND p710_native_dialogue_audio_tests)
set_tests_properties(audio.original_dialogue_gap_contracts PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")

# Actual original lookup, quantizer and overlay entry points. Uncalled engine
# functions are discarded rather than replaced with successful fixture stubs.
add_executable(p710_native_keymap_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_keymap_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/KeyMap.cpp"
  "${P710_ENGINE_ROOT}/Utils/Text Utils.cpp")
add_executable(p710_native_quantizer_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_quantizer_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/Quantize.cpp"
  "${P710_ENGINE_ROOT}/Utils/Quantize Wrap.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/phys math.cpp")
add_executable(p710_native_overlay_scroll_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_overlay_scroll_tests.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/Render Dirty.cpp")
foreach(target p710_native_keymap_tests p710_native_quantizer_tests p710_native_overlay_scroll_tests)
  target_compile_options(${target} PRIVATE -ffunction-sections -fdata-sections)
  target_link_options(${target} PRIVATE -Wl,--gc-sections)
  target_link_libraries(${target} PRIVATE p710_engine_contract)
endforeach()
# OBJECT-library objects must be supplied directly to these fixture executables;
# interface usage requirements alone do not include their implementation objects.
target_link_libraries(p710_native_keymap_tests PRIVATE p710_input p710_video p710_presentation
  p710_render p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
target_link_libraries(p710_native_overlay_scroll_tests PRIVATE p710_background p710_render
  p710_resources p710_images p710_fileman p710_fileman_vfs bfVFS)
add_test(NAME input.original_keymap_bindings COMMAND p710_native_keymap_tests)
set_tests_properties(input.original_keymap_bindings PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=x11")
add_test(NAME rendering.original_quantizer COMMAND p710_native_quantizer_tests)
add_test(NAME video.original_overlay_scroll COMMAND p710_native_overlay_scroll_tests)

add_executable(p710_native_winfont_tests
  "${P710_ENGINE_ROOT}/tests/native-core/native_winfont_tests.cpp"
  "${P710_ENGINE_ROOT}/Utils/Font Control.cpp"
  "${P710_ENGINE_ROOT}/Utils/WordWrap.cpp")
target_compile_options(p710_native_winfont_tests PRIVATE -ffunction-sections -fdata-sections)
target_link_options(p710_native_winfont_tests PRIVATE -Wl,--gc-sections)
target_link_libraries(p710_native_winfont_tests PRIVATE p710_engine_contract p710_winfont
  p710_ui p710_input p710_cursor p710_video p710_presentation p710_render p710_resources
  p710_images p710_fileman p710_fileman_vfs p710_fileman_services p710_memory p710_debug p710_text bfVFS)
add_test(NAME fonts.original_winfont_tooltips COMMAND p710_native_winfont_tests
  "${CMAKE_CURRENT_BINARY_DIR}/foundation/artifacts/rendering"
  "${P710_PUBLIC_REGULAR_FONT}" "${P710_PUBLIC_BOLD_FONT}")
set_tests_properties(fonts.original_winfont_tooltips PROPERTIES FIXTURES_REQUIRED render_inputs)
add_library(p710_engine_font_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Utils/Font Control.cpp"
  "${P710_ENGINE_ROOT}/Utils/WordWrap.cpp"
  "${P710_ENGINE_ROOT}/Tactical/SoldierTooltips.cpp")
target_link_libraries(p710_engine_font_compile PRIVATE p710_engine_contract)

# Compile the genuine pointer producers, queue/pending/face storage and
# consumers with the same contract in both native and instrumented builds.
add_library(p710_engine_abi_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Tactical/Dialogue Control.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Interface Dialogue.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Handle Items.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Strategic Movement.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Faces.cpp"
  "${P710_ENGINE_ROOT}/TacticalAI/NPC.cpp"
  "${P710_ENGINE_ROOT}/Utils/Sound Control.cpp")
target_link_libraries(p710_engine_abi_compile PRIVATE p710_engine_contract)
add_library(p710_engine_renderer_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Ja2/HelpScreen.cpp"
  "${P710_ENGINE_ROOT}/Ja2/Loading Screen.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Map Screen Interface Map.cpp"
  "${P710_ENGINE_ROOT}/Strategic/mapscreen.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Minigame.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/overhead map.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Interface Items.cpp"
  "${P710_ENGINE_ROOT}/Tactical/SkillMenu.cpp"
  "${P710_ENGINE_ROOT}/Tactical/VehicleMenu.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Map Screen Interface Map Inventory.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Assignments.cpp"
  "${P710_ENGINE_ROOT}/Strategic/LuaInitNPCs.cpp"
  "${P710_ENGINE_ROOT}/Ja2/SaveLoadGame.cpp"
  "${P710_ENGINE_ROOT}/Ja2/SaveLoadScreen.cpp"
  "${P710_ENGINE_ROOT}/sgp/Random.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/worlddef.cpp")
target_link_libraries(p710_engine_renderer_compile PRIVATE p710_engine_contract)
add_library(p710_engine_platform_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Utils/Timer Control.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/Render Dirty.cpp"
  "${P710_ENGINE_ROOT}/Tactical/LOS.cpp"
  "${P710_ENGINE_ROOT}/TileEngine/Interactive Tiles.cpp"
  "${P710_ENGINE_ROOT}/Utils/Text Input.cpp"
  "${P710_ENGINE_ROOT}/Utils/message.cpp"
  "${P710_ENGINE_ROOT}/i18n/Multi Language Graphic Utils.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Interface.cpp"
  "${P710_ENGINE_ROOT}/Tactical/ShopKeeper Interface.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Soldier Init List.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Turn Based Input.cpp"
  "${P710_ENGINE_ROOT}/Utils/popup_class.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Auto Resolve.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Strategic Movement Costs.cpp"
  "${P710_ENGINE_ROOT}/Strategic/XML_ExtraItems.cpp"
  "${P710_ENGINE_ROOT}/Tactical/XML_HiddenNames.cpp"
  "${P710_ENGINE_ROOT}/Laptop/files.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Overhead.cpp")
target_link_libraries(p710_engine_platform_compile PRIVATE p710_engine_contract)
add_library(p710_engine_world_platform_compile OBJECT EXCLUDE_FROM_ALL
  "${P710_ENGINE_ROOT}/Ja2/JA2 Splash.cpp"
  "${P710_ENGINE_ROOT}/Ja2/jascreens.cpp"
  "${P710_ENGINE_ROOT}/Laptop/IMP Begin Screen.cpp"
  "${P710_ENGINE_ROOT}/Tactical/DisplayCover.cpp"
  "${P710_ENGINE_ROOT}/Tactical/Handle UI.cpp"
  "${P710_ENGINE_ROOT}/Strategic/Strategic Event Handler.cpp"
  "${P710_ENGINE_ROOT}/Strategic/UndergroundInit.cpp"
  "${P710_ENGINE_ROOT}/Utils/Animated ProgressBar.cpp"
  "${P710_ENGINE_ROOT}/Utils/KeyMap.cpp"
  "${P710_ENGINE_ROOT}/Utils/Quantize.cpp"
  "${P710_ENGINE_ROOT}/Utils/Quantize Wrap.cpp")
target_link_libraries(p710_engine_world_platform_compile PRIVATE p710_engine_contract)

# Narrow builds point to genuine startup translation units while the complete
# graph remains available. Multiplayer, sound, cursor and clock dependencies
# remain explicit integration work; there is no synthetic success executable.
foreach(pair "menu|MainMenuScreen.cpp" "loop|gameloop.cpp" "init|Init.cpp" "screens|Screens.cpp" "features|FeaturesScreen.cpp" "newgame|GameInitOptionsScreen.cpp")
  string(REPLACE "|" ";" parts "${pair}")
  list(GET parts 0 short)
  list(GET parts 1 source)
  add_library(p710_engine_${short}_compile OBJECT EXCLUDE_FROM_ALL
    "${P710_ENGINE_ROOT}/Ja2/${source}")
  target_link_libraries(p710_engine_${short}_compile PRIVATE p710_engine_contract)
endforeach()
