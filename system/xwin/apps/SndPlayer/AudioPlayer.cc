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

#define MINIMP3_IMPLEMENTATION
#include <minimp3/minimp3.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

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

enum AudioFormat {
    FORMAT_UNKNOWN,
    FORMAT_MP3,
    FORMAT_WAV,
    FORMAT_OGG
};

#define CTRL_PCM_DEV_HW         (0xF0)
#define CTRL_PCM_DEV_PRPARE     (0xF2)
#define CTRL_PCM_BUF_AVAIL      (0xF3)

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

// OGG Vorbis file callbacks
typedef struct {
    uint8_t *data;
    int size;
    int pos;
} OggVorbis_DataSource;

static size_t ogg_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
    OggVorbis_DataSource *ds = (OggVorbis_DataSource *)datasource;
    size_t bytes_to_read = size * nmemb;
    size_t bytes_available = ds->size - ds->pos;

    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
    }

    if (bytes_to_read > 0) {
        memcpy(ptr, ds->data + ds->pos, bytes_to_read);
        ds->pos += bytes_to_read;
    }

    return bytes_to_read;
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

typedef int (*pcm_hook_t)(void *data);

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

class AudioPlayer {
public:
    AudioPlayer() {
        pcmDev = NULL;
        fileData = NULL;
        streamPos = NULL;
        bytesLeft = 0;
        totalBytes = 0;
        simples = 0;
        playing = false;
        paused = false;
        eof = false;
        currentMs = 0;
        totalMs = 0;
        format = FORMAT_UNKNOWN;
        wavDataOffset = 0;
        wavDataSize = 0;
        wavBitDepth = 16;
        oggChannels = 2;
        oggBitstream = 0;
        zeroReadCount = 0;
        oggDs.data = NULL;
        oggDs.size = 0;
        oggDs.pos = 0;
        memset(&mp3dec, 0, sizeof(mp3dec));
        memset(&info, 0, sizeof(info));
    }

    ~AudioPlayer() {
        stop();
    }

    bool load(const char* path, const char* device) {
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

    bool loadMp3(const char* device) {
        mp3dec_init(&mp3dec);
        simples = mp3dec_decode_frame(&mp3dec, streamPos, bytesLeft, sampleBuf, &info);

        if (simples == 0) {
            free(fileData);
            fileData = NULL;
            return false;
        }

        devicePath = device;
        channels = info.channels;
        if (channels != 1 && channels != 2) channels = 2;

        sampleRate = info.hz;
        if (sampleRate != 8000 && sampleRate != 16000 && sampleRate != 32000 &&
            sampleRate != 44100 && sampleRate != 48000 && sampleRate != 96000) {
            sampleRate = 44100;
        }

        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = 16;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 1024;
        config.period_count = 4;
        config.start_threshold = 1024 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            free(fileData);
            fileData = NULL;
            return false;
        }

        streamPos += info.frame_bytes;
        bytesLeft -= info.frame_bytes;

        totalMs = estimateTotalMs();
        currentMs = 0;
        playing = false;
        paused = false;

        return true;
    }

    bool loadWav(const char* device) {
        int dataOffset, dataSize;
        if (parseWavHeader((uint8_t*)fileData, totalBytes, &sampleRate, &channels, &wavBitDepth, &dataOffset, &dataSize) < 0) {
            free(fileData);
            fileData = NULL;
            return false;
        }

        devicePath = device;
        if (channels != 1 && channels != 2) channels = 2;

        if (sampleRate != 8000 && sampleRate != 16000 && sampleRate != 32000 &&
            sampleRate != 44100 && sampleRate != 48000 && sampleRate != 96000) {
            sampleRate = 44100;
        }

        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = wavBitDepth;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 2048;
        config.period_count = 2;
        config.start_threshold = 2048 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            free(fileData);
            fileData = NULL;
            return false;
        }

        wavDataOffset = dataOffset;
        wavDataSize = dataSize;
        streamPos = (uint8_t*)fileData + wavDataOffset;
        bytesLeft = wavDataSize;

        // Estimate total time for WAV
        int bytesPerSample = (wavBitDepth / 8) * channels;
        int totalSamples = wavDataSize / bytesPerSample;
        totalMs = (totalSamples * 1000) / sampleRate;
        currentMs = 0;
        playing = false;
        paused = false;

