# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

This repo is the porting basis for **Fire Fight** (Chaos Works, 1996), a Win95/DirectX top-down shooter, aimed at modern tools and other operating systems. It holds:
- the original retail game and engine source,
- the game data, with hires art only,
- the CD soundtrack as FLAC.

Everything was selected by rule from the original archive. `README.md` is about the game, for the public repository: its history, the rights situation, screenshots, building, playing, credits and the license. `docs/porting.md` documents the port: its status, the repository contents, the provenance, the selection rules, what was left out, and the full data, sprite and music details. Comments and many identifiers are in Polish.

- **Version 1.1 only.** This source is v1.1 (Aug 1996). The retail CD is 1.2, but it differs only in the executables (its `PARAMS.VOL` repacks the same files), and no 1.2 source exists. Don't try to reproduce 1.2 behaviour.
- **Original archive:** `docs/original-archive.md` covers what the original archive (outside this repo) offers the port.
  - It has the 1.1 executables built from this source, which confirm the MSVC `rand`/`qsort` and the x87 precision.
  - The 1.1 Release exe runs on Windows 11 from a copy patched with `tools/archive/patch_hooks.py` ("Running 1.1" in that doc). Its title, mission screen, mission 1 and an attract demo match the port's frames pixel for pixel.
  - It has the shipped sprite caches. The port's sprite build matches them for every sprite and palette loaded: bounds, pixel data and tables.
  - The archive is never committed and exists only on the Windows PC (`D:\_Projects\FireFight`). What it proves is committed as golden files and test vectors, so other machines work from the repo alone.
  - Without the archive, don't guess its contents. A task that needs it (a new comparison, disassembly, running the original game) belongs on the Windows PC. See "Working without the archive" in `docs/original-archive.md`.
  - `tools/archive/ffarchive.py` and `crt_vectors.py` produce and check those files.
- **On the Mac:** `docs/macos.md` covers checking a branch, what a failing test means there, and what must wait for the Windows PC.
- **Port plan:** SDL2 + CMake, in phases with exit criteria, in `docs/porting-plan.md`. Follow its phase order and its determinism rules.
- **Status: phases 1–6 done; the game is playable on keyboard, mouse and game controller, with sound and the CD soundtrack.** The original sources build on every platform. Runtime (phase 2), video (phase 3), input (phase 4) and sound (phase 6, SDL2_mixer) are on SDL2. A full mission played by hand on each device is still to be confirmed; the sound was checked by ear on Windows. **All 8 original demos replay in sync to the end on Windows, Linux and macOS**, and so do four golden demos recorded with the port (phase 5). Network play (phase 7) runs over ENet on a LAN, by address or LAN discovery (`--host`, `--join <address>|lan`); internet play, the relay server and the input delay are still to do. **Now: the browser build (phase 9, WebAssembly), before the rest of phases 7 and 8.** The Node build (`node-emscripten`) replays every original and golden demo in sync and matches the title frames and the sprite build, so the simulation is bit-exact in WebAssembly; the browser build is next.

## Build

