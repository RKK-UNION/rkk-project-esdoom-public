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

#ifndef OPL_COMPAT_H
#define OPL_COMPAT_H

#include <stdio.h>
#include <stdlib.h>

/* Chocolate Doom 3.1.1 calls SDL endian helpers and a couple of wrappers
   that doomgeneric does not provide. */
static inline unsigned short opl_swap16(unsigned short value)
{
    return (unsigned short)((value << 8) | (value >> 8));
}

static inline unsigned int opl_swap32(unsigned int value)
{
    return (value << 24) | ((value << 8) & 0x00ff0000u) | ((value >> 8) & 0x0000ff00u) | (value >> 24);
}

#define SDL_SwapBE16(value) opl_swap16((unsigned short)(value))
#define SDL_SwapBE32(value) opl_swap32((unsigned int)(value))
#define M_fopen fopen
#define I_Realloc realloc

/* doomgeneric's doomtype.h has no PACKED_STRUCT. Chocolate Doom 3.1.1 uses it
   for the GENMIDI and MIDI chunk layouts, which must stay byte-packed. */
#ifndef PACKED_STRUCT
#if defined(_WIN32) && !defined(__clang__)
#define PACKED_STRUCT(...) struct __VA_ARGS__ __attribute__((packed, gcc_struct))
#else
#define PACKED_STRUCT(...) struct __VA_ARGS__ __attribute__((packed))
#endif
#endif

typedef enum {
    opl_doom1_1_666,
    opl_doom2_1_666,
    opl_doom_1_9
} opl_driver_ver_t;

#endif
