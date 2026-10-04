# Working on the Mac

The Mac (Apple Silicon) builds and tests the port. It doesn't have the original archive: that stays on the Windows PC (see [original-archive.md](original-archive.md#working-without-the-archive)). Everything the archive proves is in the repository as golden files and test vectors.

One-time setup (tools, vcpkg, clone): see "Quick start on macOS" in `README.md`.

## Checking a branch before it goes to `main`

Branch pushes run CI on Windows and Linux only. macOS is checked here, by hand, before `main` moves (CLAUDE.md, "CI"):

```sh
git fetch
git switch <branch>              # for example lores-bounds
git pull --ff-only
cmake --workflow --preset macos-clang
```

The run configures, builds Debug and Release, and runs every test in both. It must end with `100% tests passed` twice. The tests are `smoke`, `headless_run`, `golden_title`, `sprite_build`, `crt_vectors`, the eight `demo_<name>`, the four `golden_<name>`, `input_play` and `data_files`.

Then fast-forward `main`, if CI on the branch is green too (`gh run list --branch <branch>`):

```sh
git switch main
git pull --ff-only
git merge --ff-only <branch>
git push                         # runs the full matrix again, macOS included
```

If the fast-forward fails, `main` has moved. Rebase the branch on `main`, push it, and check again.

## When a test fails on the Mac

Golden files describe the original game. Never regenerate them to make a Mac run pass. The same files pass on Windows and Linux, so a failure here is a platform difference to find and fix.

| Test | Usually means | Look at |
|---|---|---|
| `data_files` | The checkout changed bytes in `data/` or `music/`, typically line endings. `.gitattributes` keeps `data/**` binary. | `git config core.autocrlf` must not be `true`. Re-checkout just those folders: `rm -rf data music && git checkout -- data music`. |
| `sprite_build` | The sprite build differs from the original's caches: bounds, pixel data or palette tables. | The failing lines are in the test output. All lines are in `build/macos-clang/source/sprite_build/*.sorted.txt`. |
| `golden_title` | Rendering differs. The title frames are bit-identical on every platform, for example after `-ffp-contract=off` stopped Clang fusing `a*b+c` on arm64. | The frames in `build/macos-clang/source/golden_title/`. Compare them with the same frames from a Linux or Windows build. |
| `headless_run` | Start-up, the title loop or shutdown broke. | The log in the test output (the game echoes it to stderr). |
| `demo_<name>`, `golden_<name>` | An original or golden demo went out of sync (`out of sync after N of M blocks`) or stopped early. All 8 replay in sync on every platform. | The log in the test output, with the state dump at the divergence. `docs/porting-plan.md`, phase 5, lists the suspects. |
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
