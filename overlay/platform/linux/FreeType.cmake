# P710 native snapshot update: platform/linux/FreeType.cmake
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# Explicitly user-approved project-local SDK; no fallback to unrelated headers.
set(P710_FREETYPE_ROOT "" CACHE PATH "Pinned FreeType 2.15.0 private prefix")
if(NOT P710_FREETYPE_ROOT)
  get_filename_component(_p710_ft_project "${P710_SOURCE_ROOT}/../.." ABSOLUTE)
  set(P710_FREETYPE_ROOT "${_p710_ft_project}/build/freetype-2.15.0-portable/prefix")
endif()
find_package(freetype 2.15.0 EXACT CONFIG REQUIRED
  PATHS "${P710_FREETYPE_ROOT}/lib/cmake/freetype" NO_DEFAULT_PATH)
