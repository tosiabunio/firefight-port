# Runs the game headless and compares the frames it saves with golden hashes.
#
#   cmake [-DEMULATOR=<node>] -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DEXPECTED=<file>
#         -DARGS="<options>" -P check_frames.cmake
#
# EXPECTED holds one "<sha256>  <frame file>" line per frame (the format of `sha256sum`). The
# frames are written by --shot-every into PREF, which is emptied first. EMULATOR runs the
# Emscripten build's firefight.js.

file(REMOVE_RECURSE "${PREF}")
separate_arguments(args NATIVE_COMMAND "${ARGS}")
execute_process(
  COMMAND ${EMULATOR} "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --fast ${args}
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "firefight exited with ${result}:\n${output}")
endif()

file(STRINGS "${EXPECTED}" lines)
set(failed 0)
foreach(line IN LISTS lines)
  if(line MATCHES "^([0-9a-f]+)  (.+)$")
    set(expected_hash "${CMAKE_MATCH_1}")
    set(frame "${CMAKE_MATCH_2}")
    if(NOT EXISTS "${PREF}/${frame}")
      message(SEND_ERROR "missing frame ${frame}")
      set(failed 1)
      continue()
    endif()
    file(SHA256 "${PREF}/${frame}" actual_hash)
    if(actual_hash STREQUAL expected_hash)
      message(STATUS "ok ${frame}")
    else()
      message(SEND_ERROR "${frame}: ${actual_hash}, expected ${expected_hash}")
      set(failed 1)
    endif()
  endif()
endforeach()
if(failed)
  message(FATAL_ERROR "frames differ from ${EXPECTED} (they are kept in ${PREF})")
endif()
