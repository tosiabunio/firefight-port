# Porting Fire Fight

This document is for people working on the port. The [README](../README.md) is about the game, and how to build and play it.

The repository holds the original Fire Fight material needed to port the game to modern tools and other operating systems: the **retail** edition, with **hires art only**. Everything was copied from the original archive by rule; nothing was hand-picked. On top of that sits the port: an SDL2 + CMake build that runs the original code on Windows, Linux and macOS.

- [`porting-plan.md`](porting-plan.md): the plan, phase by phase, with what each phase found and fixed.
- [`original-archive.md`](original-archive.md): what the original archive holds beyond this subset (the built 1.1 executables, the shipped sprite caches, the 1.2 CD), what was checked against it, and how to run the original game.
- [`macos.md`](macos.md): checking a branch on the Mac before it goes into `main`.
- [`../CLAUDE.md`](../CLAUDE.md): a dense summary of the code base, the build and the rules of this repository.

## Status

| Phase | What | State |
|---|---|---|
| 0 | Build skeleton: CMake, vcpkg, CI | Done |
| 1 | The original code compiles and links on every platform | Done |
| 2 | Runtime on SDL2: entry point, events, timer, files, settings | Done |
| 3 | Video: SDL window, the blitter and fonts in C++, sprites built in memory | Done. The title, mission screen, a mission and a demo match the original 1.1 game pixel for pixel; the menus beyond the mission screen are not compared yet |
| 4 | Input: keyboard, mouse, game controllers | Done. A full mission played by hand on each device is still to be confirmed |
| 5 | Determinism and the demo regression suite | Done. All 8 original demos and 4 golden demos replay in sync on Windows, Linux and macOS |
| 6 | Sound and the CD soundtrack (SDL2_mixer) | Done. Checked by ear on Windows |
| 7 | Network play (ENet) | In progress. LAN play by address and by LAN discovery works; internet play, the relay server and the input delay are to do |
| 8 | Replace the launcher (in-game options, key bindings, network menus); packaging | Started: packages for the three systems and a release workflow. The menus are to do |
| 9 | The browser: a WebAssembly build (Emscripten) | In progress, before the rest of phases 7 and 8. Under Node.js all 8 original demos and the 4 golden demos replay in sync, and the title frames and the sprite build match. In Chrome the title, the attract demos and a mission play on the keyboard, with the music, and the settings and pilots survive a reload. The mouse, game controllers, sound by ear, Firefox and Safari are still to check |

The tests (`ctest`) check the dependencies, a headless run, the title frames, the sprite build against the original's caches, the MSVC 4 CRT clones, the original and golden demos, sound, network play, input and the data files. [`CLAUDE.md`](../CLAUDE.md) lists them.

## Repository contents

| Path | Contents |
|---|---|
| `source/game/` | All 26 translation units linked into `FIREFGHT.EXE`, plus `resource.rc` (progress dialog, version info) and `icon1.ico`. The port added `main.cpp` (entry point), `input_script.cpp` (test input) and `stale*` (an emulated uninitialised read) |
| `source/engine/<module>/` | The CW engine modules `1ba 1cw 1ee 1io 1lg 1mm 1rg 1sp 1ss`, without their test harnesses. `1sp/asm/` holds the original TASM blitters (the port has C++ versions) |
| `source/engine/common/` | `first.h` and `1cw_strg.h`, project headers that only existed in the original `z:\lib` |
| `source/regdata/` | The settings model (`RegData`) shared by the game and the launcher. `headers.h` is the launcher's header, with its MFC parts behind `#ifdef _MFC_VER` |
| `source/compat/` | Port layer: MSVC CRT extensions, clones of MSVC 4's `rand` and `qsort`, and the remaining Win32 stand-ins |
| `data/` | Retail game data, laid out exactly as the `*.dir` manifests expect (see below) |
| `music/` | The CD soundtrack, `track02.flac` … `track09.flac` (lossless rips of the CD tracks) |
| `web/` | The browser build's page (`index.html`) and the script it adds to the game (`pre.js`) |
| `docs/` | This document, the plan, the archive notes, the Mac notes and the screenshots |
| `CMakeLists.txt`, `CMakePresets.json`, `vcpkg.json`, `cmake/` | Build system |
| `tests/` | Test scripts, input scripts, golden files and golden demos |
| `tools/smoke/` | Dependency smoke test (SDL2 window and paletted present, SDL2_mixer FLAC/WAV, ENet loopback) |
| `tools/fonts/` | Generates the C++ fonts from the original font data |
| `tools/layout/` | `stale_memory.py`: generates `source/game/stale_gen.cpp` from clang's 32-bit MSVC record layouts (the original's object layout, which one uninitialised read depends on) |
| `tools/archive/` | Run where the original archive is: `ffarchive.py` (volumes, sprite caches, data provenance, sprite checks), `crt_vectors.py` (MSVC `qsort`/`rand` test vectors from the original exe) and `patch_hooks.py` (makes the original 1.1 exe run on Windows 11) |

