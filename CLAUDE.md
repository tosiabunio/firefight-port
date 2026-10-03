# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

This repo is the porting basis for **Fire Fight** (Chaos Works, 1996), a Win95/DirectX top-down shooter, aimed at modern tools and other operating systems. It holds:
- the original retail game and engine source,
- the game data, with hires art only,
- the CD soundtrack as FLAC.

Everything was selected by rule from the original archive. `README.md` documents the provenance, the selection rules, what was left out, and the full data, sprite and music details. Comments and many identifiers are in Polish.

- **Version 1.1 only.** This source is v1.1 (Aug 1996). The retail CD is 1.2, but it differs only in the executables (its `PARAMS.VOL` repacks the same files), and no 1.2 source exists. Don't try to reproduce 1.2 behaviour.
- **Original archive:** `docs/original-archive.md` covers what the original archive (outside this repo) offers the port.
  - It has the 1.1 executables built from this source, which confirm the MSVC `rand`/`qsort` and the x87 precision.
  - It has the shipped sprite caches, an exact oracle for the in-memory sprite build. They show 15 sprites whose phase bounds the port gets wrong.
  - `tools/archive/ffarchive.py` runs the checks.
- **Port plan:** SDL2 + CMake, in phases with exit criteria, in `docs/porting-plan.md`. Follow its phase order and its determinism rules.
- **Status: phase 3 done; the game shows in a window but has no input or sound yet.** The original sources build on every platform. Runtime (phase 2) and video (phase 3) are on SDL2. Sound, input and network still go through `source/compat/`, whose inert Win32/DirectX stand-ins phases 4–7 replace. Without input, the game runs its title loop and attract demos; sound is inactive until phase 6.

## Build

- **Build system:** CMake (≥ 3.25) with presets, Ninja Multi-Config, and vcpkg manifest mode (`vcpkg.json`, pinned baseline; `VCPKG_ROOT` must be set).
- **Presets:** `windows-msvc`, `linux-gcc`, `linux-clang`, `macos-clang` (arm64 only, macOS 11+).
  - Build presets are `<preset>-debug` and `<preset>-release`; test presets use the same names.
  - All-in-one: `cmake --workflow --preset <preset>` (configure, build Debug and Release, run all tests).
  - Single test: `ctest --preset <preset>-debug -R smoke`. Tests:
    - `smoke`: the dependencies.
    - `headless_run`: the game runs headless for 30 s against `data/` (title loop, then an attract demo).
    - `golden_title`: in `--fast` mode, every 20th of the first 200 title frames must match `tests/golden/title_frames.sha256`. The frames are bit-identical on all platforms. If rendering changes on purpose, regenerate the hashes from `--headless --fast --shot-every 20 --quit-frames 200` (`tests/check_frames.cmake`).
- **Windows on this machine:** MSVC is not on `PATH`. Run from an x64 Developer PowerShell, or call `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat` first. `VCPKG_ROOT` can be the bundled `…\BuildTools\VC\vcpkg`.
- **Compile options:** `cmake/FFCompileOptions.cmake` sets them. `ff_common_options` (every target) adds `-fwrapv -fno-strict-aliasing -fsigned-char`; `ff_modern_options` (new code) adds strict warnings.
- **Dependency targets:** `cmake/FFDependencies.cmake` normalises them to `ff::sdl2`, `ff::mixer` and `ff::enet`.
- **Targets:** `tools/smoke/ff_smoke` is the dependency smoke test. `source/CMakeLists.txt` defines `compat`, `cwengine` (all engine modules), `regdata` and `firefight`. The original sources use `ff_legacy_options` (tolerates `char*` string literals and MSVC pragmas); new code uses `ff_modern_options`.
- **Running:** `firefight [--data <dir>] [--pref <dir>] [options] [switch[=value] ...]`. `source/game/main.cpp` lists the options: `--headless`, `--fullscreen`, `--stretch` (4:3), `--fast` (no clock: one simulation step per frame), `--demo <name>`, `--quit-after <s>`, `--quit-frames <n>`, `--shots <s>`, `--shot-every <n>` (frame dumps).
  - Data: `--data`, else `data/` next to the executable, else the repository's `data/` (development builds).
  - Preferences: `--pref`, else `SDL_GetPrefPath("Chaos Works", "Fire Fight")`. It holds the log (`Firefght.log`, also echoed to stderr), `settings.ini` (the former registry), the pilot files, recorded demos and screenshots.
  - Other arguments are the original engine switches (`debug=1`, `check`, ...).
  - Sprites and palette tables are built from the FLC masters in memory on every start (about 1 s); nothing is written to `data/`.
