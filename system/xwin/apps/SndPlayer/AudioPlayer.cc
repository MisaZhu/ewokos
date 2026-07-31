#define MINIMP3_IMPLEMENTATION
#include "AudioPlayer.h"

#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/proto.h>
#include <ewoksys/devcmd.h>
#include <ewoksys/klog.h>

// WAV file format definitions
#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164

struct wav_chunk_fmt {
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct wav_riff_header {
    uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t wave_id;
};

struct wav_chunk_header {
    uint32_t id;
    uint32_t sz;
};

#define CTRL_PCM_DEV_HW         (0xF0)
#define CTRL_PCM_DEV_PRPARE     (0xF2)
#define CTRL_PCM_BUF_AVAIL      (0xF3)
#define PCM_WAIT_SLEEP_MS       1
#define OGG_DECODE_FRAMES       1152
#define OGG_WRITE_CHUNK_FRAMES  2048
#define WAV_WRITE_CHUNK_FRAMES  1024
#define MP3_STREAM_BUFFER_SIZE  (32 * 1024)
#define MP3_STREAM_REFILL_LOW   4096

#ifndef EPIPE
#define EPIPE 32
#endif

// OGG Vorbis file callbacks
static size_t ogg_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    ssize_t bytes_read;

    if (size == 0 || nmemb == 0) {
        return 0;
    }

    if (ds == NULL || ds->fd < 0) {
        return 0;
    }

    bytes_read = read(ds->fd, ptr, size * nmemb);
    if (bytes_read <= 0) {
        return 0;
    }
    return (size_t)bytes_read / size;
}

static int ogg_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    if (ds == NULL || ds->fd < 0) {
        return -1;
    }
    return (lseek(ds->fd, (off_t)offset, whence) < 0) ? -1 : 0;
}

static int ogg_close_func(void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    if (ds == NULL || ds->fd < 0) {
        return 0;
    }

    close(ds->fd);
    ds->fd = -1;
    return 0;
}

static long ogg_tell_func(void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    off_t pos;

    if (ds == NULL || ds->fd < 0) {
        return -1;
    }

    pos = lseek(ds->fd, 0, SEEK_CUR);
    return (long)pos;
}

static int16_t ogg_float_to_s16(float sample)
{
    int value;
    if (sample > 1.0f) {
        sample = 1.0f;
    } else if (sample < -1.0f) {
        sample = -1.0f;
    }

    if (sample >= 0.0f) {
        value = (int)(sample * 32767.0f + 0.5f);
    } else {
        value = (int)(sample * 32768.0f - 0.5f);
    }

    if (value > 32767) {
        value = 32767;
    } else if (value < -32768) {
        value = -32768;
    }
    return (int16_t)value;
}

struct pcm_config {
    int bit_depth;
    int rate;
    int channels;
    int period_size;
    int period_count;
    int start_threshold;
    int stop_threshold;
};

struct pcm_t {
    int fd;
    int prepared;
    int running;
    char name[32];
    int framesize;
    struct pcm_config config;
    int (*hook)(void *data);
    void *user_data;
};

