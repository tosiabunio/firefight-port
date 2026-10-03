# Third-party dependencies, normalised to three interface targets:
#   ff::sdl2   SDL2 (plus SDL2main where the platform needs it)
#   ff::mixer  SDL2_mixer
#   ff::enet   ENet
# Works with vcpkg (the default, see vcpkg.json) and with distro packages.

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)

add_library(ff_sdl2 INTERFACE)
target_link_libraries(ff_sdl2 INTERFACE
  $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
  $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>)
add_library(ff::sdl2 ALIAS ff_sdl2)

add_library(ff_mixer INTERFACE)
target_link_libraries(ff_mixer INTERFACE
  $<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>)
add_library(ff::mixer ALIAS ff_mixer)

# vcpkg ships a CMake package; distros usually only ship pkg-config metadata.
find_package(unofficial-enet CONFIG QUIET)
add_library(ff_enet INTERFACE)
if(TARGET unofficial::enet::enet)
  target_link_libraries(ff_enet INTERFACE unofficial::enet::enet)
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(ENET REQUIRED IMPORTED_TARGET libenet)
  target_link_libraries(ff_enet INTERFACE PkgConfig::ENET)
endif()
if(WIN32)
  target_link_libraries(ff_enet INTERFACE ws2_32 winmm)
endif()
add_library(ff::enet ALIAS ff_enet)
