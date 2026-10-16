/*
ES-DOOM
Copyright (C) RKK UNION

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "doom_bridge.h"

#include "d_event.h"
#include "doomgeneric.h"
#include "doomstat.h"
#include "s_sound.h"

#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#define ESDOOM_PATH_MAX 1024

static jmp_buf g_jmp;
static volatile int g_jmp_ready;
static volatile int g_quit;
static int g_started;
static char g_error[512];
static char g_config_dir[ESDOOM_PATH_MAX];
static char g_iwad[ESDOOM_PATH_MAX];
#define ESDOOM_EXTRA_MAX 24

static char g_arg0[] = "esdoom";
static char g_flag_iwad[] = "-iwad";
static char g_flag_nogui[] = "-nogui";
static char g_extra_store[ESDOOM_EXTRA_MAX][ESDOOM_PATH_MAX];
static char *g_argv[4 + ESDOOM_EXTRA_MAX + 1];

extern void DG_QueueKey(int pressed, int sdl_key);
extern const char *DG_WindowTitle(void);
extern int messageToPrint;
extern boolean menuactive;

static void set_error(const char *msg)
{
    if (msg == NULL)
        msg = "ES-DOOM error";
    snprintf(g_error, sizeof(g_error), "%s", msg);
}

const char *DG_HostConfigDir(void)
{
    if (g_config_dir[0] == '\0')
        return NULL;
    return g_config_dir;
}

void DG_HostFatal(const char *msg)
{
    set_error(msg);
    if (g_jmp_ready)
        longjmp(g_jmp, 1);
    fprintf(stderr, "ES-DOOM: %s\n", g_error);
}

void DG_HostQuit(void)
{
    messageToPrint = 0;
    menuactive = false;
    g_quit = 1;
    if (g_jmp_ready)
        longjmp(g_jmp, 2);
}

static int copy_path(char *dst, const char *src, const char *what)
{
    size_t n;

    if (src == NULL || src[0] == '\0') {
        snprintf(g_error, sizeof(g_error), "%s path is empty", what);
        return -1;
    }
    n = strlen(src);
    if (n >= ESDOOM_PATH_MAX) {
        snprintf(g_error, sizeof(g_error), "%s path is too long", what);
        return -1;
    }
    memcpy(dst, src, n + 1);
    return 0;
}

static void ensure_trailing_sep(char *path)
{
    size_t n = strlen(path);
    char sep;

    if (n == 0 || n + 1 >= ESDOOM_PATH_MAX)
        return;
    if (path[n - 1] == '/' || path[n - 1] == '\\')
        return;
#ifdef _WIN32
    sep = '\\';
#else
    sep = '/';
#endif
    path[n] = sep;
    path[n + 1] = '\0';
}

static int append_extra(int argc, const char *args)
{
    int slot = 0;
    const char *cursor;

    if (args == NULL)
        return argc;

    cursor = args;
    while (*cursor && slot < ESDOOM_EXTRA_MAX) {
        const char *start;
        size_t length;

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
            cursor++;
        if (*cursor == '\0')
            break;
        start = cursor;
        while (*cursor && *cursor != ' ' && *cursor != '\t' && *cursor != '\n' && *cursor != '\r')
            cursor++;
        length = (size_t)(cursor - start);
        if (length >= ESDOOM_PATH_MAX) {
            snprintf(g_error, sizeof(g_error), "launch argument is too long");
            return -1;
        }
        memcpy(g_extra_store[slot], start, length);
        g_extra_store[slot][length] = '\0';
        g_argv[argc++] = g_extra_store[slot];
        slot++;
    }
    if (slot == ESDOOM_EXTRA_MAX) {
        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
            cursor++;
        if (*cursor != '\0') {
            snprintf(g_error, sizeof(g_error), "too many launch arguments");
            return -1;
        }
    }
    return argc;
}

int doom_init(const char *iwad, const char *config_dir, const char *args)
{
    int argc;

    g_quit = 0;
    if (g_started)
        return 0;

    g_error[0] = '\0';
    if (copy_path(g_iwad, iwad, "IWAD") != 0)
        return -1;
    if (copy_path(g_config_dir, config_dir, "config") != 0)
        return -1;
    ensure_trailing_sep(g_config_dir);

    g_argv[0] = g_arg0;
    g_argv[1] = g_flag_iwad;
    g_argv[2] = g_iwad;
    g_argv[3] = g_flag_nogui;
    argc = append_extra(4, args);
    if (argc < 0)
        return -1;
    g_argv[argc] = NULL;

    g_jmp_ready = 1;
    if (setjmp(g_jmp) != 0) {
        g_jmp_ready = 0;
        g_started = 0;
        return -1;
    }

    doomgeneric_Create(argc, g_argv);
    g_jmp_ready = 0;
    g_started = 1;
    return 0;
}

void doom_set_volumes(int sfx, int music, int mouse)
{
    if (!g_started)
        return;
    if (sfx < 0)
        sfx = 0;
    if (sfx > 15)
        sfx = 15;
    if (music < 0)
        music = 0;
    if (music > 15)
        music = 15;
    if (mouse < 0)
        mouse = 0;
    if (mouse > 9)
        mouse = 9;
    sfxVolume = sfx;
    musicVolume = music;
    mouseSensitivity = mouse;
    S_SetSfxVolume(sfx * 8);
    S_SetMusicVolume(music * 8);
}

int doom_step(void)
{
    int jumped;

    if (g_quit)
        return 1;
    if (!g_started) {
        set_error("ES-DOOM is not running");
        return -1;
    }

    g_jmp_ready = 1;
    jumped = setjmp(g_jmp);
    if (jumped != 0) {
        g_jmp_ready = 0;
        if (jumped == 2)
            return 1;
        g_started = 0;
        return -1;
    }

    doomgeneric_Tick();
    g_jmp_ready = 0;
    if (g_quit)
        return 1;
    return 0;
}

void doom_send_key(int sdl_key, int pressed)
{
    if (!g_started)
        return;
    DG_QueueKey(pressed, sdl_key);
}

void doom_send_mouse(int dx, int dy, int buttons)
{
    event_t event;

    if (!g_started)
        return;

    memset(&event, 0, sizeof(event));
    event.type = ev_mouse;
    event.data1 = buttons;
    event.data2 = dx;
    event.data3 = -dy;
    D_PostEvent(&event);
}

uint32_t *doom_framebuffer(int *w, int *h)
{
    if (w != NULL)
        *w = DOOMGENERIC_RESX;
    if (h != NULL)
        *h = DOOMGENERIC_RESY;
    return (uint32_t *)DG_ScreenBuffer;
}

void doom_query(DoomStatus *out)
{
    player_t *player;

    if (out == NULL)
        return;

    memset(out, 0, sizeof(*out));
    if (!g_started)
        return;

    out->gamestate = (int)gamestate;
    out->episode = gameepisode;
    out->map = gamemap;
    if (consoleplayer >= 0 && consoleplayer < MAXPLAYERS) {
        player = &players[consoleplayer];
        out->health = player->health;
        out->playerstate = (int)player->playerstate;
        out->killcount = player->killcount;
    }
}

const char *doom_last_error(void)
{
    return g_error;
}

const char *doom_window_title(void)
{
    return DG_WindowTitle();
}

extern void DG_AudioActive(int active);

void doom_audio_active(int active)
{
    DG_AudioActive(active);
}
