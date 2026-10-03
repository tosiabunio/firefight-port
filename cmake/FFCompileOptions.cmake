# Compile options shared by every Fire Fight target.

# Flags every target gets. The original code relies on wrapping signed arithmetic,
# type punning through casts and a signed `char`, so make all three explicit.
# -ffp-contract=off: Clang (on arm64 in particular) and GCC may otherwise fuse a*b+c into one
# multiply-add, which rounds differently from separate operations; the simulation's float
# arithmetic must give the same results on every platform.
function(ff_common_options target)
  if(MSVC)
    # MSVC: signed char, no strict aliasing, wrapping arithmetic and no contraction (/fp:precise
    # without /fp:contract) are already the default.
    target_compile_options(${target} PRIVATE /utf-8 /Zc:__cplusplus)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  else()
    target_compile_options(${target} PRIVATE -fwrapv -fno-strict-aliasing -fsigned-char -ffp-contract=off)
  endif()
endfunction()

# New code written for the port: strict warnings.
function(ff_modern_options target)
  ff_common_options(${target})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# The original 1996 sources (game, engine, regdata). Pre-standard MSVC 4 C++: string literals
# bound to `char*` (~1,540 sites) and MSVC-only pragmas are tolerated for now; everything
# else is fixed in the source.
function(ff_legacy_options target)
  ff_common_options(${target})
  if(MSVC)
    target_compile_options(${target} PRIVATE /Zc:strictStrings- /wd4996)
  else()
    target_compile_options(${target} PRIVATE -Wno-write-strings -Wno-unknown-pragmas)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_compile_options(${target} PRIVATE -Wno-c++11-compat-deprecated-writable-strings
        -Wno-ignored-pragmas -Wno-pragma-pack)
    endif()
  endif()
endfunction()
