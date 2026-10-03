# Fire Fight port plan: SDL2 + CMake, cross-platform

Status: **proposal**. Based on a code survey of this repo (Oct 2026). File references are relative to `source/`.

## Goals

- **Gameplay:** v1.1 retail gameplay, faithful down to the simulation, on **Windows, Linux and macOS**, 64-bit.
- **Art and audio:** hires art only (640×400), with the CD soundtrack from `music/`.
- **Fidelity bar:** the 8 original attract-mode demos (`data/demo/*.rec`) replay to the end **in sync** on every platform. This test only works if the simulation is bit-exact, and bit-exactness is also what makes cross-platform network play possible.

**Non-goals (for now):** new art, widescreen or higher internal resolution, rewriting gameplay, the shareware edition, lores mode, the level editor.

## Strategy

**Keep the game code, keep the engine's API, replace the engine's platform layer.**

| Part | Size (non-blank lines) | Platform code | Plan |
|---|---|---|---|
| `game/` | 30,160 | ~0.1% (`WinMain`, progress dialog, CD check, a few `timeGetTime`) | Keep. Mechanical modernisation only |
| `engine/` C++ | ~17,400 | concentrated in drivers (`1sp_vdrv`, `1ss*`, `1ee_netw/timr/kbrd/mous/joys`, `1rg`, `1lg`, `1cw`) | Rewrite the drivers on SDL2 in place. Keep module APIs |
| `engine/1sp/asm` | 5,745 TASM | 100% | Replace with C++ (blitter ~350 lines; fonts converted to bitmaps by a script) |
| `regdata/` | 926 | registry-backed | Back it with a settings file |

- **Edit in place:** the original files are edited where they are. The initial commit `d73171a` is the pristine original, so `git diff d73171a` always shows exactly what the port changed.
- **Compat library:** old-compiler behaviours the simulation depends on are cloned in a small new `source/compat/` library. The modules don't each fix them separately.

## Toolchain and dependencies

| Item | Choice |
|---|---|
| Language | C++17 (`std::filesystem`, `static_assert`). Compiled as 64-bit only |
| Build | CMake ≥ 3.21 with `CMakePresets.json`: `windows-msvc`, `linux-gcc`, `linux-clang`, `macos-clang`, each in debug and release |
| Dependencies | vcpkg manifest mode (`vcpkg.json`, pinned baseline): `sdl2`, `sdl2-mixer` (with FLAC), `enet`. Plain `find_package` also works for distro packages on Linux |
| Compilers | MSVC 2022, GCC, Clang/AppleClang |
| Safety flags | `-fwrapv -fno-strict-aliasing -fsigned-char` on GCC/Clang. The code relies on wrapping arithmetic, type punning and signed `char`, which matters on Linux ARM64. MSVC already behaves this way |
| CI | GitHub Actions matrix: Windows/MSVC, Ubuntu/GCC+Clang, macOS/AppleClang. macOS and Windows runner minutes count at a multiple of Linux minutes against the private-repo allowance, so run macOS on `main` and nightly only |

**Local environment** (on the current development PC):
- **Windows:** Visual Studio 2022 Build Tools (MSVC 14.44, with bundled CMake, Ninja and vcpkg) are installed under `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`. They aren't on `PATH`, so use a Developer PowerShell or `vcvars64.bat`.
- **Linux:** `wsl --install Ubuntu` gives local GCC/Clang.
- **macOS:** CI only, unless a Mac is available.

SDL2 is the stated target. The platform code will sit only in the engine drivers, so a later move to SDL3 would stay contained.

## Phases

Each phase ends with something runnable and an exit check. Phases 5–7 can overlap once phase 4 is done.

### Phase 0: build skeleton and CI
- Top-level `CMakeLists.txt` with targets:
  - `cwengine`: static library of all engine modules;
  - `regdata`;
  - `compat`;
  - `firefight`: the executable.
- Define `EXCLUDE_LIBS` globally to neutralise the `#pragma comment(lib)` lines.
- Add `CMakePresets.json`, `vcpkg.json` and a CI workflow.
- A hello-SDL window target proves that the dependencies work on all three OSes.
- **Exit:** CI is green on Windows, Linux and macOS with an SDL window that opens and closes.

