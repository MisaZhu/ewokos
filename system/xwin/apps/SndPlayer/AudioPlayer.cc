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

// OGG Vorbis file callbacks
static size_t ogg_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    size_t bytes_available;
    size_t items_to_read;
    size_t bytes_to_read;

    if (size == 0 || nmemb == 0) {
        return 0;
    }

    bytes_available = ds->size - ds->pos;
    items_to_read = bytes_available / size;
    if (items_to_read > nmemb) {
        items_to_read = nmemb;
    }
    bytes_to_read = items_to_read * size;

    if (items_to_read > 0) {
        memcpy(ptr, ds->data + ds->pos, bytes_to_read);
        ds->pos += bytes_to_read;
    }

    return items_to_read;
}

static int ogg_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    int new_pos = 0;

    switch (whence) {
        case SEEK_SET:
            new_pos = (int)offset;
            break;
        case SEEK_CUR:
            new_pos = ds->pos + (int)offset;
            break;
        case SEEK_END:
            new_pos = ds->size + (int)offset;
            break;
        default:
            return -1;
    }

    if (new_pos < 0 || new_pos > ds->size) {
        return -1;
    }

    ds->pos = new_pos;
    return 0;
}

static int ogg_close_func(void *datasource)
{
    return 0;
}

static long ogg_tell_func(void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    return ds->pos;
}

static int g_ogg_decode_debug_calls = 0;

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
    enum {
        SLEEP_TIME_MS = 5,
    };
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