- **Build system:** CMake (≥ 3.25) with presets, Ninja Multi-Config, and vcpkg manifest mode (`vcpkg.json`, pinned baseline; `VCPKG_ROOT` must be set).
- **Presets:** `windows-msvc`, `linux-gcc`, `linux-clang`, `macos-clang` (arm64 only, macOS 11+), and `node-emscripten`: the game as WebAssembly, headless under Node.js, for the tests (phase 9). It needs no vcpkg, only the Emscripten SDK (`EMSDK` set by its `emsdk_env`; CI pins 6.0.11) and CMake and Ninja on `PATH`. `cmake/FFEmscripten.cmake` has its options.
  - Build presets are `<preset>-debug` and `<preset>-release`; test presets use the same names.
  - All-in-one: `cmake --workflow --preset <preset>` (configure, build Debug and Release, run all tests).
  - Single test: `ctest --preset <preset>-debug -R smoke`. Tests:
    - `smoke`: the dependencies.
    - `headless_run`: the game runs headless for 30 s against `data/` (title loop, then an attract demo).
    - `golden_title`: in `--fast` mode, every 20th of the first 200 title frames must match `tests/golden/title_frames.sha256`. The frames are bit-identical on all platforms. If rendering changes on purpose, regenerate the hashes from `--headless --fast --shot-every 20 --quit-frames 200` (`tests/check_frames.cmake`).
    - `sprite_build`: `check` mode loads every mission's level with `sprite_dump=1`. The phase bounds, pixel data CRCs and palette table CRCs must match `tests/golden/sprite_bounds.txt` and `sprite_data.txt`, which match the original's prebuilt caches. A changed dump must pass `tools/archive/ffarchive.py sprites` on the Windows PC before it replaces the golden files (`tests/check_sprites.cmake`).
    - `data_files`: every file in `data/` and `music/` must have its SHA-256 from `tests/golden/data_files.sha256`, with nothing missing or extra. This catches line-ending or encoding conversion (`tests/check_data.cmake`).
    - `crt_vectors`: the MSVC 4 `rand` and `qsort` clones (`source/compat/msvc4.h`) must reproduce `tests/golden/crt_rand.txt` and `crt_qsort.txt`, taken from the 1.1 exe.
    - `demo_<name>`: each of the 8 original attract demos (`level1` … `level4C`) must replay in sync to the end of its recording (`--headless --fast --demo <name>`).
    - `golden_<name>`: the same for the golden demos recorded with the port (`tests/demos/<name>.rec`, played with `--demo !<path>`). They cover other worlds and both skills. To record one, write a script with `tests/demos/flight.py` and run `tests/record_demo.cmake`; a deliberate simulation change means re-recording them.
    - `sound_play`: demo `level1` replays with the sound on through SDL's `disk` audio driver (no device needed). The sound system must start, the mission's track play, the output not be silent, and the replay stay in sync (`tests/check_sound.cmake`).
    - `net_play`, `net_lan`: a host and a client on one machine play a two-player game over ENet, headless and `--fast`, each with its input script (`tests/input/net_host.txt`, `net_client.txt`). The client joins by address, or by LAN discovery (`--join lan`). Both must reach the mission, pass the per-frame sync check and end normally (`tests/check_net.cmake`).
    - `input_play`: `tests/input/mission1.txt` starts the first mission from the title and flies it with the keyboard, the mouse and a virtual game controller, switching with F11. The input states it produces (`input_dump=1`) must match `tests/golden/input_states.txt`. They don't depend on the simulation, so they are the same on every platform. After a deliberate change, check the new dump by hand before it replaces the golden file (`tests/check_input.cmake`).
    - On `node-emscripten`, `net_*`, `sound_play` and `input_play` are disabled: no ENet, no FLAC in the SDK's SDL2_mixer, no virtual joystick in its SDL2. Scripted tests start the game through `EMULATOR` (Node). Emscripten's file system is POSIX, so on a Windows host these tests give the game relative paths.
- **Windows on this machine:** MSVC is not on `PATH`. Run from an x64 Developer PowerShell, or call `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat` first. `VCPKG_ROOT` can be the bundled `…\BuildTools\VC\vcpkg`. For `node-emscripten`, run the SDK's `emsdk_env.ps1` and put the Build Tools' CMake and Ninja (`…\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin` and `…\Ninja`) on `PATH`.
- **Compile options:** `cmake/FFCompileOptions.cmake` sets them. `ff_common_options` (every target) adds `-fwrapv -fno-strict-aliasing -fsigned-char`; `ff_modern_options` (new code) adds strict warnings.
- **Dependency targets:** `cmake/FFDependencies.cmake` normalises them to `ff::sdl2`, `ff::mixer` and `ff::enet`.
- **Targets:** `tools/smoke/ff_smoke` is the dependency smoke test. `source/CMakeLists.txt` defines `compat`, `cwengine` (all engine modules), `regdata` and `firefight`. The original sources use `ff_legacy_options` (tolerates `char*` string literals and MSVC pragmas); new code uses `ff_modern_options`.
- **Running:** `firefight [--data <dir>] [--pref <dir>] [--music <dir>] [options] [switch[=value] ...]`. `source/game/main.cpp` lists the options: `--headless` (also turns sound off, unless `--sound`), `--fullscreen`, `--stretch` (4:3), `--fast` (no clock: one simulation step per frame), `--demo <name>` (or `--demo !<path>` for a demo file), `--quit-after <s>`, `--quit-frames <n>`, `--shots <s>`, `--shot-every <n>` (frame dumps), `--input <file>` (an input script: keys, mouse and a virtual game controller at given frames; format in `source/game/input_script.cpp`), `--host <players>` / `--join <address>` (or `--join lan`: the first open game found on the LAN) / `--port <port>` (a network game, default port 19960). With `--demo` the exit code is 3 when the replay went out of sync and 4 when it stopped before the end of the recording. Relative paths in options mean the current directory (the game then changes to the data directory).
  - Data: `--data`, else `data/` next to the executable, else the repository's `data/` (development builds).
  - Music: `--music`, else `music/` next to the data directory.
  - Preferences: `--pref`, else `SDL_GetPrefPath("Chaos Works", "Fire Fight")`. It holds the log (`Firefght.log`, also echoed to stderr), `settings.ini` (the former registry), the pilot files, recorded demos and screenshots.
  - Other arguments are the original engine switches (`debug=1`, `check`, ...) and the port's test switches (`sprite_dump=1`, `input_dump=1`).
  - Sprites and palette tables are built from the FLC masters in memory on every start (about 1 s); nothing is written to `data/`.
