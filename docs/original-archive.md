# The original archive

This repository was cut from the original Fire Fight archive: the developers' tree with source, tools, data, built executables and the retail 1.2 CD image. The archive is not in this repository and is not a git repo. This document records what it holds that matters to the port, and what was checked against it on 2026-10-04.

Paths below are relative to the archive root, the directory holding `FF/`, `LIB/` and `BIN/`. The root's location differs per machine. The archive's own `CLAUDE.md` describes its layout and how the original was built. `tools/archive/ffarchive.py` reproduces every check here (see [Tools](#tools)).

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
- **Running 1.1:** `FF/WORK.RTL` is a complete loose-file installation (exe, loader, manifests, caches, FLC masters). Nobody has run it yet. Notes:
  - Copy it elsewhere first, keeping file dates. In loose-file mode the engine writes its log there and rebuilds any cache whose master's date no longer matches the date recorded in the cache.
  - The `WORK.RTL` exe is the Production build and checks for the CD; the `RELEASE` exe doesn't.
  - Start it through `LOADER.EXE`: the game needs the registry values the launcher writes.
  - The CD's `Instructions.txt` covers Windows Vista–8 (Windows 98 compatibility, 640×480, "Do not use Direct Draw"); Windows 11 is untested.

## Findings for phase 5 (determinism)

### MSVC `rand`: confirmed
`0x488d50`: `holdrand = holdrand*214013 + 2531011; return (holdrand>>16) & 0x7fff`. The multiply is built from `lea`/`shl`, so a search for the usual `imul` constant misses it.

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

`shortsort` keeps the *first* maximum (`if (comp(p, max) > 0) max = p`). Call sites:
- `1sp_scr2.cpp:115`: collision results;
- `1sp_lev.cpp:768`: the visible level objects, after `look_at` has run builders;
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
- **Verified:** `check` mode with `sprite_bounds=1` loads every mission's level. All 382 distinct sprites match the shipped caches in every phase (`ffarchive.py bounds`), including their hires and collision bounds. The `sprite_bounds` test keeps this against `tests/golden/sprite_bounds.txt`.
- **Visible effect:** the title centres the copyright line with its bound `r` (`world.cpp:2561`). It now sits one lores pixel further left, as in the original, so the title frame hashes were regenerated.
- **Demo effect,** on Windows/MSVC, whose CRT `rand` is the MSVC LCG; `--headless --fast --demo <name>`:

| Demo | Before the fix | After |
|---|---|---|
| `level1`, `level2`, `level3`, `level4`, `level1c`, `level2c`, `level3c` | desync at checks 0x364–0x2791 | **in sync to the end of the recording** (1,261–4,281 frames) |
| `level4c` | desync at 0x1165 | desync at 0x1606 |

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

- **An exact 1.1 oracle:** the original 1.1 exe can record new demos, with no 1.0 ambiguity. That needs only a working run (see [Executables](#executables)).

## Oracles for checking the port

- **Sprite caches** (`FF/WORK.RTL/**/*.SPH|SPL|SPC|SPP`): exactly what shipped. The retail game never rebuilt sprites, because volumes hold no FLC masters.
  - Format (`1sp_ldsa.cpp`):
    - `"CWE sprite\0"`;
    - `int` header size 36;
    - header: `file_level` (12), `source_datetime`, `mirrors`, `onecolor`, `scale`, `data_offset[2]`, `data_size`, `phases`;
    - `data_size` bytes of RLE data, with no pointers;
    - per phase `{uint32 def[2]; short ox, oy, sx, sy}`, where `def` is an offset into the RLE data;
    - the palette, 256 × 4 bytes.
  - The phase bounds now match (see above). The RLE data and the 7 shipped palette tables (`.spp`) can be compared one to one with the port's in-memory build too; that is not done yet.
- **The 1.1 exe**, for reference screenshots (phase 3 exit) and new demos (phase 5).

## Tools

`tools/archive/ffarchive.py` (Python 3.8+, no dependencies):

```sh
python tools/archive/ffarchive.py --archive <root> bounds <pref>/sprite_bounds.txt   # the port's bounds vs the shipped caches
python tools/archive/ffarchive.py bin2iso "<cd>.bin" ff12.iso         # then extract FIREFGHT/ with 7-Zip
python tools/archive/ffarchive.py --archive <root> compare-cd <dir>   # every volume entry vs data/ and WORK.RTL
python tools/archive/ffarchive.py unpack <dir>/PARAMS.VOL [outdir]
python tools/archive/ffarchive.py cache <root>/FF/WORK.RTL/!GLOBAL/SPRITES/BOHATER2.SPL
```

The disassembly above used Capstone and pefile on `FF/C/GAME/RELEASE/FIREFGHT.EXE`.
