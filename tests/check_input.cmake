# Plays an input script headless and compares the input states it produced with a golden file.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DSCRIPT=<file> -DEXPECTED=<file>
#         -DFRAMES=<n> -P check_input.cmake
#
# input_dump=1 (see Eem::dump_state) writes PREF/input_states.txt: the local player's input
# state at each tick where it changes. It depends only on the script, not on the simulation, so
# it is the same on every platform. It is what demos record and network play sends.
# After a deliberate change, check the new PREF/input_states.txt by hand before it replaces the
# golden file.

file(REMOVE_RECURSE "${PREF}")
execute_process(
  COMMAND "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --fast
          --input "${SCRIPT}" --quit-frames "${FRAMES}" input_dump=1
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "firefight exited with ${result}:\n${output}")
endif()
if(NOT EXISTS "${PREF}/input_states.txt")
  message(FATAL_ERROR "no ${PREF}/input_states.txt:\n${output}")
endif()

# Lines as lists; a typed ';' would split a list item, so it is masked.
function(read_lines file var)
  file(READ "${file}" text)
  string(REPLACE "\r\n" "\n" text "${text}")
  string(REPLACE ";" "<semicolon>" text "${text}")
  string(REGEX REPLACE "\n$" "" text "${text}")
  string(REPLACE "\n" ";" text "${text}")
  set(${var} "${text}" PARENT_SCOPE)
endfunction()
read_lines("${PREF}/input_states.txt" actual)
read_lines("${EXPECTED}" expected)
list(LENGTH actual actual_count)
list(LENGTH expected expected_count)
set(failed 0)
foreach(i RANGE 0 999999)
  if(i GREATER_EQUAL actual_count AND i GREATER_EQUAL expected_count)
    break()
  endif()
  set(a "(none)")
  set(e "(none)")
  if(i LESS actual_count)
    list(GET actual ${i} a)
  endif()
  if(i LESS expected_count)
    list(GET expected ${i} e)
  endif()
  if(NOT a STREQUAL e)
    message(SEND_ERROR "line ${i}:\n  expected ${e}\n  actual   ${a}")
    set(failed 1)
    break()
  endif()
endforeach()
if(failed)
  message(FATAL_ERROR "input states differ from ${EXPECTED} (all lines: ${PREF}/input_states.txt)")
endif()
message(STATUS "${actual_count} lines match ${EXPECTED}")
