from __future__ import print_function

import ctypes
import os
import sys
import time


def cstr(text):
    if isinstance(text, bytes):
        return text
    return text.encode("utf-8")


def text(raw):
    if not raw:
        return u""
    if not isinstance(raw, bytes):
        return raw
    return raw.decode("utf-8", "replace")


class DoomStatus(ctypes.Structure):
    _fields_ = [
        ("gamestate", ctypes.c_int),
        ("episode", ctypes.c_int),
        ("map", ctypes.c_int),
        ("health", ctypes.c_int),
        ("playerstate", ctypes.c_int),
        ("killcount", ctypes.c_int),
    ]


def load():
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "game", "mods", "esdoom"))
    if os.name == "nt":
        path = os.path.join(root, "rkk_esdoom_lib.dll")
    else:
        path = os.path.join(root, "rkk_esdoom_lib.so")
    lib = ctypes.CDLL(path)
    lib.doom_init.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]
    lib.doom_init.restype = ctypes.c_int
    lib.doom_step.argtypes = []
    lib.doom_step.restype = ctypes.c_int
    lib.doom_send_key.argtypes = [ctypes.c_int, ctypes.c_int]
    lib.doom_send_key.restype = None
    lib.doom_send_mouse.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int]
    lib.doom_send_mouse.restype = None
    lib.doom_framebuffer.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
    lib.doom_framebuffer.restype = ctypes.POINTER(ctypes.c_uint32)
    lib.doom_query.argtypes = [ctypes.POINTER(DoomStatus)]
    lib.doom_query.restype = None
    lib.doom_last_error.argtypes = []
    lib.doom_last_error.restype = ctypes.c_char_p
    return lib


def nonzero_frame(lib):
    w = ctypes.c_int()
    h = ctypes.c_int()
    ptr = lib.doom_framebuffer(ctypes.byref(w), ctypes.byref(h))
    if not ptr or w.value <= 0 or h.value <= 0:
        return 0, 0, 0
    count = int(w.value) * int(h.value)
    raw = ctypes.string_at(ptr, count * 4)
    lit = 0
    i = 0
    while i < len(raw):
        if raw[i] or raw[i + 1] or raw[i + 2]:
            lit += 1
        i += 4
    return int(w.value), int(h.value), lit


def main():
    mode = sys.argv[1]
    lib = load()
    if mode == "missing":
        cfg = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "native", "build-win", "smoke-cfg"))
        if not os.path.isdir(cfg):
            os.makedirs(cfg)
        rc = lib.doom_init(cstr(os.path.join(cfg, "no-such.wad")), cstr(cfg), None)
        err = text(lib.doom_last_error())
        print("missing_rc=%s" % rc)
        print("missing_err=%s" % err)
        print("still_alive=1")
        return 0 if rc != 0 and err else 1

    if mode == "play":
        iwad = sys.argv[2]
        cfg = sys.argv[3]
        if not os.path.isdir(cfg):
            os.makedirs(cfg)
        rc = lib.doom_init(cstr(iwad), cstr(cfg), None)
        print("init_rc=%s" % rc)
        if rc != 0:
            print("init_err=%s" % text(lib.doom_last_error()))
            return 1
        w, h, lit = nonzero_frame(lib)
        print("frame=%sx%s lit=%s" % (w, h, lit))
        lib.doom_send_key(119, 1)  # SDLK_w
        lib.doom_send_key(119, 0)
        lib.doom_send_mouse(8, -3, 1)
        started = time.time()
        changed = 0
        last = lit
        for _i in range(35):
            step = lib.doom_step()
            if step != 0:
                print("step_err=%s" % text(lib.doom_last_error()))
                return 1
            w, h, lit = nonzero_frame(lib)
            if lit != last:
                changed += 1
                last = lit
            time.sleep(0.028)
        elapsed = time.time() - started
        status = DoomStatus()
        lib.doom_query(ctypes.byref(status))
        print("steps_elapsed=%.3f changed=%s lit=%s" % (elapsed, changed, lit))
        print("state=%s ep=%s map=%s health=%s pst=%s kills=%s" % (
            status.gamestate, status.episode, status.map,
            status.health, status.playerstate, status.killcount))
        title = text(lib.doom_window_title())
        print("title=%s" % title)
        if w != 640 or h != 400 or lit < 100:
            return 1
        if elapsed < 0.7:
            print("too_fast=1")
            return 1
        return 0

    print("usage: smoke_test.py missing|play IWAD CFG")
    return 2


if __name__ == "__main__":
    sys.exit(main())
