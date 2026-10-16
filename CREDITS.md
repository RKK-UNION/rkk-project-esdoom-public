# ES-DOOM — Credits & Licenses

**ES-DOOM** is an open-source modification that runs a DOOM engine inside **Everlasting Summer** (Бесконечное лето). The Ren'Py script talks to one C shared library through ctypes.

---

## Components

| Piece | Where | Origin | License |
| :--- | :--- | :--- | :--- |
| **Ren'Py script** | `game/mods/esdoom/rkk_esdoom.rpy` | RKK UNION | **MIT** |
| **Shared library, as shipped** | `rkk_esdoom_lib.dll` / `.so` | Combined work below | **GPL-2.0-or-later** |
| **DOOM port** | built from doomgeneric | [ozkl/doomgeneric](https://github.com/ozkl/doomgeneric), on the Id Software DOOM source | **GPL-2.0-or-later** |
| **OPL music** | `native/opl/` (`opl.c`, `opl_queue`, `i_oplmusic.c`, `midifile.c`) | Chocolate Doom 3.1.1, Simon Howard | **GPL-2.0-or-later** |
| **OPL3 emulator** | `native/opl/opl3.c` | Nuked OPL3 1.8, Alexey Khokholov (Nuke.YKT) | **GPL-2.0-or-later** |
| **Host glue** | `doom_bridge`, `doomgeneric_renpy`, `i_renpysound`, `opl_host`, `opl_compat` | RKK UNION, linked into the same library | **GPL-2.0-or-later** |
| **Default IWAD** | `freedoom1.wad` (Freedoom Phase 1), shipped with the mod | Freedoom contributors | **BSD-3-Clause** |
| **RKK Hook (rkk-es-sdk)** | not in this mod; comes with RKK Project 410. Optional, recommended | RKK UNION | **MIT** |
| **RKK Project 410** | not in this mod. Strongly recommended | RKK UNION | **Proprietary** |

Ren'Py itself is the host the novel already ships. This mod does not include Ren'Py. Everlasting Summer runs Ren'Py 7.4; the script is also written for Ren'Py 8. Ren'Py is MIT, Copyright (C) 2004-2026 Tom Rothamel, [https://www.renpy.org/](https://www.renpy.org/).

---

## 1. Script

* **Author:** RKK UNION
* **Website:** [http://rkk-union.com](http://rkk-union.com)
* **Contact:** team@rkk-union.com
* **Repository:** [https://github.com/RKK-UNION/rkk-project-esdoom-public](https://github.com/RKK-UNION/rkk-project-esdoom-public)
* **License:** [MIT](#mit-license-rkk_esdoomrpy), for `rkk_esdoom.rpy` only

The script loads `rkk_esdoom_lib` with ctypes. It is not a Python C extension and does not link Python's C API.

---

## 2. Shared library (`rkk_esdoom_lib`)

The Windows DLL and the Linux `.so` are one GPL-2.0-or-later program. Source for this build is the `native/` tree in the repository above, plus the doomgeneric sources the build copies in. The GPL text that came with the vendored music code is `native/opl/COPYING.md`.

* **doomgeneric** — [https://github.com/ozkl/doomgeneric](https://github.com/ozkl/doomgeneric). The game and renderer come from the DOOM source Id Software released under GPL-2.0-or-later. Copyright (C) 1993-1996 Id Software, Inc.
* **Chocolate Doom 3.1.1 music module** — `opl.c`, `opl_queue`, `i_oplmusic.c`, `midifile.c`, from tag `chocolate-doom-3.1.1`. Copyright (C) 2005-2014 Simon Howard. `i_oplmusic.c` also carries Copyright (C) 1993-1996 Id Software, Inc. [https://github.com/chocolate-doom/chocolate-doom](https://github.com/chocolate-doom/chocolate-doom)
* **Nuked OPL3 1.8** — `opl3.c` / `opl3.h`. Copyright (C) 2013-2018 Alexey Khokholov (Nuke.YKT). [https://github.com/nukeykt/Nuked-OPL3](https://github.com/nukeykt/Nuked-OPL3). The upstream header thanks the MAME team (Jarek Burczynski, Tatsuyuki Satoh), carbon14 and opl3, Matthew Gambrell and Olli Niemitalo, and John McMaster and digshadow for hardware information. That information is not a separate library in this build.
* **ES-DOOM host glue** — `doom_bridge.c`, `doomgeneric_renpy.c`, `i_renpysound.c`, `opl_host.c`, `opl_compat.h`. These files exist so the engine can run under Ren'Py. They are compiled into `rkk_esdoom_lib` and go out under the same GPL-2.0-or-later terms as the rest of that library.

Music is the Chocolate Doom OPL driver on Nuked OPL3, mixed into the same 22050 Hz stereo buffer as the sound effects. `libsamplerate` is compiled out (`use_libsamplerate = 0`). No SDL and no system MIDI API are linked. On Windows the buffer is played with `waveOut` (`winmm`). On Linux the library `dlopen`s the system `libasound.so.2` and does not list it as `NEEDED`.

---

## 3. Game data

The mod includes **freedoom1.wad** (Freedoom Phase 1) and uses it as the default IWAD. License: BSD-3-Clause. Copyright and the full text are the `COPYING` file from the Freedoom release that file came from: [https://freedoom.github.io/](https://freedoom.github.io/).

`esdoom.ini` can name another IWAD ahead of that default. These files are not part of the mod: `freedoom2.wad`, shareware `doom1.wad`, and commercial `doom.wad`, `doom2.wad`, `tnt.wad`, `plutonia.wad`. The player adds a copy they are allowed to use.

---

## 4. RKK Hook and RKK Project 410

ES-DOOM runs without either of these. Both are recommended.

* **RKK Hook (rkk-es-sdk)** — optional in-game companion. It is not inside this mod. It is installed with RKK Project 410. Copyright (C) RKK UNION. The hook script (`RKKModManager.rpy`) is MIT; the license text is in that file. With the hook, the mod can report its title, leave notes, and unlock achievements. The script's `rkk_*` shims do nothing when the hook is absent.
* **RKK Project 410 [Mod Manager]** — strongly recommended. Closed license. Not included in ES-DOOM. Workshop page: [RKK Project 410 [Mod Manager]](https://steamcommunity.com/sharedfiles/filedetails/?id=3765901208).

---

## 5. Host game and trademarks

* **Everlasting Summer (Бесконечное лето):** Soviet Games. This mod is a fan modification for that game.
* **Ren'Py:** Tom Rothamel and contributors. MIT. Not shipped here.
* **Steam:** trademark of Valve Corporation.
* **DOOM:** registered trademark of id Software LLC / ZeniMax Media Inc. / Bethesda Softworks.

ES-DOOM is an independent fan modification. It is not affiliated with, endorsed by, or sponsored by Soviet Games, Valve Corporation, id Software LLC, Bethesda Softworks, or ZeniMax Media Inc.

---

## License texts

### MIT License (`rkk_esdoom.rpy`)

The same text is stored as [LICENSE-MIT.txt](LICENSE-MIT.txt). It covers the Ren'Py script, not the shared library and not `freedoom1.wad`.

```text
Copyright (C) RKK UNION (http://rkk-union.com, team@rkk-union.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### GNU GPL v2.0 or later (shared library)

`rkk_esdoom_lib` and the sources under `native/` are GPL-2.0-or-later. The verbatim GNU GPL version 2 is [LICENSE](LICENSE), copied from <https://www.gnu.org/licenses/gpl-2.0.txt>. The same text is also kept next to the vendored music sources as [native/opl/COPYING.md](native/opl/COPYING.md). "Or any later version" is the grant in the file headers, not a change to the GPL text.

Source for this build: [https://github.com/RKK-UNION/rkk-project-esdoom-public](https://github.com/RKK-UNION/rkk-project-esdoom-public)

### BSD-3-Clause (`freedoom1.wad`)

[LICENSE-BSD.txt](LICENSE-BSD.txt) is the Freedoom `COPYING` (Copyright © 2001-2024, Contributors to the Freedoom project), copied from <https://github.com/freedoom/freedoom>. The sentence at the end of that file refers to [Freedoom's contributor list](https://github.com/freedoom/freedoom/blob/master/CREDITS), not to this `CREDITS.md`. Project page: [https://freedoom.github.io/](https://freedoom.github.io/).

### MIT (RKK Hook) and the manager

RKK Hook (`rkk-es-sdk` / `RKKModManager.rpy`) is MIT, Copyright (C) RKK UNION. It ships with RKK Project 410, not with ES-DOOM.

RKK Project 410 itself is closed-license software: [Steam Workshop](https://steamcommunity.com/sharedfiles/filedetails/?id=3765901208).
