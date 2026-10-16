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

#ifndef OPL_HOST_H
#define OPL_HOST_H

#include <stdint.h>

/* Wait until the mixer has advanced the OPL clock by us microseconds. */
void OPL_Host_Delay(uint64_t us);

/* Freeze the OPL clock without closing the chip. */
void OPL_Host_Suspend(int suspend);

/*
 * Render OPL into dst (stereo int16, added to what is already there).
 * dst == NULL advances the clock and drops the samples.
 */
void OPL_Host_Mix(int16_t *dst, int frames);

#endif
