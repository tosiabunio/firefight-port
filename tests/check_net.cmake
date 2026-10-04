# Plays a two-player network game on this machine: a host and a client, both headless and
# --fast, each with its own input script (accept the level in the lobby, then fly). Checks that
# they connected, both reached the mission, the per-frame sync check never failed, and both ended
# normally.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -DSCRIPTS=<dir> -DPORT=<port> -P check_net.cmake
#
# execute_process runs its commands at the same time (as a pipeline), which is how the two
# peers run side by side; the client retries until the host is listening.

cmake_minimum_required(VERSION 3.25)

file(REMOVE_RECURSE "${PREF}")
file(MAKE_DIRECTORY "${PREF}/host" "${PREF}/client")
set(common --data "${DATA}" --headless --fast --port ${PORT} --quit-frames 1200)
execute_process(
  COMMAND "${FIREFIGHT}" ${common} --pref "${PREF}/host" --host 2
          --input "${SCRIPTS}/net_host.txt"
  COMMAND "${FIREFIGHT}" ${common} --pref "${PREF}/client" --join 127.0.0.1
          --input "${SCRIPTS}/net_client.txt"
  RESULTS_VARIABLE results
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output
  TIMEOUT 240)

set(failed 0)
foreach(peer host client)
  list(POP_FRONT results result)
  set(log "${PREF}/${peer}/Firefght.log")
  if(NOT EXISTS "${log}")
    message(SEND_ERROR "${peer}: no log (exit ${result})")
    set(failed 1)
    continue()
  endif()
  file(READ "${log}" text)
  if(NOT result STREQUAL "0")
    message(SEND_ERROR "${peer} exited with ${result}")
    set(failed 1)
  endif()
  if(peer STREQUAL "host")
    set(connected "network: player 2 (")
  else()
    set(connected "network: joined as player 2")
  endif()
  foreach(expected "${connected}" "all players connected" "reading directory: net1.dir"
                   "INTERACTIVE section entered" "completed successfully")
    string(FIND "${text}" "${expected}" at)
    if(at EQUAL -1)
      message(SEND_ERROR "${peer}: \"${expected}\" missing from ${log}")
      set(failed 1)
    endif()
  endforeach()
  # "sychronization" (sic) and "synchronization": both failures of the sync check.
  foreach(unexpected "chronization failed" "without object_deleted")
    string(FIND "${text}" "${unexpected}" at)
    if(NOT at EQUAL -1)
      message(SEND_ERROR "${peer}: \"${unexpected}\" in ${log}")
      set(failed 1)
    endif()
  endforeach()
endforeach()
if(failed)
  # The last lines of the processes' output (both logs are echoed to stderr, and a crash reports
  # there too) and of each log.
  file(WRITE "${PREF}/output.txt" "${output}")
  foreach(file output.txt host/Firefght.log client/Firefght.log)
    if(EXISTS "${PREF}/${file}")
      file(STRINGS "${PREF}/${file}" lines)
      list(LENGTH lines n)
      if(n GREATER 40)
        math(EXPR first "${n} - 40")
        list(SUBLIST lines ${first} 40 lines)
      endif()
      list(JOIN lines "\n" text)
      message("--- end of ${file}:\n${text}")
    endif()
  endforeach()
  message(FATAL_ERROR "the network game failed (logs in ${PREF})")
endif()
message(STATUS "host and client played in sync")