static int pcm_param_set(struct pcm_t *pcm, struct pcm_config *config)
{
    proto_t in, out;
    PF->init(&in)->add(&in, config, sizeof(struct pcm_config));
    PF->init(&out);
    int ret = 0;
    ret = dev_cntl(pcm->name, CTRL_PCM_DEV_HW, &in, &out);
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

static struct pcm_t* pcm_open(const char *name, struct pcm_config *config)
{
    struct pcm_t* pcm;

    if (!is_valid_config(config)) {
        return NULL;
    }

    pcm = (struct pcm_t*)calloc(1, sizeof(struct pcm_t));
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

    int temp = pcm_param_set(pcm, &pcm->config);
    if (temp != 0) {
        close(pcm->fd);
        free(pcm);
        return NULL;
    }

    return pcm;
}

static int pcm_prepare(struct pcm_t *pcm)
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

static int pcm_try_write(struct pcm_t *pcm, const void* data, unsigned int count) {
    if (count == 0) return 0;

    /*
     * Returns bytes actually written (>= 0) or a negative errno. The
     * server uses partial-write semantics: an XRUN mid-write returns
     * the bytes consumed so far, so the caller must advance by the
     * returned count instead of treating a short write as an error.
     */
    if (pcm->running == 0) {
        int err = pcm_prepare(pcm);
        if (err != 0) {
            return err;
        }

        int written = write(pcm->fd, data, count);
        if (written > 0) {
            pcm->running = 1;
        }
        return written;
    }

    return write(pcm->fd, data, count);
}

static int pcm_buf_avail(struct pcm_t *pcm)
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

static int wait_avail(struct pcm_t *pcm, int *avail, int time_out_ms)
{
    *avail = 0;
    int ret = 0;
    int period_bytes = pcm->config.period_size * pcm->framesize;
    int min_avail = period_bytes / 4;
    int max_try_count = time_out_ms / PCM_WAIT_SLEEP_MS;
    int try_count = 0;

    if (min_avail < pcm->framesize) {
        min_avail = pcm->framesize;
    }

    for(;;) {
        ret = pcm_buf_avail(pcm);
        if (ret < 0) {
            break;
        }

        if (ret >= min_avail) {
            *avail = ret;
            break;
        }

        if(try_count++ >= max_try_count) {
            break;
        }

        //proc_yield();
        proc_usleep(1000);
    }

    return ret;
}

static int pcm_write(struct pcm_t *pcm, const void* data, unsigned int count) {
    if (count == 0) return 0;

    int period_bytes = pcm->config.period_size * pcm->framesize;
    int avail = 0;
    int bytes = (int)count;
    int written = 0;
    int offset = 0;
    int copy_bytes = 0;
    int ret = 0;
    int xrun_retry = 0;

    copy_bytes = bytes < period_bytes ? bytes : period_bytes;
    while (bytes > 0) {
        ret = wait_avail(pcm, &avail, 2000);
        if (ret == -EPIPE) {
            /*
             * XRUN (underrun or resume after pause): the server keeps
             * returning -EPIPE until we re-prepare, so recover here
             * instead of aborting playback. Bound retries to avoid
             * spinning if prepare keeps failing.
             */
            if (xrun_retry++ >= 5) {
                break;
            }
            pcm->prepared = 0;
            pcm->running = 0;
            if (pcm_prepare(pcm) != 0) {
                proc_usleep(10000);
            }
            continue;
        }

        if (ret < 0 || (avail == 0 && bytes > 0)) {
            break;
        }

        copy_bytes = bytes < avail ? bytes : avail;

        ret = pcm_try_write(pcm, (const char*)data + offset, copy_bytes);
        if (ret == -EPIPE) {
            /* XRUN hit inside write(): recover the same way */
            if (xrun_retry++ >= 5) {
                break;
            }
            pcm->prepared = 0;
            pcm->running = 0;
            if (pcm_prepare(pcm) != 0) {
                proc_usleep(10000);
            }
            continue;
        }
        if (ret < 0) {
            break;
        }
        /*
         * Only real write progress proves the XRUN recovery worked;
         * wait_avail succeeds right after re-prepare even when the
         * engine is wedged, so resetting there defeats the retry cap.
         */
        if (ret > 0) {
            xrun_retry = 0;
        }
        offset += ret;
        written += ret;
        bytes -= ret;
        copy_bytes = bytes < period_bytes ? bytes : period_bytes;
    }

    return (written == (int)count ? 0 : -1);
}

static int pcm_close(struct pcm_t *pcm)
{
    if (pcm == NULL) {
        return 0;
    }
    close(pcm->fd);
    free(pcm);
    return 0;
}

// Check file extension to determine audio format
static AudioFormat getAudioFormat(const char* path) {
    const char* ext = strrchr(path, '.');
    if (ext == NULL) return FORMAT_UNKNOWN;

    if (strcasecmp(ext, ".mp3") == 0) return FORMAT_MP3;
    if (strcasecmp(ext, ".wav") == 0) return FORMAT_WAV;
    if (strcasecmp(ext, ".ogg") == 0) return FORMAT_OGG;

    // Try to detect by file header
    int fd = open(path, O_RDONLY);
    if (fd < 0) return FORMAT_UNKNOWN;

    char header[4];
    int ret = read(fd, header, 4);
    close(fd);

    if (ret == 4) {
        if (header[0] == 'O' && header[1] == 'g' && header[2] == 'g' && header[3] == 'S') {
            return FORMAT_OGG;
        }
        // Check for RIFF/WAVE
        if (header[0] == 'R' && header[1] == 'I' && header[2] == 'F' && header[3] == 'F') {
            return FORMAT_WAV;
        }
        // MP3 files start with sync word 0xFFE or ID3 tag
        uint32_t hdr = *(uint32_t*)header;
        if ((hdr & 0xFFE00000) == 0xFFE00000 || hdr == 0x44493349) return FORMAT_MP3;
    }

    return FORMAT_UNKNOWN;
}

// Parse WAV file header directly from fd and leave the fd positioned at data.
static int readFully(int fd, void* buf, int size) {
    int total = 0;

    while (total < size) {
        int rd = read(fd, (uint8_t*)buf + total, size - total);
        if (rd <= 0) {
            return -1;
        }
        total += rd;
    }
    return total;
}

static int skipFully(int fd, int size) {
    if (size <= 0) {
        return 0;
    }
    return (lseek(fd, size, SEEK_CUR) < 0) ? -1 : 0;
}

static int parseWavHeaderFd(int fd, int* sampleRate, int* channels, int* bitDepth, int* dataOffset, int* dataSize) {
    wav_riff_header riff;
    wav_chunk_fmt fmt;
    int foundFmt = 0;
    int foundData = 0;
    int pos = 0;

    if (lseek(fd, 0, SEEK_SET) < 0) {
        return -1;
    }

    if (readFully(fd, &riff, (int)sizeof(riff)) != (int)sizeof(riff)) {
        return -1;
    }
    pos = (int)sizeof(riff);

    if (riff.riff_id != ID_RIFF || riff.wave_id != ID_WAVE) {
        return -1;
    }

    while (!foundData) {
        wav_chunk_header chunk;
        int paddedSize;

        if (readFully(fd, &chunk, (int)sizeof(chunk)) != (int)sizeof(chunk)) {
            return -1;
        }
        pos += (int)sizeof(chunk);
        paddedSize = (int)chunk.sz + ((chunk.sz & 1U) ? 1 : 0);

        if (chunk.id == ID_FMT) {
            int remain;

            if ((int)chunk.sz < (int)sizeof(fmt)) {
                return -1;
            }
            if (readFully(fd, &fmt, (int)sizeof(fmt)) != (int)sizeof(fmt)) {
                return -1;
            }
            remain = paddedSize - (int)sizeof(fmt);
            if (skipFully(fd, remain) != 0) {
                return -1;
            }
            pos += paddedSize;
            foundFmt = 1;
        } else if (chunk.id == ID_DATA) {
            *dataOffset = pos;
            *dataSize = (int)chunk.sz;
            foundData = 1;
            break;
        } else {
            if (skipFully(fd, paddedSize) != 0) {
                return -1;
            }
            pos += paddedSize;
        }
    }

    if (!foundFmt || !foundData) {
        return -1;
    }

    *sampleRate = fmt.sample_rate;
    *channels = fmt.num_channels;
    *bitDepth = fmt.bits_per_sample;
    return 0;
}

static int32_t wavReadS24(const uint8_t* src) {
    int32_t value = (int32_t)((uint32_t)src[0] |
                              ((uint32_t)src[1] << 8) |
                              ((uint32_t)src[2] << 16));
    if ((value & 0x00800000) != 0) {
        value |= ~0x00FFFFFF;
    }
    return value;
}

static int32_t wavReadS32(const uint8_t* src) {
    return (int32_t)((uint32_t)src[0] |
                     ((uint32_t)src[1] << 8) |
                     ((uint32_t)src[2] << 16) |
                     ((uint32_t)src[3] << 24));
}

static void wavFillPreviewSamples(const uint8_t* src, int frames, int channels, int bitDepth, int16_t* dst) {
    int totalSamples = frames * channels;

    for (int i = 0; i < totalSamples; i++) {
        const uint8_t* in = src + (i * (bitDepth / 8));
        int32_t value = 0;

        switch (bitDepth) {
        case 8:
            value = ((int32_t)in[0] - 128) << 8;
            break;
        case 16:
            value = (int16_t)((uint16_t)in[0] | ((uint16_t)in[1] << 8));
            break;
        case 24:
            value = wavReadS24(in) >> 8;
            break;
        case 32:
            value = wavReadS32(in) >> 16;
            break;
        default:
            value = 0;
            break;
        }
        dst[i] = (int16_t)value;
    }
}

static uint32_t estimateMp3TotalMs(const uint8_t* data, int size) {
    mp3dec_t dec;
    mp3dec_frame_info_t frameInfo;
    int16_t frameBuf[MINIMP3_MAX_SAMPLES_PER_FRAME];
    const uint8_t* pos = data;
    int bytesLeft = size;
    uint64_t totalSamples = 0;
    int streamRate = 0;

    mp3dec_init(&dec);

    while (bytesLeft > 0) {
        int decodedSamples = mp3dec_decode_frame(&dec, pos, bytesLeft, frameBuf, &frameInfo);
        if (frameInfo.frame_bytes <= 0) {
            break;
        }

        if (decodedSamples > 0 && frameInfo.hz > 0) {
            if (streamRate == 0) {
                streamRate = frameInfo.hz;
            }

            if (frameInfo.hz == streamRate) {
                totalSamples += (uint64_t)decodedSamples;
            } else {
                totalSamples += ((uint64_t)decodedSamples * (uint64_t)streamRate) / (uint64_t)frameInfo.hz;
            }
        }

        pos += frameInfo.frame_bytes;
        bytesLeft -= frameInfo.frame_bytes;
    }

    if (streamRate > 0) {
        return (uint32_t)((totalSamples * 1000ULL) / (uint64_t)streamRate);
    }
    return 0;
}

bool AudioPlayer::refillMp3Stream() {
    int carry;
    int total;

    if (sourceFd < 0 || mp3StreamBuf == NULL || mp3StreamBufSize <= 0) {
        return false;
    }

    carry = bytesLeft;
    if (carry < 0) {
        carry = 0;
    }

    if (carry > 0 && streamPos != mp3StreamBuf) {
        memmove(mp3StreamBuf, streamPos, (size_t)carry);
    }
    streamPos = mp3StreamBuf;
    total = carry;

    while (total < mp3StreamBufSize) {
        int rd = read(sourceFd, mp3StreamBuf + total, mp3StreamBufSize - total);
        if (rd < 0) {
            return false;
        }
        if (rd == 0) {
            mp3StreamEof = true;
            break;
        }
        total += rd;
        if (total >= MP3_STREAM_REFILL_LOW) {
            break;
        }
    }

    bytesLeft = total;
    return true;
}

int AudioPlayer::decodeNextMp3Frame(mp3dec_t* dec, mp3dec_frame_info_t* frameInfo, int16_t* outSamples) {
    for (;;) {
        int decodedSamples;

        if (bytesLeft < MP3_STREAM_REFILL_LOW && !mp3StreamEof) {
            if (!refillMp3Stream()) {
                return -1;
            }
        }

        if (bytesLeft <= 0) {
            return 0;
        }

        decodedSamples = mp3dec_decode_frame(dec, streamPos, bytesLeft, outSamples, frameInfo);
        if (frameInfo->frame_bytes > 0) {
            streamPos += frameInfo->frame_bytes;
            bytesLeft -= frameInfo->frame_bytes;
            if (decodedSamples > 0) {
                return decodedSamples;
            }
            continue;
        }

        if (mp3StreamEof) {
            return 0;
        }

        if (bytesLeft >= mp3StreamBufSize) {
            streamPos++;
            bytesLeft--;
        }

        if (!refillMp3Stream()) {
            return -1;
        }
    }
}

bool AudioPlayer::resetMp3Stream() {
    if (sourceFd < 0 || mp3StreamBuf == NULL) {
        return false;
    }

    if (lseek(sourceFd, 0, SEEK_SET) < 0) {
        return false;
    }

    streamPos = mp3StreamBuf;
    bytesLeft = 0;
    mp3StreamEof = false;
    return refillMp3Stream();
}

uint32_t AudioPlayer::estimateMp3StreamTotalMs() {
    mp3dec_t dec;
    mp3dec_frame_info_t frameInfo;
    int16_t frameBuf[MINIMP3_MAX_SAMPLES_PER_FRAME];
    uint8_t *buf = NULL;
    uint8_t *pos = NULL;
    int fd = -1;
    int left = 0;
    int eof = 0;
    int streamRate = 0;
    uint64_t totalSamples = 0;
    uint32_t totalMs = 0;

    if (sourcePath == NULL) {
        return 0;
    }

    fd = open(sourcePath, O_RDONLY);
    if (fd < 0) {
        return 0;
    }

    buf = (uint8_t*)malloc(MP3_STREAM_BUFFER_SIZE);
    if (buf == NULL) {
        close(fd);
        return 0;
    }

    mp3dec_init(&dec);
    pos = buf;

    for (;;) {
        int carry = left;
        int total = 0;
        int decodedSamples;

        if (carry > 0 && pos != buf) {
            memmove(buf, pos, (size_t)carry);
        }
        pos = buf;
        total = carry;

        while (total < MP3_STREAM_BUFFER_SIZE) {
            int rd = read(fd, buf + total, MP3_STREAM_BUFFER_SIZE - total);
            if (rd < 0) {
                eof = 1;
                break;
            }
            if (rd == 0) {
                eof = 1;
                break;
            }
            total += rd;
            if (total >= MP3_STREAM_REFILL_LOW) {
                break;
            }
        }

        left = total;
        if (left <= 0) {
            break;
        }

        decodedSamples = mp3dec_decode_frame(&dec, pos, left, frameBuf, &frameInfo);
        if (frameInfo.frame_bytes > 0) {
            pos += frameInfo.frame_bytes;
            left -= frameInfo.frame_bytes;
            if (decodedSamples > 0 && frameInfo.hz > 0) {
                if (streamRate == 0) {
                    streamRate = frameInfo.hz;
                }
                if (frameInfo.hz == streamRate) {
                    totalSamples += (uint64_t)decodedSamples;
                } else {
                    totalSamples += ((uint64_t)decodedSamples * (uint64_t)streamRate) / (uint64_t)frameInfo.hz;
                }
            }
            continue;
        }

        if (eof) {
            break;
        }

        if (left >= MP3_STREAM_BUFFER_SIZE) {
            pos++;
            left--;
        }
    }

    if (streamRate > 0) {
        totalMs = (uint32_t)((totalSamples * 1000ULL) / (uint64_t)streamRate);
    }

    free(buf);
    close(fd);
    return totalMs;
}

// AudioPlayer implementation
AudioPlayer::AudioPlayer() {
    pcmDev = NULL;
    fileData = NULL;
    streamPos = NULL;
    bytesLeft = 0;
    totalBytes = 0;
    sourceFd = -1;
    sourcePath = NULL;
    mp3StreamBuf = NULL;
    mp3StreamBufSize = 0;
    mp3StreamEof = false;
    simples = 0;
    playing = false;
    paused = false;
    eof = false;
    writeFailed = false;
    currentMs = 0;
    totalMs = 0;
    format = FORMAT_UNKNOWN;
    wavDataOffset = 0;
    wavDataSize = 0;
    wavBitDepth = 16;
    wavBytesPerFrame = 0;
    wavStreamBuf = NULL;
    wavStreamBufSize = 0;
    oggChannels = 2;
    oggBitstream = 0;
    zeroReadCount = 0;
    mp3dec = NULL;
    info = NULL;
    oggVf = NULL;
    oggDs = NULL;
    oggDecodeBuffer = NULL;
    oggStereoBuffer = NULL;
    sampleBuf = NULL;
}

AudioPlayer::~AudioPlayer() {
    stop();
}

bool AudioPlayer::load(const char* path, const char* device) {
    stop();
    format = getAudioFormat(path);

    if (format == FORMAT_UNKNOWN) {
        return false;
    }

    if (format == FORMAT_OGG) {
        bool ok = loadOgg(path, device);
        if (!ok) {
            stop();
        }
        return ok;
    }

    if (format == FORMAT_MP3) {
        bool ok = loadMp3(path, device);
        if (!ok) {
            stop();
        }
        return ok;
    }

    if (format == FORMAT_WAV) {
        bool ok = loadWav(path, device);
        if (!ok) {
            stop();
        }
        return ok;
    }

    return false;
}

void AudioPlayer::play() {
    if (pcmDev == NULL) return;
    playing = true;
    paused = false;
    eof = false;
}

void AudioPlayer::pause() {
    playing = false;
    paused = true;
}

void AudioPlayer::stop() {
    playing = false;
    paused = false;
    eof = false;

    if (pcmDev != NULL) {
        pcm_close(pcmDev);
        pcmDev = NULL;
    }

    if (format == FORMAT_OGG && oggVf != NULL) {
        ov_clear(oggVf);
        free(oggVf);
        oggVf = NULL;
    }

    if (oggDs != NULL) {
        free(oggDs);
        oggDs = NULL;
    }

    if (sampleBuf != NULL) {
        free(sampleBuf);
        sampleBuf = NULL;
    }

    if (mp3dec != NULL) {
        free(mp3dec);
        mp3dec = NULL;
    }

    if (info != NULL) {
        free(info);
        info = NULL;
    }

    if (oggDecodeBuffer != NULL) {
        free(oggDecodeBuffer);
        oggDecodeBuffer = NULL;
    }

    if (oggStereoBuffer != NULL) {
        free(oggStereoBuffer);
        oggStereoBuffer = NULL;
    }

    if (sourceFd >= 0) {
        close(sourceFd);
        sourceFd = -1;
    }

    if (sourcePath != NULL) {
        free(sourcePath);
        sourcePath = NULL;
    }

    if (mp3StreamBuf != NULL) {
        free(mp3StreamBuf);
        mp3StreamBuf = NULL;
    }

    if (wavStreamBuf != NULL) {
        free(wavStreamBuf);
        wavStreamBuf = NULL;
    }

    if (fileData != NULL) {
        free(fileData);
        fileData = NULL;
    }

    streamPos = NULL;
    bytesLeft = 0;
    totalBytes = 0;
    mp3StreamBufSize = 0;
    mp3StreamEof = false;
    simples = 0;
    currentMs = 0;
    totalMs = 0;
    wavDataOffset = 0;
    wavDataSize = 0;
    wavBitDepth = 16;
    wavBytesPerFrame = 0;
    wavStreamBufSize = 0;
    oggChannels = 2;
    oggBitstream = 0;
    zeroReadCount = 0;
    format = FORMAT_UNKNOWN;
}

void AudioPlayer::replay(const char* device) {
    if (format == FORMAT_MP3 && sourceFd < 0) return;
    if (format == FORMAT_WAV && (sourceFd < 0 || wavStreamBuf == NULL || sampleBuf == NULL)) return;
    if (format == FORMAT_OGG && oggVf == NULL) return;

    if (pcmDev != NULL) {
        pcm_close(pcmDev);
        pcmDev = NULL;
    }

    if (format == FORMAT_MP3) {
        replayMp3(device);
    } else if (format == FORMAT_WAV) {
        replayWav(device);
    } else if (format == FORMAT_OGG) {
        replayOgg(device);
    }
}

void AudioPlayer::reopenDevice(const char* device) {
    struct pcm_config config;

    if (pcmDev != NULL) {
        pcm_close(pcmDev);
        pcmDev = NULL;
    }

    config.bit_depth = 16;
    config.rate = sampleRate;
    config.channels = channels;
    config.period_size = 1024;
    config.period_count = 4;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;

    if (format == FORMAT_WAV) {
        config.bit_depth = wavBitDepth;
    } else if (format == FORMAT_OGG) {
        config.channels = 2;
        config.period_size = 2048;
        config.start_threshold = 2048 * 2;
        config.stop_threshold = 0;
    }

    pcmDev = pcm_open(device, &config);
    if (pcmDev != NULL) {
        devicePath = device;
        writeFailed = false;
        eof = false;
    }
}

bool AudioPlayer::seekToProgress(float progress) {
    bool wasPlaying;
    uint32_t targetMs;
    bool ok;

    if (!isLoaded() || totalMs == 0 || devicePath == NULL) {
        return false;
    }

    if (progress < 0.0f) {
        progress = 0.0f;
    } else if (progress > 1.0f) {
        progress = 1.0f;
    }

    wasPlaying = playing;
    targetMs = (uint32_t)((float)totalMs * progress);

    if (pcmDev != NULL) {
        pcm_close(pcmDev);
        pcmDev = NULL;
    }

    ok = false;
    if (format == FORMAT_MP3) {
        ok = seekMp3(targetMs);
    } else if (format == FORMAT_WAV) {
        ok = seekWav(targetMs);
    } else if (format == FORMAT_OGG) {
        ok = seekOgg(targetMs);
    }

    if (!ok) {
        stop();
        return false;
    }

    reopenDevice(devicePath);
    if (pcmDev == NULL) {
        stop();
        return false;
    }

    currentMs = targetMs;
    if (currentMs > totalMs) {
        currentMs = totalMs;
    }
    paused = !wasPlaying;
    playing = wasPlaying;
    eof = false;
    writeFailed = false;
    return true;
}

bool AudioPlayer::decodeFrame() {
    bool ok;

    if (format == FORMAT_WAV) {
        if (bytesLeft <= 0) {
            eof = true;
            playing = false;
            paused = false;
            return false;
        }
    }

    if (format == FORMAT_MP3) {
        ok = decodeMp3Frame();
    } else if (format == FORMAT_WAV) {
        ok = decodeWavFrame();
    } else if (format == FORMAT_OGG) {
        ok = decodeOggFrame();
    } else {
        ok = false;
    }

    if (!ok && (eof || writeFailed)) {
        playing = false;
        paused = false;
        if (eof && totalMs > 0 && currentMs < totalMs) {
            currentMs = totalMs;
        }
    }

    return ok;
}

bool AudioPlayer::isPlaying() { return playing; }
bool AudioPlayer::isPaused() { return paused; }
bool AudioPlayer::isLoaded() { return pcmDev != NULL; }
bool AudioPlayer::isEof() { return eof; }
bool AudioPlayer::isWriteFailed() { return writeFailed; }
int16_t* AudioPlayer::getSampleBuf() { return sampleBuf; }
int AudioPlayer::getSimples() { return simples; }

int AudioPlayer::getChannels() { return channels; }
int AudioPlayer::getSampleRate() { return sampleRate; }
uint32_t AudioPlayer::getCurrentMs() { return currentMs; }
uint32_t AudioPlayer::getTotalMs() { return totalMs; }
bool AudioPlayer::isOgg() { return format == FORMAT_OGG; }

// Private methods implementation

bool AudioPlayer::loadMp3(const char* path, const char* device) {
    int decodedSamples;

    sourceFd = open(path, O_RDONLY);
    if (sourceFd < 0) {
        return false;
    }

    sourcePath = strdup(path);
    if (sourcePath == NULL) {
        return false;
    }

    mp3StreamBuf = (uint8_t*)malloc(MP3_STREAM_BUFFER_SIZE);
    if (mp3StreamBuf == NULL) {
        return false;
    }
    mp3StreamBufSize = MP3_STREAM_BUFFER_SIZE;
    streamPos = mp3StreamBuf;
    bytesLeft = 0;
    mp3StreamEof = false;
    if (!refillMp3Stream()) {
        return false;
    }

    mp3dec = (mp3dec_t*)calloc(1, sizeof(mp3dec_t));
    info = (mp3dec_frame_info_t*)calloc(1, sizeof(mp3dec_frame_info_t));
    sampleBuf = (int16_t*)calloc(MINIMP3_MAX_SAMPLES_PER_FRAME, sizeof(int16_t));
    if (mp3dec == NULL || info == NULL || sampleBuf == NULL) {
        return false;
    }
    mp3dec_init(mp3dec);

    decodedSamples = decodeNextMp3Frame(mp3dec, info, sampleBuf);
    if (decodedSamples <= 0) {
        return false;
    }

    channels = info->channels;
    sampleRate = info->hz;
    simples = 1;

    struct pcm_config config;
    config.bit_depth = 16;
    config.rate = sampleRate;
    config.channels = channels;
    config.period_size = 1024;
    config.period_count = 4;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;

    pcmDev = pcm_open(device, &config);
    if (pcmDev == NULL) {
        return false;
    }

    devicePath = device;
    if (!resetMp3Stream()) {
        return false;
    }
    mp3dec_init(mp3dec);
    totalMs = estimateTotalMs();
    return true;
}

bool AudioPlayer::loadWav(const char* path, const char* device) {
    int wavSampleRate, wavChannels, wavBits;
    int previewSamples;

    sourceFd = open(path, O_RDONLY);
    if (sourceFd < 0) {
        return false;
    }

    if (parseWavHeaderFd(sourceFd, &wavSampleRate, &wavChannels, &wavBits, &wavDataOffset, &wavDataSize) != 0) {
        return false;
    }

    sourcePath = strdup(path);
    if (sourcePath == NULL) {
        return false;
    }

    sampleRate = wavSampleRate;
    channels = wavChannels;
    wavBitDepth = wavBits;
    wavBytesPerFrame = channels * (wavBitDepth / 8);
    if (wavBytesPerFrame <= 0) {
        return false;
    }

    wavStreamBufSize = wavBytesPerFrame * WAV_WRITE_CHUNK_FRAMES;
    wavStreamBuf = (uint8_t*)malloc(wavStreamBufSize);
    if (wavStreamBuf == NULL) {
        return false;
    }

    previewSamples = WAV_WRITE_CHUNK_FRAMES * channels;
    sampleBuf = (int16_t*)calloc(previewSamples, sizeof(int16_t));
    if (sampleBuf == NULL) {
        return false;
    }

    if (lseek(sourceFd, wavDataOffset, SEEK_SET) < 0) {
        return false;
    }

    streamPos = wavStreamBuf;
    bytesLeft = wavDataSize;
    simples = 0;

    struct pcm_config config;
    config.bit_depth = wavBitDepth;
    config.rate = sampleRate;
    config.channels = channels;
    config.period_size = 1024;
    config.period_count = 4;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;

    pcmDev = pcm_open(device, &config);
    if (pcmDev == NULL) {
        return false;
    }

    devicePath = device;
    totalMs = estimateTotalMs();
    return true;
}

bool AudioPlayer::loadOgg(const char* path, const char* device) {
    int fd;

    oggVf = (OggVorbis_File*)calloc(1, sizeof(OggVorbis_File));
    oggDs = (OggVorbis_DataSource*)calloc(1, sizeof(OggVorbis_DataSource));
    if (oggVf == NULL || oggDs == NULL) {
        return false;
    }

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return false;
    }
    oggDs->fd = fd;

    ov_callbacks callbacks;
    callbacks.read_func = ogg_read_func;
    callbacks.seek_func = ogg_seek_func;
    callbacks.close_func = ogg_close_func;
    callbacks.tell_func = ogg_tell_func;

    if (ov_open_callbacks(oggDs, oggVf, NULL, 0, callbacks) < 0) {
        return false;
    }

    vorbis_info *vi = ov_info(oggVf, -1);
    if (vi == NULL) {
        return false;
    }
    int hs = ov_halfrate_p(oggVf);
    if (hs < 0) {
        hs = 0;
    }

    sampleRate = vi->rate >> hs;
    if (sampleRate <= 0) {
        sampleRate = vi->rate;
    }
    oggChannels = vi->channels;
    channels = 2;
    simples = 0;

    struct pcm_config config;
    config.bit_depth = 16;
    config.rate = sampleRate;
    config.channels = 2;
    config.period_size = 2048;
    config.period_count = 4;
    config.start_threshold = 2048 * 2;
    config.stop_threshold = 0;

    pcmDev = pcm_open(device, &config);
    if (pcmDev == NULL) {
        return false;
    }

    sampleBuf = (int16_t*)calloc(OGG_WRITE_CHUNK_FRAMES * 2, sizeof(int16_t));
    if (sampleBuf == NULL) {
        return false;
    }

    devicePath = device;
    totalMs = estimateTotalMs();
    return true;
}

