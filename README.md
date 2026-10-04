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
| `docs/macos.md` | Working on the Mac: checking a branch before `main`, test failures, what needs the Windows PC |
| `docs/original-archive.md` | What the original archive holds beyond this subset (built 1.1 executables, shipped sprite caches, the 1.2 CD), and what was verified against it |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `cmake/` | Build system |
| `source/compat/` | Port layer: MSVC CRT extensions and stand-ins for the Win32/DirectX APIs (inert stubs until the SDL2 drivers replace them) |
| `tools/smoke/` | Dependency smoke test (SDL2 window and paletted present, SDL2_mixer FLAC/WAV, ENet loopback) |
| `tools/layout/` | `stale_memory.py`: generates `source/game/stale_gen.cpp` from clang's 32-bit MSVC record layouts (the original's object layout, which one uninitialised read depends on) |
| `tools/archive/` | Run where the original archive is: `ffarchive.py` (volumes, sprite caches, data provenance, sprite checks) and `crt_vectors.py` (MSVC `qsort`/`rand` test vectors from the original exe) |

## Building

The port is in progress (see `docs/porting-plan.md`). Phases 1–6 are in place: the original game compiles on all three platforms, its runtime runs on SDL2, it draws in an SDL window, it takes keyboard, mouse and game controller input, and it plays its sounds and the CD soundtrack (from `music/`) through SDL2_mixer. Network play is still an inert stub, so for now the game is single player. The simulation reproduces the original's: the 8 original attract demos replay in sync on every platform. F11 cycles the control sets (keyboard, mouse steering, game controller); with mouse steering the window captures the mouse while it has focus. Useful options (all in `source/game/main.cpp`):

- `--fullscreen`: desktop full screen.
- `--stretch`: 4:3 like a CRT; the default is square pixels.
- `--headless`: no window.
- `--fast`: no clock; one simulation step per frame.
- `--demo level1`: play one recorded demo.
- `--shot-every <n>` / `--shots <s>`: dump frames.
- `--input <file>`: play an input script (keys, mouse, a virtual game controller); the format is in `source/game/input_script.cpp`.
- `--music <dir>`: the soundtrack; the default is `music/` next to the data directory.

Settings, pilots, the log and frame dumps live in the SDL preferences directory (override with `--pref <dir>`). The sound and music volumes are set in the game's options and kept in `settings.ini`.

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

### Quick start on macOS (Apple Silicon)

1. Install the tools, one time only:

   ```sh
   xcode-select --install                      # Xcode Command Line Tools, if not installed yet
   brew install cmake ninja pkg-config autoconf automake libtool
   git clone https://github.com/microsoft/vcpkg ~/vcpkg
   ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
   ```

2. Make `VCPKG_ROOT` permanent:

   ```sh
   echo 'export VCPKG_ROOT="$HOME/vcpkg"' >> ~/.zshrc && source ~/.zshrc
   ```

3. Get the repo. It is private, so log in first with `gh auth login` or use your Git credentials:

   ```sh
   gh repo clone tosiabunio/firefight-port      # or: git clone https://github.com/tosiabunio/firefight-port.git
   cd firefight-port
   ```

4. Configure, build Debug + Release and run the tests:

   ```sh
   cmake --workflow --preset macos-clang
   ```

   The first run builds SDL2, SDL2_mixer, libFLAC and ENet through vcpkg, which takes a few minutes. Later runs reuse vcpkg's binary cache. The run should end with `100% tests passed` for both Debug and Release.

5. Run the smoke test with a real window and sound:

   ```sh
   build/macos-clang/tools/smoke/Debug/ff_smoke --music music/track02.flac --wav 'data/!global/sounds/beephi.wav'
   ```

   **Expected:** a 640×400 window with moving diagonal colour bands for a few seconds. After it closes, you hear a short beep, then a quarter of a second of the title music. The terminal lists each check and ends with `PASSED (0 failed checks)`. The first lines report the SDL, SDL_mixer and ENet versions and the video/audio drivers in use (`cocoa` and `coreaudio`).

## Provenance and version

- **Source:** the source and the data in `data/` form a consistent pair, both version 1.1.
  - The source snapshot dates from 31 Aug 1996, which is version 1.1. The original build scripts produced the `ff11up` patch from it.
  - `data/` comes from `FF/WORK.RTL`. Every file in the retail CD's data volumes (dated May 1996) is byte-identical to `data/` or to a sprite cache in `FF/WORK.RTL`.
- **The 1.2 CD differs only in its executables:** a newer `FIREFGHT.EXE`/`LOADER.EXE` (Jul 1997). Its `PARAMS.VOL` is dated 1997 but repacks the same `.tdf` files. No source exists for 1.2, so this tree reproduces 1.1. See `docs/original-archive.md`.

## What was deliberately left out

