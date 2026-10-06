# Working on the Mac

The Mac (Apple Silicon) builds and tests the port. It doesn't have the original archive: that stays on the Windows PC (see [original-archive.md](original-archive.md#working-without-the-archive)). Everything the archive proves is in the repository as golden files and test vectors.

One-time setup (tools, vcpkg, clone): see "Quick start on macOS" in [`porting.md`](porting.md).

## Checking a branch before it goes to `main`

Branch pushes run CI on Windows and Linux only. macOS is checked here, by hand, before `main` moves (CLAUDE.md, "CI"):

```sh
git fetch
git switch <branch>              # for example lores-bounds
git pull --ff-only
cmake --workflow --preset macos-clang
```

The run configures, builds Debug and Release, and runs every test in both. It must end with `100% tests passed` twice. The tests are `smoke`, `headless_run`, `golden_title`, `sprite_build`, `crt_vectors`, the eight `demo_<name>`, the four `golden_<name>`, `sound_play`, `net_play`, `net_lan`, `input_play` and `data_files`.

Then fast-forward `main`, if CI on the branch is green too (`gh run list --branch <branch>`):

```sh
git switch main
git pull --ff-only
git merge --ff-only <branch>
git push                         # runs the full matrix again, macOS included
```

If the fast-forward fails, `main` has moved. Rebase the branch on `main`, push it, and check again.

## Checking the browser build

CI builds the WebAssembly versions on Linux and runs the Node tests, but only a person can play the browser build. With the Emscripten SDK and vorbis-tools installed (step 6 of the quick start in [`porting.md`](porting.md)):

```sh
source ~/emsdk/emsdk_env.sh
cmake --workflow --preset web-emscripten
python3 -m http.server -d build/web-emscripten/web/Release
```

Open `http://localhost:8000/` (or the GitHub Pages site, https://tosiabunio.github.io/firefight-port/) in Chrome, Safari 27 and Firefox 153, or newer, and click Play. In each browser, check:

- [ ] **Title and demos:** the title sequence and the attract demos play. `?demo=level1&fast` checks the simulation: the page must end with "The game has ended." and the console must say `in sync to the end`.
- [ ] **Keyboard:** Return twice starts the first mission from the title. Fly, turn, fire and open the Esc menu.
- [ ] **Mouse steering:** F11 switches to the mouse, and the page locks the pointer. The right button flies towards the pointer and the left button fires. Esc releases the pointer; note what the game still receives.
- [ ] **Game controller:** connect it and press one of its buttons. Browsers reveal a gamepad to a page only after a button press, and the game picks it up even mid-game. F11 switches to the controller. The d-pad or the left stick steers, A fires and B is turbo.
- [ ] **Sound and music:** the effects and the mission's track play, and Esc → Sound changes their volumes.
- [ ] **Function keys:** F1 shows the help, F5 and F11 switch the control set, and the browser does nothing with them (no reload, no full screen).
- [ ] **Saving:** a changed volume, and a pilot's progress, are still there after a reload.
- [ ] **Quitting:** Esc → Mission → Quit Game ends on the page's "The game has ended." with Play again.
- [ ] **The Asyncify build:** `?asyncify` loads the build for browsers without JSPI (the console says `Fire Fight: the Asyncify build`). Replay `?asyncify&demo=level1&fast`, and end a mission with ABORT MISSION. In the Debug build (`build/web-emscripten/web/Debug`), a suspension inside a `catch` block traps with `RuntimeError: unreachable`.

The game pauses whenever its page loses the focus, as on the desktop. Record the results in `docs/porting-plan.md` (phase 9, step 2's progress).

## When a test fails on the Mac

Golden files describe the original game. Never regenerate them to make a Mac run pass. The same files pass on Windows and Linux, so a failure here is a platform difference to find and fix.

| Test | Usually means | Look at |
|---|---|---|
| `data_files` | The checkout changed bytes in `data/` or `music/`, typically line endings. `.gitattributes` keeps `data/**` binary. | `git config core.autocrlf` must not be `true`. Re-checkout just those folders: `rm -rf data music && git checkout -- data music`. |
| `sprite_build` | The sprite build differs from the original's caches: bounds, pixel data or palette tables. | The failing lines are in the test output. All lines are in `build/macos-clang/source/sprite_build/*.sorted.txt`. |
| `golden_title` | Rendering differs. The title frames are bit-identical on every platform, for example after `-ffp-contract=off` stopped Clang fusing `a*b+c` on arm64. | The frames in `build/macos-clang/source/golden_title/`. Compare them with the same frames from a Linux or Windows build. |
| `headless_run` | Start-up, the title loop or shutdown broke. | The log in the test output (the game echoes it to stderr). |
| `demo_<name>`, `golden_<name>` | An original or golden demo went out of sync (`out of sync after N of M blocks`) or stopped early. All 8 replay in sync on every platform. | The log in the test output, with the state dump at the divergence. `docs/porting-plan.md`, phase 5, lists the suspects. |
| `sound_play` | Demo `level1` with the sound on, through SDL's `disk` audio driver: the sound system didn't start, the soundtrack wasn't found or its track didn't play, the output was silent, or the replay went out of sync. | The log in the test output. The mixed output is `build/macos-clang/source/sound_play/sound.raw`. `no music: SDL_mixer has no FLAC support` means SDL2_mixer was built without the `libflac` feature (`vcpkg.json`). |
| `net_play`, `net_lan` | A two-player network game on this machine (a host and a client over ENet on UDP port 19962, or 19964 with LAN discovery) didn't connect, didn't reach the mission, failed the per-frame sync check, or didn't end normally. | Each peer's log is in `build/macos-clang/source/net_play/` (or `net_lan/`), under `host/` and `client/` (`Firefght.log`); the test prints the end of both when it fails. A sync failure there is a platform difference in the simulation, as with the demos. If the macOS firewall asks, allow the connection: it stays on the loopback address. |
| `input_play` | An input state differs: a key's scan code or text, the mouse position or buttons, or the joystick bits. The states don't depend on the simulation, so they are the same on every platform. | The first differing line is in the test output; all of them are in `build/macos-clang/source/input_play/input_states.txt`. |

## What needs the Windows PC

Anything that reads the archive:
- a new comparison with the original caches or data;
- disassembly or emulation of the original exe;
- running the original game for screenshots or demos;
- **replacing a golden file that came from the archive.** That covers `data_files.sha256`, `sprite_*.txt` and `crt_*.txt`; such a change must first pass `tools/archive/ffarchive.py` on the PC.

If a task on the Mac needs one of these, stop and say so instead of guessing. The list of open archive tasks is in [original-archive.md](original-archive.md#still-needs-the-archive).

## Notes

- **The original demos replay in sync on the Mac too.** The `demo_<name>` tests replay all 8 (`firefight --headless --fast --demo level1`). A desync on the Mac alone is a platform difference to find, like any other test failure here; the suspects are listed in `docs/porting-plan.md` (phase 5).
- **Builds are arm64 only, macOS 11 or later** (`CMAKE_OSX_ARCHITECTURES=arm64`).