        return true;
    }

    void play() {
        if (pcmDev == NULL) return;
        playing = true;
        paused = false;
        eof = false;
    }

    void pause() {
        playing = false;
        paused = true;
    }

    void stop() {
        playing = false;
        paused = false;
        eof = false;

        if (pcmDev != NULL) {
            pcm_close(pcmDev);
            pcmDev = NULL;
        }

        if (format == FORMAT_OGG) {
            ov_clear(&oggVf);
            oggDs.data = NULL;
            oggDs.size = 0;
            oggDs.pos = 0;
        }

        if (fileData != NULL) {
            free(fileData);
            fileData = NULL;
        }
    }

    void replay(const char* device) {
        if (fileData == NULL) return;

        // Close existing PCM device if open
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

    void replayMp3(const char* device) {
        // Reopen PCM device
        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = 16;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 1024;
        config.period_count = 4;
        config.start_threshold = 1024 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            return;
        }

        // Reset stream position
        streamPos = (uint8_t*)fileData;
        bytesLeft = totalBytes;

        int firstSamples = mp3dec_decode_frame(&mp3dec, streamPos, bytesLeft, sampleBuf, &info);
        if (firstSamples == 0) {
            pcm_close(pcmDev);
            pcmDev = NULL;
            return;
        }

        streamPos += info.frame_bytes;
        bytesLeft -= info.frame_bytes;

        currentMs = 0;
        playing = true;
        paused = false;
        eof = false;
    }

    void replayWav(const char* device) {
        // Reopen PCM device
        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = wavBitDepth;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 2048;
        config.period_count = 2;
        config.start_threshold = 2048 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            return;
        }

        // Reset stream position
        streamPos = (uint8_t*)fileData + wavDataOffset;
        bytesLeft = wavDataSize;

        currentMs = 0;
        playing = true;
        paused = false;
        eof = false;
    }

    bool isPlaying() { return playing; }
    bool isPaused() { return paused; }
    bool isLoaded() { return pcmDev != NULL; }
    bool isEof() { return eof; }
    int16_t* getSampleBuf() { return sampleBuf; }
    int getSimples() { return simples; }
    int getChannels() { return channels; }
    int getSampleRate() { return sampleRate; }
    uint32_t getCurrentMs() { return currentMs; }
    uint32_t getTotalMs() { return totalMs; }

    bool decodeFrame() {
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

    bool decodeMp3Frame() {
        if (simples == 0) {
            eof = true;
            return false;
        }

        int decodedSamples = mp3dec_decode_frame(&mp3dec, streamPos, bytesLeft, sampleBuf, &info);
        if (decodedSamples == 0) {
            return false;
        }

        if (playing) {
            int ret = pcm_write(pcmDev, sampleBuf, decodedSamples * 2 * channels);
            if (ret == 0) {
                currentMs += (decodedSamples * 1000) / sampleRate;
                streamPos += info.frame_bytes;
                bytesLeft -= info.frame_bytes;
            }
        } else {
            streamPos += info.frame_bytes;
            bytesLeft -= info.frame_bytes;
        }

        return true;
    }

    bool decodeWavFrame() {
        int periodBytes = 2048 * (wavBitDepth / 8) * channels;
        int toWrite = bytesLeft < periodBytes ? bytesLeft : periodBytes;

        if (toWrite <= 0) {
            eof = true;
            return false;
        }

        if (playing) {
            int ret = pcm_write(pcmDev, streamPos, toWrite);
            if (ret == 0) {
                int bytesPerSample = (wavBitDepth / 8) * channels;
                int samples = toWrite / bytesPerSample;
                currentMs += (samples * 1000) / sampleRate;
                streamPos += toWrite;
                bytesLeft -= toWrite;
            }
        } else {
            streamPos += toWrite;
            bytesLeft -= toWrite;
        }

        // For WAV, we need to fill the sample buffer for spectrum visualization
        int samplesToCopy = toWrite / ((wavBitDepth / 8) * channels);
        if (samplesToCopy > MINIMP3_MAX_SAMPLES_PER_FRAME) samplesToCopy = MINIMP3_MAX_SAMPLES_PER_FRAME;
        
        // Convert to 16-bit samples for spectrum
        if (wavBitDepth == 16) {
            memcpy(sampleBuf, streamPos - toWrite, samplesToCopy * 2 * channels);
        } else if (wavBitDepth == 8) {
            for (int i = 0; i < samplesToCopy * channels; i++) {
                int8_t sample = *((int8_t*)(streamPos - toWrite) + i);
                sampleBuf[i] = sample << 8;
            }
        }
        simples = samplesToCopy;

        return true;
    }

    bool loadOgg(const char* device) {
        oggDs.data = (uint8_t*)fileData;
        oggDs.size = totalBytes;
        oggDs.pos = 0;

        ov_callbacks callbacks;
        callbacks.read_func = ogg_read_func;
        callbacks.seek_func = ogg_seek_func;
        callbacks.close_func = ogg_close_func;
        callbacks.tell_func = ogg_tell_func;

        if (ov_open_callbacks(&oggDs, &oggVf, NULL, 0, callbacks) < 0) {
            free(fileData);
            fileData = NULL;
            return false;
        }

        vorbis_info *vi = ov_info(&oggVf, -1);
        devicePath = device;
        channels = vi->channels;
        if (channels != 1 && channels != 2) channels = 2;

        // Save original OGG channel count for decoding
        oggChannels = channels;

        // Force stereo output for PCM device
        channels = 2;

        sampleRate = vi->rate;
        if (sampleRate != 8000 && sampleRate != 16000 && sampleRate != 32000 &&
            sampleRate != 44100 && sampleRate != 48000 && sampleRate != 96000) {
            sampleRate = 44100;
        }

        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = 16;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 2048;
        config.period_count = 4;
        config.start_threshold = 2048 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            ov_clear(&oggVf);
            free(fileData);
            fileData = NULL;
            return false;
        }

        ogg_int64_t totalTime = ov_time_total(&oggVf, -1);
        totalMs = (totalTime > 0) ? (uint32_t)(totalTime * 1000) : 0;
        currentMs = 0;
        playing = false;
        paused = false;
        eof = false;
        simples = 1152; // Default to reasonable size for spectrum display
        zeroReadCount = 0;

        return true;
    }

    void replayOgg(const char* device) {
        oggDs.pos = 0;
        ov_pcm_seek(&oggVf, 0);

        struct pcm_config config;
        memset(&config, 0, sizeof(config));
        config.bit_depth = 16;
        config.rate = sampleRate;
        config.channels = channels;
        config.period_size = 2048;
        config.period_count = 4;
        config.start_threshold = 2048 * channels;
        config.stop_threshold = 0;

        pcmDev = pcm_open(device, &config);
        if (pcmDev == NULL) {
            return;
        }

        currentMs = 0;
        playing = true;
        paused = false;
        eof = false;
        simples = 1;
        zeroReadCount = 0;
    }

    bool decodeOggFrame() {
        static int callCount = 0;
        callCount++;
        long bytes_read = ov_read(&oggVf, oggDecodeBuffer, 8192, 0, 2, 1, &oggBitstream);

        if (callCount <= 5) {
            // Debug output can be enabled here
        }

        if (bytes_read == 0) {
            eof = true;
            return false;
        }

        if (bytes_read < 0) {
            return true;
        }

        char *output_buf = oggDecodeBuffer;
        int output_bytes = bytes_read;

        if (oggChannels == 1) {
            int16_t *mono_buf = (int16_t *)oggDecodeBuffer;
            int16_t *stereo_buf = (int16_t *)oggStereoBuffer;
            int mono_samples = bytes_read / 2;
            for (int i = mono_samples - 1; i >= 0; i--) {
                stereo_buf[i * 2] = mono_buf[i];
                stereo_buf[i * 2 + 1] = mono_buf[i];
            }
            output_buf = oggStereoBuffer;
            output_bytes = mono_samples * 4;
        }

        if (playing) {
            int ret = pcm_write(pcmDev, output_buf, output_bytes);
            currentMs += (uint32_t)((output_bytes * 1000) / (sampleRate * 4));
        }

        int samples = output_bytes / 4;
        if (samples > MINIMP3_MAX_SAMPLES_PER_FRAME / 2) {
            samples = MINIMP3_MAX_SAMPLES_PER_FRAME / 2;
        }

        memcpy(sampleBuf, output_buf, samples * 4);
        simples = samples;

        return true;
    }

private:
    mp3dec_t mp3dec;
    mp3dec_frame_info_t info;
    struct pcm_t* pcmDev;
    void* fileData;
    uint8_t* streamPos;
    int bytesLeft;
    int totalBytes;
    int simples;
    int16_t sampleBuf[MINIMP3_MAX_SAMPLES_PER_FRAME];
    bool playing;
    bool paused;
    bool eof;
    int channels;
    int sampleRate;
    uint32_t currentMs;
    uint32_t totalMs;
    const char* devicePath;
    AudioFormat format;
    int wavDataOffset;
    int wavDataSize;
    int wavBitDepth;
    OggVorbis_File oggVf;
    int oggBitstream;
    int oggChannels;
    int zeroReadCount;
    OggVorbis_DataSource oggDs;
    char oggDecodeBuffer[32768];
    char oggStereoBuffer[32768];

    uint32_t estimateTotalMs() {
        if (info.frame_bytes <= 0) return 0;
        uint32_t totalFrames = bytesLeft / info.frame_bytes + 1;
        return (totalFrames * simples * 1000) / sampleRate;
    }
};
