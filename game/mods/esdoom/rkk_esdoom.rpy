# ES-DOOM inside Everlasting Summer — DOOM Engine Integration
# Compatible with Ren'Py 7.4+ (Python 2.7) and Ren'Py 8.x (Python 3).
# Copyright (C) RKK UNION
# Site:    http://rkk-union.com
# Email:   team@rkk-union.com
# GitHub:  https://github.com/RKK-UNION/rkk-project-esdoom-public
#
# Unofficial fan modification for Everlasting Summer / Бесконечное лето.
# Not affiliated with, endorsed by, or sponsored by Soviet Games, Valve
# Corporation, id Software, Bethesda Softworks, or ZeniMax Media Inc.
# DOOM is a registered trademark of id Software LLC / ZeniMax Media Inc.
#
# P.S. Gecodet in Deutschland
# P.P.S. Weil sogar ein russisches Anime-Camp ein bisschen deutsche Ordnung braucht ;d
# -----------------------------------------------------------------------------
# License Architecture & Component Credits
# -----------------------------------------------------------------------------
# 1. This script (rkk_esdoom.rpy):
#    Copyright (C) RKK UNION. MIT License. Text below and in
#    LICENSE-MIT.txt at the repository root.
#    The native library is GPL-2.0-or-later (LICENSE).
#    freedoom1.wad is BSD-3-Clause (LICENSE-BSD.txt).
#
# 2. Shared library rkk_esdoom_lib.dll / .so, as one program:
#    GNU GPL v2.0 or later. Source: https://github.com/RKK-UNION/rkk-project-esdoom-public
#    GPL text for the vendored music code: native/opl/COPYING.md
#    Linked into that library:
#      doomgeneric (https://github.com/ozkl/doomgeneric), on the DOOM
#      source released by Id Software, Inc., Copyright (C) 1993-1996.
#      Chocolate Doom 3.1.1 OPL music (opl.c, opl_queue, i_oplmusic,
#      midifile). Copyright (C) 2005-2014 Simon Howard.
#      Copyright (C) 1993-1996 Id Software, Inc. on i_oplmusic.c.
#      Nuked OPL3 1.8 (opl3.c). Copyright (C) 2013-2018 Alexey Khokholov
#      (Nuke.YKT). https://github.com/nukeykt/Nuked-OPL3
#      ES-DOOM host glue (doom_bridge, doomgeneric_renpy, i_renpysound,
#      opl_host, opl_compat), distributed inside the same library.
#    This script calls the library through ctypes and does not link
#    Python's C API. No SDL, system MIDI, libsamplerate, or separate
#    sound library is linked in. Windows playback uses waveOut (winmm).
#    Linux playback dlopens the system libasound.so.2.
#
# 3. Game data:
#    The mod ships freedoom1.wad (Freedoom Phase 1) as the default IWAD.
#    Freedoom is BSD-3-Clause: https://freedoom.github.io/
#    Its COPYING is the one distributed with that wad.
#    esdoom.ini still accepts another file first in the list. freedoom2.wad,
#    shareware doom1.wad, and commercial doom.wad, doom2.wad, tnt.wad,
#    and plutonia.wad are not included.
#
# 4. RKK Hook (rkk-es-sdk), optional, recommended:
#    In-game companion shipped with RKK Project 410, not with this mod.
#    Copyright (C) RKK UNION. MIT License (RKKModManager.rpy).
#    Without it the mod still runs; notes, achievements, and the manager
#    entry stay inactive.
#
# 5. RKK Project 410 [Mod Manager], closed license, strongly recommended:
#    Standalone manager. Not included here.
#    https://steamcommunity.com/sharedfiles/filedetails/?id=3765901208
#
# -----------------------------------------------------------------------------
# License — MIT (for this script file)
# -----------------------------------------------------------------------------
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# -----------------------------------------------------------------------------

