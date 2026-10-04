# Records a golden demo with the port: plays an input script headless, in debug mode (the
# mission screen's demo menu needs it), with the given skill, and copies the recording to OUT.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DSCRIPT=<file> -DSKILL=normal|hard
#         -DFRAMES=<n> -DOUT=<file.rec> -P record_demo.cmake
#
# The scripts in tests/demos come from tests/demos/flight.py. Record with a Release build and
# replay the result before committing it (the golden_* tests do).

file(REMOVE_RECURSE "${PREF}")
file(MAKE_DIRECTORY "${PREF}")
# The launcher's skill setting, read when the pilot is created.
file(WRITE "${PREF}/settings.ini" "[PLAYER:default]\nskill level=str:${SKILL}\n")
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env cwdiags=extended
          "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --fast
          --input "${SCRIPT}" --quit-frames "${FRAMES}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "firefight exited with ${result}:\n${output}")
endif()
if(NOT output MATCHES "demo [^\n]*demo_000.rec saved - 1 players, [0-9]+ bytes, ([0-9]+) blocks")
  message(FATAL_ERROR "no demo saved:\n${output}")
endif()
file(COPY_FILE "${PREF}/demo_000.rec" "${OUT}")
message(STATUS "${OUT}: ${CMAKE_MATCH_1} blocks")