### Phase 1: make the original code compile everywhere, with no behaviour change
- **Mechanical fixes** (sites counted with MSVC 2022 trial builds):

  | Fix | Sites |
  |---|---|
  | Implicit-`int` constants | 26 |
  | Loop variables used after their `for`. Every site is a compile error, none is a silent rebind (checked with C4288) | 134 |
  | Delete the never-used `<iostream.h>`/`<fstream.h>`/`<iomanip.h>` includes | — |
  | `register` (invalid in C++17) | 19 |
  | Non-const references bound to temporaries | 12 |
  | `__DATE__" "__TIME__` spacing (`1lg.h:20`) | 1 |
  | Extra qualification `Comm::set_lineunique` (`1lg.h:122`) | 1 |
  | Two-phase lookup (`queue.h:44`) | 1 |
  | Private `Radar::TargetX/Y` accesses (`my.cpp:790,2407`) | 2 |
  | MSVC-only `(short)x++` cast-as-lvalue (`1ee_main.cpp:427-453`) | — |
- **String literals:** about 1,540 literal→`char*` sites. Build with `/Zc:strictStrings-` and `-Wno-write-strings` at first, and const-correct over time.
- **CRT shims** (`compat/crt.h`):
  - `strcmpi` (132 calls), `strupr`, `strlwr`, `strnicmp`, `_splitpath`, `_MAX_PATH`, `_vsnprintf`;
  - `_findfirst` (`int` handle truncation), `_open`/`_read`/… mapped onto stdio.
- **64-bit fixes:**
  - pointer truncation: `1sp_ldsa.cpp:61,69`, `1sp_scr1.cpp:104,121`, `1mm.cpp:365,367`, `1io_cmpr.cpp:42`, `1io_io.cpp` `File_Info.offset`, the timer callback `DWORD_PTR`, and 17 MCI `(DWORD)&parms`;
  - packed structs holding pointers: `Heap::mcb`, `Sprite::Data`, `Level::plane_buf`;
  - `Text`'s unaligned buffer layout (`1io_txt.cpp:498-525`).
- **`static_assert`s:**
  - `sizeof(Player_state)==154`, `sizeof(DemoInfo)==264`, `sizeof(Demo_header)==24`;
  - every `FastAlloc` object ≤ the 1024-byte slot. `Splinters` is already 992 bytes on x64, and release builds don't check;
  - little-endian.
- **Stubs:** platform calls get no-op stubs so the program links.
- **Exit:** `firefight` builds and links on all three OSes. A `--headless` run gets through `Game::init_all` against `data/` and logs what it loaded.

### Phase 2: core runtime on SDL (no video yet)
- **Entry point and shutdown:** `SDL_main` replaces `WinMain`. Shutdown becomes an explicit call; today it relies on a static destructor ordered by `#pragma init_seg`, which GCC and Clang ignore.
- **`1lg`:**
  - logs go to `SDL_GetPrefPath` and the console;
  - `MessageBox` becomes `SDL_ShowMessageBox`;
  - `CHECK`'s `__asm int 3` becomes `SDL_TriggerBreakpoint`;
  - drop the single-instance mutex and the "respawn the loader on exit" path.
- **`1rg` + `regdata`:**
  - the registry becomes an INI-style settings file in the pref path;
  - **every default the launcher used to write is built in.** Without this, init fails: the `hires mode`/`lores mode` values (`1cw.cpp:314-326`) and the 23-control × 3-set key bindings (`1regdata.cpp:166-238`);
  - pilot progress (a 3388-byte, XOR-scrambled registry blob, `pilots.cpp:20-165`) moves to one file per pilot, with the same bytes;
  - the `GUID` fields become fixed-width.
- **`1io`:**
  - stdio and `std::filesystem` replace the CRT calls;
  - data root: next to the executable, overridable with `--data`;
  - lowercase plus `/` path resolution;
  - the `.dir` manifest merge happens in memory, not through a temp file;
  - drop `.vol` and Xio support (not needed for loose files).
- **`1mm`:** back it with `malloc` (the existing `MMU_USE_WINDOWS_HEAP` path) using 16-byte alignment. Keep the owner-name diagnostics.
- **`1ee` timer:**
  - ticks are generated **on the main thread** from `SDL_GetPerformanceCounter`, every 33 ms (the original period, 1000/30 rounded down, i.e. 30.3 Hz);
  - keep the 4-frame queue cap, so a slow machine slows the game down rather than spiralling;
  - wait with `SDL_WaitEventTimeout` instead of busy-spinning;
  - this also removes the original unsynchronised timer-thread race.
- **Event pump:** `Comm::process_messages` → `SDL_PollEvent`.
  - `SDL_QUIT` → throw `Closed`.
  - Focus loss → the original behaviour: single player freezes (`standby`); network play keeps simulating without drawing.
- **Exit:** `--headless` boots to the title loop and runs the simulation on a clock.

