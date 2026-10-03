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
    - splinter sprites are detected by their `hires` entry, since the port data has no `lores` lines;
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
| Original demos desync even with the compat clones (possible 1.0 → 1.1 change) | Lose the strongest exactness oracle | Golden demos from the port. Debug traces. Reference screenshots from the retail game |
| Simulation side effects in the render path (builders in `look_at`) | Headless runs or a different frame pacing change outcomes | Keep the original render cadence; investigate in phase 5 |
| `FastAlloc` slot overflow on other ABIs (992/1024 bytes on MSVC x64; GCC/Clang unknown) | Memory corruption | `static_assert` all object sizes; enlarge the slot if needed. Doesn't affect determinism |
| Blitter reimplementation not pixel-exact | Visual differences; collision order changes | Unit-test each mode against the asm semantics. Screenshot diffs |
| Uninitialised `ACannon::global_time` | Behaviour depended on stale memory | Bit-exact goal: characterise first. Emulate it if the original demos need it; otherwise fix it and document the change |
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
