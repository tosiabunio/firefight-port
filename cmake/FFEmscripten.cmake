# The Emscripten build (phase 9): the game as WebAssembly. FF_WEB_PLATFORM says where it runs:
#   node  headless under Node.js, on the host's file system. The tests run this build, so they
#         show whether the simulation stays bit-exact in WebAssembly.
#   web   in a web browser (FF_BROWSER): a page, the game with JSPI and with Asyncify, its data
#         as one preloaded package and the music as Ogg Vorbis, all static files in web/<config>
#         of the build directory (see source/CMakeLists.txt). No tests: the node build runs them.

set(FF_WEB_PLATFORM "node" CACHE STRING "Where the Emscripten build runs (node or web)")
set_property(CACHE FF_WEB_PLATFORM PROPERTY STRINGS node web)
if(NOT FF_WEB_PLATFORM MATCHES "^(node|web)$")
  message(FATAL_ERROR "FF_WEB_PLATFORM=${FF_WEB_PLATFORM}: must be node or web")
endif()
set(FF_BROWSER OFF)
if(FF_WEB_PLATFORM STREQUAL "web")
  set(FF_BROWSER ON)
endif()

# The control flow relies on C++ exceptions (TerminateMission, Closed, Failure, ...), which
# Emscripten doesn't catch by default. Native WebAssembly exceptions, for every target.
add_compile_options(-fwasm-exceptions)
add_link_options(-fwasm-exceptions
  -sALLOW_MEMORY_GROWTH=1
  -sSTACK_SIZE=8MB  # Emscripten's default is 64 KB; Linux gives the main thread 8 MB
  -sEXIT_RUNTIME=1) # static destructors and the quit manager run; main's result is the exit code

if(FF_WEB_PLATFORM STREQUAL "node")
  add_link_options(
    -sENVIRONMENT=node
    -sNODERAWFS=1      # the host's file system: the repository's data/ and the paths the tests give
    -sNODE_HOST_ENV=1) # the host's environment (cwdiags=extended)
else()
  add_compile_definitions(FF_BROWSER)
  # The game keeps its blocking loops. JSPI or Asyncify (one executable each, see
  # source/CMakeLists.txt) suspends it while the browser shows a frame, plays the sound and
  # delivers input (compat/browser.cpp, SDL_Delay).
  add_link_options(
    -sENVIRONMENT=web
    -sINVOKE_RUN=0  # the page starts the game on a click: sound, pointer lock and full screen need one
    -sFORCE_FILESYSTEM=1  # the data package is loaded by its own script (file_packager)
    # FS: the log is /tmp/Firefght.log, for Module.FS.readFile in the browser's console.
    -sEXPORTED_RUNTIME_METHODS=callMain,FS
    -lidbfs.js)     # the preferences directory, kept in IndexedDB (web/pre.js)
endif()
