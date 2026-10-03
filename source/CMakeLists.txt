# Game targets. The original code is pre-standard MSVC 4 C++, so these targets use
# ff_legacy_options; new code (compat) uses ff_modern_options.

# --- compat: CRT shims and the Win32/DirectX stand-ins (see compat/win32.h) --------------
add_library(compat STATIC
  compat/crt.cpp
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
  game/sysset.cpp
  game/tools.cpp
  game/view.cpp
  game/world.cpp)
target_include_directories(firefight PRIVATE game)
# UNPROTECT: no CD-ROM check (the original retail builds except "Production" defined it too).
target_compile_definitions(firefight PRIVATE UNPROTECT)
target_link_libraries(firefight PRIVATE regdata cwengine)
ff_legacy_options(firefight)

# Phase 1 exit check: boot headless against data/ through Game::init_all, then shut down
# cleanly. Rebuilds missing sprite caches in data/ (git-ignored); the log goes to TEMP and is
# echoed to stderr, where the test requires the loaded sprite totals and a clean finish.
add_test(NAME headless_init COMMAND firefight --data "${FF_DATA_DIR}" --headless)
set_tests_properties(headless_init PROPERTIES
  ENVIRONMENT "TEMP=${CMAKE_CURRENT_BINARY_DIR}"
  PASS_REGULAR_EXPRESSION "99 sprites, 2510 phases.*heap ok.*Program status:  completed successfully"
  FAIL_REGULAR_EXPRESSION "execution failed"
  TIMEOUT 300)
