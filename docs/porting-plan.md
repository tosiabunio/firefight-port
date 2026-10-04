# Fire Fight port plan: SDL2 + CMake, cross-platform

Status: **accepted.** Decisions recorded 2026-10-03 (see [Decisions](#decisions)). Based on a code survey of this repo. File references are relative to `source/`.

## Goals

- **Gameplay:** v1.1 retail gameplay, faithful down to the simulation, on **Windows (x64), Linux (x86-64) and macOS (Apple Silicon)**.
- **Art and audio:** hires art only (640×400, square pixels by default), with the CD soundtrack from `music/`.
- **Multiplayer:** up to 4 players over **LAN and the internet**.
- **Fidelity bar: aim for bit-exact.** The 8 original attract-mode demos (`data/demo/*.rec`) should replay to the end **in sync** on every platform. This test only works if the simulation is bit-exact, and bit-exactness is also what makes cross-platform network play possible. A divergence is acceptable only if it is proven to come from a 1.0 → 1.1 gameplay change in the original code. Any such divergence is documented, and the port-recorded golden demos take over as the gate.

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
| Targets | Windows x64, Linux x86-64, macOS **arm64 only** (`CMAKE_OSX_ARCHITECTURES=arm64`, deployment target macOS 11, the first Apple Silicon release). No universal binary |
| Dependencies | vcpkg manifest mode (`vcpkg.json`, pinned baseline): `sdl2`, `sdl2-mixer` (with FLAC), `enet`, and optionally `miniupnpc` for automatic port mapping. Plain `find_package` also works for distro packages on Linux |
| Compilers | MSVC 2022, GCC, Clang/AppleClang |
| Safety flags | `-fwrapv -fno-strict-aliasing -fsigned-char` on GCC/Clang. The code relies on wrapping arithmetic, type punning and signed `char`, which matters on Linux ARM64. MSVC already behaves this way |
| CI | GitHub Actions matrix: Windows/MSVC, Ubuntu/GCC+Clang, macOS arm64/AppleClang. macOS and Windows runner minutes count at a multiple of Linux minutes against the private-repo allowance. Run macOS on `main` and nightly only; day-to-day macOS testing happens on the local Mac |

**Local environment** (on the current development PC):
- **Windows:** Visual Studio 2022 Build Tools (MSVC 14.44, with bundled CMake, Ninja and vcpkg) are installed under `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`. They aren't on `PATH`, so use a Developer PowerShell or `vcvars64.bat`.
- **Linux:** `wsl --install Ubuntu` gives local GCC/Clang.
- **macOS:** a local Apple Silicon Mac. It needs Xcode Command Line Tools, CMake and vcpkg (or Homebrew `sdl2`, `sdl2_mixer`, `enet`).

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
- **Outcome** (2026-10-03). Exit met on every CI preset (Windows/MSVC, Linux GCC and Clang, Debug and Release) and on macOS (AppleClang, local). The `headless_init` CTest checks the loaded sprite totals and a clean shutdown in the log. The `headless_init` CTest runs the exit check.
  - **Stubs:** `compat/win32.h` replaces `<windows.h>`, `<ddraw.h>`, `<dsound.h>`, `<dplay.h>`, `<mmsystem.h>` and friends **on every platform, Windows included**, so all builds run the same stubs. Creation calls return inert dummy handles; DirectX is never loaded (`LoadLibrary` returns `NULL`); MCI, mixer, wave-out and joysticks report no device. The clocks, environment, temp files and `VirtualAlloc` (zero-filled) are real. The registry is in memory for the run, with a fallback for the two values the launcher always wrote and nothing else defaults (`spr/hires mode` and `spr/lores mode` = 1, RegData's defaults: 640×480 and 320×240).
  - **CRT:** `compat/crt.h` maps the low-level I/O onto POSIX file descriptors rather than stdio. `_findfirst` handles are small integers, and the call sites now store `intptr_t`.
  - **Asm:** the inline asm in `Color::distance`, `Screen::copy` and `Comm::assert_box` is now exact C++. `_pconv`, `_ptouch`, `_pcsum` and `_fast_xlat` are exact ports (`1sp_asm.cpp`). `_uniput` and the font glyph routines are no-op stubs until phase 3. `CHECK`'s `int 3` calls `DebugBreak()`.
  - **Headless:** `--headless` adds the engine switches `headless=1 sos_none=1`. `Video` then skips DirectDraw and draws normal mode (no `upside_down`) into the memory device `VD_bitmap`. `WinMain` stops after `init_all` until phase 2.
  - **Pulled forward from phase 2:**
    - manifest paths resolve to lowercase with `/` (in `File::parse_label`), and the `<name>.dir` names are lowercased;
    - the log path is no longer upper-cased;
    - shutdown order: `#pragma init_seg(lib)` became a "nifty counter" (`Comm_init` in `1lg.h`), so the quit manager is still created before and shut down after every static. Apple's linker ignores `init_priority`, so that route is closed;
    - `File::create` creates missing directories, because the sprite cache directories are not in the repository.
  - **Other fixes:**
    - the static `Listmanager::run` is renamed `run_all`, because C++ forbids it hiding the virtual `Object::run`;
    - `friend class Weapon` needed a forward declaration;
    - splinter sprites are detected by their `hires` entry, since the port data has no `lores` lines (undone on 2026-10-04, when the lines came back);
    - `SysSet::init` writes one past `ranges`/`defaults`, so they have a spare slot. `data` keeps its size because it is saved with the pilot;
    - the Xio hash table holds 64-bit pointers. Xio has to stay: **demo files are Xio-compressed**;
    - `Heap::mcb` is 48 bytes and 16-byte aligned, and blocks are rounded to 16. `Sprite::Data` and `plane_buf` are no longer packed. `Phase` stays packed: its arrays follow a variable-length name in the same buffer;
    - `Text` aligns its pointer arrays and value slots.
  - **MSVC runtime:** `main` installs a no-op invalid-parameter handler and sends CRT reports to stderr. MSVC 4 returned an error for bad arguments; the modern debug CRT opens a modal dialog instead, which hung CI on the original double `_close` of `cwe.ini` in `Cwe::init`.
  - **Not a problem:** `(short)(queue[i])++` is not a cast-as-lvalue. Postfix `++` binds tighter than the cast, so every compiler increments the whole `int`, MSVC 4 included.
  - **Sizes on arm64:** largest `FastAlloc` object is `Splinters` at 960 bytes. `Player_state`, `DemoInfo` and `Demo_header` match the original.
  - **Cross-platform check:** the 132 sprite caches rebuilt from the FLC masters are byte-identical between Linux/GCC and macOS/Clang.
  - **For phase 5:** `MyCannon`'s constructor reads the member `type` before it is set (`Turn(0,(type==1)?…)`, `my.cpp:3458`). On MSVC 4 it read stale `FastAlloc` slot memory.

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
  - drop `.vol` support (not needed for loose files). Keep Xio's decompressor: the demo files use it.
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
- **Outcome** (2026-10-03). Exit met: `headless_run` boots headless, runs the title loop on the 30 Hz clock until it starts an attract demo (level GREEN 6), and quits after 30 s as if the window were closed. Idle CPU is near zero.
  - **Entry:** `main.cpp` is `SDL_main`; `WinMain` became `game_main(command_line)` and the command line reaches `Cmd_line` directly. New options: `--pref`, `--quit-after` (engine switch `quit_after=<ms>`). Data directory: `--data`, else `data/` next to the executable, else the repository's `data/` in development builds.
  - **Shutdown stays implicit.** Phase 1's nifty counter already orders the final quits after every static destructor, on every compiler. An explicit call from `main` would run them before statics such as `Layout::text` free their heap blocks. The rule that follows: state used by a quit procedure must outlive static destruction (the registry stores are never destroyed).
  - **Pump:** SDL events reach the original window-procedure chain as the Win32 messages they replace (`SDL_QUIT` as `WM_CLOSE`, focus as `WM_ACTIVATEAPP`), so `Eem`'s standby logic is unchanged. The pump also runs the timers. `Eem::read` sleeps until the next tick when the previous read found nothing.
  - **Timer:** `Timer` keeps its API. A timer that falls more than 1 s behind skips ahead instead of firing a burst. `Eem` queues ticks directly in network mode too (the original's `WM_TIMER` detour existed only to leave the timer thread).
  - **Heap:** zero-filled `malloc` blocks behind the original header. Zero-filling makes reads of uninitialised heap memory identical on every platform; the original arena started zeroed but recycled blocks with old contents.
  - **Files:** stdio and `stat`; manifests merged in memory; volumes removed. Demo files, the log, screenshots and the build-mode report use stdio and the preferences directory. The original opened demo files in text mode on Windows (`O_BINARY` passed as the permission argument), and its screenshot name buffer could overflow.
  - **Settings:** `settings.ini` with typed entries (`dword:`, `str:`, `hex:`). Only `spr/hires mode` and `spr/lores mode` need built-in launcher defaults; the key bindings already default to `RegData::DefControls`. Pilot progress: one file per pilot, same 3388 bytes. The `GUID` fields were already fixed-width (`compat/win32.h`).
  - **Overload hazard:** C++11 and later only use the string-literal→`char*` conversion as a last resort, so `print(0,0,"%s",msg)` chose `Screen::print(char*,...)` with a NULL format and crashed. That overload set now takes `const char*`. A scan of all overload sets with `char*` parameters found no other case where a literal changes the choice.
  - **compat:** the stand-ins nothing calls any more are gone (registry, command line, temp files, environment, message loop, mutexes, multimedia timer, message boxes), and so are the CRT file-descriptor and directory-search shims.

### Phase 3: video (`1sp`)
- **Framebuffer and present:**
  - window and renderer, with one **640×400 8-bit software framebuffer**, i.e. the hires mode with `use_640x400`;
  - force hires, so the lores startup mode (`Video::init`, `1sp_vide.cpp:504`) and `upside_down` stay off;
  - present each frame by converting palette → ARGB8888 using the original gamma tables. Output is 6-bit×4, so the maximum is 252, not 255;
  - emulate the Windows static palette entries 0–9 and 246–255;
  - draw to a streaming texture with letterboxing.
- **Display options:**
  - **square pixels by default:** 640×400 letterboxed, as the original looked in its 640×480 mode, with integer scaling where the window allows it;
  - 4:3 CRT stretch as an option;
  - desktop fullscreen.
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
- **Exit:** the title, menus, mission briefing and missions render and match reference screenshots from the original game. Capture those from the retail CD image, set up as described in the archive's `Instructions.txt`, or from the 1.1 build in the archive, which runs on Windows 11 once patched ([original-archive.md](original-archive.md#running-11)).
- **Outcome** (2026-10-04). Everything above is in place.
  - **Compared with the original 1.1 game** the same day ([original-archive.md](original-archive.md#reference-screenshots-phase-3)). The title, the mission screen, mission 1 and the attract demo `level4c` render pixel for pixel like the port's frame of the same simulation step.
  - **Explained differences:** the window's rounded corners in grabs of the original; the mission screen's background and the speech timing, which follow the wall clock; the overlays the original's screenshot key leaves out.
  - **Still open:** 33 snowflakes that differ in one demo frame, and the menus beyond the mission screen.
  - **Rendering is bit-identical across platforms.** The `golden_title` test runs the title sequence in `--fast` mode, whose logo flight uses float arithmetic. Every 20th of its first 200 frames is identical on Windows/MSVC x64, Linux GCC and Clang x86-64 and macOS arm64.
  - **Floats:** that needed `-ffp-contract=off` on GCC/Clang. Clang on arm64 otherwise fuses `a*b+c` into one multiply-add, which rounds differently from x86. This matters for the simulation too.
  - **Blitter:** `_uniput` is one row walker with a run operation per mode (`1sp_asm.cpp`). It follows the assembly's clipping, including its edge cases, and is commented in place. The collision scan keeps the right-to-left run order and the LIFO result order.
  - **Fonts:** `tools/fonts/convert_fonts.py` runs each compiled glyph routine of `asm/1sp_afhi.asm`/`1sp_aflo.asm` symbolically and writes row bitmasks to `1sp_font.cpp`.
  - **Video:** `VD_sdl` replaces both the DirectDraw and the GDI-bitmap devices; the window is created by `Spr::create_window`. Display options are command-line switches for now (`--fullscreen`, `--stretch`); the phase 8 menus take them over. The game draws on every batch of ticks, presented with vsync.
  - **Sprites:** built in memory on every load in about 1 s (`keep_prepared`). Nothing is written to `data/`, and the "building …" screens the original showed for a missing cache are gone.
    - Lores bounds come from `Lsprite::measure` (the lores master at the lores target's scale, no pixels).
    - **Fixed against the archive (2026-10-04):** this first took the `source` FLC at 2×1, because the port data had dropped the `lores`/`lsource` lines. That was wrong for 15 sprites, among them the level sprite `bronie` (ship upgrades), whose bounds decide when its builders fire.
      - The manifests are the original files again, plus the 22 lores masters.
      - All 382 sprites the game loads now match the shipped caches. The `sprite_build` test keeps them so, along with their pixel data and the palette tables.
      - Details in [original-archive.md](original-archive.md#lores-bounds-15-sprites-were-wrong-fixed).
  - **Test tools, pulled forward from phase 5:**
    - `--fast` runs without the clock: one simulation step per frame, game time still 1/30 s per tick (`Eem::untimed`).
    - `--demo <name>` plays one recorded demo.
    - With both, every demo desynced early. Since the lores bounds fix, 7 of the 8 replay in sync to the end on Windows/MSVC, whose CRT `rand` is the MSVC LCG. `level4c` still desyncs, at check 0x1606. Other platforms need the phase 5 `rand` clone first. (Phase 5 brought all 8 into sync.)
  - **Not ported:** sound stays inactive until phase 6 (the DirectSound stub would fail init). Mouse mapping waits for phase 4 (done there).

### Phase 4: input; single player becomes playable
- **Keyboard:**
  - an `SDL_Scancode` → **PC set-1 scancode** table, with +128 for E0 keys;
  - the demos and the stored bindings use set-1 codes, so the code space must stay;
  - filter auto-repeat as before;
  - keep the hard-coded US shift table for chat text, so chat stays deterministic.
- **Mouse:** relative mode for mouse steering, scaled to logical coordinates.
- **Joystick:** `SDL_GameController`, falling back to `SDL_Joystick`. Feed the original 6 digital axes (±) and 32 buttons, with the dead zone as a percentage.
- **Exit:** a full mission can be played on keyboard, mouse and gamepad, and F11 cycles the control sets.
- **Outcome** (2026-10-04). Input runs on SDL. The `input_play` test starts the first mission from the title and flies it with each control set in turn, switching with F11. Still to do: play a full mission by hand on keyboard, mouse and a real game controller.
  - **Events:** `Comm::input_proc` gets every SDL event after the window procedure chain, and `Eem::sdl_event` hands it to `Kbd`, `Mouse` and `Joy`. The Windows hooks and the winmm joystick calls are gone from the drivers and from `compat/`.
  - **Keyboard:**
    - A table maps SDL scancodes to PC set-1 codes (E0 keys +128), so the stored bindings and the demos keep their codes. Pause is 45 and Num Lock E0 45, as Windows reported them.
    - Auto-repeats are dropped; `last_key` filtered them in the original.
    - The text in the player state comes from a fixed US table, which is what `MapVirtualKey` gave, plus the original shift table with its gaps (Shift+Space gives no character).
    - **WASD** (port addition, 2026-10-04): W, S, A and D work alongside the cursor keys, as a second key for whatever control a cursor key is bound to, in the keyboard and mouse control sets. `RegData::ParallelKey` picks it and `Vkey` checks it. Menus still take only the cursor keys, so names can be typed.
    - A virtual key matches its key with or without the E0 prefix (`Vkey::complete`). So the Left Ctrl binding (Fire2) also fires on Right Ctrl, and the cursor-key bindings answer to the keypad arrows, in the original as in the port.
  - **Mouse:**
    - Coordinates are window coordinates (were screen coordinates). The clip rectangle is the picture's area in the window, which `VD_sdl::present` reports when it changes, so the original scaling to 320×200 is unchanged. Headless, it is the framebuffer.
    - Moves are coalesced as Windows did. The position is pushed once per message pump and before each button.
    - There are no double clicks, because the original window class had no `CS_DBLCLKS`.
    - Mouse steering (`SysSet::action` calls `Mouse::set_relative`) puts SDL in relative mode while the window has focus. The pointer is hidden and stays on the picture, as in the full-screen original.
    - The original's default 640×480 mode mapped the mouse over 480 lines for the 400 drawn, so vertical motion ran at 5/6 speed. The port maps over the picture, as the original's 640×400 mode did.
  - **Joystick:**
    - The first SDL game controller, else a plain joystick, is opened at start or when plugged in. Unplugging it releases its buttons.
    - A controller reads as an Xbox pad did through winmm: X/Y the left stick, Z the triggers, R/U the right stick, buttons A, B, X, Y, LB, RB, Back, Start, LS, RS. Its D-pad also sets the X/Y bits.
    - The dead zone keeps winmm's meaning: a percentage of the full range.
    - The timer still polls it every tick. With `--fast`, which has no timer, `Eem::read` does.
  - **Original bug fixed:** the joystick event halves were sign-extended `short`s. With button 16 or 32 down, every event flag was set, and the event passed for a key (writing out of bounds) or for a tick.
  - **Test tools:**
    - `--input <file>` (`input_script.cpp`) pushes SDL key, mouse and virtual game controller events at given frames.
    - `input_dump=1` writes the local player's input state for every tick where it changes (`input_states.txt`).
    - `input_play` compares that dump with `tests/golden/input_states.txt`. The states don't depend on the simulation, so they are the same on every platform, and they are exactly what demos and network play exchange.
    - Headless runs ignore real joysticks and open only an input script's virtual controller.
  - **macOS:** F11 is Show Desktop by default. Press fn+F11, or turn the shortcut off, to cycle the control sets.

### Phase 5: determinism and the demo regression suite
- **Clone the old-toolchain behaviours in `compat/`:**
  1. **MSVC `rand()`.** `Rand::init` fills its table with `srand(0)` and 1024 × CRT `rand()` (`gobj.cpp:39-42`). glibc and Apple's libc produce different numbers, so embed the MSVC LCG. Confirmed in the 1.1 exe; `tests/golden/crt_rand.txt` has the table it must produce.
  2. **MSVC 4 `qsort`.** The collision results are sorted with every priority 0 (`1sp.h:267-268`, `1sp_scr2.cpp:107-121`), and so are the visible level objects (`1sp_lev.cpp:768`). The order of equal keys is whatever MSVC's algorithm produced. The 1.1 exe holds the classic pre-2005 CRT `qsort`; [original-archive.md](original-archive.md#msvc-4-qsort-confirmed) has it. Test the clone against `tests/golden/crt_qsort.txt`: 435 runs of the original, emulated.
  3. **x87 floats: no switch needed.** The CRT ran x87 at 53-bit precision.
     - `METRONQUALITY` is 1024, so the `(int)(seconds*METRONQUALITY)` tick conversions are exact under x87 and SSE alike. The earlier ×1000 analysis (0.7 s → 699 vs 700) was wrong.
     - What can differ is a float result the original used straight from the register: converted to `int` or fed into the next operation. Evaluate those in `double` (sites in [original-archive.md](original-archive.md#x87-the-tick-conversions-are-exact)).
  4. **Lores bounds: done early** (2026-10-04, see phase 3). This was the main demo desync on Windows; [original-archive.md](original-archive.md#lores-bounds-15-sprites-were-wrong-fixed) has before/after results.
- **Headless demo runner:**
  - `firefight --headless --play-demo <file> --strict-sync` exits non-zero on `Eem_demo_sync_failure`. The original catches that exception silently and just ends the demo;
  - it also checks that the replay reached the demo's last frame;
  - register it with CTest for all 8 demos.
- **Built-in sync signal:** each frame the game compares the sum over ships of x + y + angle + life, plus the `Rand` index (`game.cpp:909-937`).
- **Diagnostic traces:** `randdebug`, `shipdebug` and `netdebug` (`MAIN.TDF` levels) already dump traces for diagnosing a divergence.
- **If an original demo desyncs, suspects in order:**
  1. the compat behaviours above;
  2. collision scan order;
  3. phase bounds: the `sprite_build` test checks them against the shipped caches;
  4. render-path side effects: `World::display` calls `look_at` **with builders** (`world.cpp:74-81`). Until that is proven harmless, keep the original cadence of one render per batch of ticks, and run the same calls in headless mode;
  5. the uninitialised `ACannon::global_time` read (`alien.cpp:1726`, `==` instead of `=`): emulated, see the outcome below;
  6. finally, a real 1.0 → 1.1 gameplay change. The demos were recorded on 20 May 1996; this source is August 1996. Every gameplay `.tdf` predates the demos, so only code changed; the archive's file dates list the candidate files ([original-archive.md](original-archive.md#the-demos-and-10--11)).
- **Golden demos:** record a further set with the port across several missions and skill levels. CI replays them on all three OSes; this is the cross-platform determinism gate.
- **1.1 demos:** the archive holds the original 1.1 executables, built from this source. The Release exe runs on Windows 11 once patched ([original-archive.md](original-archive.md#running-11)), so demos it records would be an exact 1.1 oracle without the 1.0 question. None has been recorded yet.
- **Exit:** the original demos replay in sync, or any divergence is explained and documented, and the golden demos pass on Windows, Linux and macOS.
- **Outcome** (2026-10-04). Exit met: **all 8 original demos replay in sync to the end of their recordings** on Windows/MSVC x64, Linux GCC and Clang x86-64 and macOS arm64, in Debug and Release. The `demo_<name>` tests replay each one. Four golden demos recorded with the port on macOS cover other worlds and both skills, and the `golden_<name>` tests replay them in sync on all four CI configurations too.
  - **Clones:** `compat/msvc4.*` has MSVC 4's `rand`/`srand` and `qsort`. `Rand::init`, the collision results, the visible level objects and the text labels use them on every platform; the CRT's are no longer called. The `crt_vectors` test checks both against the vectors from the 1.1 exe.
  - **The stale cannon timer was the last cause.**
    - `ACannon`'s constructor has `global_time==Mp::BEYONDTIME`, so the original read whatever its FastAlloc slot held at that offset: 232 in the 32-bit MSVC layout.
    - Slots are reused last freed first. A cannon built away from the screen counted that value down and was removed when it went negative. It was then rebuilt in the same slot the next tick, inheriting the negative value, so whether it stayed or churned depended on the slot's history.
    - The demos need the exact value. With 0, `level2C` desyncs; with `BEYONDTIME`, `level4C` does. They are what Windows and the Mac got from their own (different) layouts.
    - **Emulated:** `Stale_memory` (`game/stale.*`) keeps the word for each slot as the original's memory held it. Every deletion of a game object first writes the bytes its own 32-bit layout has at offsets 232–235. A slot never used holds 0, and pointers stand in as a non-null address (0x00400000) or 0.
    - `tools/layout/stale_memory.py` generates the per-class table (`stale_gen.cpp`) from clang's `i386-pc-windows-msvc` record layouts, which follow MSVC's rules. 25 of the 69 game object classes reach offset 232; the most common previous occupant is an `Unblock`, whose `Posit::y` sits there.
    - A wrong layout would have shown: with this table, both `level2C` and `level4C` replay in sync, as do the other six. The 1.1 exe confirms offset 232 for `ACannon` in its constructor and `run` ([original-archive.md](original-archive.md#acannonglobal_time-at-offset-232-confirmed)).
    - `FastAlloc::operator delete` warns if a game object was deleted without `Stale_memory::object_deleted`; the tests fail on that warning.
  - **x87 floats:** the sites in [original-archive.md](original-archive.md#x87-the-tick-conversions-are-exact), checked one by one.
    - `1sp_lev.cpp`'s region sizes (`(int)(max_sx*mulx)`) can't differ: `max_sx` and `max_sy` are 8192 in every level, a power of two, so the float product is exact.
    - The parallax of the backgrounds, fog and clouds (`neutral.cpp`, `(int)(mx*world->get_x())`) and the title's logo flight (`world.cpp`, `Header`) used a float result straight from the x87 register. They are now evaluated in `double`, which reproduces the 53-bit result exactly: a product of two floats is exact in a double, and the original then rounded or truncated once. Both only move pictures, and the title frames didn't change.
    - `tools.cpp` already computes in `double`; `params.cpp`'s `MB2SHOOTCOUNT` is exact with the shipped data.
  - **Golden demos** (`tests/demos`): gray1 and gray2 on normal skill, net3 and green3 on hard, about 1,900 ticks of play each. The original demos cover only green6, brown2, green1 and white1.
    - `tests/demos/flight.py` writes an input script that selects the mission, opens the mission screen's demo menu (debug mode) and records it, then flies with random but fixed keys: forward, turns, both fire buttons, strafing, turbo, weapon and inventory keys.
    - `tests/record_demo.cmake` records one: it sets the skill in `settings.ini`, runs the script headless with `cwdiags=extended` and copies the recording. Recorded on macOS; replayed in sync in Debug and Release.
  - **Demo runner:** `--demo <name>` logs whether the replay stayed in sync to the end of the recording and exits with 3 when it diverged, 4 when it stopped early. `--demo !<path>` plays a demo file. It replaces the planned `--play-demo --strict-sync`. `main.cpp` passes the argument on as given, because the engine upper-cases its switches.

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
  - Found so far: with sound off, the original already counts a sample's length by the wall clock (`GetTickCount`), so the messages advance in real time ([original-archive.md](original-archive.md#reference-screenshots-phase-3)).
  - The callers seen so far only change which message shows, volumes and one sound effect. `MManager::run` draws `RAND` twice every step whatever the messages do.
- **Exit:** audio matches the original by ear: positioning, priorities, speech ducking and music per mission.
- **Outcome** (2026-10-04). Exit met: `1ss` runs on SDL2_mixer, and by ear on Windows the sound plays correctly.
  - **Samples:** the voice model is the original's, line for line: `total_channels` channels (8 by default), one voice per `Sample`, a replay restarting it unless the new volume is lower, and stealing the first channel with the same or a lower priority when all are busy.
    - SDL_mixer channels take the place of the DirectSound buffers.
    - The `logvol` volume and the pan are DirectSound attenuations in hundredths of a dB. They become linear gains for `Mix_Volume` and `Mix_SetPanning`; a positive pan attenuates the left channel.
    - Samples are converted to the device's format when loaded (an in-memory WAV image for `Mix_LoadWAV_RW`). The device opens at its own rate; the mode's primary buffer format no longer matters.
    - `sos_safe=1` (WaveOut) keeps its single voice and rules on one SDL_mixer channel. `sos_none=1` keeps the original's silent emulation, which headless runs use.
    - Without an audio device the game runs silent with a warning (the original failed).
  - **Music:** `CD::play(N)` streams `music/track{N+1:02}.flac`, looped when the original asked MCI for the notification that restarted the track. `CD::tracks` counts the files (8). `--music <dir>` overrides the default, `music/` next to the data directory. With `sos_none` nothing plays; the original still played the CD.
  - **Focus:** losing it pauses the samples and the music, and getting it back resumes them. The original stopped the CD and restarted the track from its beginning.
  - **Volumes:** the system-wide `Mixer` is gone. Its replacement keeps the game's sound volume (`Mix_MasterVolume`) and music volume (`Mix_VolumeMusic`), 0–`VOLUME_MAX` as before, in `settings.ini` (`sos/sound volume`, `sos/music volume`); the game had read them back from the system mixer, so it never stored them.
    - Both default to half (−6 dB). The original's sound card mixed the CD in analogue; here music and samples share one digital mix.
    - Measured on demo `level1` through the `disk` driver: at full volume 0.47% of the output samples clip (the effects alone 0.16%, as they saturated in DirectSound too); at half, none.
  - **Removed:** the MIDI `Song` path (shareware only) is a stub. The DirectSound, MCI, mixer and wave-out stand-ins are gone from `compat/win32.h`.
  - **`Sample::playing()`:** with sound on, a message lasts as long as its sample really plays, as in the original with a sound card. Demos stay in sync with sound on (`sound_play`).
  - **Tests:** headless runs keep `sos_none=1`; `--sound` keeps the sound in a headless run. `sound_play` replays `level1` with the sound on through SDL's `disk` audio driver. It checks the start-up, the mission's track, that the output isn't silent, and the sync.

### Phase 7: network play over LAN and the internet (`1ee_netw`)
- **ENet replaces DirectPlay.**
  - Keep the engine's message layer and lockstep model: packed `Player_state`/`User_block`, with chat inside the states.
  - ENet takes over transport, sessions and the roster.
  - The protocol becomes port-only (bump `NET_VERSION`). Compatibility with the original game isn't a goal.
- **Topology: host relay.** The original is a full mesh of up to 4 players. In the port, clients connect only to the host, which forwards each player's per-frame state to the others. The simulation stays lockstep and peer-symmetric; the host is authoritative only for the session and the roster.
- **Reachability, in order of implementation:**
  1. **LAN:** join by address, or by LAN discovery broadcast.
  2. **Internet, direct:** the host opens a UDP port, either automatically through UPnP/NAT-PMP (`miniupnpc`) or by manual port forwarding. IPv4 and IPv6.
  3. **Internet, relay server:** a small standalone relay in `tools/relay/` (same CMake project, headless, runs on any cheap VPS).
     - Host and clients connect outbound only, so no port forwarding is needed.
     - Players join with a short session code.
     - This also covers hosts behind carrier-grade NAT, where neither direct option works.
- **Latency:**
  - The original lockstep tolerates at most 4 frames (~132 ms) of delay, which suits a LAN but not internet round trips plus the extra relay hop.
  - **Make the input delay a session parameter.** The host chooses it from round-trip times measured at join, and every peer uses the same value.
  - The delay only shifts when inputs take effect; it doesn't change the simulation rules. So it is compatible with determinism, and games recorded online still replay.
- **Hardening:** the original code trusts every packet, which was fine on a 1996 LAN. On the internet every received message is untrusted:
  - validate length, type and player id before parsing;
  - cap message rates;
  - use a version handshake;
  - offer an optional session password;
  - fuzz the message parser in CI.
- **Disconnects:** keep the original rules (a leaving player's state is zeroed, and the game ends if the host leaves). Add timeout detection so a vanished peer can't stall lockstep forever.
- **Setup:** command-line `--host` / `--join <address|code>` first. This replaces the launcher's DirectPlay wizard and the registry `[eem]` values. The phase 8 menus then replace the command line.
- **Exit:** a 4-player game across Windows, Linux and macOS runs 30 minutes without a sync failure, both on a LAN and over the internet through the relay with ~100 ms round trips. This requires phase 5.
- **Progress** (2026-10-04): LAN play by address works; the rest of this phase is to do.
  - **Transport** (`1ee_enet.*`): ENet with the host relaying. Clients connect to the host only, and the host forwards each message to the player it is addressed to. Player ids are 1 for the host and 2–4 in joining order. Everything goes on one reliable, ordered channel, so `Net`'s resend layer (sequence numbers, CRC-16, requests, pings, purges) is gone. `1ee_enet.cpp` is kept apart from the engine headers because `<enet/enet.h>` brings in the real Windows socket headers.
    - Messages: HELLO (version, name), WELCOME (id), REFUSE (version, full, started), START (players, game info, roster), DATA (from, to, the engine's message), LEFT. Each is checked before use (length, type, a client's DATA must carry its own id), and `NET_VERSION` is now 4.
  - **Setup:** `--host <players>` hosts a game for 2–4 players and waits for them; `--join <address>` joins one; `--port <port>` overrides the default 19960. They become the engine switches `net_host`, `net_join` and `net_port`, read by `Eem::init_multi` in place of the launcher's registry values. The player name is still `[eem] player name`, else the user name.
    - The host sends its `[eem] game info` (the deathmatch rules) with the roster, or says it has none. A client stores it, or removes its own, so every player decodes the same rules (`RegData`'s defaults when there are none).
  - **Fixes the port needed:**
    - **Series race.** A peer that finishes an `Eem::start`/`stop` series first sends the next series' messages early. The original dropped them, and user blocks carried no series at all, so the setup exchange failed. Its 300 ms purge in every `Eem::stop` used to hide this. Now `User_block` has a `serie` byte, and a message of a later series waits in a per-player queue until this peer starts that series (`Eem::apply_message`, `apply_pending`).
    - **Headless counts as active.** `Eem::read` returns to its caller while a network frame is pending only when the window is focused (`eem_InNetActive`); a headless run never gets focus, so the lobby never drew.
    - **No RLE8 packet compression** (`cwe.ini` asks for it): its compressor reads past its source and its decompressor writes without bounds. Incoming states and user blocks are size-checked.
  - **Disconnects:** a client that leaves is announced to the others (LEFT), whose `Eem` zeroes its input as before. If the host leaves, each client goes on alone, the original's rule. ENet notices a vanished peer within 10 s.
  - **Tests:** `net_play` runs a host and a client on one machine, headless and `--fast`, each with an input script that accepts the level in the lobby and flies in the deathmatch. Both must reach the mission, pass the per-frame sync check and end normally. A forced desync (`syncfail` with `cwdiags=extended`) is detected on both sides. A three-player run by hand (two relayed clients) also stayed in sync.
  - **Still to do:** LAN discovery; internet play (UPnP/NAT-PMP, IPv6) and the relay server; the session-wide input delay; the rest of the hardening (rate caps, password, fuzzing the parser); a 4-player game across the three systems; the menus of phase 8.

### Phase 8: replace the launcher; packaging
- **In-game options**, extending the original `Menu` system, cover:
  - video (scaling, fullscreen, square pixels or 4:3);
  - audio volumes;
  - **key binding editing** (the original only edited bindings in the launcher; `menu.cpp:1049-1097` only shows them);
  - pilot management;
  - **network host/join** (address, LAN list, relay session code, password).
- **Packaging:** CPack Windows zip/installer, a macOS `.app` (signing and notarisation only if it is distributed), a Linux tarball/AppImage. CI publishes the artifacts.

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Original demos desync even with the compat clones (possible 1.0 → 1.1 change) | Lose the strongest exactness oracle | Golden demos from the port. Debug traces. Reference screenshots from the retail game. Demos recorded with the original 1.1 exe from the archive |
| Simulation side effects in the render path (builders in `look_at`) | Headless runs or a different frame pacing change outcomes | Keep the original render cadence; investigate in phase 5 |
| `FastAlloc` slot overflow on other ABIs (992/1024 bytes on MSVC x64; GCC/Clang unknown) | Memory corruption | `static_assert` all object sizes; enlarge the slot if needed. Doesn't affect determinism |
| Blitter reimplementation not pixel-exact | Visual differences; collision order changes | Unit-test each mode against the asm semantics. Screenshot diffs |
| Uninitialised `ACannon::global_time` | Behaviour depended on stale memory | Characterised: the original demos need it. Emulated with the original's 32-bit layout (`Stale_memory`, phase 5) |
| Internet play through NAT | Players can't connect | UPnP/NAT-PMP mapping, manual forwarding, and the relay server for everything else |
| Internet latency and jitter | Lockstep stalls | Session-wide input delay chosen from measured round trips. The frame cap already slows the game rather than desyncing it |
| Untrusted packets from the internet | Crashes or exploits through the 1996 parser | Validate before parsing, rate limits, parser fuzzing in CI |
| Relay server hosting | Running cost; availability | A tiny stateless forwarder on a cheap VPS. LAN and direct connections still work without it |
| Private-repo CI minutes (macOS multiplier) | Cost | macOS on `main`/nightly; daily testing on the local Mac; Linux for most checks |

## Decisions

Recorded 2026-10-03.

| # | Question | Decision | Effect on the plan |
|---|---|---|---|
| 1 | Fidelity bar | **Aim for bit-exact** | The phase 5 compat clones are required. A divergence is accepted only if it is proven to be a 1.0 → 1.1 change |
| 2 | Multiplayer scope | **Internet as well as LAN** | Phase 7: relay server, UPnP/NAT-PMP, session-wide input delay, packet hardening |
| 3 | Default presentation | **Square pixels** | 640×400 letterboxed with integer scaling. 4:3 stretch remains an option |
| 4 | macOS | **Apple Silicon only; a local Mac is available** | arm64-only builds, macOS 11+. Day-to-day testing on the Mac, CI on `main`/nightly |
| 5 | Launcher replacement | **In-game options** | Phase 8 menus: video, audio, key bindings, pilots, network host/join |
