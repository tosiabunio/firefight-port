# Fire Fight: porting basis

This is the subset of the original Fire Fight (Chaos Works, 1996) material needed to port the game to modern tools and other operating systems. It covers the **retail** edition and **hires art only**. Everything here was copied from the original tree one level up (`..`) by rule. Nothing was hand-picked.

| Path | Contents |
|---|---|
| `source/game/` | All 26 translation units linked into `FIREFGHT.EXE`, plus `resource.rc` (progress dialog, version info) and `icon1.ico` |
| `source/engine/<module>/` | Library sources of the CW engine modules `1ba 1cw 1ee 1io 1lg 1mm 1rg 1sp 1ss`, without their test harnesses. `1sp/asm/` holds the TASM sprite/font blitters |
| `source/engine/common/` | `first.h` and `1cw_strg.h`. These are project headers that only existed in the original `z:\lib` |
| `source/regdata/` | The settings model (`RegData`) shared by the game and the launcher. `headers.h` is the launcher's header, and its MFC parts sit behind `#ifdef _MFC_VER` |
| `data/` | Retail game data, laid out exactly as the `*.dir` manifests expect (see below) |
| `music/` | CD-audio soundtrack, `track02.flac` … `track09.flac` (lossless rips of the CD tracks) |
| `docs/porting-plan.md` | The port plan: SDL2 + CMake, phases, decisions |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `cmake/` | Build system |
| `tools/smoke/` | Dependency smoke test (SDL2 window and paletted present, SDL2_mixer FLAC/WAV, ENet loopback) |

## Building

The port is in progress (see `docs/porting-plan.md`). The game targets are switched off (`FF_BUILD_GAME=OFF`) until phase 1, so the build currently produces only the smoke test.

