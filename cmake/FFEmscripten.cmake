# The Emscripten build (phase 9): the game as WebAssembly. FF_WEB_PLATFORM says where it runs:
#   node  headless under Node.js, on the host's file system. The tests run this build, so they
#         show whether the simulation stays bit-exact in WebAssembly.
# The browser build comes later.

set(FF_WEB_PLATFORM "node" CACHE STRING "Where the Emscripten build runs (node)")
set_property(CACHE FF_WEB_PLATFORM PROPERTY STRINGS node)
if(NOT FF_WEB_PLATFORM STREQUAL "node")
  message(FATAL_ERROR "FF_WEB_PLATFORM=${FF_WEB_PLATFORM}: only node is supported so far")
endif()

# The control flow relies on C++ exceptions (TerminateMission, Closed, Failure, ...), which
# Emscripten doesn't catch by default. Native WebAssembly exceptions, for every target.
add_compile_options(-fwasm-exceptions)
add_link_options(-fwasm-exceptions
  -sALLOW_MEMORY_GROWTH=1
  -sSTACK_SIZE=8MB)  # Emscripten's default is 64 KB; Linux gives the main thread 8 MB

if(FF_WEB_PLATFORM STREQUAL "node")
  add_link_options(
    -sENVIRONMENT=node
    -sNODERAWFS=1     # the host's file system: the repository's data/ and the paths the tests give
    -sNODE_HOST_ENV=1 # the host's environment (cwdiags=extended)
    -sEXIT_RUNTIME=1) # static destructors and the quit manager run; main's result is the exit code
endif()
