# Packaging (cpack): the game with its data and soundtrack, the README and the license.
#   Windows: a zip with firefight.exe, the DLLs it needs and the MSVC runtime
#   Linux:   a tar.gz (built on Ubuntu 24.04, so it needs glibc 2.39 or newer)
#   macOS:   a disk image with "Fire Fight.app", signed ad hoc only (not notarised)
# The game finds data/ next to its executable, or in the app's Resources folder on macOS, and
# music/ next to data/ (game/main.cpp).
#
#   cmake --build --preset <preset>-release
#   cpack --config build/<preset>/CPackConfig.cmake -C Release -B build/<preset>/package

if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)  # normalise install destinations such as ./data
endif()

if(APPLE)
  set(app "Fire Fight.app/Contents")
  install(TARGETS firefight RUNTIME DESTINATION "${app}/MacOS")
  configure_file("${CMAKE_CURRENT_LIST_DIR}/Info.plist.in" "${PROJECT_BINARY_DIR}/Info.plist" @ONLY)
  install(FILES "${PROJECT_BINARY_DIR}/Info.plist" DESTINATION "${app}")
  install(FILES "${CMAKE_CURRENT_LIST_DIR}/firefight.icns" DESTINATION "${app}/Resources")
  set(resources "${app}/Resources")
elseif(WIN32)
  install(TARGETS firefight
    RUNTIME_DEPENDENCIES  # the MSVC runtime comes from InstallRequiredSystemLibraries below
      PRE_EXCLUDE_REGEXES "[Aa][Pp][Ii]-[Mm][Ss]-.*" "[Ee][Xx][Tt]-[Mm][Ss]-.*"
                          "[Vv][Cc][Rr][Uu][Nn][Tt][Ii][Mm][Ee].*" "[Mm][Ss][Vv][Cc][Pp].*"
                          "[Cc][Oo][Nn][Cc][Rr][Tt].*" "[Uu][Cc][Rr][Tt][Bb][Aa][Ss][Ee].*"
      POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*\\.dll"
      DIRECTORIES $<TARGET_FILE_DIR:firefight>
    RUNTIME DESTINATION .)
  set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
  include(InstallRequiredSystemLibraries)
  set(resources .)
else()
  install(TARGETS firefight RUNTIME DESTINATION .)
  set(resources .)
endif()

install(DIRECTORY "${FF_DATA_DIR}/" DESTINATION "${resources}/data")
install(DIRECTORY "${FF_MUSIC_DIR}/" DESTINATION "${resources}/music")
install(FILES "${PROJECT_SOURCE_DIR}/README.md" "${PROJECT_SOURCE_DIR}/LICENSE" DESTINATION .)

if(APPLE)
  # The linker signs only the executable; the whole bundle needs a (here ad hoc) signature,
  # or macOS calls a downloaded app damaged.
  install(CODE [[
    execute_process(COMMAND codesign --force --deep --sign - "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/Fire Fight.app"
                    RESULT_VARIABLE signed)
    if(NOT signed EQUAL 0)
      message(FATAL_ERROR "codesign failed")
    endif()
  ]])
endif()

set(CPACK_PACKAGE_NAME "firefight")
set(CPACK_PACKAGE_VENDOR "Chaos Works (1996); port by Maciej Miąsik")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fire Fight, the 1996 top-down shooter by Chaos Works")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(WIN32)
  set(CPACK_GENERATOR ZIP)
  set(CPACK_SYSTEM_NAME windows-x64)
elseif(APPLE)
  set(CPACK_GENERATOR DragNDrop)
  set(CPACK_SYSTEM_NAME macos-arm64)
  set(CPACK_DMG_VOLUME_NAME "Fire Fight")
else()
  set(CPACK_GENERATOR TGZ)
  set(CPACK_SYSTEM_NAME linux-x64)
endif()
set(CPACK_PACKAGE_FILE_NAME "firefight-${PROJECT_VERSION}-${CPACK_SYSTEM_NAME}")
include(CPack)
