/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_ewokosaudio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>

/* EwokOS PCM audio support - based on wavplayer's pcm.c */

#define CTRL_PCM_DEV_HW				(0xF0)
#define CTRL_PCM_DEV_HW_FREE		(0xF1)
#define CTRL_PCM_DEV_PRPARE			(0xF2)
#define CTRL_PCM_BUF_AVAIL			(0xF3)

#define	EPIPE					(32)

struct pcm_config {
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

struct pcm {
    int fd;
    int prepared;
    int running;
    char name[32];
    int framesize;
    struct pcm_config config;
};

static int pcm_buf_avail(struct pcm *pcm)
{
    proto_t in, out;
    PF->init(&in);
    PF->init(&out);
    int ret = 0;

    ret = dev_cntl(pcm->name, CTRL_PCM_BUF_AVAIL, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);
    }

    PF->clear(&in);
    PF->clear(&out);
    return ret;
}

static int support_rate(unsigned int rate) {
    switch (rate) {
    case 8000:
    case 16000:
    case 32000:
    case 44100:
    case 48000:
    case 96000:
        return 1;
    }
    return 0;
}

static int support_channels(unsigned int channels) {
    if (channels != 2) {
        return 0;
    }
    return 1;
}

static int support_bit_depth(unsigned int bit_depth) {
    switch (bit_depth) {
    case 16:
    case 24:
    case 32:
        return 1;
    default:
        return 0;
    }
}

static int is_valid_config(struct pcm_config *config)
{
    if (!support_bit_depth(config->bit_depth) ||
        !support_channels(config->channels) ||
        !support_rate(config->rate)) {
        return 0;
    }

    if (config->period_size == 0 || config->period_count == 0) {
        return 0;
    }

    if (config->start_threshold == 0) {
        config->start_threshold = config->period_size;
    }

    if (config->stop_threshold == 0) {
        config->stop_threshold = config->period_size * config->period_count;
    }
    return 1;
}

static int pcm_prepare(struct pcm *pcm)
{
    if (pcm->prepared) {
        return 0;
    }

    proto_t in, out;
    PF->init(&in);
    PF->init(&out);
    int ret = dev_cntl(pcm->name, CTRL_PCM_DEV_PRPARE, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);
    }
    PF->clear(&in);
    PF->clear(&out);

    if (ret == 0) {
        pcm->prepared = 1;
    }
    return ret;
}

static int pcm_try_write(struct pcm *pcm, const void* data, unsigned int count)
{
    if (count == 0) {
        return 0;
    }

    for (;;) {
        if (pcm->running == 0) {
            int err = pcm_prepare(pcm);
            if (err != 0) {
                return err;
            }

            int written = write(pcm->fd, data, count);
            if (written != (int)count) {
                return -1;
            }
            pcm->running = 1;
            return 0;
        }

        int ret = write(pcm->fd, data, count);
        return (ret == (int)count ? 0 : -1);
    }
}

static int wait_avail(struct pcm *pcm, int *avail, int time_out_ms)
{
    enum { SLEEP_TIME_MS = 5 };
    *avail = 0;
    int ret = 0;
    int period_bytes = pcm->config.period_size * 4;
    int max_try_count = time_out_ms / SLEEP_TIME_MS;
    int try_count = 0;

    for(;;) {
        ret = pcm_buf_avail(pcm);
        if (ret < 0) {
            break;
        }

        if (ret >= period_bytes) {
            *avail = ret;
            break;
        }

        if(try_count++ >= max_try_count) {
            break;
        }

        proc_usleep(SLEEP_TIME_MS * 1000);
    }

    return ret;
}

static int pcm_write(struct pcm *pcm, const void* data, unsigned int count) {
    int period_bytes = 0;
    int avail = 0;
    int bytes = (int)count;
    int written = 0;
    int offset = 0;
    int copy_bytes = 0;
    int ret = 0;

    period_bytes = pcm->config.period_size * 4;
    copy_bytes = bytes < period_bytes ? bytes : period_bytes;
    while (bytes > 0) {
        ret = wait_avail(pcm, &avail, 2000);
        if (ret == -EPIPE) {
            copy_bytes = (bytes < period_bytes ? bytes : period_bytes);
            pcm->prepared = 0;
            pcm->running = 0;
            continue;
        }

        if (ret < 0 || (avail == 0)) {
            break;
        }

        copy_bytes = bytes < avail ? bytes : avail;

        ret = pcm_try_write(pcm, (const char*)data + offset, copy_bytes);
        if (ret == 0) {
            offset += copy_bytes;
            written += copy_bytes;
            bytes -= copy_bytes;
            copy_bytes = bytes < period_bytes ? bytes : period_bytes;
        }
    }

    return (written == (int)count ? 0 : -1);
}