- **Packages and releases:** `cmake/FFPackaging.cmake` (CPack): a Windows zip, a Linux tar.gz and a macOS disk image with `Fire Fight.app`, each with the game, `data/` and `music/`. A version tag `v<VERSION>` (the `VERSION` in `CMakeLists.txt`, now 0.7.0) runs `.github/workflows/release.yml`, which builds and tests the packages and publishes them as a GitHub release. On Windows the game is a GUI program (`WIN32_EXECUTABLE`) that attaches to the console of a terminal it was started from.
- **CI:** `.github/workflows/ci.yml` runs the workflow presets; it shares its setup steps (build tools, MSVC environment, vcpkg and its cache) with the release workflow in `.github/actions/setup`. Which platforms run:
  - branch pushes: Windows and Linux, including `node-emscripten` on Linux;
  - pull requests: macOS only;
  - `main`, nightly and manual runs: everything.

  Workflow, so `main` has been built on all three OSes:
  1. Work on a branch and push it; CI builds Windows and Linux.
  2. Run `cmake --workflow --preset macos-clang` on the local Mac.
  3. When both are green, fast-forward `main` to the branch and push (`git merge --ff-only`). That push runs the full matrix again.

  No pull request is needed; open one only when the user asks for a review. Documentation-only changes may go straight to `main`.
- **Archive-only material:** the launcher, the LED level editor, makefiles, shareware data, lores sprites (`data/` has only the 22 lores masters, for their bounds) and the design docs exist only in the original archive, outside this repo.

## Layout

| Path | Contents |
|---|---|
| `source/compat/` | Port layer: `crt.h` (MSVC CRT extensions), `msvc4.h` (MSVC 4 `rand`/`qsort`) and `win32.h` (Win32 stand-ins, used on every platform, Windows included). Original sources include these instead of `<windows.h>`, `<io.h>` and friends. Drivers stop using `win32.h` as they move to SDL |
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
  - No data file has been edited. The manifests keep their `lores`/`lsource` lines: lores bounds count towards the phase bounds.
- **License:** code written for the port is 0BSD (`LICENSE`). The original 1996 code, data and music have no license; they are kept for preservation (README, "About this repository").
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
- **1ee** [eem]: input (keyboard, mouse, joystick, virtual keys), timer (main thread, serviced by the message pump), network lockstep with sync checks, and demo record/playback.
  - `Net` (`1ee_netw.cpp`) runs on the ENet transport in `1ee_enet.cpp`, which must not include the engine headers (`<enet/enet.h>` brings in the real Windows headers). The host relays for the clients; player ids are 1 for the host and 2–4 for the clients. The Emscripten build links `1ee_enet_none.cpp` instead: same interface, no network play.
  - Messages from a later `Eem::start` series wait in a per-player queue until this peer starts it (`Eem::apply_message`); `User_block` carries its series.
  - Input arrives as SDL events through `Comm::input_proc` (`Eem::sdl_event`). Keys keep the PC set-1 scan codes (E0 keys +128) that the bindings and demos use. Mouse coordinates are window coordinates, clipped to the picture's area (reported by `VD_sdl::present`); mouse steering turns on SDL relative mode. Joysticks are SDL game controllers (else plain joysticks), read as the original's 32 buttons and 6 digital axes.
  - Controls are virtual keys (`Vkey`, built by `RegData` from the bindings). A virtual key matches its key with or without the E0 prefix (Right Ctrl fires like Left Ctrl). Port addition: W, S, A and D are a second key for whatever control a cursor key is bound to (`RegData::ParallelKey`); menus take only the cursor keys.