- **CI:** `.github/workflows/ci.yml` runs the workflow presets. Which platforms run:
  - branch pushes: Windows and Linux;
  - pull requests: macOS only;
  - `main`, nightly and manual runs: everything.

  Workflow, so `main` has been built on all three OSes:
  1. Work on a branch and push it; CI builds Windows and Linux.
  2. Run `cmake --workflow --preset macos-clang` on the local Mac.
  3. When both are green, fast-forward `main` to the branch and push (`git merge --ff-only`). That push runs the full matrix again.

  No pull request is needed; open one only when the user asks for a review. Documentation-only changes may go straight to `main`.
- **Archive-only material:** the launcher, the LED level editor, makefiles, shareware data, lores art and the design docs exist only in the original archive, outside this repo.

## Layout

| Path | Contents |
|---|---|
| `source/compat/` | Port layer: `crt.h` (MSVC CRT extensions) and `win32.h` (Win32/DirectX/MCI stand-ins, used on every platform, Windows included). Original sources include these instead of `<windows.h>`, `<io.h>` and friends. Drivers stop using `win32.h` as they move to SDL |
| `source/game/` | Game code. Every `.cpp` includes only `headers.h`, which pulls in the engine and all game headers in dependency order. Add new headers there |
| `source/engine/<module>/` | CW engine libraries; `1sp/asm/` holds the TASM blitters |
| `source/engine/common/` | `first.h` and `1cw_strg.h` |
| `source/regdata/` | `RegData`, the settings model shared with the (absent) launcher. Its `headers.h` has MFC parts behind `#ifdef _MFC_VER` |
| `data/` | Game data in the engine's loose-file layout (`*.dir` manifests at the root) |
| `music/` | `track02.flac` … `track09.flac` |

Include paths for a build: `source`, `source/game`, `source/regdata`, `source/engine/common` and every `source/engine/<module>`.

## Rules for this repo

- **`data/` is byte-exact original data:** CP852 text with CRLF line endings, plus binaries. `.gitattributes` keeps Git from converting it.
  - Never re-encode or reformat data files. The in-game fonts index glyphs by byte value.
  - So far the only edit has been removing `lores`/`lsource` lines from the manifests.
- **Lowercase names:** keep new file names lowercase.
- **Path resolution:** manifest paths are mixed-case DOS paths (the original engine `strupr()`s them). Resolve them as lowercase with `\` changed to `/`, relative to `data/`.
- **Source files** are UTF-8. Git normalises their line endings.
- **Port changes to the original code** stay minimal and say why in a short comment (`// was ...`, `// MSVC 4 ...`). The pristine original is `d73171a`.
- **Build caches:** the original's `*.sph`, `*.spc`, `*.spp` and `*.spl` caches are no longer written (phase 3 builds sprites in memory). They stay git-ignored in case old checkouts have them.

## Engine (`source/engine`, umbrella header `1cw.h`)

Modules are mostly static-class singletons with `init`/`quit`. Each header auto-links its lib with `#pragma comment(lib, ...)` unless `EXCLUDE_LIBS` is defined. The old module names in brackets are still used as the section names in `data/cwe.ini`.
- **1lg** [log]: `Log`, `Comm` (SDL message pump feeding the original window-proc chain, message boxes, quit manager), the `FAILURE`/`CHECK`/`MESSAGE`/`DBG_*` macros, and the `Failure` exception.
  - The quit manager runs the final quits after all static destructors (`Comm_init` in `1lg.h`). Anything a quit procedure touches must outlive static destruction.
- **1mm** [mmu]: guarded `Heap` (zero-filled `malloc` blocks with per-block owner names), `Heap_object`, `Fast_heap`.
- **1ba** [bas]: basic containers (`Bitflag`, `Bit`, `Pointer`, hash arrays).
- **1rg** [reg]: `Registry` (now `settings.ini` in the preferences directory) and `Cmd_line`.
- **1io** [xio/txt]:
  - `File` virtual filesystem on stdio. `File::access_on("update,global,params", ...)` merges the listed manifests (`<name>.dir`) in memory. The original `.vol` volume support is gone.
  - `Xio` compression (the demo files use it).
  - `Text`, the parser for every `area … endarea` / `key = value` data file.
- **1ee** [eem]: input (keyboard, mouse, joystick, virtual keys), timer (main thread, serviced by the message pump), DirectPlay networking with sync checks, and demo record/playback.
- **1sp** [spr]:
  - 8-bit palettized video on SDL (`VD_sdl` in `1sp_vdrv.cpp`: 640×400 framebuffer, palette → ARGB texture, letterboxed) and sprites (hires plus collision). Hires only, never upside down.
  - The blitter `_uniput` (`1sp_asm.cpp`, ported from `asm/1sp_aput.asm`; its collision scan order feeds the simulation) and the fonts (`1sp_font.cpp`, generated by `tools/fonts/convert_fonts.py`).
  - The FLIC reader and `.def` level loading.
  - Sprite build from FLC masters into memory (`1sp_lmai.cpp`, `1sp_lreb.cpp`, `1sp_ldsa.cpp`), including the lores bounds (`Lsprite::measure`). **Palette index 255 is the transparent key colour.**
