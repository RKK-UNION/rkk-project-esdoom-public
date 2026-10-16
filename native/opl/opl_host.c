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

/* Nuked OPL3 backend for the Chocolate Doom music module.
   The clock advances only when the host mixer asks for samples, so music
   stays locked to the same stream as the sound effects. */

#include "opl_host.h"

#include "opl.h"
#include "opl3.h"
#include "opl_internal.h"
#include "opl_queue.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
__asm__(".symver pthread_mutex_lock,pthread_mutex_lock@GLIBC_2.2.5");
__asm__(".symver pthread_mutex_unlock,pthread_mutex_unlock@GLIBC_2.2.5");
__asm__(".symver pthread_mutex_init,pthread_mutex_init@GLIBC_2.2.5");
#endif

#define OPL_SLICE_MAX 4096

typedef struct {
    unsigned int rate;
    unsigned int enabled;
    unsigned int value;
    uint64_t expire_time;
} opl_timer_t;

#ifdef _WIN32
static CRITICAL_SECTION callback_mutex;
static CRITICAL_SECTION queue_mutex;
#else
static pthread_mutex_t callback_mutex;
static pthread_mutex_t queue_mutex;
#endif

static opl_callback_queue_t *callback_queue;
static uint64_t current_time;
static int opl_paused;
static uint64_t pause_offset;
static opl3_chip opl_chip;
static int opl_opl3mode;
static int16_t *mix_buffer;
static unsigned int mixing_freq;
static int register_num;
static volatile int opl_live;
static volatile int opl_hold;
static volatile int delay_done;

static opl_timer_t timer1 = { 12500, 0, 0, 0 };
static opl_timer_t timer2 = { 3125, 0, 0, 0 };

static void queue_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&queue_mutex);
#else
    pthread_mutex_lock(&queue_mutex);
#endif
}

static void queue_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&queue_mutex);
#else
    pthread_mutex_unlock(&queue_mutex);
#endif
}

static void callback_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&callback_mutex);
#else
    pthread_mutex_lock(&callback_mutex);
#endif
}

static void callback_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&callback_mutex);
#else
    pthread_mutex_unlock(&callback_mutex);
#endif
}

static void AdvanceTime(unsigned int nsamples)
{
    opl_callback_t callback;
    void *callback_data;
    uint64_t us;

    queue_lock();
    us = ((uint64_t)nsamples * OPL_SECOND) / mixing_freq;
    current_time += us;
    if (opl_paused)
        pause_offset += us;

    while (!OPL_Queue_IsEmpty(callback_queue)
        && current_time >= OPL_Queue_Peek(callback_queue) + pause_offset) {
        if (!OPL_Queue_Pop(callback_queue, &callback, &callback_data))
            break;
        queue_unlock();
        callback_lock();
        callback(callback_data);
        callback_unlock();
        queue_lock();
    }
    queue_unlock();
}

static void FillBuffer(int16_t *buffer, unsigned int nsamples)
{
    OPL3_GenerateStream(&opl_chip, mix_buffer, nsamples);
    if (buffer == NULL)
        return;
    {
        unsigned int i;
        for (i = 0; i < nsamples * 2; ++i) {
            int mixed = (int)buffer[i] + (int)mix_buffer[i];
            if (mixed > 32767)
                mixed = 32767;
            if (mixed < -32768)
                mixed = -32768;
            buffer[i] = (int16_t)mixed;
        }
    }
}

static void Render(int16_t *dst, int frames)
{
    int filled = 0;

    if (!opl_live || opl_hold || frames <= 0)
        return;

    while (filled < frames) {
        uint64_t next_callback_time;
        uint64_t nsamples;

        queue_lock();
        if (opl_paused || OPL_Queue_IsEmpty(callback_queue)) {
            nsamples = (uint64_t)(frames - filled);
        } else {
            next_callback_time = OPL_Queue_Peek(callback_queue) + pause_offset;
            nsamples = (next_callback_time - current_time) * mixing_freq;
            nsamples = (nsamples + OPL_SECOND - 1) / OPL_SECOND;
            if (nsamples > (uint64_t)(frames - filled))
                nsamples = (uint64_t)(frames - filled);
            if (nsamples == 0)
                nsamples = 1;
        }
        queue_unlock();

        if (nsamples > OPL_SLICE_MAX)
            nsamples = OPL_SLICE_MAX;
        FillBuffer(dst == NULL ? NULL : dst + filled * 2, (unsigned int)nsamples);
        filled += (int)nsamples;
        AdvanceTime((unsigned int)nsamples);
    }
}

void OPL_Host_Mix(int16_t *dst, int frames)
{
    Render(dst, frames);
}

