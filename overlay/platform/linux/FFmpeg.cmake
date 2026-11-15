# P710 native snapshot update: platform/linux/FFmpeg.cmake
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

# The user-approved, signature-verified decoder SDK is built outside the kept
# Windows checkout. Never silently fall back to unrelated system codec headers.
set(P710_FFMPEG_ROOT "" CACHE PATH "Pinned FFmpeg 7.1.5 decoder SDK prefix")
if(NOT P710_FFMPEG_ROOT)
  get_filename_component(_p710_project "${P710_SOURCE_ROOT}/../.." ABSOLUTE)
  set(P710_FFMPEG_ROOT "${_p710_project}/build/ffmpeg-7.1.5-portable/prefix")
endif()
find_path(P710_FFMPEG_INCLUDE libavcodec/avcodec.h
  PATHS "${P710_FFMPEG_ROOT}/include" NO_DEFAULT_PATH REQUIRED)
foreach(_lib avformat avcodec avutil swresample swscale)
  if(NOT TARGET FFmpeg::${_lib})
    find_library(P710_FFMPEG_${_lib} NAMES ${_lib}
      PATHS "${P710_FFMPEG_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
    add_library(FFmpeg::${_lib} SHARED IMPORTED GLOBAL)
    set_target_properties(FFmpeg::${_lib} PROPERTIES
      IMPORTED_LOCATION "${P710_FFMPEG_${_lib}}"
      INTERFACE_INCLUDE_DIRECTORIES "${P710_FFMPEG_INCLUDE}")
  endif()
endforeach()