## Building, for development

The [README](../README.md#building-from-source) has the requirements and the basic commands. The presets are `windows-msvc`, `linux-gcc`, `linux-clang` and `macos-clang`; each has `-debug` and `-release` build and test presets.

`node-emscripten` builds the game as WebAssembly and runs its tests under Node.js (phase 9). It needs the Emscripten SDK (CI uses 6.0.11): `EMSDK` set by its `emsdk_env` script, and CMake and Ninja on the `PATH`. It needs no vcpkg.

`web-emscripten` builds the game for a web browser (phase 9), with the same SDK. `cmake --workflow --preset web-emscripten` builds Debug and Release; it has no tests, because the Node build runs them. The music is encoded with `oggenc` from vorbis-tools; without it the build has no music. The result is a directory of static files, `build/web-emscripten/web/<config>/`: the page (`index.html`), the game (`firefight.js` and `.wasm`), the data as one package (`firefight.data`) and the music as Ogg Vorbis (`music/`, 27 MB). Any static web server can serve it:

```sh
python3 -m http.server -d build/web-emscripten/web/Release   # then open http://localhost:8000/
```

The browser needs JSPI: Chrome or Edge 137+, Firefox 153+, Safari 27 or newer. Options in the URL become command-line options: `?demo=level1&fast` plays one demo, as fast as the display allows, and ends. The log goes to the browser's console, and the whole of it is in `Module.FS.readFile('/tmp/Firefght.log', {encoding: 'utf8'})`. The settings, pilots and recorded demos are kept in the site's IndexedDB. The game pauses when its page loses the focus, as it does when its window does on the desktop.

```sh
cmake --workflow --preset linux-gcc            # configure, build Debug + Release, run all tests
# or step by step:
cmake --preset linux-gcc
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
```

Options for testing (all in `source/game/main.cpp`):

- `--headless`: no window, sound or input devices; `--sound` keeps the sound.
- `--fast`: no clock, one simulation step per frame, so a run is the same frame for frame.
- `--demo <name>`: play one recorded demo (`level1` … `level4C`, or `!<path>` for a demo file). The exit code is 3 when the replay goes out of sync, 4 when it stops early.
- `--shot-every <n>` / `--shots <s>`: dump frames to the preferences directory.
- `--input <file>`: play an input script (keys, mouse, a virtual game controller at given frames; the format is in `source/game/input_script.cpp`).
- `--quit-after <s>`, `--quit-frames <n>`: quit as if the window were closed.

**Packages** (`cmake/FFPackaging.cmake`): `cpack --config build/<preset>/CPackConfig.cmake -C Release -B build/<preset>/package` makes a zip on Windows (with the DLLs and the MSVC runtime), a tar.gz on Linux and a disk image with `Fire Fight.app` on macOS (signed ad hoc, not notarised). Each holds the game, `data/`, `music/`, the README and the license; the game finds `data/` next to its executable, or in the app's `Resources` on macOS. Pushing a version tag (`v0.7.0`, matching `VERSION` in `CMakeLists.txt`) runs `.github/workflows/release.yml`: it builds, tests and publishes the three packages as a GitHub release.

To see the smoke test's window and hear its audio, run it without `--headless`:

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

3. Get the repository:

   ```sh
   git clone https://github.com/tosiabunio/firefight-port.git
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

6. Optionally, the WebAssembly builds (phase 9): the Emscripten SDK at CI's version, and the Ogg Vorbis encoder for the browser build's music:

   ```sh
   git clone --depth 1 https://github.com/emscripten-core/emsdk.git ~/emsdk
   ~/emsdk/emsdk install 6.0.11 && ~/emsdk/emsdk activate 6.0.11
   brew install vorbis-tools
   source ~/emsdk/emsdk_env.sh                 # in each new shell, before these presets
   cmake --workflow --preset node-emscripten   # the tests under Node.js
   cmake --workflow --preset web-emscripten    # the browser build (see "Building, for development")
   ```

## Provenance and version

- **Source:** the source and the data in `data/` form a consistent pair, both version 1.1.
  - The source snapshot dates from 31 Aug 1996, which is version 1.1. The original build scripts produced the `ff11up` patch from it.
  - `data/` comes from the retail working tree, `FF/WORK.RTL`. Every file in the retail CD's data volumes (dated May 1996) is byte-identical to `data/` or to a sprite cache in `FF/WORK.RTL`.
- **The 1.2 CD differs only in its executables:** a newer `FIREFGHT.EXE`/`LOADER.EXE` (Jul 1997). Its `PARAMS.VOL` is dated 1997 but repacks the same `.tdf` files. No source exists for 1.2, so this tree reproduces 1.1. See [`original-archive.md`](original-archive.md).

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
- **Data bytes:** data files are **byte-for-byte originals**. Text data is CP852 with CRLF line endings. The in-game fonts index glyphs by byte value, so don't re-encode them. No data file has been edited; the `data_files` test checks every file's SHA-256.

## Data layout

- **Manifests:** each `data/<volume>.dir` describes one original volume and maps *logical names* to files. Code never opens data by path, except `cwe.ini` and the `*.dir` manifests. It asks for logical names, e.g. `mainparams`, `back`, `brown_1_1_1`.
  - Syntax: `area <name> … endarea` blocks and `key = value [flags]` entries, with `;` starting a comment. Parsed by `Text` in `source/engine/1io/1io_txt.cpp`.
  - A `-` before a path means "not packed into the shipped volume", i.e. it marks a source file.
  - Load order: `update,global,params` is mounted at startup. Each mission then mounts `update,<world>,<world><n>`, plus `<world><n>s` (mission speech) when spoken dialogs are on. `header` and `demo` are mounted for the title/statistics screens and demo playback.
- **Worlds:** `brown`, `gray`, `green`, `white` (single-player) and `net` (multiplayer, plus two campaign missions). Each world directory contains:
  - `<world>.def` — level file from the LED editor: type and sprite tables, plus per-level plane maps.
  - `<world><n>m.tdf` — mission script: weather, checkpoints, bonus objectives, enable/disable actions.
  - `<world><n>t.tdf` — object type parameters.
- **Global data:** `data/!global/` holds the global tunables (`main.tdf` → class `Mp`, `object.tdf` → class `Op`), menus, print layouts, `text.txt` (all UI strings, via `GAMETXT("LABEL")`, and the end credits) and `missions.tdf` (mission order and music).
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
- **Platform-bound code in the original, by module** (all replaced now, by SDL2 and ENet):
  - `1sp`: DirectDraw video, 8-bit palette, and the asm blitters.
  - `1ss`: DirectSound and MCI CD audio. CD audio is now `music/`.
  - `1ee`: Win32 keyboard, mouse, joystick and timer, DirectPlay networking, and demo record/playback.
  - `1rg`: Windows registry and command line.
  - `1lg`: Win32 window procedure and message boxes.
  - `1io`: `io.h` file API and temp files.
  - `regdata`: registry-backed settings.
  - Throughout: pre-standard C++ such as `<iostream.h>` and implicit-`int` constants.
- **Port changes to the original code** stay minimal and say why in a short comment (`// was ...`, `// port: ...`). The pristine original is commit `d73171a`.
- **Determinism:** the simulation is deterministic lockstep, driven by input frames, with randomness only through `RAND`. Network play and demos depend on that. Demo playback throws `Eem_demo_sync_failure` when the replay diverges, so the recordings in `data/demo/` act as built-in regression tests for an accurate port. Note that `Game::DEMOVERSION` must match.