void AudioPlayer::replayMp3(const char* device) {
    if (!resetMp3Stream()) {
        eof = true;
        return;
    }
    currentMs = 0;
    eof = false;
    writeFailed = false;
    paused = false;
    simples = 1;

    mp3dec_init(mp3dec);

    struct pcm_config config;
    config.bit_depth = 16;
    config.rate = sampleRate;
    config.channels = channels;
    config.period_size = 1024;
    config.period_count = 4;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;

    pcmDev = pcm_open(device, &config);
    if (pcmDev != NULL) {
        devicePath = device;
    }
}

void AudioPlayer::replayWav(const char* device) {
    if (sourceFd < 0 || wavStreamBuf == NULL || sampleBuf == NULL) {
        eof = true;
        return;
    }
    if (lseek(sourceFd, wavDataOffset, SEEK_SET) < 0) {
        eof = true;
        return;
    }

    streamPos = wavStreamBuf;
    bytesLeft = wavDataSize;
    currentMs = 0;
    eof = false;
    writeFailed = false;
    simples = 0;

    struct pcm_config config;
    config.bit_depth = wavBitDepth;
    config.rate = sampleRate;
    config.channels = channels;
    config.period_size = 1024;
    config.period_count = 4;
    config.start_threshold = 1024;
    config.stop_threshold = 1024 * 4;

    pcmDev = pcm_open(device, &config);
    if (pcmDev != NULL) {
        devicePath = device;
    }
}

