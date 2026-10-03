# Phase 0 dependency smoke test: SDL2 window + paletted present, SDL2_mixer FLAC/WAV, ENet loopback.
add_executable(ff_smoke ff_smoke.cpp)
ff_modern_options(ff_smoke)
target_link_libraries(ff_smoke PRIVATE ff::sdl2 ff::mixer ff::enet)

add_test(NAME smoke
  COMMAND ff_smoke --headless --frames 30
          --music "${FF_MUSIC_DIR}/track02.flac"
          --wav "${FF_DATA_DIR}/!global/sounds/beephi.wav")
