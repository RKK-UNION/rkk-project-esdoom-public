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

#include "doomgeneric.h"
#include "doomkeys.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

#define KEYQUEUE_SIZE 16

#define SDLK_SCANCODE_MASK (1 << 30)
#define SDLK_FROM_SCANCODE(sc) ((sc) | SDLK_SCANCODE_MASK)

#define SDL_SCANCODE_LCTRL 224
#define SDL_SCANCODE_LSHIFT 225
#define SDL_SCANCODE_LALT 226
#define SDL_SCANCODE_RCTRL 228
#define SDL_SCANCODE_RSHIFT 229
#define SDL_SCANCODE_RALT 230
#define SDL_SCANCODE_F1 58
#define SDL_SCANCODE_F2 59
#define SDL_SCANCODE_F3 60
#define SDL_SCANCODE_F4 61
#define SDL_SCANCODE_F5 62
#define SDL_SCANCODE_F6 63
#define SDL_SCANCODE_F7 64
#define SDL_SCANCODE_F8 65
#define SDL_SCANCODE_F9 66
#define SDL_SCANCODE_F10 67
#define SDL_SCANCODE_F11 68
#define SDL_SCANCODE_F12 69
#define SDL_SCANCODE_LEFT 80
#define SDL_SCANCODE_RIGHT 79
#define SDL_SCANCODE_UP 82
#define SDL_SCANCODE_DOWN 81

#define SDLK_RETURN '\r'
#define SDLK_ESCAPE '\033'
#define SDLK_BACKSPACE '\b'
#define SDLK_TAB '\t'
#define SDLK_SPACE ' '
#define SDLK_MINUS '-'
#define SDLK_EQUALS '='
#define SDLK_LEFT SDLK_FROM_SCANCODE(SDL_SCANCODE_LEFT)
#define SDLK_RIGHT SDLK_FROM_SCANCODE(SDL_SCANCODE_RIGHT)
#define SDLK_UP SDLK_FROM_SCANCODE(SDL_SCANCODE_UP)
#define SDLK_DOWN SDLK_FROM_SCANCODE(SDL_SCANCODE_DOWN)
#define SDLK_LCTRL SDLK_FROM_SCANCODE(SDL_SCANCODE_LCTRL)
#define SDLK_RCTRL SDLK_FROM_SCANCODE(SDL_SCANCODE_RCTRL)
#define SDLK_LSHIFT SDLK_FROM_SCANCODE(SDL_SCANCODE_LSHIFT)
#define SDLK_RSHIFT SDLK_FROM_SCANCODE(SDL_SCANCODE_RSHIFT)
#define SDLK_LALT SDLK_FROM_SCANCODE(SDL_SCANCODE_LALT)
#define SDLK_RALT SDLK_FROM_SCANCODE(SDL_SCANCODE_RALT)
#define SDLK_F1 SDLK_FROM_SCANCODE(SDL_SCANCODE_F1)
#define SDLK_F2 SDLK_FROM_SCANCODE(SDL_SCANCODE_F2)
#define SDLK_F3 SDLK_FROM_SCANCODE(SDL_SCANCODE_F3)
#define SDLK_F4 SDLK_FROM_SCANCODE(SDL_SCANCODE_F4)
#define SDLK_F5 SDLK_FROM_SCANCODE(SDL_SCANCODE_F5)
#define SDLK_F6 SDLK_FROM_SCANCODE(SDL_SCANCODE_F6)
#define SDLK_F7 SDLK_FROM_SCANCODE(SDL_SCANCODE_F7)
#define SDLK_F8 SDLK_FROM_SCANCODE(SDL_SCANCODE_F8)
#define SDLK_F9 SDLK_FROM_SCANCODE(SDL_SCANCODE_F9)
#define SDLK_F10 SDLK_FROM_SCANCODE(SDL_SCANCODE_F10)
#define SDLK_F11 SDLK_FROM_SCANCODE(SDL_SCANCODE_F11)
#define SDLK_F12 SDLK_FROM_SCANCODE(SDL_SCANCODE_F12)

static unsigned short s_key_queue[KEYQUEUE_SIZE];
static unsigned int s_key_write = 0;
static unsigned int s_key_read = 0;