- **1sp** [spr]:
  - 8-bit palettized video on SDL (`VD_sdl` in `1sp_vdrv.cpp`: 640×400 framebuffer, palette → ARGB texture, letterboxed) and sprites (hires plus collision). Hires only, never upside down.
  - The blitter `_uniput` (`1sp_asm.cpp`, ported from `asm/1sp_aput.asm`; its collision scan order feeds the simulation) and the fonts (`1sp_font.cpp`, generated by `tools/fonts/convert_fonts.py`).
  - The FLIC reader and `.def` level loading.
  - Sprite build from FLC masters into memory (`1sp_lmai.cpp`, `1sp_lreb.cpp`, `1sp_ldsa.cpp`), including the lores bounds (`Lsprite::measure`: lores is measured, never built). Phase bounds are simulation state: level-object culling and builders, and on-screen tests. **Palette index 255 is the transparent key colour.**
- **1ss** [sos]: samples and the CD soundtrack on SDL2_mixer.
  - Samples keep the original voice model: 8 channels (up to 16), one voice per sample, stealing by the manifest priority. DirectSound's volumes and pans (hundredths of a dB) become linear gains. `sos_safe=1` is the old single-voice WaveOut mode; `sos_none=1` is silent (headless runs).
  - `CD` streams `music/track{N+1:02}.flac` for CD track N. Losing focus pauses samples and music.
  - `Mixer` holds the game's sound and music volumes (it used to change the system-wide mixer). They are saved in `settings.ini` (`sos/sound volume`, `sos/music volume`) and default to half: music and samples share one digital mix, which clips at full volume.
- **1cw** [cwe]: `Cwe::init` reads `cwe.ini` and starts every other module.

## Game (`source/game`)

- **Globals:** `world` (World: level, views, builders) and `gamemanager` (GameManager: mission scripting).
- **Lifecycle:** `main` (SDL_main, `main.cpp`) → `game_main` (formerly `WinMain`) → `Game::init_all → Game::go`, then for each mission `Game::loop_init → main_loop → loop_quit`. Control flow relies on exceptions: `TerminateMission`, `TerminateGame`, `Closed`, `Failure`, `Eem_net_sync_failure`, `Eem_demo_sync_failure`.
- **Deterministic lockstep:** `KbdStat::run()` hands out fixed input frames, and `run_service()` advances the simulation for each one. Rendering (`display_service`, then `Display::copy2vga`) is decoupled from it.
  - Network play and demos replay inputs and must stay deterministic. In simulation code use `RAND` (`Rand::rand(FILE_LINE)`), never `rand()` or wall-clock time. `Rand` is unlocked only around the simulation step.
  - Old-toolchain behaviour the simulation depends on is cloned, not left to the platform: `compat/msvc4.h` has MSVC 4's `rand` (only `Rand::init` uses it) and `qsort` (every sort). Use `msvc4_qsort` for any new sort whose keys can tie.
  - **Stale memory:** the original read `ACannon::global_time` uninitialised, so it saw what the FastAlloc slot's earlier occupants left at offset 232 of their 32-bit MSVC layout. `Stale_memory` (`stale.h`) reproduces that, and the original demos depend on it. Every `delete` of a game object must call `Stale_memory::object_deleted` first. If a game object class changes, regenerate `stale_gen.cpp` with `tools/layout/stale_memory.py`.
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

## Data and music (details in `docs/porting.md`)

- **Manifests:** each `data/<volume>.dir` maps logical names to files. Code opens data only by logical name. The exceptions are `cwe.ini` and the manifests themselves.
- **Mount order:** at startup the game mounts `update,global,params`. Each mission then mounts `update,<world>,<world><n>`, plus `<world><n>s` (speech) when dialogs are on.
- **Worlds:** `brown`, `gray`, `green`, `white` and `net`.
- **Sprites:**
  - Masters are 8-bit FLC files in `data/flics/`.
  - `hires` is built from `hsource` or else `source`, at 1×1.
  - `collis` is built from `csource` or else `source`, at 8×1.
  - `lores` is only measured, from `lsource` or else `source`, at 2×1 unless the target line says `1x1`.
  - Palette tables are built from the palette FLC.
  - The target names (`hires = …sph`) are still required in the manifests, but the files are never read or written.
- **Music:** song number N in `data/!global/missions.tdf` (`headersong`, `footersong`, `songs`, `netsongs`) means `music/track{N+1:02}.flac`. The original engine skipped the CD's data track. Mission songs loop; the mission screen's `headersong` plays once.
- **Platform-bound code to replace:** none; every driver is on SDL2 or ENet.
