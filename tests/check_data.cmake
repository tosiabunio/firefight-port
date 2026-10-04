# Checks that data/ and music/ hold exactly the original files: every file listed in EXPECTED must
# exist with that SHA-256, and no other file may be there.
#
#   cmake -DROOT=<repository> -DEXPECTED=<file> -P check_data.cmake
#
# EXPECTED holds one "<sha256>  <path>" line per file, paths relative to ROOT (the format of
# `sha256sum`). Every file was checked against the original archive with
# `tools/archive/ffarchive.py provenance`.

file(STRINGS "${EXPECTED}" lines)
set(failed 0)
set(listed "")
foreach(line IN LISTS lines)
  string(REGEX REPLACE "\r$" "" line "${line}")
  if(line MATCHES "^([0-9a-f]+)  (.+)$")
    set(expected_hash "${CMAKE_MATCH_1}")
    set(path "${CMAKE_MATCH_2}")
    list(APPEND listed "${path}")
    if(NOT EXISTS "${ROOT}/${path}")
      message(SEND_ERROR "missing: ${path}")
      set(failed 1)
      continue()
    endif()
    file(SHA256 "${ROOT}/${path}" actual_hash)
    if(NOT actual_hash STREQUAL expected_hash)
      message(SEND_ERROR "changed: ${path}")
      set(failed 1)
    endif()
  endif()
endforeach()

file(GLOB_RECURSE present LIST_DIRECTORIES false RELATIVE "${ROOT}" "${ROOT}/data/*" "${ROOT}/music/*")
list(REMOVE_ITEM present ${listed})
foreach(path IN LISTS present)
  message(SEND_ERROR "not in ${EXPECTED}: ${path}")
  set(failed 1)
endforeach()

list(LENGTH listed count)
if(failed)
  message(FATAL_ERROR "data/ or music/ differ from ${EXPECTED}")
endif()
message(STATUS "${count} files match ${EXPECTED}")