void AudioPlayer::replayOgg(const char* device) {
    if (oggVf == NULL) {
        eof = true;
        return;
    }

    if (ov_pcm_seek(oggVf, 0) != 0) {
        eof = true;
        return;
    }

    currentMs = 0;
    eof = false;
    writeFailed = false;
    simples = 0;
    zeroReadCount = 0;

    struct pcm_config config;
    config.bit_depth = 16;
    config.rate = sampleRate;
    config.channels = 2;
    config.period_size = 2048;
    config.period_count = 4;
    config.start_threshold = 2048 * 2;
    config.stop_threshold = 0;

    pcmDev = pcm_open(device, &config);
    if (pcmDev != NULL) {
        devicePath = device;
    }
}

bool AudioPlayer::seekMp3(uint32_t targetMs) {
    mp3dec_t scratchDec;
    mp3dec_frame_info_t scratchInfo;
    int16_t scratchBuf[MINIMP3_MAX_SAMPLES_PER_FRAME];
    uint32_t elapsedMs;
    int decodedSamples;

    if (sourceFd < 0 || sampleRate <= 0 || !resetMp3Stream()) {
        return false;
    }

    mp3dec_init(&scratchDec);
    elapsedMs = 0;

    while (elapsedMs < targetMs) {
        decodedSamples = decodeNextMp3Frame(&scratchDec, &scratchInfo, scratchBuf);
        if (decodedSamples <= 0) {
            break;
        }

        if (scratchInfo.hz > 0) {
            uint32_t frameMs = (uint32_t)((decodedSamples * 1000) / scratchInfo.hz);
            if ((elapsedMs + frameMs) > targetMs) {
                break;
            }
            elapsedMs += frameMs;
        }
    }

    simples = 1;
    eof = false;
    writeFailed = false;
    paused = false;
    if (mp3dec != NULL) {
        mp3dec_init(mp3dec);
    }
    return true;
}

