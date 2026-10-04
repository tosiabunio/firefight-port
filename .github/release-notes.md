Fire Fight @VERSION@: the 1996 top-down shooter by Chaos Works, running on Windows, Linux and macOS.

Each package holds the game, its data and the CD soundtrack (lossless FLAC), so it is about 260 MB.

| Package | For | How to run |
|---|---|---|
| `firefight-@VERSION@-windows-x64.zip` | Windows 10 or 11, 64-bit | Unzip it and run `firefight.exe`. Windows may warn about an unrecognised app: choose *More info*, then *Run anyway*. |
| `firefight-@VERSION@-linux-x64.tar.gz` | 64-bit Linux with glibc 2.39 or newer (Ubuntu 24.04, Fedora 40 and later) | Unpack it and run `./firefight`. |
| `firefight-@VERSION@-macos-arm64.dmg` | macOS 11 or newer on Apple Silicon | Drag *Fire Fight* to *Applications*. The app is not notarised: the first time, macOS refuses to open it; allow it in *System Settings → Privacy & Security* (*Open Anyway*). |

**What works:** the single-player game with sound and music, on keyboard, mouse or a game controller, and network play on a local network (`--host 2`, `--join lan`).

**Not yet:** internet play, and in-game menus for key bindings and network games.

The [README](https://github.com/tosiabunio/firefight-port#readme) has the controls, the options, and the rights situation: the code written for the port is 0BSD, while the original 1996 game comes with no license and is published to preserve it.