- **1ss** [sos]: DirectSound samples, mixer, songs, and MCI CD audio.
- **1cw** [cwe]: `Cwe::init` reads `cwe.ini` and starts every other module.

## Game (`source/game`)

- **Globals:** `world` (World: level, views, builders) and `gamemanager` (GameManager: mission scripting).
- **Lifecycle:** `main` (SDL_main, `main.cpp`) → `game_main` (formerly `WinMain`) → `Game::init_all → Game::go`, then for each mission `Game::loop_init → main_loop → loop_quit`. Control flow relies on exceptions: `TerminateMission`, `TerminateGame`, `Closed`, `Failure`, `Eem_net_sync_failure`, `Eem_demo_sync_failure`.
- **Deterministic lockstep:** `KbdStat::run()` hands out fixed input frames, and `run_service()` advances the simulation for each one. Rendering (`display_service`, then `Display::copy2vga`) is decoupled from it.
  - Network play and demos replay inputs and must stay deterministic. In simulation code use `RAND` (`Rand::rand(FILE_LINE)`), never `rand()` or wall-clock time. `Rand` is unlocked only around the simulation step.
  - Demo playback throws `Eem_demo_sync_failure` when the replay diverges, so `data/demo/*.rec` act as regression tests for an accurate port. `Game::DEMOVERSION` must match.
- **Object model** (`gobj.h`): `Object` (a `FastAlloc` with a virtual `run()`) combined with mixins that inherit `virtual Posit`: `Turn`/`Move`, `Visible`/`Shadow`, `Colis`/`LowColis`, `Life`, `Link`, `Radar`, `Sound`, `Handle`.
  - To destroy an object, call `KILLME` (queued in `Kill`, processed at the next step). Don't `delete` it directly.
  - Concrete classes: `my.*` (player `Myship`, base, capsules, bricks), `myweap.*`, `alien.*`, `alienwea.*`, `neutral.*`.
- **Level objects:** `LevImp` (`build.*`) registers `Buildable` builders keyed by the type names in the world's `.def`. Each builder spawns game objects configured by `<world><n>t.tdf` areas (`type = obj_destroy`, `life = …`).
- **Missions:** `GameManager` (`gman.cpp`) runs `<world><n>m.tdf`: weather, checkpoints, bonus objectives, and enable/disable actions tied to events.
- **Tunables and text:** `Mp` (`data/!global/main.tdf`, logical name `mainparams`) and `Op` (`object.tdf`, `objectparams`) live in `params.*`. UI strings come from `GAMETXT("LABEL")`, which reads `!global/text.txt`.
- **Original defines:**
  - Retail builds don't define `SHAREWARE`. `UNPROTECT` disables the CD-ROM check.
  - `_DEBUG` turns on `HI_DEBUG`, which makes global `operator new` fail. Allocate with `NEW(Type(...), owner_name)` / `Heap_object`, or with `FastAlloc` for game objects.
- **Diagnostics:** setting the environment variable `cwdiags=extended` clears `Comm::production`. That enables the debug entries in `main.tdf` and these command-line switches:
  - `check` loads every level, which rebuilds all sprites.
  - Debug switches: `memorydebug`, `debugkeys`, `netdebug`, `randdebug`, `shipdebug`, `syncfail`.

## Data and music (details in `README.md`)

- **Manifests:** each `data/<volume>.dir` maps logical names to files. Code opens data only by logical name. The exceptions are `cwe.ini` and the manifests themselves.
- **Mount order:** at startup the game mounts `update,global,params`. Each mission then mounts `update,<world>,<world><n>`, plus `<world><n>s` (speech) when dialogs are on.
- **Worlds:** `brown`, `gray`, `green`, `white` and `net`.
- **Sprites:**
  - Masters are 8-bit FLC files in `data/flics/`.
  - `hires` is built from `hsource` or else `source`, at 1×1.
  - `collis` is built from `csource` or else `source`, at 8×1.
  - Palette tables are built from the palette FLC.
  - The target names (`hires = …sph`) are still required in the manifests, but the files are never read or written.
- **Music:** song number N in `data/!global/missions.tdf` (`headersong`, `footersong`, `songs`, `netsongs`) means `music/track{N+1:02}.flac`. The original engine skipped the CD's data track.
- **Platform-bound code to replace:**
  - `1ss`: DirectSound and MCI CD audio.
  - `1ee`: Win32 input and DirectPlay.