bool AudioPlayer::seekWav(uint32_t targetMs) {
    uint64_t targetFrame;
    uint64_t byteOffset;

    if (sourceFd < 0 || sampleRate <= 0 || channels <= 0 || wavBitDepth <= 0 || wavBytesPerFrame <= 0) {
        return false;
    }

    targetFrame = ((uint64_t)targetMs * (uint64_t)sampleRate) / 1000ULL;
    byteOffset = targetFrame * (uint64_t)wavBytesPerFrame;
    if (byteOffset > (uint64_t)wavDataSize) {
        byteOffset = wavDataSize;
    }

    if (lseek(sourceFd, wavDataOffset + (int)byteOffset, SEEK_SET) < 0) {
        return false;
    }

    streamPos = wavStreamBuf;
    bytesLeft = wavDataSize - (int)byteOffset;
    simples = 0;
    eof = (bytesLeft <= 0);
    writeFailed = false;
    paused = false;
    return true;
}

bool AudioPlayer::seekOgg(uint32_t targetMs) {
    double targetSec;

    if (oggVf == NULL) {
        return false;
    }

    targetSec = (double)targetMs / 1000.0;
    if (ov_time_seek(oggVf, targetSec) != 0) {
        return false;
    }

    simples = 0;
    eof = false;
    writeFailed = false;
    paused = false;
    zeroReadCount = 0;
    oggBitstream = 0;
    return true;
}