init -1500 python:
    if "rkk_note" not in dir(store):
        def rkk_note(text, tag=None):
            return None
    if "rkk_report_mod_version" not in dir(store):
        def rkk_report_mod_version(mod_label, version):
            return None
    if "rkk_report_mod_title" not in dir(store):
        def rkk_report_mod_title(mod_label, title):
            return None
    if "rkk_is_companion_available" not in dir(store):
        def rkk_is_companion_available():
            return False
    if "rkk_companion_info" not in dir(store):
        def rkk_companion_info():
            return {"available": False}
    if "rkk_get_active_mods" not in dir(store):
        def rkk_get_active_mods():
            return {}
    if "rkk_set_context" not in dir(store):
        def rkk_set_context(key, value):
            return None
    if "rkk_get_context" not in dir(store):
        def rkk_get_context():
            return {}
    if "rkk_open_manager" not in dir(store):
        def rkk_open_manager():
            return None
    if "rkk_emit" not in dir(store):
        def rkk_emit(event, data=None):
            return False
    if "rkk_subscribe" not in dir(store):
        def rkk_subscribe(event, fn, owner=None, replay=False):
            return False
    if "rkk_unsubscribe" not in dir(store):
        def rkk_unsubscribe(event=None, fn=None, owner=None):
            return None
    if "rkk_bus_get" not in dir(store):
        def rkk_bus_get(event):
            return None
    if "rkk_bus_history" not in dir(store):
        def rkk_bus_history(event=None, limit=None):
            return []
    if "rkk_unlock_achievement" not in dir(store):
        def rkk_unlock_achievement(mod_label, ach_id, title, desc=None, notify=True):
            return False
    if "rkk_has_achievement" not in dir(store):
        def rkk_has_achievement(mod_label, ach_id):
            return False


