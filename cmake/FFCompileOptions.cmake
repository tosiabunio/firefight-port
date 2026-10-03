# Compile options shared by every Fire Fight target.

# Flags every target gets. The original code relies on wrapping signed arithmetic,
# type punning through casts and a signed `char`, so make all three explicit.
function(ff_common_options target)
  if(MSVC)
    # MSVC: signed char, no strict aliasing and wrapping arithmetic are already the default.
    target_compile_options(${target} PRIVATE /utf-8 /Zc:__cplusplus)
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
  else()
    target_compile_options(${target} PRIVATE -fwrapv -fno-strict-aliasing -fsigned-char)
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
