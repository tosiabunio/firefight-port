# Game targets. The original code is pre-standard MSVC 4 C++, so these targets use
# ff_legacy_options; new code (compat) uses ff_modern_options.

# --- compat: CRT shims and the Win32/DirectX stand-ins (see compat/win32.h) --------------
add_library(compat STATIC
  compat/browser.cpp
  compat/crt.cpp
  compat/msvc4.cpp
  compat/win32.cpp)
# Sources include these as <compat/crt.h> and <compat/win32.h>.
target_include_directories(compat PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
ff_modern_options(compat)

set(FF_ENGINE_MODULES 1ba 1cw 1ee 1io 1lg 1mm 1rg 1sp 1ss)

# --- cwengine: the CW engine modules ------------------------------------------------------
add_library(cwengine STATIC
  engine/1ba/1ba.cpp
  engine/1cw/1cw.cpp
  engine/1ee/1ee_demo.cpp
  engine/1ee/1ee_joys.cpp
  engine/1ee/1ee_kbrd.cpp
  engine/1ee/1ee_main.cpp
  engine/1ee/1ee_mous.cpp
  engine/1ee/1ee_netw.cpp
  engine/1ee/1ee_timr.cpp
  engine/1ee/1ee_virt.cpp
  engine/1io/1io_cmpr.cpp
  engine/1io/1io_io.cpp
  engine/1io/1io_txt.cpp
  engine/1lg/1lg_comm.cpp
  engine/1lg/1lg_main.cpp
  engine/1mm/1mm.cpp
  engine/1rg/1rg.cpp
  engine/1sp/1sp_asm.cpp
  engine/1sp/1sp_data.cpp
  engine/1sp/1sp_flic.cpp
  engine/1sp/1sp_font.cpp
  engine/1sp/1sp_ldsa.cpp
  engine/1sp/1sp_lev.cpp
  engine/1sp/1sp_lmai.cpp
  engine/1sp/1sp_lreb.cpp
  engine/1sp/1sp_main.cpp
  engine/1sp/1sp_pale.cpp
  engine/1sp/1sp_scr1.cpp
  engine/1sp/1sp_scr2.cpp
  engine/1sp/1sp_vdrv.cpp
  engine/1sp/1sp_vide.cpp
  engine/1ss/1ss.cpp
  engine/1ss/1ss_mix.cpp
  engine/1ss/1ss_smp.cpp
  engine/1ss/1ss_song.cpp)
# The network transport: ENet; the Emscripten build (phase 9) has a stand-in without network
# play until its WebSocket transport.
if(EMSCRIPTEN)
  target_sources(cwengine PRIVATE engine/1ee/1ee_enet_none.cpp)
else()
  target_sources(cwengine PRIVATE engine/1ee/1ee_enet.cpp)
endif()
# Modules include each other's headers as <1xx.h>, so every module directory is public.
foreach(module IN LISTS FF_ENGINE_MODULES)
  target_include_directories(cwengine PUBLIC engine/${module})
endforeach()
target_include_directories(cwengine PUBLIC engine/common)
# EXCLUDE_LIBS disables the `#pragma comment(lib, ...)` auto-linking in the module headers.
target_compile_definitions(cwengine PUBLIC EXCLUDE_LIBS
  $<$<AND:$<CONFIG:Debug>,$<NOT:$<CXX_COMPILER_ID:MSVC>>>:_DEBUG>)
target_link_libraries(cwengine PUBLIC compat ff::sdl2 ff::mixer ff::enet)
ff_legacy_options(cwengine)

# --- regdata: settings model shared with the original launcher ----------------------------
add_library(regdata STATIC regdata/1regdata.cpp)
target_include_directories(regdata PUBLIC regdata)
target_link_libraries(regdata PUBLIC cwengine)
ff_legacy_options(regdata)

# --- firefight: the game ------------------------------------------------------------------
add_executable(firefight
  game/main.cpp
  game/alien.cpp
  game/alienwea.cpp
  game/build.cpp
  game/data.cpp
  game/debug.cpp
  game/game.cpp
  game/gman.cpp
  game/gobj.cpp
  game/input_script.cpp
  game/item.cpp
  game/kbdsta.cpp
  game/list.cpp
  game/lut.cpp
  game/menu.cpp
  game/msg.cpp
  game/my.cpp
  game/myweap.cpp
  game/neutral.cpp
  game/nonet.cpp
  game/params.cpp
  game/pilots.cpp
  game/print.cpp
  game/queue.cpp
  game/stale.cpp
  game/stale_gen.cpp
  game/sysset.cpp
  game/tools.cpp
  game/view.cpp
  game/world.cpp)
target_include_directories(firefight PRIVATE game)
# UNPROTECT: no CD-ROM check (the original retail builds except "Production" defined it too).
target_compile_definitions(firefight PRIVATE UNPROTECT
  # Development fallback for the data directory (see game/main.cpp).
  FF_DEFAULT_DATA_DIR="${FF_DATA_DIR}")
target_link_libraries(firefight PRIVATE regdata cwengine)
ff_legacy_options(firefight)
if(WIN32)
  # A GUI program, with the original icon; started from a terminal it uses that console.
  target_sources(firefight PRIVATE game/win_console.cpp game/firefight.rc)
  set_target_properties(firefight PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# --- the browser build (phase 9) ----------------------------------------------------------
# Static files in web/<config> of the build directory, ready for any static web server:
#   index.html                     the page (web/index.html), which starts the game on a click
#   firefight.js, firefight.wasm   the game
#   firefight.data                 data/ as one package, preloaded by the page, mounted at /data
#   music/track02.ogg ...          the soundtrack as Ogg Vorbis, transcoded from music/*.flac
#                                  with oggenc (vorbis-tools); the page downloads it after the data
# It has no tests: the node build runs them.
if(FF_BROWSER)
  set(FF_WEB_DIR "${CMAKE_BINARY_DIR}/web/$<CONFIG>")
  set_target_properties(firefight PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${FF_WEB_DIR}")
  target_link_options(firefight PRIVATE
    "SHELL:--preload-file '${FF_DATA_DIR}@/data'"
    "SHELL:--pre-js '${CMAKE_SOURCE_DIR}/web/pre.js'")
  set_property(TARGET firefight APPEND PROPERTY LINK_DEPENDS "${CMAKE_SOURCE_DIR}/web/pre.js")

  # About 104 kbit/s at quality 3: 27 MB for the 35 minutes, instead of 232 MB of FLAC.
  find_program(FF_OGGENC oggenc)
  file(GLOB FF_FLAC_TRACKS "${FF_MUSIC_DIR}/track*.flac")
  set(FF_WEB_TRACKS "")
  set(FF_WEB_MUSIC "")
  if(FF_OGGENC)
    foreach(flac IN LISTS FF_FLAC_TRACKS)
      get_filename_component(track "${flac}" NAME_WE)
      set(ogg "${FF_WEB_DIR}/music/${track}.ogg")
      add_custom_command(OUTPUT "${ogg}"
        COMMAND "${FF_OGGENC}" -Q -q 3 -o "${ogg}" "${flac}"
        DEPENDS "${flac}"
        COMMENT "Encoding ${track} as Ogg Vorbis"
        VERBATIM)
      list(APPEND FF_WEB_MUSIC "${ogg}")
      list(APPEND FF_WEB_TRACKS "'${track}.ogg'")
    endforeach()
  else()
    message(WARNING "oggenc (vorbis-tools) not found: the browser build has no music")
  endif()
  add_custom_target(web_music ALL DEPENDS ${FF_WEB_MUSIC})

  # The page, with the version and the track list.
  list(JOIN FF_WEB_TRACKS ", " FF_WEB_TRACKS)
  configure_file("${CMAKE_SOURCE_DIR}/web/index.html" "${CMAKE_CURRENT_BINARY_DIR}/index.html" @ONLY)
  file(GENERATE OUTPUT "${FF_WEB_DIR}/index.html" INPUT "${CMAKE_CURRENT_BINARY_DIR}/index.html")
  return()
endif()

# Tests that run the game from a script (tests/check_*.cmake) start it through EMULATOR: Node.js
# for the Emscripten build, whose firefight is a .js file; nothing on the desktop. Tests that run
# a target directly get the emulator from CMake.
set(FF_EMULATOR "-DEMULATOR=${CMAKE_CROSSCOMPILING_EMULATOR}")

# The paths the tests give the game. Emscripten's file system under Node is POSIX and can't take
# Windows drive paths (D:/...), so on a Windows host the Emscripten tests get paths relative to
# their working directory, this binary directory.
if(EMSCRIPTEN AND CMAKE_HOST_WIN32)
  file(RELATIVE_PATH FF_TEST_DATA "${CMAKE_CURRENT_BINARY_DIR}" "${FF_DATA_DIR}")
  file(RELATIVE_PATH FF_TEST_DEMOS "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/tests/demos")
  set(FF_TEST_BIN ".")
else()
  set(FF_TEST_DATA "${FF_DATA_DIR}")
  set(FF_TEST_DEMOS "${CMAKE_SOURCE_DIR}/tests/demos")
  set(FF_TEST_BIN "${CMAKE_CURRENT_BINARY_DIR}")
endif()

# Phase 2 exit check: boot headless against data/, run the title loop on the 30 Hz clock until
# it starts an attract demo (after 20 s), then quit as if the window were closed. Rebuilds
# missing sprite caches in data/ (git-ignored). The log is echoed to stderr, where the test
# requires the loaded sprite totals, the demo's interactive section and a clean finish.
add_test(NAME headless_run
  COMMAND firefight --data "${FF_TEST_DATA}" --pref "${FF_TEST_BIN}/pref"
          --headless --quit-after 30)
set_tests_properties(headless_run PROPERTIES
  PASS_REGULAR_EXPRESSION "99 sprites, 2510 phases.*INTERACTIVE section entered.*quit time reached.*heap ok.*Program status:  completed successfully"
  FAIL_REGULAR_EXPRESSION "execution failed"
  TIMEOUT 300)

# Phase 3 rendering check: in fast mode (one simulation step per frame) every 20th frame of the
# title sequence (publisher logos, then the logo flying in) must match its golden hash on every
# platform. It covers the palette tables, the sprite build from the FLC masters, the blitter,
# the fonts, the palette conversion and the title's float motion.
add_test(NAME golden_title
  COMMAND ${CMAKE_COMMAND} ${FF_EMULATOR}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/golden_title
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/title_frames.sha256
          "-DARGS=--shot-every 20 --quit-frames 200"
          -P ${CMAKE_SOURCE_DIR}/tests/check_frames.cmake)
set_tests_properties(golden_title PROPERTIES TIMEOUT 300)

# Sprite build check: check mode loads the level of every mission. The phase bounds, pixel data
# and palette tables of everything it builds must match the golden files, which match the
# original's prebuilt caches. The phase bounds decide which level objects look_at sees and builds,
# so they are simulation state.
add_test(NAME sprite_build
  COMMAND ${CMAKE_COMMAND} ${FF_EMULATOR}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/sprite_build
          -DGOLDEN=${CMAKE_SOURCE_DIR}/tests/golden
          -P ${CMAKE_SOURCE_DIR}/tests/check_sprites.cmake)
set_tests_properties(sprite_build PROPERTIES TIMEOUT 300)

# Phase 5: the MSVC 4 rand and qsort clones must reproduce the original exe's results
# (tests/golden/crt_*.txt, from tools/archive/crt_vectors.py).
add_executable(crt_vectors ${CMAKE_SOURCE_DIR}/tests/crt_vectors.cpp)
target_link_libraries(crt_vectors PRIVATE compat)
ff_modern_options(crt_vectors)
add_test(NAME crt_vectors
  COMMAND crt_vectors ${CMAKE_SOURCE_DIR}/tests/golden/crt_rand.txt
          ${CMAKE_SOURCE_DIR}/tests/golden/crt_qsort.txt)

# Phase 5 determinism check: the 8 original attract demos (data/demo) must replay in sync to the
# end of their recordings. --demo exits with 3 when the replay goes out of sync, 4 when it stops
# early (game.cpp, Game::go).
foreach(demo IN ITEMS level1 level2 level3 level4 level1C level2C level3C level4C)
  add_test(NAME demo_${demo}
    COMMAND firefight --data "${FF_TEST_DATA}" --pref "${FF_TEST_BIN}/demo_${demo}"
            --headless --fast --demo ${demo})
  set_tests_properties(demo_${demo} PROPERTIES
    PASS_REGULAR_EXPRESSION "in sync to the end"
    FAIL_REGULAR_EXPRESSION "without object_deleted"
    TIMEOUT 300)
endforeach()

# Golden demos recorded with the port (tests/demos, made by tests/record_demo.cmake from the
# input scripts next to them): missions and skills the original demos don't cover. They must
# replay in sync to the end on every platform, the cross-platform determinism gate.
foreach(demo IN ITEMS gray1 net3 green3 gray2)
  add_test(NAME golden_${demo}
    COMMAND firefight --data "${FF_TEST_DATA}" --pref "${FF_TEST_BIN}/golden_${demo}"
            --headless --fast --demo "!${FF_TEST_DEMOS}/${demo}.rec")
  set_tests_properties(golden_${demo} PROPERTIES
    PASS_REGULAR_EXPRESSION "in sync to the end"
    FAIL_REGULAR_EXPRESSION "without object_deleted"
    TIMEOUT 300)
endforeach()

# Phase 6 sound check: demo level1 replays with the sound on, through SDL's disk audio driver.
# The sound system must start, the mission's soundtrack track play and the output not be silent,
# and the replay must stay in sync (tests/check_sound.cmake).
add_test(NAME sound_play
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/sound_play
          -P ${CMAKE_SOURCE_DIR}/tests/check_sound.cmake)
set_tests_properties(sound_play PROPERTIES TIMEOUT 300)

# Phase 7 network check: a host and a client on this machine play a two-player game over ENet,
# each with an input script (accept the level in the lobby, then fly). Both must reach the mission,
# never fail the per-frame sync check, and end normally (tests/check_net.cmake).
add_test(NAME net_play
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/net_play
          -DSCRIPTS=${CMAKE_SOURCE_DIR}/tests/input
          -DPORT=19962
          -P ${CMAKE_SOURCE_DIR}/tests/check_net.cmake)
set_tests_properties(net_play PROPERTIES TIMEOUT 300)
# The same, with the client finding the game by LAN discovery (--join lan).
add_test(NAME net_lan
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/net_lan
          -DSCRIPTS=${CMAKE_SOURCE_DIR}/tests/input
          -DPORT=19964 -DJOIN=lan
          -P ${CMAKE_SOURCE_DIR}/tests/check_net.cmake)
set_tests_properties(net_lan PROPERTIES TIMEOUT 300)

# Phase 4 input check: an input script starts the first mission from the title and flies it with
# the keyboard, the mouse and a virtual game controller, switching with F11. The input states it
# produces (key scan codes and text, mouse position and buttons, joystick bits, virtual keys) must
# match the golden file on every platform: demos and network play exchange exactly these.
add_test(NAME input_play
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_TEST_DATA}
          -DPREF=${FF_TEST_BIN}/input_play
          -DSCRIPT=${CMAKE_SOURCE_DIR}/tests/input/mission1.txt
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/input_states.txt
          -DFRAMES=1000
          -P ${CMAKE_SOURCE_DIR}/tests/check_input.cmake)
set_tests_properties(input_play PROPERTIES TIMEOUT 300)

# Not in the Emscripten build yet (phase 9): it has no ENet (net_*), its SDL2_mixer plays no FLAC
# (sound_play needs the mission's track) and its SDL2 has no virtual game controller (input_play).
if(EMSCRIPTEN)
  set_tests_properties(sound_play net_play net_lan input_play PROPERTIES DISABLED TRUE)
endif()

# Data check: every file in data/ and music/ must have the SHA-256 listed in the golden file,
# which was checked against the original archive, and no file may be missing or extra.
add_test(NAME data_files
  COMMAND ${CMAKE_COMMAND}
          -DROOT=${CMAKE_SOURCE_DIR}
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/data_files.sha256
          -P ${CMAKE_SOURCE_DIR}/tests/check_data.cmake)
