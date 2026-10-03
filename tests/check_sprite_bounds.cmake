# Runs the game in check mode, which loads the level of every mission, and compares the phase
# bounds of every sprite it loads with a golden file.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DEXPECTED=<file> -P check_sprite_bounds.cmake
#
# The game writes PREF/sprite_bounds.txt (switch sprite_bounds=1, see Sprite::load). EXPECTED holds
# its distinct lines, sorted, and matches the original's prebuilt sprite caches. After a deliberate
# change, check PREF/sprite_bounds.sorted.txt against the original archive with
# `tools/archive/ffarchive.py bounds` before it replaces EXPECTED.

file(REMOVE_RECURSE "${PREF}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env cwdiags=extended
          "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --fast check sprite_bounds=1
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "firefight exited with ${result}:\n${output}")
endif()
if(NOT output MATCHES "BUILD_SPRITES last level processed successfully")
  message(FATAL_ERROR "check mode did not get through every level:\n${output}")
endif()

function(read_sorted file var)
  file(STRINGS "${file}" lines)
  list(TRANSFORM lines REPLACE "\r$" "")
  list(REMOVE_DUPLICATES lines)
  list(SORT lines)
  set(${var} "${lines}" PARENT_SCOPE)
endfunction()

read_sorted("${PREF}/sprite_bounds.txt" actual)
read_sorted("${EXPECTED}" expected)
if(NOT actual OR NOT expected)
  message(FATAL_ERROR "no sprite bounds in ${PREF}/sprite_bounds.txt or ${EXPECTED}")
endif()
list(JOIN actual "\n" text)
file(WRITE "${PREF}/sprite_bounds.sorted.txt" "${text}\n")

set(missing ${expected})
list(REMOVE_ITEM missing ${actual})
set(extra ${actual})
list(REMOVE_ITEM extra ${expected})
list(LENGTH actual count)
if(missing OR extra)
  foreach(line IN LISTS missing)
    message(SEND_ERROR "expected: ${line}")
  endforeach()
  foreach(line IN LISTS extra)
    message(SEND_ERROR "actual:   ${line}")
  endforeach()
  message(FATAL_ERROR "sprite bounds differ from ${EXPECTED} (all of them: ${PREF}/sprite_bounds.sorted.txt)")
endif()
message(STATUS "${count} sprites match ${EXPECTED}")
