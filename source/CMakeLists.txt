# Game targets. The original code is pre-standard MSVC 4 C++, so these targets use
# ff_legacy_options; new code (compat) uses ff_modern_options.

# --- compat: CRT shims and the Win32/DirectX stand-ins (see compat/win32.h) --------------
add_library(compat STATIC
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

# Phase 2 exit check: boot headless against data/, run the title loop on the 30 Hz clock until
# it starts an attract demo (after 20 s), then quit as if the window were closed. Rebuilds
# missing sprite caches in data/ (git-ignored). The log is echoed to stderr, where the test
# requires the loaded sprite totals, the demo's interactive section and a clean finish.
add_test(NAME headless_run
  COMMAND firefight --data "${FF_DATA_DIR}" --pref "${CMAKE_CURRENT_BINARY_DIR}/pref"
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
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_DATA_DIR}
          -DPREF=${CMAKE_CURRENT_BINARY_DIR}/golden_title
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/title_frames.sha256
          "-DARGS=--shot-every 20 --quit-frames 200"
          -P ${CMAKE_SOURCE_DIR}/tests/check_frames.cmake)
set_tests_properties(golden_title PROPERTIES TIMEOUT 300)

# Sprite build check: check mode loads the level of every mission. The phase bounds, pixel data
# and palette tables of everything it builds must match the golden files, which match the
# original's prebuilt caches. The phase bounds decide which level objects look_at sees and builds,
# so they are simulation state.
add_test(NAME sprite_build
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_DATA_DIR}
          -DPREF=${CMAKE_CURRENT_BINARY_DIR}/sprite_build
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
    COMMAND firefight --data "${FF_DATA_DIR}" --pref "${CMAKE_CURRENT_BINARY_DIR}/demo_${demo}"
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
    COMMAND firefight --data "${FF_DATA_DIR}" --pref "${CMAKE_CURRENT_BINARY_DIR}/golden_${demo}"
            --headless --fast --demo "!${CMAKE_SOURCE_DIR}/tests/demos/${demo}.rec")
  set_tests_properties(golden_${demo} PROPERTIES
    PASS_REGULAR_EXPRESSION "in sync to the end"
    FAIL_REGULAR_EXPRESSION "without object_deleted"
    TIMEOUT 300)
endforeach()

# Phase 4 input check: an input script starts the first mission from the title and flies it with
# the keyboard, the mouse and a virtual game controller, switching with F11. The input states it
# produces (key scan codes and text, mouse position and buttons, joystick bits, virtual keys) must
# match the golden file on every platform: demos and network play exchange exactly these.
add_test(NAME input_play
  COMMAND ${CMAKE_COMMAND}
          -DFIREFIGHT=$<TARGET_FILE:firefight> -DDATA=${FF_DATA_DIR}
          -DPREF=${CMAKE_CURRENT_BINARY_DIR}/input_play
          -DSCRIPT=${CMAKE_SOURCE_DIR}/tests/input/mission1.txt
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/input_states.txt
          -DFRAMES=1000
          -P ${CMAKE_SOURCE_DIR}/tests/check_input.cmake)
set_tests_properties(input_play PROPERTIES TIMEOUT 300)

# Data check: every file in data/ and music/ must have the SHA-256 listed in the golden file,
# which was checked against the original archive, and no file may be missing or extra.
add_test(NAME data_files
  COMMAND ${CMAKE_COMMAND}
          -DROOT=${CMAKE_SOURCE_DIR}
          -DEXPECTED=${CMAKE_SOURCE_DIR}/tests/golden/data_files.sha256
          -P ${CMAKE_SOURCE_DIR}/tests/check_data.cmake)
