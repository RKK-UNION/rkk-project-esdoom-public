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

#include "deh_str.h"
#include "i_sound.h"
#include "m_misc.h"
#include "opl_host.h"
#include "w_wad.h"
#include "z_zone.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MIX_RATE 22050
#define MIX_FRAMES 512
#define MIX_VOICES 16

int use_libsamplerate = 0;
float libsamplerate_scale = 0.65f;
char *timidity_cfg_path = "";

typedef struct {
    int16_t *pcm;
    int length;
} es_sample_t;

typedef struct {
    es_sample_t *sample;
    int pos;
    int left;
    int right;
    int active;
} es_voice_t;

static es_voice_t g_voices[MIX_VOICES];
static int g_ready;
static int g_audible;
static int g_use_prefix;
static volatile int g_device_run;

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

static CRITICAL_SECTION g_lock;
static HWAVEOUT g_wave;
static WAVEHDR g_hdrs[4];
static int16_t g_wave_buf[4][MIX_FRAMES * 2];

static HMIDIOUT g_midi;
static HANDLE g_music_thread;
#else
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>

/*
    MAGIC ASM SYMVERS TO LOAD OLD VERSIONS OF THESE FUNCTIONS ON GLIBC 2.31 
*/
__asm__(".symver dlopen,dlopen@GLIBC_2.2.5");
__asm__(".symver dlsym,dlsym@GLIBC_2.2.5");
__asm__(".symver dlclose,dlclose@GLIBC_2.2.5");
__asm__(".symver pthread_create,pthread_create@GLIBC_2.2.5");
__asm__(".symver pthread_join,pthread_join@GLIBC_2.2.5");

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_t g_audio_thread;
static void *g_alsa;
typedef struct es_pcm es_pcm_t;
static es_pcm_t *g_pcm;
static int (*g_pcm_open)(es_pcm_t **pcm, const char *name, int stream, int mode);
static int (*g_pcm_set_params)(es_pcm_t *pcm, int format, int access, unsigned int channels, unsigned int rate, int soft_resample, unsigned int latency);
static long (*g_pcm_writei)(es_pcm_t *pcm, const void *buffer, unsigned long frames);
static int (*g_pcm_prepare)(es_pcm_t *pcm);
static int (*g_pcm_drop)(es_pcm_t *pcm);
static int (*g_pcm_close)(es_pcm_t *pcm);
#endif

static void mix_lock(void)
{
#ifdef _WIN32
    EnterCriticalSection(&g_lock);
#else
    pthread_mutex_lock(&g_lock);
#endif
}

static void mix_unlock(void)
{
#ifdef _WIN32
    LeaveCriticalSection(&g_lock);
#else
    pthread_mutex_unlock(&g_lock);
#endif
}

static void ensure_lock(void)
{
    if (g_ready)
        return;
#ifdef _WIN32
    InitializeCriticalSection(&g_lock);
#endif
    g_ready = 1;
}

static void mix_frames(int16_t *dst, int frames)
{
    int frame;
    int channel;

    mix_lock();
    if (!g_audible) {
        mix_unlock();
        memset(dst, 0, (size_t)frames * 2 * sizeof(int16_t));
        OPL_Host_Mix(NULL, frames);
        return;
    }

    for (frame = 0; frame < frames; ++frame) {
        int acc_l = 0;
        int acc_r = 0;

        for (channel = 0; channel < MIX_VOICES; ++channel) {
            es_voice_t *voice = &g_voices[channel];
            int sample;

            if (!voice->active || voice->sample == NULL)
                continue;
            if (voice->pos >= voice->sample->length) {
                voice->active = 0;
                continue;
            }
            sample = voice->sample->pcm[voice->pos++];
            acc_l += sample * voice->left;
            acc_r += sample * voice->right;
        }

        acc_l >>= 8;
        acc_r >>= 8;
        if (acc_l > 32767)
            acc_l = 32767;
        if (acc_l < -32768)
            acc_l = -32768;
        if (acc_r > 32767)
            acc_r = 32767;
        if (acc_r < -32768)
            acc_r = -32768;
        dst[frame * 2] = (int16_t)acc_l;
        dst[frame * 2 + 1] = (int16_t)acc_r;
    }
    mix_unlock();
    OPL_Host_Mix(dst, frames);
}

