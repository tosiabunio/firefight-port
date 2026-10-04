# The original archive

This repository was cut from the original Fire Fight archive: the developers' tree with source, tools, data, built executables and the retail 1.2 CD image. The archive is not in this repository and is not a git repo. This document records what it holds that matters to the port, and what was checked against it on 2026-10-04.

Paths below are relative to the archive root, the directory holding `FF/`, `LIB/` and `BIN/`. The archive's own `CLAUDE.md` describes its layout and how the original was built. Its claim that the port lives in its `Port/` folder is out of date: `Port/` is an old clone, and the port is this repository.

## Working without the archive

The archive is private and is never committed. It lives on the Windows development PC only, at `D:\_Projects\FireFight`. **Everything that reads it runs there, and its results are committed**, so other machines and CI need only this repository:

| File | What it holds | Made by | Checked by |
|---|---|---|---|
| `tests/golden/data_files.sha256` | SHA-256 of every file in `data/` and `music/`. Each one was found byte-identical to its archive original (`FF/WORK.RTL`, `CDAudio`) | `ffarchive.py provenance --write` | test `data_files` |
| `tests/golden/sprite_bounds.txt` | Phase bounds of every sprite the game loads, equal to the shipped caches | a `sprite_dump=1` run, checked with `ffarchive.py sprites` | test `sprite_build` |
| `tests/golden/sprite_data.txt` | Size and CRC-32 of every sprite's hires and collision pixel data, and of every palette's three tables, equal to the shipped caches | same | test `sprite_build` |
| `tests/golden/crt_qsort.txt` | 435 runs of the original `qsort`, by element width, keys and resulting order | `crt_vectors.py`, which runs the 1.1 exe's code in an emulator | test `crt_vectors` (`compat/msvc4.h`) |
| `tests/golden/crt_rand.txt` | The original `rand` after `srand(0)`: the 1024 values of the `Rand` table | same | test `crt_vectors` |

On a machine without the archive, don't guess what it holds. Use this document and the files above. Anything that needs the archive itself is a task for the Windows PC. That covers a new comparison, disassembly, and running the original game.

**On the Windows PC:**
- Add the archive to a Claude Code session with `/add-dir D:/_Projects/FireFight`.
- `ffarchive.py` finds the archive through `--archive`, then `$FF_ARCHIVE`, then the repository's parent if it holds `FF/WORK.RTL`, then `../FireFight`.
- Disassembly and `crt_vectors.py` need Capstone, pefile and Unicorn. Install them into a throwaway virtual environment: `python -m venv <dir> && <dir>/Scripts/pip install capstone pefile unicorn`.
- Don't name a script `dis.py`: it shadows the standard module Capstone imports.

## Where things are

