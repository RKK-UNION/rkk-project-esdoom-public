# ES-DOOM

**ES-DOOM** runs a DOOM engine inside [Everlasting Summer](https://store.steampowered.com/app/331470) (Бесконечное лето). The Ren'Py script loads one C library, `rkk_esdoom_lib`, through ctypes. Freedoom Phase 1 (`freedoom1.wad`) ships as the default IWAD.

Version 1.0.0. Platforms: Windows x86_64 and Linux x86_64. The script targets Ren'Py 7.4 (Python 2.7) and Ren'Py 8 (Python 3).

This is an unofficial fan modification by [RKK UNION](http://rkk-union.com). It is not affiliated with, endorsed by, or sponsored by Soviet Games, Valve Corporation, id Software, Bethesda Softworks, or ZeniMax Media Inc. DOOM is a registered trademark of id Software LLC / ZeniMax Media Inc.

Source: <https://github.com/RKK-UNION/rkk-project-esdoom-public>

## Contents

- [Play](#play)
- [Install](#install)
- [Configuration](#configuration)
- [Build](#build)
- [Layout](#layout)
- [Licenses](#licenses)
- [Contributing](#contributing)
- [Credits](#credits)

## Play

Open the camp mods screen in Everlasting Summer and start **ES-DOOM**. The entry is registered as `store.mods["esdoom"]`.

| Action | What happens |
| :--- | :--- |
| Mouse | Captured for the session. The novel cursor is hidden until you leave. |
| Keyboard | WASD, Space, and Escape stay in DOOM. The novel keymap is restored on exit. |
| F12 | Returns to the visual novel main menu. |
| Doom's own Quit | Same return. The process stays open. |

Novel music is paused for the session and the main-menu track is restored afterwards. Doom music is Nuked OPL3 through the Chocolate Doom 3.1.1 OPL driver, mixed into the same 22050 Hz stereo stream as the sound effects. Windows playback uses `waveOut`. Linux playback opens the system `libasound.so.2` at runtime.

Leaving and entering ES-DOOM again in the same novel process continues the same Doom session. The engine is initialized once.

[RKK Hook](#rkk-hook) is optional. With it, the mod can report its title, leave notes, and unlock achievements. Without it, the game still runs.

## Install

Copy the folder `game/mods/esdoom/` into the novel:

```text
Everlasting Summer/game/mods/esdoom/
    rkk_esdoom.rpy
    rkk_esdoom_lib.dll    Windows x86_64
    rkk_esdoom_lib.so     Linux x86_64
    freedoom1.wad
    esdoom.ini            created on first launch
```

`game/script.rpy` and `game/options.rpy` in this repository are a local Ren'Py harness. They are not part of the Everlasting Summer install.

After replacing `rkk_esdoom.rpy` in an existing install, delete `rkk_esdoom.rpyc` so Ren'Py recompiles the script. If the novel already has the library loaded, restart it before trying a new DLL or `.so`.

### RKK Hook

[RKK Project 410](https://steamcommunity.com/sharedfiles/filedetails/?id=3765901208) is a separate mod manager. It is closed-license software, strongly recommended, and not included here. RKK Hook (`rkk-es-sdk`, `RKKModManager.rpy`) ships with that manager. The hook script is MIT, Copyright (C) RKK UNION. ES-DOOM's `rkk_*` calls do nothing when the hook is absent.

## Configuration

`esdoom.ini` is written next to the script on first launch. An existing file is left as it is. A change of IWAD or music mode takes effect the next time the novel process starts ES-DOOM. `controls` is read on every entry.

```ini
[wad]
iwad = freedoom1.wad, freedoom2.wad, doom1.wad, doom.wad, doom2.wad

[sound]
music = opl3
sfx = on
music_volume = 8
sfx_volume = 8

[game]
skill = 0
episode = 1
map = 1
monsters = on
fast = off
respawn = off
mouse_sensitivity = 5
controls = classic
```

| Key | Range | Meaning |
| :--- | :--- | :--- |
| `iwad` | comma-separated names | First file that exists and begins with the four bytes `IWAD`. A name may be a path. |
| `music` | `opl3`, `opl2`, `off` | Sound Blaster 16 OPL3, AdLib OPL2, or silence. |
| `sfx` | `on`, `off` | Sound effects. |
| `music_volume`, `sfx_volume` | 0–15 | Doom volume steps. |
| `skill` | 0–5 | `0` opens the Doom menu. `1`–`5` warp straight in (`1` I'm Too Young to Die … `5` Nightmare!). |
| `episode`, `map` | episode 1–4, map 1–32 | Used when `skill` is not 0. Doom II-style IWADs use `map` only. |
| `monsters`, `fast`, `respawn` | `on`, `off` | `-nomonsters`, `-fast`, `-respawn`. |
| `mouse_sensitivity` | 0–9 | Doom mouse sensitivity. |
| `controls` | `classic`, `wasd` | `classic` is arrows. `wasd` makes W A S D send the same four actions. Fire, use, run, and strafe stay on the Doom keys. |

The search list may name `freedoom2.wad`, shareware `doom1.wad`, or a commercial IWAD. Those files are not included. Place a copy you are allowed to use next to the script, or point `iwad` at it. The first four bytes must be `IWAD`.

## Build

Players can use the libraries already in `game/mods/esdoom/`. Building requires the doomgeneric sources, which are not part of this tree:

```text
external/doomgeneric-master/doomgeneric
```

Clone [ozkl/doomgeneric](https://github.com/ozkl/doomgeneric) so that path exists. CMake runs `native/apply_hosted.py` and writes a patched copy under the build directory.

The toolchain is GCC. The CMake script stops if it is invoked with MSVC. Windows builds with MinGW GCC. Linux builds with gcc, and the shipped `.so` limits itself to glibc symbols through 2.17 so it still loads on older distributions. Output name is `rkk_esdoom_lib`, with no `lib` prefix. The library lands in `game/mods/esdoom/`.

`native/build.ps1` is a local template. Its compiler, CMake, and Ninja paths point at one CLion install. Point them at your own tools, or drive CMake yourself:

```bash
cmake -G Ninja -S native -B native/build-win -DCMAKE_BUILD_TYPE=Release
ninja -C native/build-win
```

On Linux, use `native/build-linux` and the system `gcc`. Release flags are `-O2 -DNDEBUG`.

## Layout

```text
LICENSE                  GNU GPL version 2 (the native library)
LICENSE-MIT.txt          MIT (the Ren'Py script)
LICENSE-BSD.txt          BSD-3-Clause (freedoom1.wad)
CREDITS.md               Component credits and license texts
README.md
game/mods/esdoom/        What Everlasting Summer loads
native/                  Library sources, OPL music, CMake
native/opl/COPYING.md    GPL text kept with the vendored music sources
external/                doomgeneric checkout used at build time (not shipped)
```

## Licenses

The repository is not one license. [LICENSE](LICENSE) is the GNU GPL version 2 for the native engine. It does not relicense the script or `freedoom1.wad`.

| Path | Terms | Text |
| :--- | :--- | :--- |
| `native/`, `rkk_esdoom_lib.dll`, `rkk_esdoom_lib.so` | GPL-2.0-or-later | [LICENSE](LICENSE) |
| `game/mods/esdoom/rkk_esdoom.rpy` | MIT | [LICENSE-MIT.txt](LICENSE-MIT.txt) |
| `freedoom1.wad` | BSD-3-Clause | [LICENSE-BSD.txt](LICENSE-BSD.txt) |
| RKK Hook (`rkk-es-sdk`) | MIT | Ships with RKK Project 410, not in this tree |
| RKK Project 410 | Proprietary | [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3765901208) |

[LICENSE](LICENSE) is the GNU GPL version 2 published by the Free Software Foundation: <https://www.gnu.org/licenses/gpl-2.0.txt>. doomgeneric, the Chocolate Doom 3.1.1 OPL module, Nuked OPL3 1.8, and the ES-DOOM host glue are GPL version 2 or, at your option, any later version. That choice is in the source headers. [native/opl/COPYING.md](native/opl/COPYING.md) is the same GPL, kept beside the vendored music sources.

[LICENSE-BSD.txt](LICENSE-BSD.txt) is the Freedoom `COPYING` (Copyright © 2001-2024, Contributors to the Freedoom project), copied from <https://github.com/freedoom/freedoom>. The last paragraph of that file points at [Freedoom's contributor list](https://github.com/freedoom/freedoom/blob/master/CREDITS), not at [CREDITS.md](CREDITS.md) in this repository.

Full notices, versions, and the MIT text: [CREDITS.md](CREDITS.md).

## Contributing

Issues and pull requests go to <https://github.com/RKK-UNION/rkk-project-esdoom-public>.

- Keep `rkk_esdoom.rpy` under the MIT license in [LICENSE-MIT.txt](LICENSE-MIT.txt). Keep files under `native/` under GPL-2.0-or-later. Leave upstream Chocolate Doom, Nuked OPL3, Id Software, and Freedoom notices in place.
- The script runs on Python 2.7 and Python 3. No f-strings, and no syntax that only one of those accepts.
- The library is a C ABI used through ctypes. One binary serves both Ren'Py 7.4 and Ren'Py 8. A Python C extension is out of scope.
- Windows and Linux, x86_64 only. The Linux build must stay within glibc 2.17.
- Do not add commercial IWADs, shareware `doom1.wad`, or `freedoom2.wad`. `freedoom1.wad` is the data file this mod ships.
- `external/` is a local doomgeneric tree. Patch it through `native/apply_hosted.py` rather than editing a generated copy by hand.
- `native/build.ps1` contains machine-local paths. Leave those paths out of a change that is meant for everyone else.

Questions: [team@rkk-union.com](mailto:team@rkk-union.com).

## Credits

[CREDITS.md](CREDITS.md) lists every component, its origin, and its license: the Ren'Py script, doomgeneric, Chocolate Doom 3.1.1, Nuked OPL3 1.8, the host glue, Freedoom, RKK Hook, and RKK Project 410.

[RKK UNION](http://rkk-union.com) · [team@rkk-union.com](mailto:team@rkk-union.com) · <https://github.com/RKK-UNION/rkk-project-esdoom-public>