init python:
    import os
    import struct
    import time
    import ctypes
    from io import BytesIO
    import pygame_sdl2 as pygame

    ESDOOM_VERSION = "1.0.0"
    ESDOOM_LABEL = "esdoom"
    ESDOOM_GS_LEVEL = 0
    ESDOOM_PST_DEAD = 1
    ESDOOM_HELD_KEYS = (
        "dismiss",
        "dismiss_hard_pause",
        "game_menu",
        "skip",
        "toggle_skip",
        "hide_windows",
        "screenshot",
        "toggle_fullscreen",
        "console",
        "help",
        "quit",
        "iconify",
    )
    ESDOOM_MOUSE_BIT = {1: 1, 3: 2, 2: 4}

    esdoom_lib = None
    esdoom_view = None
    esdoom_running = False
    esdoom_failed = False
    esdoom_error = u""
    esdoom_title = u""
    esdoom_view_w = 1440
    esdoom_view_h = 1080
    esdoom_keymap = None
    esdoom_allow_skipping = None
    esdoom_show_mouse = None
    esdoom_mouse_dx = 0
    esdoom_mouse_dy = 0
    esdoom_mouse_ready = False
    esdoom_controls = u"classic"
    esdoom_status = None
    esdoom_saw_status = False
    esdoom_notify_hide_at = 0.0
    esdoom_run_doom = False


    class DoomStatus(ctypes.Structure):
        _fields_ = [
            ("gamestate", ctypes.c_int),
            ("episode", ctypes.c_int),
            ("map", ctypes.c_int),
            ("health", ctypes.c_int),
            ("playerstate", ctypes.c_int),
            ("killcount", ctypes.c_int),
        ]


    def esdoom_cstr(text):
        if text is None:
            return None
        if isinstance(text, bytes):
            return text
        return text.encode("utf-8")


    def esdoom_text(raw):
        if not raw:
            return u""
        if isinstance(raw, bytes):
            return raw.decode("utf-8", "replace")
        return raw


    def esdoom_viewport():
        sw = int(renpy.config.screen_width)
        sh = int(renpy.config.screen_height)
        vw = int(sh * 4 // 3)
        vh = sh
        if vw > sw:
            vw = sw
            vh = int(sw * 3 // 4)
        if vw < 1:
            vw = sw
        if vh < 1:
            vh = sh
        return vw, vh


    def esdoom_lib_name():
        if renpy.windows:
            return "rkk_esdoom_lib.dll"
        if renpy.linux:
            return "rkk_esdoom_lib.so"
        return None


    def esdoom_mod_dir():
        # Same place Everlasting Summer loads every other mod from.
        return os.path.join(renpy.config.gamedir, "mods", "esdoom")


    def esdoom_lib_path():
        name = esdoom_lib_name()
        if not name:
            return None
        path = os.path.join(esdoom_mod_dir(), name)
        if os.path.isfile(path):
            return path
        try:
            packed = renpy.loader.transfn("mods/esdoom/" + name)
        except Exception:
            packed = None
        if packed and os.path.isfile(packed):
            return packed
        return None


    def esdoom_frame_surface(raw, fw, fh):
        # pygame_sdl2 7.4 has no image.frombuffer. A top-down 32-bit BMP
        # is BGRA, which matches the hosted framebuffer, including alpha.
        info = struct.pack(
            "<IiiHHIIiiII",
            40, int(fw), -int(fh), 1, 32, 0, len(raw), 0, 0, 0, 0,
        )
        off = 14 + len(info)
        filehdr = struct.pack("<2sIHHI", b"BM", off + len(raw), 0, 0, off)
        return pygame.image.load(BytesIO(filehdr + info + raw), "esdoom.bmp")


    def esdoom_byte(buf, index):
        value = buf[index]
        if isinstance(value, int):
            return value
        return ord(value)


    def esdoom_elf_x64(path):
        handle = open(path, "rb")
        try:
            hdr = handle.read(20)
        finally:
            handle.close()
        if len(hdr) < 20 or hdr[:4] != b"\x7fELF":
            return False
        if esdoom_byte(hdr, 4) != 2:
            return False
        machine = esdoom_byte(hdr, 18) + (esdoom_byte(hdr, 19) << 8)
        return machine == 62


    ESDOOM_INI_DEFAULT = (
        u"; ES-DOOM by RKK UNION - Settings file\n"
        u"; F12 returns to main menu.\n"
        u"; IWAD and music changes take effect on restart.\n"
        u"\n"
        u"[wad]\n"
        u"iwad = freedoom1.wad, freedoom2.wad, doom1.wad, doom.wad, doom2.wad\n"
        u"\n"
        u"[sound]\n"
        u"music = opl3\n"
        u"sfx = on\n"
        u"music_volume = 8\n"
        u"sfx_volume = 8\n"
        u"\n"
        u"[game]\n"
        u"; skill: 0 = menu, 1-5 = direct start (1: Young ... 5: Nightmare)\n"
        u"skill = 0\n"
        u"episode = 1\n"
        u"map = 1\n"
        u"monsters = on\n"
        u"fast = off\n"
        u"respawn = off\n"
        u"mouse_sensitivity = 5\n"
        u"; controls: classic (arrows) or wasd (W A S D stand in for the arrows)\n"
        u"controls = classic\n"
    )

    ESDOOM_COMMERCIAL_WADS = (
        "doom2.wad",
        "freedoom2.wad",
        "tnt.wad",
        "plutonia.wad",
    )

    def esdoom_bind(lib):
        lib.doom_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
        lib.doom_init.restype = ctypes.c_int
        lib.doom_set_volumes.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.doom_set_volumes.restype = None
        lib.doom_step.argtypes = []
        lib.doom_step.restype = ctypes.c_int
        lib.doom_send_key.argtypes = [ctypes.c_int, ctypes.c_int]
        lib.doom_send_key.restype = None
        lib.doom_send_mouse.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.doom_send_mouse.restype = None
        lib.doom_framebuffer.argtypes = [
            ctypes.POINTER(ctypes.c_int),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.doom_framebuffer.restype = ctypes.POINTER(ctypes.c_uint32)
        lib.doom_query.argtypes = [ctypes.POINTER(DoomStatus)]
        lib.doom_query.restype = None
        lib.doom_last_error.argtypes = []
        lib.doom_last_error.restype = ctypes.c_char_p
        lib.doom_window_title.argtypes = []
        lib.doom_window_title.restype = ctypes.c_char_p
        lib.doom_audio_active.argtypes = [ctypes.c_int]
        lib.doom_audio_active.restype = None


    def esdoom_load_lib():
        global esdoom_lib
        if esdoom_lib is not None:
            return None
        path = esdoom_lib_path()
        if not path:
            if renpy.linux:
                return u"Oops, rkk_esdoom_lib.so is missing"
            if renpy.windows:
                return u"Oops, rkk_esdoom_lib.dll is missing"
            return u"This ES-DOOM build is for Windows and Linux only"
        if renpy.linux and not esdoom_elf_x64(path):
            return u"Oops, rkk_esdoom_lib.so is not an ELF x86_64"
        try:
            lib = ctypes.CDLL(path)
            esdoom_bind(lib)
        except Exception as exc:
            return u"Error: Failed to open ES-DOOM library: %s" % esdoom_text(str(exc))
        esdoom_lib = lib
        return None


    def esdoom_config_dir():
        folder = os.path.join(renpy.config.savedir, "esdoom")
        if not os.path.isdir(folder):
            os.makedirs(folder)
        return folder


    def esdoom_ini_path():
        return os.path.join(esdoom_mod_dir(), "esdoom.ini")


    def esdoom_ensure_ini():
        path = esdoom_ini_path()
        if os.path.isfile(path):
            return path
        data = ESDOOM_INI_DEFAULT
        if not isinstance(data, bytes):
            data = data.encode("utf-8")
        try:
            handle = open(path, "wb")
        except Exception:
            return None
        try:
            handle.write(b"\xef\xbb\xbf")
            handle.write(data)
        finally:
            handle.close()
        return path


    def esdoom_ini_strip(value):
        value = value.strip()
        cut = len(value)
        for mark in (" ;", " #", "\t;", "\t#"):
            at = value.find(mark)
            if at >= 0 and at < cut:
                cut = at
        value = value[:cut].strip()
        if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
            return value[1:-1]
        return value


    def esdoom_read_ini():
        esdoom_ensure_ini()
        path = esdoom_ini_path()
        values = {}
        if not os.path.isfile(path):
            return values
        handle = open(path, "rb")
        try:
            raw = handle.read()
        finally:
            handle.close()
        if raw.startswith(b"\xef\xbb\xbf"):
            raw = raw[3:]
        text = raw.decode("utf-8", "replace")
        section = ""
        for line in text.splitlines():
            line = line.strip()
            if not line or line.startswith(";") or line.startswith("#"):
                continue
            if line.startswith("[") and line.endswith("]"):
                section = line[1:-1].strip().lower()
                continue
            if "=" not in line:
                continue
            key, value = line.split("=", 1)
            values[(section, key.strip().lower())] = esdoom_ini_strip(value)
        return values


    def esdoom_ini_on(value, default):
        token = value.strip().lower()
        if token in ("1", "on", "yes", "true", u"да"):
            return True
        if token in ("0", "off", "no", "false", u"нет"):
            return False
        return default


    def esdoom_ini_int(value, default, low, high):
        try:
            number = int(value.strip())
        except Exception:
            return default
        if number < low:
            return low
        if number > high:
            return high
        return number


    def esdoom_music_mode(value):
        token = value.strip().lower()
        if token in ("opl3", "sb16", "stereo"):
            return "opl3"
        if token in ("opl2", "adlib", "sb", "mono"):
            return "opl2"
        if token in ("off", "none", "0", "no"):
            return "off"
        return "opl3"


    def esdoom_controls_mode(value):
        token = value.strip().lower()
        if token == "wasd":
            return u"wasd"
        return u"classic"


    def esdoom_settings():
        values = esdoom_read_ini()

        def pick(section, key, default):
            if (section, key) in values and values[(section, key)] != "":
                return values[(section, key)]
            return default

        return {
            "iwad": pick("wad", "iwad", u"freedoom1.wad, freedoom2.wad, doom1.wad, doom.wad, doom2.wad"),
            "music": esdoom_music_mode(pick("sound", "music", u"opl3")),
            "sfx": esdoom_ini_on(pick("sound", "sfx", u"on"), True),
            "music_volume": esdoom_ini_int(pick("sound", "music_volume", u"8"), 8, 0, 15),
            "sfx_volume": esdoom_ini_int(pick("sound", "sfx_volume", u"8"), 8, 0, 15),
            "skill": esdoom_ini_int(pick("game", "skill", u"0"), 0, 0, 5),
            "episode": esdoom_ini_int(pick("game", "episode", u"1"), 1, 1, 4),
            "map": esdoom_ini_int(pick("game", "map", u"1"), 1, 1, 32),
            "monsters": esdoom_ini_on(pick("game", "monsters", u"on"), True),
            "fast": esdoom_ini_on(pick("game", "fast", u"off"), False),
            "respawn": esdoom_ini_on(pick("game", "respawn", u"off"), False),
            "mouse_sensitivity": esdoom_ini_int(pick("game", "mouse_sensitivity", u"5"), 5, 0, 9),
            "controls": esdoom_controls_mode(pick("game", "controls", u"classic")),
        }


    def esdoom_layout_key(key):
        if esdoom_controls != u"wasd":
            return key
        if key == pygame.K_w:
            return pygame.K_UP
        if key == pygame.K_a:
            return pygame.K_LEFT
        if key == pygame.K_s:
            return pygame.K_DOWN
        if key == pygame.K_d:
            return pygame.K_RIGHT
        return key


    def esdoom_locate_wad(name, folder):
        name = name.strip().strip('"')
        if not name:
            return None
        if os.path.isabs(name) or ("/" in name) or ("\\" in name):
            path = name
        else:
            path = os.path.join(folder, name)
        if os.path.isfile(path):
            return path
        return None


    def esdoom_is_iwad(path):
        # doom_init passes this path as -iwad. A PWAD or any other file
        # still fails inside the engine, so only the IWAD signature is valid.
        handle = open(path, "rb")
        try:
            magic = handle.read(4)
        finally:
            handle.close()
        return magic == b"IWAD"


    def esdoom_find_iwad(settings):
        folder = esdoom_mod_dir()
        rejected = False
        for name in settings["iwad"].split(","):
            found = esdoom_locate_wad(name, folder)
            if not found:
                continue
            if esdoom_is_iwad(found):
                return found, None
            rejected = True
        if rejected:
            return None, u"Not an IWAD file ;("
        return None, u"IWAD file not found ;("


    def esdoom_commercial(path):
        return os.path.basename(path).lower() in ESDOOM_COMMERCIAL_WADS


    def esdoom_launch_args(settings, iwad):
        mode = settings["music"]
        if mode == "opl2":
            os.environ["DMXOPTION"] = "-opl2"
        elif mode == "off":
            os.environ["DMXOPTION"] = ""
        else:
            os.environ["DMXOPTION"] = "-opl3"
        tokens = []
        if mode == "off":
            tokens.append("-nomusic")
        if not settings["sfx"]:
            tokens.append("-nosfx")
        if settings["skill"] > 0:
            tokens.append("-skill")
            tokens.append(str(settings["skill"]))
            tokens.append("-warp")
            if esdoom_commercial(iwad):
                tokens.append(str(settings["map"]))
            else:
                tokens.append(str(settings["episode"]))
                tokens.append(str(settings["map"]))
        if not settings["monsters"]:
            tokens.append("-nomonsters")
        if settings["fast"]:
            tokens.append("-fast")
        if settings["respawn"]:
            tokens.append("-respawn")
        return " ".join(tokens)


    def esdoom_last_error():
        if esdoom_lib is None:
            return u"Error: ES-DOOM"
        text = esdoom_text(esdoom_lib.doom_last_error())
        if text:
            return text
        return u"Error: ES-DOOM"


    def esdoom_grab_input():
        global esdoom_keymap
        global esdoom_allow_skipping
        esdoom_keymap = {}
        for name in ESDOOM_HELD_KEYS:
            if name in renpy.config.keymap:
                esdoom_keymap[name] = list(renpy.config.keymap[name])
                renpy.config.keymap[name] = []
        try:
            renpy.display.behavior.clear_keymap_cache()
        except Exception:
            pass
        esdoom_allow_skipping = renpy.config.allow_skipping
        renpy.config.allow_skipping = False
        try:
            renpy.config.skipping = None
        except Exception:
            pass
        try:
            pygame.key.set_repeat(0)
        except Exception:
            pass
        esdoom_ungrab_mouse()


    def esdoom_ungrab_mouse():
        try:
            if pygame.event.get_grab():
                pygame.mouse.set_visible(True)
            pygame.event.set_grab(False)
        except Exception:
            pass


    def esdoom_mouse_center():
        size = None
        try:
            size = renpy.display.draw.physical_size
        except Exception:
            size = None
        if not size or not size[0] or not size[1]:
            try:
                size = pygame.display.get_size()
            except Exception:
                return None
        return (int(size[0] // 2), int(size[1] // 2))


    def esdoom_mouse_delta():
        global esdoom_mouse_dx
        global esdoom_mouse_dy
        global esdoom_mouse_ready
        center = esdoom_mouse_center()
        if center is None:
            return
        try:
            x, y = pygame.mouse.get_pos()
            pygame.mouse.set_pos(center)
        except Exception:
            return
        if not esdoom_mouse_ready:
            esdoom_mouse_ready = True
            return
        esdoom_mouse_dx += int(x) - center[0]
        esdoom_mouse_dy += int(y) - center[1]


    def esdoom_hide_cursor():
        global esdoom_show_mouse
        try:
            interface = renpy.game.interface
        except Exception:
            return
        if interface is None:
            return
        if esdoom_show_mouse is None:
            esdoom_show_mouse = bool(interface.show_mouse)
        interface.show_mouse = False
        try:
            interface.set_mouse(False)
        except Exception:
            pass


    def esdoom_flush_mouse():
        global esdoom_mouse_dx
        global esdoom_mouse_dy
        esdoom_mouse_delta()
        if esdoom_lib is None or not esdoom_mouse_ready:
            return
        dx = esdoom_mouse_dx
        dy = esdoom_mouse_dy
        esdoom_mouse_dx = 0
        esdoom_mouse_dy = 0
        buttons = 0
        if esdoom_view is not None:
            buttons = int(esdoom_view.buttons)
        esdoom_lib.doom_send_mouse(int(dx), int(dy), buttons)


    def esdoom_release_mouse():
        global esdoom_show_mouse
        global esdoom_mouse_dx
        global esdoom_mouse_dy
        global esdoom_mouse_ready
        esdoom_ungrab_mouse()
        esdoom_mouse_dx = 0
        esdoom_mouse_dy = 0
        esdoom_mouse_ready = False
        if esdoom_show_mouse is not None:
            try:
                renpy.game.interface.show_mouse = esdoom_show_mouse
            except Exception:
                pass
            esdoom_show_mouse = None


    def esdoom_release_input():
        global esdoom_keymap
        global esdoom_allow_skipping
        if esdoom_keymap is not None:
            for name, keys in esdoom_keymap.items():
                renpy.config.keymap[name] = keys
            esdoom_keymap = None
            try:
                renpy.display.behavior.clear_keymap_cache()
            except Exception:
                pass
        if esdoom_allow_skipping is not None:
            renpy.config.allow_skipping = esdoom_allow_skipping
            esdoom_allow_skipping = None
        esdoom_release_mouse()


    def esdoom_pause_audio(flag):
        for channel in ("music", "sound", "voice"):
            try:
                renpy.music.set_pause(flag, channel)
            except Exception:
                pass


    def esdoom_map_name(status):
        if status.episode > 0 and status.map > 0:
            return u"E%dM%d" % (status.episode, status.map)
        if status.map > 0:
            return u"MAP%02d" % status.map
        return u""


    def esdoom_award_ran_doom(status):
        global esdoom_run_doom
        if esdoom_run_doom or status.gamestate != ESDOOM_GS_LEVEL:
            return
        esdoom_run_doom = True
        if rkk_has_achievement(ESDOOM_LABEL, "but_can_it_run_doom"):
            return
        rkk_note(u"ES-DOOM level %s" % esdoom_map_name(status), "esdoom")
        rkk_unlock_achievement(
            ESDOOM_LABEL,
            "but_can_it_run_doom",
            u"But can it run Doom?",
            u"Запустить DOOM... в визуальной новелле? Нет проблем!",
        )
        esdoom_arm_notify_hide(1.5)


    def esdoom_poll():
        global esdoom_status
        global esdoom_saw_status
        global esdoom_title
        if esdoom_lib is None:
            return
        status = DoomStatus()
        esdoom_lib.doom_query(ctypes.byref(status))
        title = esdoom_text(esdoom_lib.doom_window_title())
        if title:
            esdoom_title = title
        previous = esdoom_status
        esdoom_status = status
        esdoom_award_ran_doom(status)
        if not esdoom_saw_status:
            esdoom_saw_status = True
            return
        if previous is None:
            return
        if (status.episode != previous.episode or status.map != previous.map) and status.gamestate == ESDOOM_GS_LEVEL:
            name = esdoom_map_name(status)
            rkk_set_context("esdoom", name)
            rkk_emit("esdoom.map", {
                "episode": int(status.episode),
                "map": int(status.map),
                "name": name,
            })
            rkk_note(u"ES-DOOM map %s" % name, "esdoom")
        if status.playerstate == ESDOOM_PST_DEAD and previous.playerstate != ESDOOM_PST_DEAD and status.gamestate == ESDOOM_GS_LEVEL:
            rkk_note(u"ES-DOOM death on %s" % esdoom_map_name(status), "esdoom")
            if not rkk_has_achievement(ESDOOM_LABEL, "first_blood"):
                rkk_unlock_achievement(
                    ESDOOM_LABEL,
                    "first_blood",
                    u"Первая смерть в новелле",
                    u"Погибнуть в визуальной новелле... и как у тебя это вообще получилось?",
                )
                esdoom_arm_notify_hide(1.5)


    class DoomDisplayable(renpy.Displayable):
        def __init__(self, **kwargs):
            super(DoomDisplayable, self).__init__(**kwargs)
            self.buttons = 0
            self._raw = None

        def render(self, width, height, st, at):
            fw = 640
            fh = 400
            if esdoom_running and esdoom_lib is not None:
                esdoom_hide_cursor()
                cw = ctypes.c_int()
                ch = ctypes.c_int()
                ptr = esdoom_lib.doom_framebuffer(ctypes.byref(cw), ctypes.byref(ch))
                if ptr and cw.value > 0 and ch.value > 0:
                    fw = int(cw.value)
                    fh = int(ch.value)
                    raw = ctypes.string_at(ptr, fw * fh * 4)
                    self._raw = raw
                    rv = renpy.Render(fw, fh)
                    src = esdoom_frame_surface(raw, fw, fh)
                    dest = renpy.display.pgrender.surface((fw, fh), True)
                    dest.blit(src, (0, 0))
                    renpy.display.render.mutated_surface(dest)
                    rv.blit(dest, (0, 0))
                    return rv
            return renpy.Render(fw, fh)

        def _mouse_buttons(self, ev, down):
            bit = ESDOOM_MOUSE_BIT.get(getattr(ev, "button", 0), 0)
            if not bit:
                return
            if down:
                self.buttons = self.buttons | bit
            else:
                self.buttons = self.buttons & ~bit

        def event(self, ev, x, y, st):
            if not esdoom_running or esdoom_lib is None:
                return None
            esdoom_hide_cursor()
            if ev.type == pygame.KEYDOWN and ev.key == pygame.K_F12:
                return "exit"
            if ev.type in (pygame.KEYDOWN, pygame.KEYUP):
                pressed = 1 if ev.type == pygame.KEYDOWN else 0
                esdoom_lib.doom_send_key(esdoom_layout_key(int(ev.key)), pressed)
                raise renpy.IgnoreEvent()
            if ev.type == pygame.MOUSEBUTTONDOWN:
                self._mouse_buttons(ev, True)
                esdoom_lib.doom_send_mouse(0, 0, int(self.buttons))
                raise renpy.IgnoreEvent()
            if ev.type == pygame.MOUSEBUTTONUP:
                self._mouse_buttons(ev, False)
                esdoom_lib.doom_send_mouse(0, 0, int(self.buttons))
                raise renpy.IgnoreEvent()
            if ev.type == pygame.MOUSEMOTION:
                esdoom_mouse_delta()
                raise renpy.IgnoreEvent()
            return None


    esdoom_view = DoomDisplayable()


    def esdoom_arm_notify_hide(delay):
        global esdoom_notify_hide_at
        esdoom_notify_hide_at = time.time() + delay


    def esdoom_hide_notify_if_due():
        global esdoom_notify_hide_at
        if esdoom_notify_hide_at <= 0 or time.time() < esdoom_notify_hide_at:
            return
        esdoom_notify_hide_at = 0.0
        try:
            renpy.hide_screen("notify")
            renpy.restart_interaction()
        except Exception:
            pass


    def esdoom_pump():
        global esdoom_failed
        if not esdoom_running or esdoom_failed or esdoom_lib is None:
            return None
        esdoom_hide_notify_if_due()
        esdoom_flush_mouse()
        rc = esdoom_lib.doom_step()
        if rc == 1:
            renpy.end_interaction("exit")
            return None
        if rc != 0:
            esdoom_failed = True
            message = esdoom_last_error()
            rkk_note(message, "esdoom")
            try:
                renpy.notify(message)
            except Exception:
                pass
            renpy.end_interaction("error")
            return None
        esdoom_poll()
        renpy.redraw(esdoom_view, 0)
        return None


    def esdoom_session_begin():
        global esdoom_running
        global esdoom_failed
        global esdoom_error
        global esdoom_title
        global esdoom_view_w
        global esdoom_view_h
        global esdoom_status
        global esdoom_saw_status
        global esdoom_notify_hide_at
        global esdoom_run_doom
        global esdoom_controls

        esdoom_notify_hide_at = 0.0
        esdoom_run_doom = False
        esdoom_error = u""
        esdoom_failed = False
        esdoom_saw_status = False
        esdoom_status = None
        esdoom_view_w, esdoom_view_h = esdoom_viewport()

        problem = esdoom_load_lib()
        if problem:
            esdoom_error = problem
            return problem

        settings = esdoom_settings()
        esdoom_controls = settings["controls"]
        iwad, wad_error = esdoom_find_iwad(settings)
        if not iwad:
            esdoom_error = wad_error
            return esdoom_error

        config_dir = esdoom_config_dir()
        rc = esdoom_lib.doom_init(
            esdoom_cstr(iwad),
            esdoom_cstr(config_dir),
            esdoom_cstr(esdoom_launch_args(settings, iwad)),
        )
        if rc != 0:
            esdoom_error = esdoom_last_error()
            return esdoom_error

        esdoom_title = esdoom_text(esdoom_lib.doom_window_title())
        esdoom_running = True
        esdoom_lib.doom_set_volumes(
            int(settings["sfx_volume"]),
            int(settings["music_volume"]),
            int(settings["mouse_sensitivity"]),
        )
        esdoom_lib.doom_audio_active(1)
        esdoom_grab_input()
        esdoom_pause_audio(True)
        try:
            renpy.suspend_rollback(True)
        except Exception:
            pass
        rkk_report_mod_version(ESDOOM_LABEL, ESDOOM_VERSION)
        rkk_report_mod_title(ESDOOM_LABEL, "ES-DOOM")
        rkk_note(u"ES-DOOM session start", "esdoom")
        rkk_set_context("esdoom", u"title")
        rkk_emit("esdoom.session", {"state": "start", "iwad": os.path.basename(iwad)})
        esdoom_poll()
        return None


    def esdoom_session_end():
        global esdoom_running
        if not esdoom_running and esdoom_keymap is None:
            return
        esdoom_running = False
        if esdoom_lib is not None:
            esdoom_lib.doom_audio_active(0)
        rkk_note(u"ES-DOOM session stop", "esdoom")
        rkk_set_context("esdoom", u"")
        rkk_emit("esdoom.session", {"state": "stop"})
        esdoom_release_input()
        esdoom_pause_audio(False)
        try:
            renpy.suspend_rollback(False)
        except Exception:
            pass
        try:
            if renpy.config.main_menu_music:
                renpy.music.play(renpy.config.main_menu_music, if_changed=True)
        except Exception:
            pass


init 999 python:
    if getattr(store, "mods", None) is None:
        store.mods = {}
    store.mods["esdoom"] = u"ES-DOOM"
    rkk_report_mod_version("esdoom", ESDOOM_VERSION)
    rkk_report_mod_title("esdoom", "ES-DOOM")
    images = getattr(store, "modsImages", None)
    poly = getattr(store, "imgsModsMenu_polyMods", None)
    menu_image = os.path.join(esdoom_mod_dir(), "menu.png")
    if images is not None and os.path.isfile(menu_image):
        try:
            images["esdoom"] = ("mods/esdoom/menu.png", False, "")
        except Exception:
            images = None
    if poly is not None and images is not None and os.path.isfile(menu_image):
        try:
            if "esdoom" not in poly:
                poly.append("esdoom")
        except Exception:
            pass


screen esdoom_screen():
    modal True
    add Solid("#000")
    add esdoom_view:
        xalign 0.5
        yalign 0.5
        xsize esdoom_view_w
        ysize esdoom_view_h
        nearest True
    key "K_F12" action Return("exit")
    timer 0.028 repeat True action Function(esdoom_pump, _update_screens=False)


screen esdoom_notice(message):
    modal True
    add Solid("#000")
    text message:
        xalign 0.5
        yalign 0.45
        size 32
    text "Press to return to the menu":
        xalign 0.5
        yalign 0.55
        size 28
    key "mouseup_1" action Return("menu")
    key "K_RETURN" action Return("menu")
    key "K_KP_ENTER" action Return("menu")
    key "K_ESCAPE" action Return("menu")
    key "K_F12" action Return("menu")


label esdoom:
    $ esdoom_error = esdoom_session_begin()
    if esdoom_error:
        window hide
        call screen esdoom_notice(esdoom_error)
    else:
        window hide
        call screen esdoom_screen
    $ esdoom_session_end()
    scene black
    jump _invoke_main_menu
