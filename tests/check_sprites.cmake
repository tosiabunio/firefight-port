# Runs the game in check mode, which loads the level of every mission, and compares the sprites
# and palette tables it builds with golden files.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DGOLDEN=<dir> -P check_sprites.cmake
#
# The switch sprite_dump=1 (see Spr::dump_printf) makes the game write PREF/sprite_bounds.txt
# (phase bounds) and PREF/sprite_data.txt (size and CRC-32 of the pixel data and palette tables).
# GOLDEN/sprite_bounds.txt and GOLDEN/sprite_data.txt hold their distinct lines, sorted, and match
# the original's prebuilt caches. After a deliberate change, check the dump against the original
# archive (`tools/archive/ffarchive.py sprites PREF`) before the PREF/*.sorted.txt files replace
# the golden ones.

file(REMOVE_RECURSE "${PREF}")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env cwdiags=extended
          "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --fast check sprite_dump=1
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

set(failed 0)
foreach(name IN ITEMS sprite_bounds sprite_data)
  read_sorted("${PREF}/${name}.txt" actual)
  read_sorted("${GOLDEN}/${name}.txt" expected)
  if(NOT actual OR NOT expected)
    message(FATAL_ERROR "no lines in ${PREF}/${name}.txt or ${GOLDEN}/${name}.txt")
  endif()
  list(JOIN actual "\n" text)
  file(WRITE "${PREF}/${name}.sorted.txt" "${text}\n")

  set(missing ${expected})
  list(REMOVE_ITEM missing ${actual})
  set(extra ${actual})
  list(REMOVE_ITEM extra ${expected})
  list(LENGTH actual count)
  if(missing OR extra)
    foreach(line IN LISTS missing)
      message(SEND_ERROR "${name} expected: ${line}")
    endforeach()
    foreach(line IN LISTS extra)
      message(SEND_ERROR "${name} actual:   ${line}")
    endforeach()
    set(failed 1)
  else()
    message(STATUS "${name}: ${count} lines match ${GOLDEN}/${name}.txt")
  endif()
endforeach()
if(failed)
  message(FATAL_ERROR "sprites differ from the golden files (all lines: ${PREF}/*.sorted.txt)")
endif()