### Phase 3: video (`1sp`)
- **Framebuffer and present:**
  - window and renderer, with one **640×400 8-bit software framebuffer**, i.e. the hires mode with `use_640x400`;
  - force hires, so the lores startup mode (`Video::init`, `1sp_vide.cpp:504`) and `upside_down` stay off;
  - present each frame by converting palette → ARGB8888 using the original gamma tables. Output is 6-bit×4, so the maximum is 252, not 255;
  - emulate the Windows static palette entries 0–9 and 246–255;
  - draw to a streaming texture with letterboxing.
- **Display options:** integer scaling, square pixels (the 640×480-with-bars look) or 4:3 CRT stretch, and desktop fullscreen.
- **Blitter:**
  - replace `_uniput`'s 8 modes with C++: put, one-colour shape, Tool, shadow, copy, two transparency variants and the collision scan;
  - the TASM macros in `asm/common.asi` and `asm/bsp.asi` give the exact semantics;
  - **keep the collision scan order** (each run right to left, results LIFO), because it feeds collision messages and so the simulation.
- **Fonts:** a one-off script turns `font_hi`'s compiled glyph code into a bitmap table.
- **Inline asm:**
  - `Color::distance`: copy `ccpower_tab` verbatim, including its quirks (octal `0121`, and `(|d|−1)²` for negative differences);
  - `Screen::copy`;
  - `put_char`.
- **Sprites:**
  - build every sprite **in memory from the FLC masters at load time**, using the existing rebuild code;
  - drop the on-disk `.sph`/`.spc`/`.spp` caches and their pointer-dependent file format;
  - **synthesise lores bounds:** phase bounds are the min/max over the hires, lores and collision variants (`1sp_lmai.cpp:329-332`). Level sprites' lores always came from the same `source` FLC, so compute the scale-2 bounds without keeping the pixels.
- **Keep these quirks:**
  - horizontal mirroring is effectively ignored (no manifest has `m+`), while vertical mirroring works;
  - floor rounding in `Screen::convert`;
  - the FLC single-chunk literal-frame read.
- **Other changes:**
  - drop the Win32 progress dialog;
  - screenshots go to the pref path.
- **Exit:** the title, menus, mission briefing and missions render and match reference screenshots from the original game. Capture those from the retail CD image, set up as described in the archive's `Instructions.txt`. Those instructions cover Windows Vista–8; Windows 11 is untested.

### Phase 4: input; single player becomes playable
- **Keyboard:**
  - an `SDL_Scancode` → **PC set-1 scancode** table, with +128 for E0 keys;
  - the demos and the stored bindings use set-1 codes, so the code space must stay;
  - filter auto-repeat as before;
  - keep the hard-coded US shift table for chat text, so chat stays deterministic.
- **Mouse:** relative mode for mouse steering, scaled to logical coordinates.
- **Joystick:** `SDL_GameController`, falling back to `SDL_Joystick`. Feed the original 6 digital axes (±) and 32 buttons, with the dead zone as a percentage.
- **Exit:** a full mission can be played on keyboard, mouse and gamepad, and F11 cycles the control sets.

### Phase 5: determinism and the demo regression suite
- **Clone the old-toolchain behaviours in `compat/`:**
  1. **MSVC `rand()`.** `Rand::init` fills its table with `srand(0)` and 1024 × CRT `rand()` (`gobj.cpp:39-42`). glibc and Apple's libc produce different numbers, so embed the MSVC LCG.
  2. **MSVC 4 `qsort`.** The collision results are sorted with every priority 0 (`1sp.h:267-268`, `1sp_scr2.cpp:107-121`), so the order of equal keys is whatever MSVC's algorithm produced.
  3. **x87 float→tick conversions.** `(int)(seconds*1000)` with `float` inputs (`params.cpp:3,781,…`) gives a different answer under x87 than under SSE for 11 of the 35 decimal values in `data/`, for example 0.7 s → 699 vs 700. Doing the multiply in `double` reproduces x87. Whether MSVC 4 kept the extended precision is unknown, so make it a switch and let the demos decide.
- **Headless demo runner:**
  - `firefight --headless --play-demo <file> --strict-sync` exits non-zero on `Eem_demo_sync_failure`. The original catches that exception silently and just ends the demo;
  - it also checks that the replay reached the demo's last frame;
  - register it with CTest for all 8 demos.