| Archive path | What it is | Use for the port |
|---|---|---|
| `FF/C/GAME/{RELEASE,FIREFGHT,DEBUG,SHAREWAR,SHWDEBUG}/FIREFGHT.EXE` | The game, built on 31 Aug 1996 from this source (see [Executables](#executables)) | Disassembly settles toolchain behaviour; running it gives reference output |
| `FF/WORK.RTL/` | Retail working tree: 1.1 `FIREFGHT.EXE` and `LOADER.EXE`, the `*.DIR` manifests with all their `lores`/`lsource` lines, the FLC masters, **the built sprite caches** (`.sph`, `.spl`, `.spc`, `.spp`) and the demos | Oracle for the in-memory sprite build; a complete loose-file install of 1.1 |
| `Fire Fight 1.2 [...] BIN+CUE/` | Retail CD 1.2: data track plus CD audio | Proof of what shipped ([Versions](#versions-and-provenance)) |
| `FF/C/CWENGINE/<module>/SOURCE/`, `FF/C/GAME/SOURCE/` | The source this repository was copied from (`d73171a`). File dates are original and date the changes since the demos | |
| `LIB/` | Published engine headers and `.lib` files (release and debug, 31 Aug 1996); `LIB/SOURCE` holds copies of the engine sources as published | `LIB/SOURCE/1SP_VIDE.CPP` is the version the exe was built from |
| `FF/C/LOADER/`, `FF/C/FFSTART/` | Launcher and CD autorun (MFC), with their built exes | Phase 8 reference: options, network wizard |
| `FF/WORK.RTL/!MISC/` | Design documents. `MEMO/*.TXT` (Polish) describe the `.tdf` labels and object types | Data reference |
| `FF/C/LED/`, `BIN/` | Level editor; the developers' DOS/Win95 tools (`AVM.EXE` builds volumes) | Not needed |
| `FF/WORK.SHW/`, `FF/INSTALL/` | Shareware edition and installers | Not needed |

## Versions and provenance

All checked file by file:

- **The data never changed after May 1996.** The CD's data volumes are dated 20–23 May 1996; only `PARAMS.VOL` is from 1997. Unpacked, every file in all 64 volumes is byte-identical to this repository's `data/` (572 files) or to the sprite caches in `FF/WORK.RTL` (1,314 files).
- **1.2 changed only the executables.** The 1.2 `PARAMS.VOL` (31 Jul 1997) is a repack: its 76 files equal `data/`. The readable strings of the 1.2 `FIREFGHT.EXE` differ from 1.1 only in the memory manager: the guarded heap's messages are gone and "windows heap usage" appears. So 1.2 apparently switched `1mm` to the Windows heap (the `MMU_USE_WINDOWS_HEAP` path). Gameplay strings are identical.
- **1.1 patch** (`FF11UP.TXT`, `FF/BUILDALL.BAT`): "works properly with DirectX 2 and has several other minor bug fixes". It shipped `FIREFGHT.EXE`, `LOADER.EXE`, `PARAMS.VOL` and documents.
- **Source vs. the 1.1 exe:** every source file predates the 31 Aug build except `CWENGINE/1SP/SOURCE/1SP_VIDE.CPP` (10 Sep 1996). That is the version in this repository. It drops one `Comm::process_messages()` from the GDI window setup, which the port replaced. `LIB/SOURCE/1SP_VIDE.CPP` is the built version. The rest of `LIB/SOURCE` is identical to the engine sources, so no older copy of any source exists.

## Executables

| Path | Configuration | Defines |
|---|---|---|
| `FF/C/GAME/FIREFGHT/FIREFGHT.EXE` | Production: the 1.1 retail exe (also in `FF/WORK.RTL`) | none; CD check active |
| `FF/C/GAME/RELEASE/FIREFGHT.EXE` | Release | `UNPROTECT` |
| `FF/C/GAME/DEBUG/FIREFGHT.EXE` | Debug | `_DEBUG`, `UNPROTECT` |
| `FF/C/GAME/SHAREWAR`, `SHWDEBUG` | Shareware (`SHWDEBUG` is from 14 Aug) | `SHAREWARE`, `UNPROTECT` |

- **Compiler:** MSVC 4.x (linker 3.10), `/G5 /ML /O2 /GX`. `/ML` links the single-threaded CRT statically, so the CRT code the game ran is in the exe. There are no `.pdb` or `.map` files.
- **Addresses:** those below are in `RELEASE/FIREFGHT.EXE` (image base `0x400000`).
- **Running 1.1:** it runs on Windows 11 from a patched copy of the Release exe; see [Running 1.1](#running-11).

## Running 1.1

The 1.1 game runs on Windows 11, windowed and without sound (first run 2026-10-04). Set it up outside the archive:

1. **Copy** `FF/WORK.RTL`, keeping file dates (`robocopy <WORK.RTL> <dir> /E /COPY:DAT /DCOPY:T`). It is a complete loose-file installation: exe, loader, manifests, caches and FLC masters.
2. **Exe:** copy `FF/C/GAME/RELEASE/FIREFGHT.EXE` over `<dir>/FIREFGHT.EXE`. The `WORK.RTL` exe is the Production build and checks for the CD; the Release exe doesn't.
3. **Patch** the copy: `python tools/archive/patch_hooks.py <dir>/FIREFGHT.EXE` (the hook bug below).
4. **Registry:** under `HKCU\Software\chaos works\Fire Fight\Retail\spr`, set the DWORDs `hires mode` = 1 (640×480; 2 is 640×400), `lores mode` = 1 and `load data mode` = 3. The launcher writes these.
   - Without the first two, `Cwe::init` falls back to a `cwe.ini` label that the retail `cwe.ini` lacks, and stops: `'cwe.ini:/spr/use_640x400' no such label`.
   - Every other setting has a default in `RegData`, so `LOADER.EXE` isn't needed.
5. **Run** `FIREFGHT.EXE debug=1 sos_none=1` in `<dir>`. `debug=1` gives a 640×400 window drawn with GDI (`spr_safe=1` would cover the whole screen); `sos_none=1` turns sound off.

What to expect:
- **The hook bug.** `Eem::init` (`1ee_main.cpp`) calls `Kbd::init` and `Mouse::init` before it sets `Eem::thread`. Both `SetWindowsHookEx` calls get thread 0 and no module, which asks for a system-wide hook: Windows 95 accepted that, NT refuses it. Unpatched, the game stops with `unable to hook keyboard [Kbd::init]`.
  - `patch_hooks.py` turns `mov eax,[Eem::thread]` (`0x47d9a1` in `Kbd::init`, `0x47fe15` in `Mouse::init`) into a call to a 12-byte stub in int3 padding at `0x403872`. The stub sets `Eem::thread` to `GetCurrentThreadId()` and returns it. `Eem::init` stores the same value a few lines later.
  - Windows' Windows 95 compatibility layer doesn't help: the game then crashes in `winmmbase.dll` during input init.
  - The CD's `Instructions.txt` (Windows Vista–8: Windows 98 compatibility, 640×480, "Do not use Direct Draw") is for the 1.2 exe. Whether 1.2 fixed the hook bug is unchecked.
- **Log:** `%TEMP%\FIREFGHT.LOG` (`Cwe::init` puts it in `TEMP`), not the game folder.
- **Cache rebuilds:** the first start rebuilds every cache whose master's date is an hour off from the date recorded in it, a daylight-saving shift (34 caches by mission 1). The rebuilt caches equal the shipped ones apart from that date, so the 1.1 sprite builder reproduces what shipped.
- **Screenshots:** Ctrl+F12 writes `%TEMP%\scrn_NNN.bmp`, the 640×400 frame as an 8-bit BMP, without view plane 9, the info layer (HUD, messages, the ring around the ship). Alt+F12 keeps it. The blinking "DEMO MODE" label is drawn after the capture (`Game::display_service`), so no screenshot has it.
  - The BMP's palette is the 6-bit VGA palette expanded with the low bits filled, and the port's frame dumps use `v<<2`. Compare in 6-bit values (`>>2`).
  - The title doesn't capture. There any key ends the title once its logo is complete (about 3 s after it appears) and opens the mission screen; otherwise the title times out after 20 s into an attract demo.
  - Keys pressed during a mission reach the game. Right Ctrl is Fire2 too: a virtual key matches its key with or without the E0 prefix (`Vkey::complete`), so the Left Ctrl binding fires on Right Ctrl, in the original as in the port. That explains the shots during the Right Ctrl+F12 screenshots. For input-free frames, grab the window instead: its client area is the frame at 1:1.
- **Esc** in a mission ends it and returns to the mission screen.
- **Chosen by wall clock:** the mission screen's background (one of the 4 frames of `title_h2.flc`, `timeGetTime()%4` in `Mysprites::init`) and the attract demo (`Game::play_demo`).
- **DirectPlay** isn't installed on Windows 11 (`dplay failed` in the log). Single player doesn't need it.

## Reference screenshots (phase 3)

Checked on 2026-10-04: the original's screenshots and window grabs against the port's per-frame dumps (`--headless --fast --shot-every 1`, with `--input` for the same keys), in 6-bit colour values. `--fast` renders every simulation step, so an animated screen matches the port frame of the same step.

| Screen | From the original | Result |
|---|---|---|
| Title | 16 window grabs over 8 s | Each equals one port frame (frames 2–185; from 185 the title holds still), apart from the window's rounded bottom corners (Windows 11) |
| Mission screen | Ctrl+F12 | Identical, once the port shows the same of the 4 random backgrounds |
| Mission 1 (`green6`), no input | 16 window grabs over the first 8 s | 15 equal a port frame (frames 414–463), apart from the corners. The 16th shows the reply "AFFIRMATIVE", which the `--fast` run hadn't reached (see below) |
| Attract demo `level4c` (`white1`, hard) | 3 × Ctrl+F12 | Port frames 48, 139 and 266: identical apart from the info overlays the capture leaves out. In the first, 33 snowflakes (2×2 dots) sit about 16 px from the port's |

- **Speech timing follows the wall clock.** With sound off (`SOS_NONE`, which the port also runs until phase 6), `Sounds::playing` counts a sample as playing until its length has passed by `GetTickCount`, and `msg.cpp` moves to the next message when it ends.
  - The original ran in real time: "LOCATE AND DESTROY …" (`g6_1_1.wav`, 3.5 s) showed until about 4.5 s, then "AFFIRMATIVE" (`g6_1_2.wav`, 0.7 s) at about 5 s.
  - `--fast` runs 250 steps in a second or two, so the port's first message was still up at frame 700.
  - The random sequence doesn't depend on it: `MManager::run` draws `RAND` twice every step either way.
- **Not explained yet:** the snowflakes in the first demo screenshot. The snow is drawn by `Background`, a simulation object, and the two later screenshots have no flake differences.
- **Not compared:** menus other than the mission screen.

## Findings for phase 5 (determinism)

### MSVC `rand`: confirmed
`0x488d50`: `holdrand = holdrand*214013 + 2531011; return (holdrand>>16) & 0x7fff`. The multiply is built from `lea`/`shl`, so a search for the usual `imul` constant misses it. `tests/golden/crt_rand.txt` holds the 1024 values after `srand(0)`, from the emulated exe.

### MSVC 4 `qsort`: confirmed
`0x48a340` (helpers `shortsort` at `0x48a4a0` and `swap` at `0x48a500`) is the classic pre-2005 CRT `qsort`. The clone must reproduce exactly this:

```c
/* lo = base, hi = last element; explicit stack of 30 (lo, hi) pairs */
recurse:
  size = (hi - lo) / width + 1;
  if (size <= 8)                                  /* CUTOFF */
    shortsort(lo, hi, width, comp);               /* repeatedly swap the max (first wins) to hi */
  else {
    swap(lo + (size / 2) * width, lo, width);     /* middle element becomes the pivot at lo */
    loguy = lo; higuy = hi + width;
    for (;;) {
      do loguy += width; while (loguy <= hi && comp(loguy, lo) <= 0);
      do higuy -= width; while (higuy > lo && comp(higuy, lo) >= 0);
      if (higuy < loguy) break;
      swap(loguy, higuy, width);
    }
    swap(lo, higuy, width);
    if (higuy - 1 - lo >= hi - loguy) {           /* byte differences, signed compare */
      if (lo + width < higuy) push(lo, higuy - width);
      if (loguy < hi) { lo = loguy; goto recurse; }
    } else {
      if (loguy < hi) push(loguy, hi);
      if (lo + width < higuy) { hi = higuy - width; goto recurse; }
    }
  }
  if (pop(&lo, &hi)) goto recurse;
```

`shortsort` keeps the *first* maximum (`if (comp(p, max) > 0) max = p`).

**Test vectors:** `tests/golden/crt_qsort.txt` holds 435 runs of the emulated original, for element widths 1, 2 and 4, with many ties.
- This transcription gives the same order in every run (`crt_vectors.py` checks it).
- A stable sort, such as glibc's merge sort, differs in 336 of the 420 non-empty runs. For example, with 20 equal keys the original swaps elements 0 and 10: its pivot swap.

Call sites:
- `1sp_scr2.cpp:115`: collision results; width 1, by priority, with ties;
- `1sp_lev.cpp:770`: the visible level objects, after `look_at` has run builders; width 2, distinct values;
- `1io_txt.cpp:545`: text groups (not simulation).

### x87: the tick conversions are exact
- **Precision:** the CRT startup calls `_controlfp(_PC_53, _MCW_PC)` (`0x48b9a0`), so the game ran at 53-bit precision. Float expressions stay in x87 registers; `(int)` conversions call `__ftol` (`0x488618`) straight from the register, with no store to `float` first.
- **`METRONQUALITY` is 1024**, not 1000. The exe multiplies by the constant `1024.0f` (`0x457913`). A power-of-two product is exact in every precision, so every `_get_time`/`*Mp::METRONQUALITY` conversion gives the same tick count under x87 and SSE. `_get_turnspeed`/`_get_brkpwr` (`*0x10000`) are exact too. No switch is needed.
- **What can still differ:** a float operation whose result the original consumed from the register, either converted to `int` or fed into the next operation. The port would round it to `float` first.
  - A single operation whose result is stored to a `float` rounds identically, because 53 ≥ 2·24+2.
  - Example: `MB2SHOOTCOUNT = (int)((float)active / fp_value("SHOOTFRQ"))` (`params.cpp:1035`); with the shipped `SHOOTFRQ = 1` it is exact.
  - Other float code to review site by site against the exe: `world.cpp`, `tools.cpp`, `neutral.cpp`, `1sp_lev.cpp:302`. Evaluate in `double` wherever the original kept the value in a register.

### Lores bounds: 15 sprites were wrong; fixed
A sprite's phase bounds are the union of its hires (/2), lores (×1) and collision (×2) bounds (`Sprite::load`). They are simulation state in two places:
- **Level objects:** the bounds decide the level regions an object occupies and whether `Level::look_at` calls its builder (`1sp_lev.cpp:579-604`, `745-753`).
- **Game objects:** the on-screen tests use them (`gobj.cpp:891`).

The original counted lores, because the launcher always wrote `spr/load mode` 3 (hires, lores and collision) when the hires volumes were present. It wrote 2 (lores and collision) only for "low resolution only", and never 1.

- **Before the fix:** the port had dropped the manifests' `lores`/`lsource` lines and measured the `source` FLC at the default 2×1. That was exact for 481 sprites, but the removed lines carried two exceptions:
  - 27 `lores =` lines had a `1x1` override (5 also `o+`);
  - 23 sprites had their own lores master, `lsource`. That is 22 FLCs, because `fog` and `cloud` share one. `radar` has an `lsource` but no targets and is never loaded.

  The shipped bounds then differed in every phase for these 15 sprites:

| Sprite | Manifests | Shipped vs. port before the fix, phase 0 (l, r, u, d) | Cause |
|---|---|---|---|
| `bronie` ("ship upgrades"): pickup capsules (`CapsuleBuild`), placed 81–254 times per world | `brown gray green net white` | (−9, 88, −16, 13) vs (−4, 44, −8, 6) | `1x1` on the same `source`: twice the size |
| `ship` | `global` | (−19, 20, −11, 19) vs (−19, 20, −11, 18) | `lsource` `bohaterl.flc` |
| `shipborder` | `global` | (−20, 21, −10, 20) vs (−19, 21, −10, 19) | `lsource` |
| `fog`, `cloud` | `global` | (0, 689, 0, 494) vs (0, 687, 0, 493) | `lsource` |
| `indicator`, `indicatore` | `global` | (−25, 26, −24, 47) vs (−12, 13, −12, 23) | `lsource` |
| `radar_point` | `global` | (−108, 110, −77, 77) vs (−54, 55, −38, 39) | `lsource` |
| `radar_target` | `global` | (−120, 121, −86, 85) vs (−60, 60, −43, 42) | `lsource` |
| `destsmoke` | `global` | (−10, 11, −10, 11) vs (−10, 10, −10, 10) | `lsource` |
| `press`, `copy` (title) | `header` | 1-pixel differences | `lsource` |

- **The fix (2026-10-04):**
  - The manifests are the original files again, byte for byte, and `data/flics/!global/` has the 22 lores masters (0.9 MB).
  - `Sprite::load` never builds lores pixels. It measures every lores target with `Lsprite::measure`, whose constructor already picks `lsource` over `source` and reads the scale flag from the target line.
  - The splinter lookup is back to the original `File::specified("lores")`.
- **Verified:** `check` mode with `sprite_dump=1` loads every mission's level. All 382 distinct sprites match the shipped caches in every phase (`ffarchive.py sprites`), including their hires and collision bounds. The `sprite_build` test keeps this against `tests/golden/sprite_bounds.txt`.
- **Visible effect:** the title centres the copyright line with its bound `r` (`world.cpp:2561`). It now sits one lores pixel further left, as in the original, so the title frame hashes were regenerated.
- **Demo effect,** on Windows/MSVC, whose CRT `rand` is the MSVC LCG; `--headless --fast --demo <name>`:

| Demo | Before the fix | After |
|---|---|---|
| `level1`, `level2`, `level3`, `level4`, `level1c`, `level2c`, `level3c` | desync at checks 0x364–0x2791 | **in sync to the end of the recording** (1,261–4,281 frames) |
| `level4c` | desync at 0x1165 | desync at 0x1606 |

  Phase 5 brought `level4c` into sync too: its desync came from `ACannon`'s uninitialised timer, which the port now reads as the original's memory held it (`Stale_memory`, see [porting-plan.md](porting-plan.md#phase-5-determinism-and-the-demo-regression-suite)). With the MSVC 4 `rand` and `qsort` clones, all 8 demos replay in sync on every platform.

### `ACannon::global_time` at offset 232: confirmed
`Stale_memory` (phase 5) assumes `ACannon::global_time` is at offset 232 (0xE8) of the original's `ACannon`, as clang's MSVC layout says. The Release exe agrees (checked 2026-10-04):
- **Constructor** (`0x40acc0`, the only user of the `"ACannon"` string at `0x49d54c`). `this` is the complete object. It stores `order`, `spx`, `spy`, `time`, `shtime`, `fr_shoot`, `shoot`, `plane`, `startangle`, `ship`, `smoke`, `param` and `smoketime` at 0xB4–0xE4, each at clang's offset. Nothing writes 0xE8: the `==` was compiled away.
- **Virtual bases:** the vbtable at `0x495308` puts `Object` at 0xF0 and `Posit` at 0xF4; `Object`'s vtordisp is at 0xEC.
- **`ACannon::run`** (`0x40bb20`, reached through the vtordisp thunk at `0x40e210` in `Object`'s vftable). `this` is the `Object` base at 0xF0, so `global_time` is `[esi-8]`:

```
0040bb42  mov eax, [esi-8]        ; global_time
0040bb45  mov ecx, [0x49f9dc]     ;   -= KbdStat::time()
0040bb4b  sub eax, ecx
0040bb4d  mov [esi-8], eax
0040bb50  jns 0x40bc46            ; < 0: the cannon is removed
0040bc3e  mov eax, [0x5126e0]     ; else global_time = Mp::BEYONDTIME
0040bc43  mov [esi-8], eax
```

This checks `ACannon` only. What the other classes hold at offset 232 (`stale_gen.cpp`) still rests on clang's layouts and on the 8 demos replaying in sync.

### The demos and 1.0 → 1.1
- **Recording:** the 8 demos were recorded on 20 May 1996, 11:45–12:02, so with a build from before 1.0 shipped. The same files are on the CD and in the shareware tree.
- **Data:** every gameplay `.tdf` predates them. Only `TITLE.TDF` (26 Aug) and `TEXT.TXT` (23 May) are later.
- **Code changed after the recording** (file dates; a date alone doesn't prove a behaviour change):

| Date (1996) | Files |
|---|---|
| 21 May, 01:34–02:56 | almost every engine source, in one sweep |
| 21 May, 21:36–21:39 | `GOBJ.CPP`, `BUILD.CPP`, `PILOTS.CPP`, `NONET.CPP`, `QUEUE.CPP`, `VIEW.CPP` |
| Jun | `1ss.h` (17 Jun), `1mm.cpp` (21 Jun) |
| Jul | `Game.h` (4 Jul); `Data.h`, `Kbdsta.*`, `Menu.h`, `1ss_song.cpp`, `1lg.h` (9 Jul); `Gman.cpp`, `World.*` (15 Jul) |
| Aug | `1lg_main.cpp` (8 Aug); `1cw.cpp`, `1lg_comm.cpp` (14 Aug); `Menu.cpp` (23 Aug); `Headers.h`, `Data.cpp` (25 Aug); `Game.cpp`, `1rg.cpp`, `1io_io.cpp`, `1sp.h` (26 Aug) |

- **An exact 1.1 oracle:** the original 1.1 exe can record new demos, with no 1.0 ambiguity. It runs now ([Running 1.1](#running-11)); no demo has been recorded with it yet.

## Oracles for checking the port

- **Sprite caches** (`FF/WORK.RTL/**/*.SPH|SPL|SPC|SPP`): exactly what shipped. The retail game never rebuilt sprites, because volumes hold no FLC masters.
  - Format (`1sp_ldsa.cpp`):
    - `"CWE sprite\0"`;
    - `int` header size 36;
    - header: `file_level` (12), `source_datetime`, `mirrors`, `onecolor`, `scale`, `data_offset[2]`, `data_size`, `phases`;
    - `data_size` bytes of RLE data, with no pointers;
    - per phase `{uint32 def[2]; short ox, oy, sx, sy}`, where `def` is an offset into the RLE data;
    - the palette, 256 × 4 bytes.
  - **Everything matches** for every sprite and palette the game loads, checked with `ffarchive.py sprites`:
    - the phase bounds (see above);
    - the hires and collision RLE data (size and CRC-32);
    - all three tables of the 7 palettes: the palettes, the closest-colour table and the transparency table.

    The `sprite_build` test keeps these against the golden files.
- **The 1.1 exe** ([Running 1.1](#running-11)): reference screenshots ([phase 3](#reference-screenshots-phase-3)) and new demos.

## Tools

`tools/archive/ffarchive.py` and `patch_hooks.py` (Python 3.8+, no dependencies), and `tools/archive/crt_vectors.py` (also needs pefile and Unicorn). Run them on the Windows PC from the repository root:

```sh
python tools/archive/ffarchive.py provenance --write tests/golden/data_files.sha256   # data/ and music/ vs the archive
# every sprite and palette the game builds, then the dump vs the shipped caches:
cwdiags=extended build/windows-msvc/source/Release/firefight.exe --headless --fast check sprite_dump=1 --pref <dir>
python tools/archive/ffarchive.py sprites <dir>
python <venv>/python tools/archive/crt_vectors.py        # tests/golden/crt_qsort.txt and crt_rand.txt
python tools/archive/ffarchive.py bin2iso "<cd>.bin" ff12.iso      # then extract FIREFGHT/ with 7-Zip
python tools/archive/ffarchive.py compare-cd <dir>                 # every CD volume entry vs data/ and WORK.RTL
python tools/archive/ffarchive.py unpack <dir>/PARAMS.VOL [outdir]
python tools/archive/ffarchive.py cache <archive>/FF/WORK.RTL/!GLOBAL/SPRITES/BOHATER2.SPL
python tools/archive/patch_hooks.py <dir>/FIREFGHT.EXE             # a copy of the Release exe (Running 1.1)
```

The disassembly above used Capstone and pefile on `FF/C/GAME/RELEASE/FIREFGHT.EXE`.

## Still needs the archive

These tasks are for the Windows PC ([Running 1.1](#running-11) has the setup):
- **New demos** recorded with the original 1.1 exe: an exact 1.1 oracle.
- **The snowflakes** that differ in the first `level4c` screenshot ([Reference screenshots](#reference-screenshots-phase-3)).
- **Menus** other than the mission screen, against the original.
