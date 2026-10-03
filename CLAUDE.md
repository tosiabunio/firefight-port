# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

This repo is the porting basis for **Fire Fight** (Chaos Works, 1996), a Win95/DirectX top-down shooter, aimed at modern tools and other operating systems. It holds:
- the original retail game and engine source,
- the game data, with hires art only,
- the CD soundtrack as FLAC.

Everything was selected by rule from the original archive. `README.md` documents the provenance, the selection rules, what was left out, and the full data, sprite and music details. Comments and many identifiers are in Polish.

- **Version 1.1 only.** This source is v1.1 (Aug 1996). The retail CD is 1.2 and has a newer executable and `PARAMS.VOL`, but no 1.2 source exists. Don't try to reproduce 1.2 behaviour.
- **No build system yet.** The code is still the original MSVC 4 / Win32 / DirectX code. It is pre-standard C++ (`<iostream.h>`, implicit-`int` constants such as `const MAX_X=24;`), and the original makefiles aren't in the repo.
- **Archive-only material:** the launcher, the LED level editor, makefiles, shareware data, lores art and the design docs exist only in the original archive, outside this repo.

## Layout

| Path | Contents |
|---|---|
| `source/game/` | Game code. Every `.cpp` includes only `headers.h`, which pulls in the engine and all game headers in dependency order. Add new headers there |
| `source/engine/<module>/` | CW engine libraries; `1sp/asm/` holds the TASM blitters |
| `source/engine/common/` | `first.h` and `1cw_strg.h` |
| `source/regdata/` | `RegData`, the settings model shared with the (absent) launcher. Its `headers.h` has MFC parts behind `#ifdef _MFC_VER` |
| `data/` | Game data in the engine's loose-file layout (`*.dir` manifests at the root) |
| `music/` | `track02.flac` … `track09.flac` |

Include paths for a build: `source/game`, `source/regdata`, `source/engine/common` and every `source/engine/<module>`.

## Rules for this repo

- **`data/` is byte-exact original data:** CP852 text with CRLF line endings, plus binaries. `.gitattributes` keeps Git from converting it.
  - Never re-encode or reformat data files. The in-game fonts index glyphs by byte value.
  - So far the only edit has been removing `lores`/`lsource` lines from the manifests.
- **Lowercase names:** keep new file names lowercase.
- **Path resolution:** manifest paths are mixed-case DOS paths (the original engine `strupr()`s them). Resolve them as lowercase with `\` changed to `/`, relative to `data/`.
- **Source files** are UTF-8. Git normalises their line endings.
- **Build caches:** `*.sph`, `*.spc`, `*.spp` and `*.spl` are regenerated caches and are git-ignored. Don't commit them.

## Engine (`source/engine`, umbrella header `1cw.h`)

Modules are mostly static-class singletons with `init`/`quit`. Each header auto-links its lib with `#pragma comment(lib, ...)` unless `EXCLUDE_LIBS` is defined. The old module names in brackets are still used as the section names in `data/cwe.ini`.
- **1lg** [log]: `Log`, `Comm` (Win32 window proc, failure/assert boxes), the `FAILURE`/`CHECK`/`MESSAGE`/`DBG_*` macros, and the `Failure` exception.
- **1mm** [mmu]: guarded `Heap` with per-block owner names, `Heap_object`, `Fast_heap`.
- **1ba** [bas]: basic containers (`Bitflag`, `Bit`, `Pointer`, hash arrays).
- **1rg** [reg]: `Registry` (Windows registry) and `Cmd_line`.
- **1io** [xio/txt]:
  - `File` virtual filesystem. `File::access_on("update,global,params", ...)` mounts the listed manifests (`<name>.dir`) in loose-file mode, or the `.vol` volumes when there are no `.dir` files. The port data has no volumes.
  - `Xio` compression.
  - `Text`, the parser for every `area … endarea` / `key = value` data file.
- **1ee** [eem]: input (keyboard, mouse, joystick, virtual keys), timer, DirectPlay networking with sync checks, and demo record/playback.
- **1sp** [spr]:
  - DirectDraw 8-bit palettized video and sprites (hires plus collision).
  - The FLIC reader and `.def` level loading.
  - Sprite rebuild from FLC masters (`1sp_lmai.cpp`, `1sp_lreb.cpp`, `1sp_ldsa.cpp`). **Palette index 255 is the transparent key colour.**
- **1ss** [sos]: DirectSound samples, mixer, songs, and MCI CD audio.
- **1cw** [cwe]: `Cwe::init` reads `cwe.ini` and starts every other module.

## Game (`source/game`)

- **Globals:** `world` (World: level, views, builders) and `gamemanager` (GameManager: mission scripting).
- **Lifecycle:** `WinMain → Game::init_all → Game::go`, then for each mission `Game::loop_init → main_loop → loop_quit`. Control flow relies on exceptions: `TerminateMission`, `TerminateGame`, `Closed`, `Failure`, `Eem_net_sync_failure`, `Eem_demo_sync_failure`.
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
  - Palette `target` files are built from the palette FLC.
- **Music:** song number N in `data/!global/missions.tdf` (`headersong`, `footersong`, `songs`, `netsongs`) means `music/track{N+1:02}.flac`. The original engine skipped the CD's data track.
- **Platform-bound code to replace:**
  - `1sp`: DirectDraw and the asm blitters.
  - `1ss`: DirectSound and MCI CD audio.
  - `1ee`: Win32 input and timer, and DirectPlay.
  - `1rg` and `regdata`: the registry.
  - `1lg`: the Win32 window and message boxes.
  - `1io`: `io.h`.