static int pcm_write(struct pcm_t *pcm, const void* data, unsigned int count) {
    if (count == 0) return 0;

    int period_bytes = pcm->config.period_size * 4;
    int avail = 0;
    int bytes = (int)count;
    int written = 0;
    int offset = 0;
    int copy_bytes = 0;
    int ret = 0;

    copy_bytes = bytes < period_bytes ? bytes : period_bytes;
    while (bytes > 0) {
        ret = wait_avail(pcm, &avail, 2000);
        if (ret < 0 || (avail == 0 && bytes > 0)) {
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

// Parse WAV file header, return data offset or -1 on error
static int parseWavHeader(const uint8_t* data, int size, int* sampleRate, int* channels, int* bitDepth, int* dataOffset, int* dataSize) {
    if (size < 44) return -1;
    
    const wav_riff_header* riff = (const wav_riff_header*)data;
    if (riff->riff_id != ID_RIFF || riff->wave_id != ID_WAVE) {
        return -1;
    }
    
    int pos = sizeof(wav_riff_header);
    wav_chunk_fmt fmt;
    int foundFmt = 0;
    int foundData = 0;
    
    while (pos + sizeof(wav_chunk_header) <= size) {
        const wav_chunk_header* chunk = (const wav_chunk_header*)(data + pos);
        pos += sizeof(wav_chunk_header);
        
        if (chunk->id == ID_FMT) {
            if (pos + sizeof(wav_chunk_fmt) > size) return -1;
            memcpy(&fmt, data + pos, sizeof(wav_chunk_fmt));
            foundFmt = 1;
            pos += chunk->sz;
        } else if (chunk->id == ID_DATA) {
            *dataOffset = pos;
            *dataSize = chunk->sz;
            foundData = 1;
            break;
        } else {
            pos += chunk->sz;
        }
    }
    
    if (!foundFmt || !foundData) return -1;
    
    *sampleRate = fmt.sample_rate;
    *channels = fmt.num_channels;
    *bitDepth = fmt.bits_per_sample;
    
    return 0;
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

// AudioPlayer implementation
AudioPlayer::AudioPlayer() {
    pcmDev = NULL;
    fileData = NULL;
    streamPos = NULL;
    bytesLeft = 0;
    totalBytes = 0;
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

    fileData = vfs_readfile(path, &bytesLeft);
    if (fileData == NULL) {
        return false;
    }

    totalBytes = bytesLeft;
    streamPos = (uint8_t*)fileData;

    if (format == FORMAT_MP3) {
        return loadMp3(device);
    } else if (format == FORMAT_WAV) {
        return loadWav(device);
    } else if (format == FORMAT_OGG) {
        return loadOgg(device);
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
    }

    if (fileData != NULL) {
        free(fileData);
        fileData = NULL;
    }
}

void AudioPlayer::replay(const char* device) {
    if (fileData == NULL) return;

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
    if (pcmDev != NULL) {
        pcm_close(pcmDev);
        pcmDev = NULL;
    }

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
        writeFailed = false;
        eof = false;
    }
}

bool AudioPlayer::decodeFrame() {
    if (format == FORMAT_MP3 || format == FORMAT_WAV) {
        if (streamPos == NULL || bytesLeft <= 0) {
            eof = true;
            return false;
        }
    }

    if (format == FORMAT_MP3) {
        return decodeMp3Frame();
    } else if (format == FORMAT_WAV) {
        return decodeWavFrame();
    } else if (format == FORMAT_OGG) {
        return decodeOggFrame();
    }

    return false;
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

bool AudioPlayer::loadMp3(const char* device) {
    mp3dec = (mp3dec_t*)calloc(1, sizeof(mp3dec_t));
    info = (mp3dec_frame_info_t*)calloc(1, sizeof(mp3dec_frame_info_t));
    sampleBuf = (int16_t*)calloc(MINIMP3_MAX_SAMPLES_PER_FRAME, sizeof(int16_t));
    if (mp3dec == NULL || info == NULL || sampleBuf == NULL) {
        return false;
    }
    mp3dec_init(mp3dec);

    int decodedSamples = mp3dec_decode_frame(mp3dec, streamPos, bytesLeft, sampleBuf, info);
    if (decodedSamples == 0 || info->frame_bytes == 0) {
        return false;
    }

    channels = info->channels;
    sampleRate = info->hz;
    simples = decodedSamples;

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
    totalMs = estimateTotalMs();
    return true;
}

bool AudioPlayer::loadWav(const char* device) {
    int wavSampleRate, wavChannels, wavBits;
    if (parseWavHeader((const uint8_t*)fileData, totalBytes, &wavSampleRate, &wavChannels, &wavBits, &wavDataOffset, &wavDataSize) != 0) {
        return false;
    }

    sampleRate = wavSampleRate;
    channels = wavChannels;
    wavBitDepth = wavBits;

    streamPos = (uint8_t*)fileData + wavDataOffset;
    bytesLeft = wavDataSize;
    sampleBuf = (int16_t*)streamPos;
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

bool AudioPlayer::loadOgg(const char* device) {
    int fileAvgBitrate = 0;
    int ovAvgBitrate = 0;
    oggVf = (OggVorbis_File*)calloc(1, sizeof(OggVorbis_File));
    oggDs = (OggVorbis_DataSource*)calloc(1, sizeof(OggVorbis_DataSource));
    if (oggVf == NULL || oggDs == NULL) {
        klog("[snd/ogg] loadOgg alloc failed\n");
        return false;
    }

    oggDs->data = (uint8_t*)fileData;
    oggDs->size = totalBytes;
    oggDs->pos = 0;

    ov_callbacks callbacks;
    callbacks.read_func = ogg_read_func;
    callbacks.seek_func = ogg_seek_func;
    callbacks.close_func = ogg_close_func;
    callbacks.tell_func = ogg_tell_func;

    if (ov_open_callbacks(oggDs, oggVf, NULL, 0, callbacks) < 0) {
        klog("[snd/ogg] ov_open_callbacks failed\n");
        return false;
    }

    vorbis_info *vi = ov_info(oggVf, -1);
    if (vi == NULL) {
        klog("[snd/ogg] ov_info failed\n");
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
    channels = oggChannels;
    simples = 1;

    oggDecodeBuffer = (char*)malloc(8192);
    if (oggChannels == 1) {
        oggStereoBuffer = (char*)malloc(8192 * 2);
    }

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

    sampleBuf = (int16_t*)calloc(MINIMP3_MAX_SAMPLES_PER_FRAME, sizeof(int16_t));
    if (sampleBuf == NULL) {
        return false;
    }

    devicePath = device;
    totalMs = estimateTotalMs();
    if (totalMs > 0) {
        fileAvgBitrate = (int)(((uint64_t)totalBytes * 8000ULL) / (uint64_t)totalMs);
    }
    ovAvgBitrate = (int)ov_bitrate(oggVf, -1);
    klog("[snd/ogg] stream rate=%d out_rate=%d channels=%d nominal_br=%d file_avg_br=%d ov_avg_br=%d total_ms=%d seekable=%d halfrate=%d\n",
         (int)vi->rate,
         sampleRate,
         (int)vi->channels,
         (int)vi->bitrate_nominal,
         fileAvgBitrate,
         ovAvgBitrate,
         (int)totalMs,
         (int)oggVf->seekable,
         hs);
    return true;
}

void AudioPlayer::replayMp3(const char* device) {
    streamPos = (uint8_t*)fileData;
    bytesLeft = totalBytes;
    currentMs = 0;
    eof = false;
    writeFailed = false;

    mp3dec_init(mp3dec);

    int decodedSamples = mp3dec_decode_frame(mp3dec, streamPos, bytesLeft, sampleBuf, info);
    if (decodedSamples > 0 && info->frame_bytes > 0) {
        simples = decodedSamples;
        streamPos += info->frame_bytes;
        bytesLeft -= info->frame_bytes;
    }

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
    streamPos = (uint8_t*)fileData + wavDataOffset;
    bytesLeft = wavDataSize;
    currentMs = 0;
    eof = false;
    writeFailed = false;
    sampleBuf = (int16_t*)streamPos;
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
    if (oggVf == NULL || oggDs == NULL) {
        eof = true;
        return;
    }

    ov_clear(oggVf);
    memset(oggVf, 0, sizeof(OggVorbis_File));
    oggDs->pos = 0;
    ov_callbacks callbacks;
    callbacks.read_func = ogg_read_func;
    callbacks.seek_func = ogg_seek_func;
    callbacks.close_func = ogg_close_func;
    callbacks.tell_func = ogg_tell_func;

    if (ov_open_callbacks(oggDs, oggVf, NULL, 0, callbacks) < 0) {
        klog("[snd/ogg] replay ov_open_callbacks failed\n");
        eof = true;
        writeFailed = true;
        return;
    }

    currentMs = 0;
    eof = false;
    writeFailed = false;
    simples = 1;
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

bool AudioPlayer::decodeMp3Frame() {
    if (eof) {
        return false;
    }

    if (simples == 0) {
        eof = true;
        return false;
    }

    int decodedSamples = mp3dec_decode_frame(mp3dec, streamPos, bytesLeft, sampleBuf, info);

    if (decodedSamples == 0) {
        return false;
    }

    if (playing) {
        int ret = pcm_write(pcmDev, sampleBuf, decodedSamples * 2 * channels);
        if (ret != 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        currentMs += (decodedSamples * 1000) / sampleRate;
        streamPos += info->frame_bytes;
        bytesLeft -= info->frame_bytes;
    } else {
        streamPos += info->frame_bytes;
        bytesLeft -= info->frame_bytes;
    }

    return true;
}

bool AudioPlayer::decodeWavFrame() {
    if (eof) {
        return false;
    }

    int bytesPerSample = (wavBitDepth / 8) * channels;
    int toWrite = bytesPerSample * 1024;
    if (toWrite > bytesLeft) {
        toWrite = bytesLeft;
    }
    if (toWrite <= 0) {
        eof = true;
        return false;
    }

    sampleBuf = (int16_t*)streamPos;
    simples = toWrite / bytesPerSample;

    if (playing) {
        int ret = pcm_write(pcmDev, streamPos, toWrite);
        if (ret != 0) {
            eof = true;
            writeFailed = true;
            return false;
        }
        int samples = toWrite / bytesPerSample;
        currentMs += (samples * 1000) / sampleRate;
        streamPos += toWrite;
        bytesLeft -= toWrite;
    } else {
        streamPos += toWrite;
        bytesLeft -= toWrite;
    }

    return true;
}

bool AudioPlayer::decodeOggFrame() {
    if (eof) {
        return false;
    }

    float **pcm_channels = NULL;
    long frames_read;
    int retries = 0;
    int decodeCall = g_ogg_decode_debug_calls++;

    if (decodeCall < 8) {
        klog("[snd/ogg] decode[%d] enter pos=%d eof=%d playing=%d\n",
             decodeCall, oggDs == NULL ? -1 : oggDs->pos, eof, playing);
    }

    do {
        frames_read = ov_read_float(oggVf, &pcm_channels, MINIMP3_MAX_SAMPLES_PER_FRAME / 2, &oggBitstream);
        if (decodeCall < 8) {
            klog("[snd/ogg] decode[%d] ov_read_float=%d retry=%d pos=%d bitstream=%d\n",
                 decodeCall, (int)frames_read, retries,
                 oggDs == NULL ? -1 : oggDs->pos, oggBitstream);
        }
        if (frames_read > 0) {
            zeroReadCount = 0;
            break;
        }
        if (frames_read == 0) {
            if (++zeroReadCount >= 2) {
                eof = true;
                return false;
            }
            continue;
        }
    } while (++retries < 4);

    if (frames_read == 0) {
        eof = true;
        return false;
    }

    if (frames_read < 0) {
        eof = true;
        return false;
    }

    int samples = (int)frames_read;
    if (samples > MINIMP3_MAX_SAMPLES_PER_FRAME / 2) {
        samples = MINIMP3_MAX_SAMPLES_PER_FRAME / 2;
    }

    for (int i = 0; i < samples; i++) {
        float left = pcm_channels[0][i];
        float right = (oggChannels > 1) ? pcm_channels[1][i] : left;
        sampleBuf[i * 2] = ogg_float_to_s16(left);
        sampleBuf[i * 2 + 1] = ogg_float_to_s16(right);
    }

    char *output_buf = (char *)sampleBuf;
    int output_bytes = samples * 4;

    if (oggChannels == 1 && oggStereoBuffer != NULL) {
        memcpy(oggStereoBuffer, output_buf, output_bytes);
        output_buf = oggStereoBuffer;
    }

    simples = samples;

    if (decodeCall < 8) {
        int16_t s0 = samples > 0 ? sampleBuf[0] : 0;
        int16_t s1 = samples > 0 ? sampleBuf[1] : 0;
        klog("[snd/ogg] decode[%d] samples=%d out_bytes=%d first=%d,%d\n",
             decodeCall, samples, output_bytes, s0, s1);
    }

    if (playing) {
        int ret = pcm_write(pcmDev, output_buf, output_bytes);
        if (decodeCall < 8) {
            klog("[snd/ogg] decode[%d] pcm_write ret=%d\n", decodeCall, ret);
        }
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
        return estimateMp3TotalMs((const uint8_t*)fileData, totalBytes);
    }

    int bytesPerSample = 2 * channels;
    int bytesPerSecond = sampleRate * bytesPerSample;
    if (bytesPerSecond > 0) {
        return (uint32_t)((totalBytes * 1000) / bytesPerSecond);
    }
    return 0;
}