| Left out | Why | Where it is in the original tree |
|---|---|---|
| Lores sprites (`.spl`) | The port is hires-only and never builds lores pixels. The manifests keep their `lores`/`lsource` lines and `data/` has the 22 `lsource` masters, because lores bounds count towards every sprite's phase bounds (see "Sprites") | `FF/WORK.RTL` |
| Built sprite caches: `.sph` (hires), `.spc` (collision), `.spp` (palette tables) | Regenerated from the FLC masters (see "Sprites"). They are compiled, engine-specific RLE formats | `FF/WORK.RTL` |
| Launcher `LOADER.EXE` (MFC options/network wizard), `FFSTART`, `INFO`, the LED level editor | Platform-specific front-ends and tools, not the game. `RegData` (kept) is the settings model the launcher edited | `FF/C/LOADER`, `FF/C/FFSTART`, `FF/C/INFO`, `FF/C/LED` |
| Makefiles (`.mak`/`.mdp`), engine test harnesses, DirectX SDK headers | MSVC 4 / Win95 only | `FF/C`, `LIB` |
| Shareware edition data, MIDI music, installer | Retail is a superset of the shareware game | `FF/WORK.SHW`, `FF/INSTALL` |
| Design documents (story, mission design, 1995–96 technical docs, mostly Polish) | Not needed to run the game. The code is the authoritative spec | `FF/WORK.RTL/!MISC` |

## Normalisation applied

- **Names:** all file and directory names are lowercase, and every `#include` in the kept sources already uses lowercase names. Paths inside manifests are mixed-case DOS paths, because the original engine `strupr()`s everything. Resolve them as `path.lower().replace('\\', '/')` relative to `data/`, which works on case-sensitive file systems.
- **Source encoding:** source files are UTF-8. The only non-ASCII bytes were in comments (Polish, CP852). `resource.rc` was Windows-1252.
- **Data bytes:** data files are **byte-for-byte originals**. Text data is CP852 with CRLF line endings. The in-game fonts index glyphs by byte value, so don't re-encode them. No data file has been edited.

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
| `lores` → `.spl` | `lsource` if present, else `source` | 2×1, or `1x1` on the target line. Only measured, never built: its bounds count towards the phase bounds |
| `target` → `.spp` | the palette FLC `source` | palette plus derived lookup tables |

Per-entry flags: `o+` = one-colour (the weather overlays: fog, cloud, night), `m+` = generate mirrored copies, `NxM` = scale override.

**Palette index 255 is the transparent key colour** (`key_color` in `1sp_lreb.cpp`). Keep it as alpha when converting frames to a modern format.

A sprite's phase bounds are the union of its hires, lores and collision bounds. They are simulation state: they decide which level objects `look_at` sees and builds, and the on-screen tests of game objects. The `sprite_build` test checks the bounds, pixel data and palette tables of everything the game loads against the original's prebuilt caches.

The rebuild path that turns FLC frames into engine sprites is the reference for anything else a converter must preserve. It lives in `source/engine/1sp/1sp_lmai.cpp`, `1sp_lreb.cpp`, `1sp_ldsa.cpp` and `1sp_flic.cpp`. With the original engine in loose-file mode, missing targets were simply rebuilt on first load. The port builds every sprite in memory on load and never writes the target files.

## Music

Track selection is data-driven. `data/!global/missions.tdf` lists `headersong`, `footersong`, `songs` (one per entry in `missions_order`) and `netsongs`. The engine skips the CD's data track (see `source/engine/1ss/1ss_song.cpp`), so **song number N is `music/track{N+1:02}.flac`**. For example, `headersong = 1` (the mission screen) plays `track02.flac`. Mission songs loop.

## Porting notes

- **Include paths:** `source/game`, `source/regdata`, `source/engine/common` and each `source/engine/<module>`. Every game `.cpp` includes only `headers.h`.
- **Original defines:** retail = no `SHAREWARE`. `UNPROTECT` disables the CD-ROM check. `_DEBUG` turns on `HI_DEBUG`, which in turn forbids global `operator new` (allocations go through the engine heap via `NEW(...)`).
- **Platform-bound code in the original, by module** (all on SDL2 now, except DirectPlay):
  - `1sp`: DirectDraw video, 8-bit palette, and the asm blitters.
  - `1ss`: DirectSound and MCI CD audio. CD audio is now `music/`.
  - `1ee`: Win32 keyboard, mouse, joystick and timer, DirectPlay networking, and demo record/playback.
  - `1rg`: Windows registry and command line.
  - `1lg`: Win32 window procedure and message boxes.
  - `1io`: `io.h` file API and temp files.
  - `regdata`: registry-backed settings.
  - Throughout: pre-standard C++ such as `<iostream.h>` and implicit-`int` constants.
- **Determinism:** the simulation is deterministic lockstep, driven by input frames, with randomness only through `RAND`. Network play and demos depend on that. Demo playback throws `Eem_demo_sync_failure` when the replay diverges, so the recordings in `data/demo/` act as built-in regression tests for an accurate port. Note that `Game::DEMOVERSION` must match.