static struct pcm* pcm_open(const char *name, struct pcm_config *config)
{
    struct pcm* pcm;

    if (!is_valid_config(config)) {
        return NULL;
    }

    pcm = calloc(1, sizeof(struct pcm));
    if (pcm == NULL) {
        return NULL;
    }

    strncpy(pcm->name, name, 31);
    memcpy(&pcm->config, config, sizeof(struct pcm_config));
    pcm->framesize = config->channels * config->bit_depth / 8;

    pcm->fd = open(name, O_RDWR);
    if (pcm->fd < 0) {
        free(pcm);
        return NULL;
    }

    proto_t in, out;
    PF->init(&in)->add(&in, config, sizeof(struct pcm_config));
    PF->init(&out);
    int temp = dev_cntl(pcm->name, CTRL_PCM_DEV_HW, &in, &out);
    if(temp == 0) {
        temp = proto_read_int(&out);
    }
    PF->clear(&in);
    PF->clear(&out);

    if (temp != 0) {
        close(pcm->fd);
        free(pcm);
        return NULL;
    }

    return pcm;
}

static int pcm_close(struct pcm *pcm)
{
    if (pcm == NULL) {
        return 0;
    }

    close(pcm->fd);
    free(pcm);
    return 0;
}

/* Audio thread data */
struct audio_thread_data {
    SDL_AudioDevice *device;
    struct pcm *pcm;
    int enabled;
};

static struct audio_thread_data audio_thread_data;
static pthread_t audio_thread_id = 0;

/* Audio thread - similar to wavplayer's push_data_to_pcm */
static void *audio_thread(void *arg)
{
    struct audio_thread_data *data = (struct audio_thread_data *)arg;
    SDL_AudioDevice *device = data->device;
    struct pcm *pcm = data->pcm;
    Uint8 *buffer = NULL;
    int buffer_size;
    void *udata;
    void (SDLCALL * fill) (void *userdata, Uint8 * stream, int len);

    /* Get callback function */
    fill = device->spec.callback;
    udata = device->spec.userdata;
    buffer_size = device->spec.size;

    /* Allocate buffer */
    buffer = (Uint8 *)malloc(buffer_size);
    if (!buffer) {
        return NULL;
    }

    /* Same delay as wavplayer's push_data_to_pcm */
    proc_usleep(200 * 1000);

    while (data->enabled) {
        /* Check device paused state (set by SDL_PauseAudio) */
        if (device->paused) {
            proc_usleep(10000);
            continue;
        }

        /* Call SDL callback to fill buffer with audio data */
        fill(udata, buffer, buffer_size);

        /* Write to PCM device */
        if (pcm_write(pcm, buffer, buffer_size) != 0) {
            /* Error writing */
        }
    }

    free(buffer);
    return NULL;
}

static void EWOKOSAUD_CloseDevice(_THIS) {
    if (audio_thread_id != 0) {
        audio_thread_data.enabled = 0;
        pthread_join(audio_thread_id, NULL);
        audio_thread_id = 0;
    }

    if (this->hidden->pcm) {
        pcm_close(this->hidden->pcm);
        this->hidden->pcm = NULL;
    }

    if (this->hidden->mixbuf) {
        SDL_free(this->hidden->mixbuf);
        this->hidden->mixbuf = NULL;
    }
    this->hidden->mixlen = 0;
}