static char s_window_title[128];

#ifdef _WIN32
static LARGE_INTEGER s_qpc_freq;
static LARGE_INTEGER s_qpc_start;
#else
static struct timespec s_time_start;
#endif

static unsigned char convert_sdl_key(int key)
{
    switch (key) {
    case SDLK_RETURN:
        return KEY_ENTER;
    case SDLK_ESCAPE:
        return KEY_ESCAPE;
    case SDLK_LEFT:
        return KEY_LEFTARROW;
    case SDLK_RIGHT:
        return KEY_RIGHTARROW;
    case SDLK_UP:
        return KEY_UPARROW;
    case SDLK_DOWN:
        return KEY_DOWNARROW;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return KEY_FIRE;
    case SDLK_SPACE:
        return KEY_USE;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return KEY_RSHIFT;
    case SDLK_LALT:
    case SDLK_RALT:
        return KEY_LALT;
    case SDLK_TAB:
        return KEY_TAB;
    case SDLK_BACKSPACE:
        return KEY_BACKSPACE;
    case SDLK_F1:
        return KEY_F1;
    case SDLK_F2:
        return KEY_F2;
    case SDLK_F3:
        return KEY_F3;
    case SDLK_F4:
        return KEY_F4;
    case SDLK_F5:
        return KEY_F5;
    case SDLK_F6:
        return KEY_F6;
    case SDLK_F7:
        return KEY_F7;
    case SDLK_F8:
        return KEY_F8;
    case SDLK_F9:
        return KEY_F9;
    case SDLK_F10:
        return KEY_F10;
    case SDLK_F11:
        return KEY_F11;
    case SDLK_F12:
        return KEY_F12;
    case SDLK_EQUALS:
        return KEY_EQUALS;
    case SDLK_MINUS:
        return KEY_MINUS;
    default:
        if (key >= 0 && key < 128)
            return (unsigned char)tolower(key);
        return 0;
    }
}

void DG_QueueKey(int pressed, int sdl_key)
{
    unsigned char key;
    unsigned short packed;
    unsigned int next;

    key = convert_sdl_key(sdl_key);
    if (key == 0)
        return;

    next = (s_key_write + 1) % KEYQUEUE_SIZE;
    if (next == s_key_read)
        return;

    packed = (unsigned short)(((pressed ? 1 : 0) << 8) | key);
    s_key_queue[s_key_write] = packed;
    s_key_write = next;
}

const char *DG_WindowTitle(void)
{
    return s_window_title;
}

void DG_Init(void)
{
    memset(s_key_queue, 0, sizeof(s_key_queue));
    s_key_write = 0;
    s_key_read = 0;
    s_window_title[0] = '\0';

#ifdef _WIN32
    QueryPerformanceFrequency(&s_qpc_freq);
    QueryPerformanceCounter(&s_qpc_start);
#else
    clock_gettime(CLOCK_MONOTONIC, &s_time_start);
#endif
}

void DG_DrawFrame(void)
{
}

void DG_SleepMs(uint32_t ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    nanosleep(&ts, NULL);
#endif
}

uint32_t DG_GetTicksMs(void)
{
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (s_qpc_freq.QuadPart == 0)
        return 0;
    return (uint32_t)(((now.QuadPart - s_qpc_start.QuadPart) * 1000) / s_qpc_freq.QuadPart);
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint32_t)((now.tv_sec - s_time_start.tv_sec) * 1000
        + (now.tv_nsec - s_time_start.tv_nsec) / 1000000);
#endif
}

int DG_GetKey(int *pressed, unsigned char *doomKey)
{
    unsigned short packed;

    if (s_key_read == s_key_write)
        return 0;

    packed = s_key_queue[s_key_read];
    s_key_read = (s_key_read + 1) % KEYQUEUE_SIZE;
    *pressed = packed >> 8;
    *doomKey = (unsigned char)(packed & 0xFF);
    return 1;
}

void DG_SetWindowTitle(const char *title)
{
    if (title == NULL)
        title = "";
    strncpy(s_window_title, title, sizeof(s_window_title) - 1);
    s_window_title[sizeof(s_window_title) - 1] = '\0';
}