bool AudioPlayer::decodeMp3Frame() {
    int decodedSamples;

    if (eof) {
        return false;
    }
    decodedSamples = decodeNextMp3Frame(mp3dec, info, sampleBuf);
    if (decodedSamples < 0) {
        return false;
    }
    if (decodedSamples == 0) {
        eof = true;
        return false;
    }

    simples = decodedSamples;

    if (playing) {
        int ret = pcm_write(pcmDev, sampleBuf, decodedSamples * 2 * channels);
        if (ret != 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        currentMs += (decodedSamples * 1000) / sampleRate;
    }

    return true;
}

bool AudioPlayer::decodeWavFrame() {
    int toWrite;
    int totalRead;
    int frames;

    if (eof) {
        return false;
    }

    if (sourceFd < 0 || wavStreamBuf == NULL || sampleBuf == NULL || wavBytesPerFrame <= 0) {
        eof = true;
        return false;
    }

    toWrite = wavBytesPerFrame * WAV_WRITE_CHUNK_FRAMES;
    if (toWrite > bytesLeft) {
        toWrite = bytesLeft;
    }
    toWrite -= (toWrite % wavBytesPerFrame);
    if (toWrite <= 0) {
        eof = true;
        return false;
    }

    totalRead = 0;
    while (totalRead < toWrite) {
        int rd = read(sourceFd, wavStreamBuf + totalRead, toWrite - totalRead);
        if (rd < 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        if (rd == 0) {
            break;
        }
        totalRead += rd;
    }

    totalRead -= (totalRead % wavBytesPerFrame);
    if (totalRead <= 0) {
        eof = true;
        return false;
    }

    frames = totalRead / wavBytesPerFrame;
    wavFillPreviewSamples(wavStreamBuf, frames, channels, wavBitDepth, sampleBuf);
    simples = frames;

    if (playing) {
        int ret = pcm_write(pcmDev, wavStreamBuf, totalRead);
        if (ret != 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        currentMs += (frames * 1000) / sampleRate;
    }

    bytesLeft -= totalRead;
    if (bytesLeft < 0 || totalRead < toWrite) {
        bytesLeft = 0;
    }
    streamPos = wavStreamBuf;

    return true;
}

bool AudioPlayer::decodeOggFrame() {
    float** pcmChannels = NULL;
    long framesRead;
    int samples;

    if (eof) {
        return false;
    }

    if (oggVf == NULL || sampleBuf == NULL) {
        eof = true;
        return false;
    }

    framesRead = ov_read_float(oggVf, &pcmChannels, OGG_DECODE_FRAMES, &oggBitstream);
    if (framesRead == 0) {
        eof = true;
        return false;
    }
    if (framesRead < 0) {
        eof = true;
        writeFailed = true;
        return false;
    }

    samples = (int)framesRead;
    if (samples > OGG_DECODE_FRAMES) {
        samples = OGG_DECODE_FRAMES;
    }

    for (int i = 0; i < samples; i++) {
        float left = pcmChannels[0][i];
        float right = (oggChannels > 1) ? pcmChannels[1][i] : left;
        sampleBuf[i * 2] = ogg_float_to_s16(left);
        sampleBuf[i * 2 + 1] = ogg_float_to_s16(right);
    }
    simples = samples;

    if (playing) {
        int ret = pcm_write(pcmDev, sampleBuf, samples * channels * (int)sizeof(int16_t));
        if (ret != 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        currentMs += (uint32_t)((samples * 1000) / sampleRate);
    }

    return true;
}

uint32_t AudioPlayer::estimateTotalMs() {
    if (format == FORMAT_OGG) {
        double totalSec = ov_time_total(oggVf, -1);
        if (totalSec > 0.0) {
            return (uint32_t)(totalSec * 1000.0);
        }
        long totalSamples = ov_pcm_total(oggVf, -1);
        if (totalSamples > 0 && sampleRate > 0) {
            return (uint32_t)((totalSamples * 1000) / sampleRate);
        }
    }

    if (format == FORMAT_WAV) {
        int bytesPerFrame = channels * (wavBitDepth / 8);
        if (bytesPerFrame > 0 && sampleRate > 0) {
            return (uint32_t)(((uint64_t)wavDataSize * 1000ULL) /
                              ((uint64_t)sampleRate * (uint64_t)bytesPerFrame));
        }
        return 0;
    }

    if (format == FORMAT_MP3) {
        return estimateMp3StreamTotalMs();
    }

    int bytesPerSample = 2 * channels;
    int bytesPerSecond = sampleRate * bytesPerSample;
    if (bytesPerSecond > 0) {
        return (uint32_t)((totalBytes * 1000) / bytesPerSecond);
    }
    return 0;
}