void OPL_Host_Suspend(int suspend)
{
    opl_hold = suspend ? 1 : 0;
}

static void DelayCallback(void *unused)
{
    (void)unused;
    delay_done = 1;
}

void OPL_Host_Delay(uint64_t us)
{
    int spins = 0;

    delay_done = 0;
    OPL_SetCallback(us, DelayCallback, NULL);
    while (!delay_done && spins < 4000) {
#ifdef _WIN32
        Sleep(1);
#else
        struct timespec pause;
        pause.tv_sec = 0;
        pause.tv_nsec = 1000000L;
        nanosleep(&pause, NULL);
#endif
        spins++;
    }
}

static unsigned int PortRead(opl_port_t port)
{
    unsigned int result = 0;

    if (port == OPL_REGISTER_PORT_OPL3)
        return 0xff;

    if (timer1.enabled && current_time > timer1.expire_time) {
        result |= 0x80;
        result |= 0x40;
    }
    if (timer2.enabled && current_time > timer2.expire_time) {
        result |= 0x80;
        result |= 0x20;
    }
    return result;
}

static void TimerEnd(opl_timer_t *timer)
{
    int tics;

    if (!timer->enabled)
        return;
    tics = 0x100 - (int)timer->value;
    timer->expire_time = current_time + ((uint64_t)tics * OPL_SECOND) / timer->rate;
}

static void WriteRegister(unsigned int reg_num, unsigned int value)
{
    switch (reg_num) {
    case OPL_REG_TIMER1:
        timer1.value = value;
        TimerEnd(&timer1);
        break;
    case OPL_REG_TIMER2:
        timer2.value = value;
        TimerEnd(&timer2);
        break;
    case OPL_REG_TIMER_CTRL:
        if (value & 0x80) {
            timer1.enabled = 0;
            timer2.enabled = 0;
        } else {
            if ((value & 0x40) == 0) {
                timer1.enabled = (value & 0x01) != 0;
                TimerEnd(&timer1);
            }
            if ((value & 0x20) == 0) {
                timer2.enabled = (value & 0x02) != 0;
                TimerEnd(&timer2);
            }
        }
        break;
    case OPL_REG_NEW:
        opl_opl3mode = value & 0x01;
        /* fall through */
    default:
        OPL3_WriteRegBuffered(&opl_chip, (Bit16u)reg_num, (Bit8u)value);
        break;
    }
}

static void PortWrite(opl_port_t port, unsigned int value)
{
    if (port == OPL_REGISTER_PORT)
        register_num = (int)value;
    else if (port == OPL_REGISTER_PORT_OPL3)
        register_num = (int)value | 0x100;
    else if (port == OPL_DATA_PORT)
        WriteRegister((unsigned int)register_num, value);
}

static void SetCallback(uint64_t us, opl_callback_t callback, void *data)
{
    queue_lock();
    OPL_Queue_Push(callback_queue, callback, data, current_time - pause_offset + us);
    queue_unlock();
}

static void ClearCallbacks(void)
{
    queue_lock();
    OPL_Queue_Clear(callback_queue);
    queue_unlock();
}

static void Lock(void)
{
    callback_lock();
}

static void Unlock(void)
{
    callback_unlock();
}

static void SetPaused(int paused)
{
    opl_paused = paused;
}

static void AdjustCallbacks(float factor)
{
    queue_lock();
    OPL_Queue_AdjustCallbacks(callback_queue, current_time, factor);
    queue_unlock();
}

static void Shutdown(void)
{
    opl_live = 0;
}

static int Init(unsigned int port_base)
{
    (void)port_base;
    mixing_freq = opl_sample_rate != 0 ? opl_sample_rate : 22050;
#ifdef _WIN32
    InitializeCriticalSection(&callback_mutex);
    InitializeCriticalSection(&queue_mutex);
#else
    pthread_mutex_init(&callback_mutex, NULL);
    pthread_mutex_init(&queue_mutex, NULL);
#endif
    callback_queue = OPL_Queue_Create();
    current_time = 0;
    opl_paused = 0;
    pause_offset = 0;
    register_num = 0;
    timer1.enabled = 0;
    timer2.enabled = 0;
    mix_buffer = malloc((size_t)mixing_freq * 2 * sizeof(int16_t));
    if (mix_buffer == NULL || callback_queue == NULL)
        return 0;
    OPL3_Reset(&opl_chip, mixing_freq);
    opl_opl3mode = 0;
    opl_hold = 0;
    opl_live = 1;
    return 1;
}

opl_driver_t opl_host_driver = {
    "host",
    Init,
    Shutdown,
    PortRead,
    PortWrite,
    SetCallback,
    ClearCallbacks,
    Lock,
    Unlock,
    SetPaused,
    AdjustCallbacks,
};