**Requirements:** CMake ≥ 3.25, Ninja, a C++17 compiler, and [vcpkg](https://github.com/microsoft/vcpkg) with the `VCPKG_ROOT` environment variable pointing at it. vcpkg builds SDL2, SDL2_mixer (with FLAC) and ENet from `vcpkg.json` on first configure.

- **Windows:**
  - Visual Studio 2022 or its Build Tools, with the *Desktop development with C++* workload. It bundles CMake, Ninja and vcpkg.
  - Build from an x64 Developer PowerShell. Set `VCPKG_ROOT` to your own vcpkg clone, or to the bundled one at `…\Microsoft Visual Studio\2022\<edition>\VC\vcpkg`.
- **Linux:** GCC or Clang, `ninja-build`, and the X11/Wayland/audio development packages that SDL needs. The CI workflow (`.github/workflows/ci.yml`) has the exact `apt` list for Ubuntu.
- **macOS (Apple Silicon only):** Xcode Command Line Tools and `brew install cmake ninja pkg-config autoconf automake libtool`.

Presets: `windows-msvc`, `linux-gcc`, `linux-clang`, `macos-clang`.

```sh
cmake --workflow --preset linux-gcc            # configure, build Debug + Release, run tests
# or step by step:
cmake --preset linux-gcc
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

To see the smoke test's window and hear the audio, run it without `--headless`:

```sh
build/linux-gcc/tools/smoke/Debug/ff_smoke --music music/track02.flac --wav 'data/!global/sounds/beephi.wav'
```

## Provenance and version

- **Source:** the source and the data in `data/` form a consistent pair, both version 1.1.
  - The source snapshot dates from 31 Aug 1996, which is version 1.1. The original build scripts produced the `ff11up` patch from it.
  - `data/` comes from `FF/WORK.RTL`. Its content matches the data volumes shipped on the retail CD (dated May 1996).
- **The 1.2 CD differs in two files:** a newer `FIREFGHT.EXE`/`LOADER.EXE` (Jul 1997) and a newer `PARAMS.VOL`. `PARAMS.VOL` holds the `.tdf` parameter files. No source exists for 1.2, so this tree reproduces 1.1.

## What was deliberately left out

| Left out | Why | Where it is in the original tree |
|---|---|---|
| Lores art: `.spl` sprites and `lsource` FLCs | The port is hires-only. 525 `lores =`/`lsource =` lines were removed from the manifests | `FF/WORK.RTL` |
| Built sprite caches: `.sph` (hires), `.spc` (collision), `.spp` (palette tables) | Regenerated from the FLC masters (see "Sprites"). They are compiled, engine-specific RLE formats | `FF/WORK.RTL` |
| Launcher `LOADER.EXE` (MFC options/network wizard), `FFSTART`, `INFO`, the LED level editor | Platform-specific front-ends and tools, not the game. `RegData` (kept) is the settings model the launcher edited | `FF/C/LOADER`, `FF/C/FFSTART`, `FF/C/INFO`, `FF/C/LED` |
| Makefiles (`.mak`/`.mdp`), engine test harnesses, DirectX SDK headers | MSVC 4 / Win95 only | `FF/C`, `LIB` |
| Shareware edition data, MIDI music, installer | Retail is a superset of the shareware game | `FF/WORK.SHW`, `FF/INSTALL` |
| Design documents (story, mission design, 1995–96 technical docs, mostly Polish) | Not needed to run the game. The code is the authoritative spec | `FF/WORK.RTL/!MISC` |

## Normalisation applied

- **Names:** all file and directory names are lowercase, and every `#include` in the kept sources already uses lowercase names. Paths inside manifests are mixed-case DOS paths, because the original engine `strupr()`s everything. Resolve them as `path.lower().replace('\\', '/')` relative to `data/`, which works on case-sensitive file systems.
- **Source encoding:** source files are UTF-8. The only non-ASCII bytes were in comments (Polish, CP852). `resource.rc` was Windows-1252.
- **Data bytes:** data files are **byte-for-byte originals**. Text data is CP852 with CRLF line endings. The in-game fonts index glyphs by byte value, so don't re-encode them. The only edit to any data file is the removal of the lores lines from the manifests.

## Data layout

- **Manifests:** each `data/<volume>.dir` describes one original volume and maps *logical names* to files. Code never opens data by path, except `cwe.ini` and the `*.dir` manifests. It asks for logical names, e.g. `mainparams`, `back`, `brown_1_1_1`.
  - Syntax: `area <name> … endarea` blocks and `key = value [flags]` entries, with `;` starting a comment. Parsed by `Text` in `source/engine/1io/1io_txt.cpp`.
  - A `-` before a path means "not packed into the shipped volume", i.e. it marks a source file.
  - Load order: `update,global,params` is mounted at startup. Each mission then mounts `update,<world>,<world><n>`, plus `<world><n>s` (mission speech) when spoken dialogs are on. `header` and `demo` are mounted for the title/statistics screens and demo playback.
- **Worlds:** `brown`, `gray`, `green`, `white` (single-player) and `net` (multiplayer). Each world directory contains:
  - `<world>.def` — level file from the LED editor: type and sprite tables, plus per-level plane maps.
  - `<world><n>m.tdf` — mission script: weather, checkpoints, bonus objectives, enable/disable actions.
  - `<world><n>t.tdf` — object type parameters.
- **Global data:** `data/!global/` holds the global tunables (`main.tdf` → class `Mp`, `object.tdf` → class `Op`), menus, print layouts, `text.txt` (all UI strings, via `GAMETXT("LABEL")`) and `missions.tdf` (mission order and music).
- **Other folders:** `data/sounds/` and `data/!global/sounds/` hold WAV samples. `data/demo/` holds the attract-mode demo recordings, which are input recordings.
- **`cwe.ini`:** engine configuration, including timer frequency, network settings and sprite scales.

## Sprites

The art masters are Autodesk Animator FLIC files (`data/flics/**.flc`): 8-bit, with an embedded palette. Each sprite `area` in a manifest names its masters and its (now absent) build targets:

| Target key (file not shipped) | Built from | Scale (`cwe.ini [spr]`) |
|---|---|---|
| `hires` → `.sph` | `hsource` if present, else `source` | 1×1, so the FLC frames are the hires art as-is |
| `collis` → `.spc` | `csource` if present, else `source` | 8×1 (collision masks) |
| `target` → `.spp` | the palette FLC `source` | palette plus derived lookup tables |

Per-entry flags: `o+` = one-colour (the weather overlays: fog, cloud, night), `m+` = generate mirrored copies, `NxM` = scale override.

**Palette index 255 is the transparent key colour** (`key_color` in `1sp_lreb.cpp`). Keep it as alpha when converting frames to a modern format.

The rebuild path that turns FLC frames into engine sprites is the reference for anything else a converter must preserve. It lives in `source/engine/1sp/1sp_lmai.cpp`, `1sp_lreb.cpp`, `1sp_ldsa.cpp` and `1sp_flic.cpp`. With the original engine in loose-file mode, missing targets are simply rebuilt on first load.

## Music

Track selection is data-driven. `data/!global/missions.tdf` lists `headersong`, `footersong`, `songs` (one per entry in `missions_order`) and `netsongs`. The engine skips the CD's data track (see `source/engine/1ss/1ss_song.cpp`), so **song number N is `music/track{N+1:02}.flac`**. For example, `headersong = 1` (the title screen) plays `track02.flac`.

## Porting notes

- **Include paths:** `source/game`, `source/regdata`, `source/engine/common` and each `source/engine/<module>`. Every game `.cpp` includes only `headers.h`.
- **Original defines:** retail = no `SHAREWARE`. `UNPROTECT` disables the CD-ROM check. `_DEBUG` turns on `HI_DEBUG`, which in turn forbids global `operator new` (allocations go through the engine heap via `NEW(...)`).
- **Platform-bound code to replace, by module:**
  - `1sp`: DirectDraw video, 8-bit palette, and the asm blitters.
  - `1ss`: DirectSound and MCI CD audio. Replace CD audio with `music/`.
  - `1ee`: Win32 keyboard, mouse, joystick and timer, DirectPlay networking, and demo record/playback.
  - `1rg`: Windows registry and command line.
  - `1lg`: Win32 window procedure and message boxes.
  - `1io`: `io.h` file API and temp files.
  - `regdata`: registry-backed settings.
  - Throughout: pre-standard C++ such as `<iostream.h>` and implicit-`int` constants.
- **Determinism:** the simulation is deterministic lockstep, driven by input frames, with randomness only through `RAND`. Network play and demos depend on that. Demo playback throws `Eem_demo_sync_failure` when the replay diverges, so the recordings in `data/demo/` act as built-in regression tests for an accurate port. Note that `Game::DEMOVERSION` must match.
