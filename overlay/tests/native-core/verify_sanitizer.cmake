# P710 native notice preparation: tests/native-core/verify_sanitizer.cmake
# New candidate test/build/documentation/metadata path.
# Native adaptation recorded in the 2026-11-12 checkpoint.
# Exact first-edit/creation date is not established; this is a checkpoint-record date.
# Scope: verify expected sanitizer failure diagnostics from dedicated canaries.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Notice inserted: 2026-11-12.
# Existing source bytes and original notices retained; no additional rights are granted.
# This record does not establish license applicability, compliance or distribution permission.

# P710 native snapshot update: tests/native-core/verify_sanitizer.cmake
# Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-14.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

if(NOT DEFINED PROBE OR NOT DEFINED KIND OR NOT DEFINED OUTPUT_DIR)
  message(FATAL_ERROR "PROBE, KIND and OUTPUT_DIR are required")
endif()
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
execute_process(COMMAND "${PROBE}" "${KIND}"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE diagnostic)
file(WRITE "${OUTPUT_DIR}/${KIND}-diagnostic.log"
  "Intentional ${KIND} canary; exit=${result}\n${output}${diagnostic}")
if("${result}" STREQUAL "0")
  message(FATAL_ERROR "Intentional ${KIND} canary returned success")
endif()
if(KIND STREQUAL "asan")
  if(NOT diagnostic MATCHES "AddressSanitizer: heap-buffer-overflow")
    message(FATAL_ERROR "Expected ASan heap-buffer-overflow diagnostic, got: ${diagnostic}")
  endif()
elseif(KIND STREQUAL "ubsan")
  if(NOT diagnostic MATCHES "runtime error: signed integer overflow")
    message(FATAL_ERROR "Expected UBSan signed-overflow diagnostic, got: ${diagnostic}")
  endif()
else()
  message(FATAL_ERROR "Unknown diagnostic kind: ${KIND}")
endif()
message(STATUS "Intentional ${KIND} failure produced the expected diagnostic")