- **Built-in sync signal:** each frame the game compares the sum over ships of x + y + angle + life, plus the `Rand` index (`game.cpp:909-937`).
- **Diagnostic traces:** `randdebug`, `shipdebug` and `netdebug` (`MAIN.TDF` levels) already dump traces for diagnosing a divergence.
- **If an original demo desyncs, suspects in order:**
  1. the three compat behaviours above;
  2. collision scan order;
  3. lores bounds;
  4. render-path side effects: `World::display` calls `look_at` **with builders** (`world.cpp:74-81`). Until that is proven harmless, keep the original cadence of one render per batch of ticks, and run the same calls in headless mode;
  5. the uninitialised `ACannon::global_time` read (`alien.cpp:1726`, `==` instead of `=`);
  6. finally, a real 1.0 → 1.1 gameplay change. The demos are dated May 1996; this source is August 1996.
- **Golden demos:** record a further set with the port across several missions and skill levels. CI replays them on all three OSes; this is the cross-platform determinism gate.
- **Exit:** the original demos replay in sync, or any divergence is explained and documented, and the golden demos pass on Windows, Linux and macOS.

### Phase 6: audio (`1ss`)
- **Samples:** SDL2_mixer, with the original voice model:
  - 8 channels by default (configurable up to 16);
  - one voice per sample (replaying restarts it);
  - priority-based stealing using the manifest priority 1–9.
- **Formats and units:** all 475 WAVs are 8-bit mono 11,025 Hz.
  - Pan is ±3000 hundredths of a dB → `Mix_SetPanning`.
  - Volume goes through the original `logvol` table → linear.
- **Music:** CD audio becomes `Mix_PlayMusic` on `music/track{N+1:02}.flac`, looped, paused when focus is lost.
- **Removed:** the system-wide `Mixer` (it changed OS volumes and never restored them). The MIDI `Song` path is shareware-only, so stub it.
- **To check:** `Sample::playing()` is polled inside the simulation step (`msg.cpp`). Confirm it can't affect simulation state; otherwise drive it by sample length in ticks so results don't depend on audio timing.
- **Exit:** audio matches the original by ear: positioning, priorities, speech ducking and music per mission.

### Phase 7: network (`1ee_netw`)
- **ENet replaces DirectPlay.** Keep the engine's message layer (`NET_VERSION`, packed `Player_state`/`User_block`, chat inside the states). ENet takes over transport, sessions and the roster.
- **Topology:** the original is a full mesh of up to 4 players. **Host relay** is proposed: clients only connect to the host, which forwards states. Only the host needs a reachable port, at the cost of one extra hop.
- **Setup:** host or join by `address:port` from the command line and settings, with LAN discovery optional. This replaces the launcher's DirectPlay wizard and the registry `[eem]` values.
- **Exit:** a 4-player game across Windows, Linux and macOS runs 30 minutes without a sync failure. This requires phase 5.

### Phase 8: replace the launcher; packaging
- **In-game options** for video (scaling, fullscreen), audio volumes, **key binding editing** (the original only edited bindings in the launcher; `menu.cpp:1049-1097` only shows them) and pilot management.
- **Packaging:** CPack Windows zip/installer, a macOS `.app` (signing and notarisation only if it is distributed), a Linux tarball/AppImage. CI publishes the artifacts.

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Original demos desync even with the compat clones (possible 1.0 → 1.1 change) | Lose the strongest exactness oracle | Golden demos from the port. Debug traces. Reference screenshots from the retail game |
| Simulation side effects in the render path (builders in `look_at`) | Headless runs or a different frame pacing change outcomes | Keep the original render cadence; investigate in phase 5 |
| `FastAlloc` slot overflow on other ABIs (992/1024 bytes on MSVC x64; GCC/Clang unknown) | Memory corruption | `static_assert` all object sizes; enlarge the slot if needed. Doesn't affect determinism |
| Blitter reimplementation not pixel-exact | Visual differences; collision order changes | Unit-test each mode against the asm semantics. Screenshot diffs |
| Uninitialised `ACannon::global_time` | Behaviour depended on stale memory | Characterise its effect, then fix it deliberately (and document it) or emulate it |
| Internet play through NAT | Full mesh rarely works | Host relay; optional relay server later |
| Private-repo CI minutes (macOS multiplier) | Cost | macOS on `main`/nightly; Linux for most checks |

## Decisions needed

1. **Fidelity bar:** bit-exact replay of the original demos (proposed), or accept small divergences in exchange for cleaner fixes (e.g. fixing the `global_time` bug)?
2. **Multiplayer scope:** LAN only, or internet play too? This decides host-relay vs a relay server and whether LAN discovery is needed.
3. **Default presentation:** square pixels (640×400 letterboxed, as in the original 640×480 mode) or 4:3 CRT stretch?
4. **macOS:** Apple Silicon only, or a universal binary? Is there a Mac for testing, or CI only?
5. **Launcher replacement:** in-game options menus (proposed), or a config file plus a small separate settings tool?
