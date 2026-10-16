"""Copy doomgeneric sources and apply the hosted-engine patches.

external/ stays untouched. The copy lives in the CMake build directory.
"""

from __future__ import print_function

import os
import shutil
import sys

SKIP_C = {
    "i_sdlsound.c",
    "i_sdlmusic.c",
    "i_allegrosound.c",
    "i_allegromusic.c",
}


def _read(path):
    with open(path, "rb") as handle:
        data = handle.read()
    return data.replace(b"\r\n", b"\n").replace(b"\r", b"\n")


def _write(path, data):
    with open(path, "wb") as handle:
        handle.write(data)


def _replace(path, old, new, label):
    data = _read(path)
    old_b = old.encode("utf-8")
    new_b = new.encode("utf-8")
    count = data.count(old_b)
    if count != 1:
        raise SystemExit("%s: expected 1 match for %s, found %d" % (path, label, count))
    _write(path, data.replace(old_b, new_b, 1))


def copy_tree(src, dst):
    if os.path.isdir(dst):
        shutil.rmtree(dst)
    os.makedirs(dst)
    for name in os.listdir(src):
        if name.startswith("doomgeneric_") and name.endswith(".c"):
            continue
        if name in SKIP_C:
            continue
        if not (name.endswith(".c") or name.endswith(".h")):
            continue
        shutil.copyfile(os.path.join(src, name), os.path.join(dst, name))


def patch(dst):
    # MinGW's string.h defines inline strncasecmp() { return _strnicmp(...); }.
    # doomtype.h's #define strncasecmp _strnicmp rewrites that inline into a
    # function that calls itself, and GCC -O2 turns the call into an infinite
    # jump. Include the system header and leave its inline alone.
    types = os.path.join(dst, "doomtype.h")
    _replace(
        types,
        "#ifdef _WIN32\n"
        "\n"
        "#define strcasecmp _stricmp\n"
        "#define strncasecmp _strnicmp\n"
        "\n"
        "#else\n",
        "#ifdef _WIN32\n"
        "\n"
        "#include <string.h>\n"
        "\n"
        "#else\n",
        "strcasecmp",
    )

    sound = os.path.join(dst, "i_sound.c")
    _replace(
        sound,
        "#if defined(FEATURE_SOUND) && !defined(__DJGPP__)\n"
        "#include <SDL_mixer.h>\n"
        "#endif\n",
        "/* Sound is mixed by i_renpysound.c, not SDL_mixer. */\n"
        "extern int use_libsamplerate;\n"
        "extern float libsamplerate_scale;\n",
        "SDL_mixer include",
    )
    _replace(
        sound,
        "    music_module = &DG_music_module;\n",
        "    music_module = &music_opl_module;\n",
        "OPL music module",
    )

    video = os.path.join(dst, "i_video.c")
    _replace(
        video,
        "            pix = (c.r << s_Fb.red.offset) |\n"
        "                  (c.g << s_Fb.green.offset) |\n"
        "                  (c.b << s_Fb.blue.offset);",
        "            pix = (c.r << s_Fb.red.offset) |\n"
        "                  (c.g << s_Fb.green.offset) |\n"
        "                  (c.b << s_Fb.blue.offset) |\n"
        "                  (0xFFu << s_Fb.transp.offset);",
        "pixel alpha",
    )

    misc = os.path.join(dst, "m_misc.c")
    _replace(
        misc,
        "#ifdef _MSC_VER\n"
        "#include <direct.h>\n"
        "#endif",
        "#include <direct.h>\n",
        "direct.h",
    )

    config = os.path.join(dst, "m_config.c")
    _replace(
        config,
        "static char *GetDefaultConfigDir(void)\n"
        "{\n"
        "    char *result = (char *)malloc(2);\n"
        "    result[0] = '.';\n"
        "    result[1] = '\\0';\n"
        "\n"
        "    return result;\n"
        "}",
        "static char *GetDefaultConfigDir(void)\n"
        "{\n"
        "    extern const char *DG_HostConfigDir(void);\n"
        "    const char *host = DG_HostConfigDir();\n"
        "    char *result;\n"
        "\n"
        "    if (host != NULL && host[0] != '\\0')\n"
        "        return M_StringDuplicate(host);\n"
        "\n"
        "    result = (char *)malloc(2);\n"
        "    result[0] = '.';\n"
        "    result[1] = '\\0';\n"
        "    return result;\n"
        "}",
        "config dir",
    )

    system = os.path.join(dst, "i_system.c")
    _replace(
        system,
        "        fprintf(stderr, \"Warning: recursive call to I_Error detected.\\n\");\n"
        "#if ORIGCODE\n"
        "        exit(-1);\n"
        "#endif\n"
        "    }\n",
        "        fprintf(stderr, \"Warning: recursive call to I_Error detected.\\n\");\n"
        "#if ORIGCODE\n"
        "        exit(-1);\n"
        "#endif\n"
        "        return;\n"
        "    }\n",
        "recursive I_Error",
    )
    _replace(
        system,
        "void I_Quit (void)\n"
        "{\n"
        "    atexit_listentry_t *entry;\n"
        "\n"
        "    // Run through all exit functions\n"
        " \n"
        "    entry = exit_funcs; \n"
        "\n"
        "    while (entry != NULL)\n"
        "    {\n"
        "        entry->func();\n"
        "        entry = entry->next;\n"
        "    }\n"
        "\n"
        "#if ORIGCODE\n"
        "    SDL_Quit();\n"
        "\n"
        "    exit(0);\n"
        "#endif\n"
        "}",
        "void I_Quit (void)\n"
        "{\n"
        "    extern void DG_HostQuit(void);\n"
        "    DG_HostQuit();\n"
        "}",
        "I_Quit",
    )

    main = os.path.join(dst, "d_main.c")
    _replace(
        main,
        "    I_Endoom(endoom);\n"
        "\n"
        "\texit(0);\n",
        "    I_Endoom(endoom);\n",
        "ENDOOM exit",
    )

    data = _read(system).decode("utf-8")
    start = data.find('    exit_gui_popup = !M_ParmExists("-nogui");')
    end_token = "// Read Access Violation emulation."
    end = data.find(end_token, start if start >= 0 else 0)
    if start < 0 or end < 0:
        raise SystemExit("I_Error tail not found in %s" % system)
    replacement = (
        "    already_quitting = false;\n"
        "    {\n"
        "        extern void DG_HostFatal(const char *msg);\n"
        "        DG_HostFatal(msgbuf);\n"
        "    }\n"
        "    return;\n"
        "}\n"
        "\n"
    )
    _write(system, (data[:start] + replacement + data[end:]).encode("utf-8"))


def main():
    if len(sys.argv) != 3:
        raise SystemExit("usage: apply_hosted.py SRC_DIR DST_DIR")
    src = sys.argv[1]
    dst = sys.argv[2]
    if not os.path.isdir(src):
        raise SystemExit("missing doomgeneric sources: %s" % src)
    copy_tree(src, dst)
    patch(dst)
    print("hosted sources: %s" % dst)


if __name__ == "__main__":
    main()