#ifdef _WIN32
static void CALLBACK wave_done(HWAVEOUT handle, UINT message, DWORD_PTR instance, DWORD_PTR param, DWORD_PTR reserved)
{
    WAVEHDR *header = (WAVEHDR *)param;

    (void)instance;
    (void)reserved;
    if (message != WOM_DONE || !g_device_run || header == NULL)
        return;
    mix_frames((int16_t *)header->lpData, MIX_FRAMES);
    header->dwBufferLength = MIX_FRAMES * 2 * sizeof(int16_t);
    waveOutWrite(handle, header, sizeof(*header));
}

static boolean open_sfx_device(void)
{
    WAVEFORMATEX format;
    int i;

    memset(&format, 0, sizeof(format));
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = MIX_RATE;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 4;
    format.nAvgBytesPerSec = MIX_RATE * 4;

    if (waveOutOpen(&g_wave, WAVE_MAPPER, &format, (DWORD_PTR)wave_done, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
        return false;

    g_device_run = 1;
    for (i = 0; i < 4; ++i) {
        memset(&g_hdrs[i], 0, sizeof(g_hdrs[i]));
        g_hdrs[i].lpData = (LPSTR)g_wave_buf[i];
        g_hdrs[i].dwBufferLength = MIX_FRAMES * 2 * sizeof(int16_t);
        waveOutPrepareHeader(g_wave, &g_hdrs[i], sizeof(g_hdrs[i]));
        mix_frames(g_wave_buf[i], MIX_FRAMES);
        waveOutWrite(g_wave, &g_hdrs[i], sizeof(g_hdrs[i]));
    }
    return true;
}

static void close_sfx_device(void)
{
    int i;

    g_device_run = 0;
    if (g_wave == NULL)
        return;
    waveOutReset(g_wave);
    for (i = 0; i < 4; ++i)
        waveOutUnprepareHeader(g_wave, &g_hdrs[i], sizeof(g_hdrs[i]));
    waveOutClose(g_wave);
    g_wave = NULL;
}
#else
static void *alsa_thread(void *unused)
{
    int16_t buffer[MIX_FRAMES * 2];

    (void)unused;
    while (g_device_run) {
        long wrote = 0;

        mix_frames(buffer, MIX_FRAMES);
        while (wrote < MIX_FRAMES && g_device_run) {
            long count = g_pcm_writei(g_pcm, buffer + wrote * 2, (unsigned long)(MIX_FRAMES - wrote));
            if (count < 0) {
                g_pcm_prepare(g_pcm);
                break;
            }
            wrote += count;
        }
    }
    return NULL;
}

static boolean open_sfx_device(void)
{
    g_alsa = dlopen("libasound.so.2", RTLD_LAZY);
    if (g_alsa == NULL)
        return false;
    g_pcm_open = dlsym(g_alsa, "snd_pcm_open");
    g_pcm_set_params = dlsym(g_alsa, "snd_pcm_set_params");
    g_pcm_writei = dlsym(g_alsa, "snd_pcm_writei");
    g_pcm_prepare = dlsym(g_alsa, "snd_pcm_prepare");
    g_pcm_drop = dlsym(g_alsa, "snd_pcm_drop");
    g_pcm_close = dlsym(g_alsa, "snd_pcm_close");
    if (g_pcm_open == NULL || g_pcm_set_params == NULL || g_pcm_writei == NULL || g_pcm_prepare == NULL || g_pcm_drop == NULL || g_pcm_close == NULL) {
        dlclose(g_alsa);
        g_alsa = NULL;
        return false;
    }
    if (g_pcm_open(&g_pcm, "default", 0, 0) != 0) {
        dlclose(g_alsa);
        g_alsa = NULL;
        return false;
    }
    /* SND_PCM_FORMAT_S16_LE = 2, SND_PCM_ACCESS_RW_INTERLEAVED = 3 */
    if (g_pcm_set_params(g_pcm, 2, 3, 2, MIX_RATE, 1, 80000) != 0) {
        g_pcm_close(g_pcm);
        g_pcm = NULL;
        dlclose(g_alsa);
        g_alsa = NULL;
        return false;
    }
    g_device_run = 1;
    if (pthread_create(&g_audio_thread, NULL, alsa_thread, NULL) != 0) {
        g_device_run = 0;
        g_pcm_close(g_pcm);
        g_pcm = NULL;
        dlclose(g_alsa);
        g_alsa = NULL;
        return false;
    }
    return true;
}

static void close_sfx_device(void)
{
    g_device_run = 0;
    if (g_pcm != NULL && g_pcm_drop != NULL)
        g_pcm_drop(g_pcm);
    if (g_audio_thread)
        pthread_join(g_audio_thread, NULL);
    g_audio_thread = 0;
    if (g_pcm != NULL && g_pcm_close != NULL)
        g_pcm_close(g_pcm);
    g_pcm = NULL;
    if (g_alsa != NULL)
        dlclose(g_alsa);
    g_alsa = NULL;
}
#endif

static void lump_name(sfxinfo_t *sfx, char *buffer, size_t size)
{
    if (sfx->link != NULL)
        sfx = sfx->link;
    if (g_use_prefix)
        M_snprintf(buffer, size, "ds%s", DEH_String(sfx->name));
    else
        M_StringCopy(buffer, DEH_String(sfx->name), size);
}

static boolean cache_sfx(sfxinfo_t *sfx)
{
    int lump;
    byte *data;
    unsigned int lump_length;
    unsigned int sample_count;
    unsigned int rate;
    unsigned int used;
    int out_count;
    int16_t *pcm;
    int i;
    es_sample_t *sample;
    char name[16];

    if (sfx->driver_data != NULL)
        return true;

    lump_name(sfx, name, sizeof(name));
    lump = W_CheckNumForName(name);
    if (lump < 0)
        return false;
    sfx->lumpnum = lump;
    data = W_CacheLumpNum(lump, PU_STATIC);
    lump_length = (unsigned int)W_LumpLength(lump);
    if (data == NULL || lump_length < 8 || data[0] != 0x03 || data[1] != 0x00) {
        W_ReleaseLumpNum(lump);
        return false;
    }

    rate = (unsigned int)data[2] | ((unsigned int)data[3] << 8);
    sample_count = (unsigned int)data[4] | ((unsigned int)data[5] << 8) | ((unsigned int)data[6] << 16) | ((unsigned int)data[7] << 24);
    if (rate < 1000 || sample_count <= 48 || sample_count > lump_length - 8) {
        W_ReleaseLumpNum(lump);
        return false;
    }

    /* DMX ignores 16 samples at each end of the lump. */
    data += 24;
    used = sample_count - 32;
    out_count = (int)(((uint64_t)used * MIX_RATE) / rate);
    if (out_count <= 0) {
        W_ReleaseLumpNum(lump);
        return false;
    }

    pcm = malloc((size_t)out_count * sizeof(int16_t));
    sample = malloc(sizeof(*sample));
    if (pcm == NULL || sample == NULL) {
        free(pcm);
        free(sample);
        W_ReleaseLumpNum(lump);
        return false;
    }

    for (i = 0; i < out_count; ++i) {
        unsigned int src = (unsigned int)(((uint64_t)i * rate) / MIX_RATE);
        int value;

        if (src >= used)
            src = used - 1;
        value = ((int)data[src] - 128) << 8;
        pcm[i] = (int16_t)value;
    }
    W_ReleaseLumpNum(lump);

    sample->pcm = pcm;
    sample->length = out_count;
    sfx->driver_data = sample;
    return true;
}

static void voice_gains(es_voice_t *voice, int vol, int sep)
{
    int left = ((254 - sep) * vol) / 127;
    int right = (sep * vol) / 127;

    if (left < 0)
        left = 0;
    if (left > 255)
        left = 255;
    if (right < 0)
        right = 0;
    if (right > 255)
        right = 255;
    voice->left = left;
    voice->right = right;
}

static boolean sfx_init(boolean use_prefix)
{
    ensure_lock();
    snd_samplerate = MIX_RATE;
    g_use_prefix = use_prefix;
    memset(g_voices, 0, sizeof(g_voices));
    if (!open_sfx_device()) {
        fprintf(stderr, "ES-DOOM: sound device did not open\n");
        return false;
    }
    return true;
}

static void sfx_shutdown(void)
{
    close_sfx_device();
}

static int sfx_lump(sfxinfo_t *sfx)
{
    char name[16];

    lump_name(sfx, name, sizeof(name));
    return W_GetNumForName(name);
}

static void sfx_update(void)
{
    int channel;

    mix_lock();
    for (channel = 0; channel < MIX_VOICES; ++channel) {
        es_voice_t *voice = &g_voices[channel];
        if (voice->active && (voice->sample == NULL || voice->pos >= voice->sample->length))
            voice->active = 0;
    }
    mix_unlock();
}

static void sfx_params(int channel, int vol, int sep)
{
    if (channel < 0 || channel >= MIX_VOICES)
        return;
    mix_lock();
    voice_gains(&g_voices[channel], vol, sep);
    mix_unlock();
}

static int sfx_start(sfxinfo_t *sfx, int channel, int vol, int sep)
{
    if (channel < 0 || channel >= MIX_VOICES)
        return -1;
    if (!cache_sfx(sfx))
        return -1;
    mix_lock();
    g_voices[channel].sample = sfx->driver_data;
    g_voices[channel].pos = 0;
    g_voices[channel].active = 1;
    voice_gains(&g_voices[channel], vol, sep);
    mix_unlock();
    return channel;
}

static void sfx_stop(int channel)
{
    if (channel < 0 || channel >= MIX_VOICES)
        return;
    mix_lock();
    g_voices[channel].active = 0;
    g_voices[channel].sample = NULL;
    mix_unlock();
}

static boolean sfx_playing(int channel)
{
    boolean playing = false;

    if (channel < 0 || channel >= MIX_VOICES)
        return false;
    mix_lock();
    if (g_voices[channel].active && g_voices[channel].sample != NULL && g_voices[channel].pos < g_voices[channel].sample->length)
        playing = true;
    else
        g_voices[channel].active = 0;
    mix_unlock();
    return playing;
}

static snddevice_t sfx_devices[] = {
    SNDDEVICE_SB,
    SNDDEVICE_PAS,
    SNDDEVICE_GUS,
    SNDDEVICE_WAVEBLASTER,
    SNDDEVICE_SOUNDCANVAS,
    SNDDEVICE_AWE32,
    SNDDEVICE_GENMIDI,
    SNDDEVICE_ADLIB,
};

sound_module_t DG_sound_module = {
    sfx_devices,
    arrlen(sfx_devices),
    sfx_init,
    sfx_shutdown,
    sfx_lump,
    sfx_update,
    sfx_params,
    sfx_start,
    sfx_stop,
    sfx_playing,
    NULL,
};


void DG_AudioActive(int active)
{
    ensure_lock();
    mix_lock();
    g_audible = active ? 1 : 0;
    mix_unlock();
    OPL_Host_Suspend(active ? 0 : 1);
}
