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

#ifndef ESDOOM_BRIDGE_H
#define ESDOOM_BRIDGE_H

#include <stdint.h>

#if defined(_WIN32)
#define DOOM_EXPORT __declspec(dllexport)
#else
#define DOOM_EXPORT __attribute__((visibility("default")))
#endif

typedef struct DoomStatus {
    int gamestate;
    int episode;
    int map;
    int health;
    int playerstate;
    int killcount;
} DoomStatus;

/* args is a whitespace-separated list of extra Doom arguments, or NULL. */
DOOM_EXPORT int doom_init(const char *iwad, const char *config_dir, const char *args);
DOOM_EXPORT void doom_set_volumes(int sfx, int music, int mouse);
DOOM_EXPORT int doom_step(void);
DOOM_EXPORT void doom_send_key(int sdl_key, int pressed);
DOOM_EXPORT void doom_send_mouse(int dx, int dy, int buttons);
DOOM_EXPORT uint32_t *doom_framebuffer(int *w, int *h);
DOOM_EXPORT void doom_query(DoomStatus *out);
DOOM_EXPORT const char *doom_last_error(void);
DOOM_EXPORT const char *doom_window_title(void);
DOOM_EXPORT void doom_audio_active(int active);

#endif