static int EWOKOSAUD_OpenDevice(_THIS, const char *devname, int iscapture) {
    struct pcm_config config;
    int bits;

    if (iscapture) {
        return SDL_SetError("Audio capture not supported on EwokOS");
    }

    /* Allocate private data */
    this->hidden = (struct SDL_PrivateAudioData *)SDL_malloc(sizeof(struct SDL_PrivateAudioData));
    if (!this->hidden) {
        return SDL_OutOfMemory();
    }
    SDL_memset(this->hidden, 0, sizeof(struct SDL_PrivateAudioData));

    /* Set up the PCM configuration */
    memset(&config, 0, sizeof(config));

    /* Convert SDL format to bit depth */
    switch (this->spec.format) {
        case AUDIO_S16SYS:
#if AUDIO_S16SYS != AUDIO_S16LSB
        case AUDIO_S16LSB:
#endif
#if AUDIO_S16SYS != AUDIO_S16MSB
        case AUDIO_S16MSB:
#endif
            bits = 16;
            break;
        default:
            bits = 16;
            this->spec.format = AUDIO_S16SYS;
            break;
    }

    config.bit_depth = bits;
    config.rate = this->spec.freq;
    config.channels = this->spec.channels;

    /* Validate and adjust parameters for EwokOS */
    if (config.channels != 2) {
        config.channels = 2;
        this->spec.channels = 2;
    }

    /* Supported rates */
    if (config.rate != 8000 && config.rate != 16000 &&
        config.rate != 32000 && config.rate != 44100 &&
        config.rate != 48000 && config.rate != 96000) {
        config.rate = 44100;
        this->spec.freq = 44100;
    }

    /* Use same parameters as wavplayer */
    config.period_size = 2048;
    config.period_count = 4;
    config.start_threshold = 0;  /* Will be set by is_valid_config */
    config.stop_threshold = 0;    /* Will be set by is_valid_config */

    /* Calculate the final parameters for this audio specification */
    SDL_CalculateAudioSpec(&this->spec);

    /* Open the PCM device */
    this->hidden->pcm = pcm_open("/dev/sound0", &config);
    if (!this->hidden->pcm) {
        SDL_free(this->hidden);
        this->hidden = NULL;
        return SDL_SetError("Failed to open EwokOS PCM audio device");
    }

    /* Allocate audio buffer */
    this->hidden->mixlen = this->spec.size;
    this->hidden->mixbuf = (Uint8 *)SDL_malloc(this->hidden->mixlen);
    if (!this->hidden->mixbuf) {
        pcm_close(this->hidden->pcm);
        SDL_free(this->hidden);
        this->hidden = NULL;
        return SDL_SetError("Failed to allocate audio buffer");
    }

    /* Setup audio thread data */
    audio_thread_data.device = this;
    audio_thread_data.pcm = this->hidden->pcm;
    audio_thread_data.enabled = 1;

    /* Create audio thread - like wavplayer does */
    if (pthread_create(&audio_thread_id, NULL, audio_thread, &audio_thread_data) != 0) {
        pcm_close(this->hidden->pcm);
        SDL_free(this->hidden->mixbuf);
        SDL_free(this->hidden);
        this->hidden = NULL;
        return SDL_SetError("Failed to create audio thread");
    }

    return 0;
}

static void EWOKOSAUD_LockDevice(_THIS) {
}

static void EWOKOSAUD_UnlockDevice(_THIS) {
}

static void EWOKOSAUD_WaitDevice(_THIS) {
    proc_usleep(1000);
}

static Uint8 *EWOKOSAUD_GetDeviceBuf(_THIS) {
    return this->hidden->mixbuf;
}

static void EWOKOSAUD_PlayDevice(_THIS) {
}

static void EWOKOSAUD_WaitDone(_THIS) {
    if (this->hidden->pcm) {
        proc_usleep(100000);
    }
}

static int EWOKOSAUD_Init(SDL_AudioDriverImpl *impl) {
    impl->OpenDevice = EWOKOSAUD_OpenDevice;
    impl->CloseDevice = EWOKOSAUD_CloseDevice;
    impl->LockDevice = EWOKOSAUD_LockDevice;
    impl->UnlockDevice = EWOKOSAUD_UnlockDevice;
    impl->WaitDevice = EWOKOSAUD_WaitDevice;
    impl->PlayDevice = EWOKOSAUD_PlayDevice;
    impl->GetDeviceBuf = EWOKOSAUD_GetDeviceBuf;
    impl->WaitDone = EWOKOSAUD_WaitDone;
    impl->ProvidesOwnCallbackThread = 1;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->HasCaptureSupport = 0;

    return 1;
}

AudioBootStrap EWOKOSAUD_bootstrap = {
    "ewokos", "SDL EwokOS audio driver", EWOKOSAUD_Init, 0
};

/* vi: set ts=4 sw=4 expandtab: */
