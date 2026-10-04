# Replays a demo with the sound on, through SDL's disk audio driver (it writes the mixed output to
# a file, so no audio device is needed), and checks that the sound system opened, the soundtrack
# was found and the mission's track played, that the output isn't silent, and that the replay
# stayed in sync to the end: sound must not touch the simulation.
#
#   cmake -DFIREFIGHT=<exe> -DDATA=<dir> -DPREF=<dir> -P check_sound.cmake
#
# The soundtrack is the music/ directory next to DATA.

file(REMOVE_RECURSE "${PREF}")
file(MAKE_DIRECTORY "${PREF}")
set(raw "${PREF}/sound.raw")
execute_process(
  COMMAND ${CMAKE_COMMAND} -E env SDL_AUDIODRIVER=disk "SDL_DISKAUDIOFILE=${raw}"
          "${FIREFIGHT}" --data "${DATA}" --pref "${PREF}" --headless --sound --fast --demo level1
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "firefight exited with ${result}:\n${output}")
endif()

foreach(expected
    "SDL_mixer: disk driver"
    "music: 8 tracks"
    "music: track 2, looped"
    "demo level1: in sync to the end")
  string(FIND "${output}" "${expected}" at)
  if(at EQUAL -1)
    message(FATAL_ERROR "\"${expected}\" missing from the log:\n${output}")
  endif()
endforeach()
foreach(unexpected "no sound" "no music" "does not load" "does not play" "without object_deleted")
  string(FIND "${output}" "${unexpected}" at)
  if(NOT at EQUAL -1)
    message(FATAL_ERROR "\"${unexpected}\" in the log:\n${output}")
  endif()
endforeach()

# 16-bit stereo at 44,100 Hz: at least half a second, and the last 64 KB not all zero.
file(SIZE "${raw}" size)
if(size LESS 88200)
  message(FATAL_ERROR "only ${size} bytes of audio output")
endif()
math(EXPR offset "${size} - 65536")
file(READ "${raw}" tail OFFSET ${offset} LIMIT 65536 HEX)
string(REGEX MATCH "[1-9a-f]" sound "${tail}")
if(NOT sound)
  message(FATAL_ERROR "the audio output is silent")
endif()
message(STATUS "${size} bytes of audio output")
