#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/Menubar.h>
#include <Widget/Container.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/timer.h>
#include <ewoksys/ipc.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <fcntl.h>

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

using namespace Ewok;

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
            printf("decodeOggFrame #%d: bytes_read=%ld\n", callCount, bytes_read);
        }

        if (bytes_read == 0) {
            printf("decodeOggFrame #%d: bytes_read=%ld\n", callCount, bytes_read);
            eof = true;
            return false;
        }

        if (bytes_read < 0) {
            if (callCount <= 5) {
                printf("decodeOggFrame #%d: decode error=%ld\n", callCount, bytes_read);
            }
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
            if (ret != 0 && callCount <= 10) {
                printf("pcm_write failed: ret=%d, output_bytes=%d\n", ret, output_bytes);
            }
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
    string devicePath;
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

class SpectrumView : public Widget {
public:
    static const int BARS = 32;

    typedef void (*StateChangeCallback)(void* userData);
    typedef void (*TimeUpdateCallback)(void* userData);
    void setPlayer(AudioPlayer* p) { player = p; }
    void setStateChangeCallback(StateChangeCallback cb, void* ud) {
        stateChangeCb = cb;
        stateChangeUserData = ud;
    }
    void setTimeUpdateCallback(TimeUpdateCallback cb, void* ud) {
        timeUpdateCb = cb;
        timeUpdateUserData = ud;
    }

protected:
    float magnitudes[BARS];
    float targetMagnitudes[BARS];
    uint32_t barColors[BARS];
    AudioPlayer* player;
    StateChangeCallback stateChangeCb;
    void* stateChangeUserData;
    TimeUpdateCallback timeUpdateCb;
    void* timeUpdateUserData;

public:
    SpectrumView() {
        player = NULL;
        stateChangeCb = NULL;
        stateChangeUserData = NULL;
        timeUpdateCb = NULL;
        timeUpdateUserData = NULL;
        for (int i = 0; i < BARS; i++) {
            magnitudes[i] = 0;
            targetMagnitudes[i] = 0;
            barColors[i] = 0xFF00FF00;
        }
    }

    void updateSpectrum(const int16_t* samples, int count, int channels) {
        if (count <= 0) return;

        const int N = 128;
        float fftBuf[N];
        float magnitudesTemp[N/2];

        for (int i = 0; i < N; i++) {
            int idx = (i * count) / N;
            if (idx >= count) idx = count - 1;
            float sample = (float)samples[idx * channels] / 32768.0f;
            float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N - 1)));
            fftBuf[i] = sample * window;
        }

        for (int k = 0; k < N/2; k++) {
            float real = 0, imag = 0;
            for (int n = 0; n < N; n++) {
                float angle = -2.0f * M_PI * k * n / N;
                real += fftBuf[n] * cosf(angle);
                imag += fftBuf[n] * sinf(angle);
            }
            magnitudesTemp[k] = sqrtf(real * real + imag * imag) / N;
        }

        int barsPerGroup = (N/2) / BARS;
        if (barsPerGroup < 1) barsPerGroup = 1;

        for (int i = 0; i < BARS; i++) {
            float sum = 0;
            int cnt = 0;
            for (int j = 0; j < barsPerGroup; j++) {
                int idx = i * barsPerGroup + j;
                if (idx < N/2) {
                    sum += magnitudesTemp[idx];
                    cnt++;
                }
            }
            if (cnt > 0) {
                targetMagnitudes[i] = (sum / cnt) * 15.0f;
            }
            if (targetMagnitudes[i] > 1.0f) targetMagnitudes[i] = 1.0f;
            if (targetMagnitudes[i] < 0) targetMagnitudes[i] = 0;
        }
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        if (player != NULL && player->isPlaying()) {
            player->decodeFrame();
            updateSpectrum(player->getSampleBuf(), player->getSimples(), player->getChannels());

            if (player->isEof()) {
                player->pause();
                if (stateChangeCb) stateChangeCb(stateChangeUserData);
            }
        }

        // Update time label during playback
        if (player != NULL && player->isLoaded() && timeUpdateCb) {
            timeUpdateCb(timeUpdateUserData);
        }

        for (int i = 0; i < BARS; i++) {
            float diff = targetMagnitudes[i] - magnitudes[i];
            if (diff > 0.01f)
                magnitudes[i] += diff * 0.3f;
            else if (diff < -0.01f)
                magnitudes[i] += diff * 0.5f;

            if (magnitudes[i] < 0) magnitudes[i] = 0;
            if (magnitudes[i] > 1) magnitudes[i] = 1;

            uint8_t g = (uint8_t)(magnitudes[i] * 200 + 55);
            uint8_t r = (uint8_t)(magnitudes[i] * 255);
            uint8_t b = (uint8_t)(100 + magnitudes[i] * 100);
            barColors[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        update();
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
        (void)theme;
        graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xFF1a1a2e);

        int barW = rect.w / BARS;
        if (barW < 2) barW = 2;
        int gap = 1;

        for (int i = 0; i < BARS; i++) {
            int barHeight = (int)(magnitudes[i] * (rect.h - 4));
            if (barHeight < 2) barHeight = 2;

            int bx = rect.x + i * barW + gap/2;
            int by = rect.y + rect.h - barHeight - 2;

            graph_fill(g, bx, by, barW - gap, barHeight, barColors[i]);

            if (barHeight > 4) {
                uint32_t highlightColor = 0xFFFFFFFF;
                graph_fill(g, bx, by, barW - gap, 2, highlightColor);
            }
        }

        uint32_t borderColor = 0xFF444466;
        graph_line(g, rect.x, rect.y, rect.x + rect.w - 1, rect.y, borderColor);
        graph_line(g, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1, borderColor);
        graph_line(g, rect.x + rect.w - 1, rect.y + rect.h - 1, rect.x, rect.y + rect.h - 1, borderColor);
        graph_line(g, rect.x, rect.y + rect.h - 1, rect.x, rect.y, borderColor);
    }
};

class ProgressBar : public Widget {
public:
    typedef void (*SeekCallback)(void* userData, float progress);

    ProgressBar() {
        progress = 0.0f;
        seekCb = NULL;
        seekUserData = NULL;
        dragging = false;
    }

    void setProgress(float p) {
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        progress = p;
        update();
    }

    float getProgress() { return progress; }

    void setSeekCallback(SeekCallback cb, void* ud) {
        seekCb = cb;
        seekUserData = ud;
    }

protected:
    float progress;
    SeekCallback seekCb;
    void* seekUserData;
    bool dragging;

    void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
        (void)theme;

        // Background
        graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xFF333344);

        // Progress fill
        int fillWidth = (int)(rect.w * progress);
        if (fillWidth > 0) {
            graph_fill(g, rect.x, rect.y, fillWidth, rect.h, 0xFF00AAFF);
        }

        // Border
        graph_line(g, rect.x, rect.y, rect.x + rect.w - 1, rect.y, 0xFF555566);
        graph_line(g, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1, 0xFF555566);
        graph_line(g, rect.x + rect.w - 1, rect.y + rect.h - 1, rect.x, rect.y + rect.h - 1, 0xFF555566);
        graph_line(g, rect.x, rect.y + rect.h - 1, rect.x, rect.y, 0xFF555566);
    }

    bool onMouse(xevent_t* ev) {
        if (ev->type == XEVT_MOUSE) {
            grect_t r = area;
            int mx = ev->value.mouse.x;
            int my = ev->value.mouse.y;

            // Check if mouse is within widget
            if (mx < r.x || mx >= r.x + r.w || my < r.y || my >= r.y + r.h) {
                return false;
            }

            if (ev->state == MOUSE_STATE_DOWN) {
                dragging = true;
                updateProgressFromMouse(mx, r);
                return true;
            } else if (ev->state == MOUSE_STATE_DRAG) {
                if (dragging) {
                    updateProgressFromMouse(mx, r);
                    return true;
                }
            } else if (ev->state == MOUSE_STATE_UP) {
                if (dragging) {
                    dragging = false;
                    updateProgressFromMouse(mx, r);
                    return true;
                }
            }
        }
        return false;
    }

    void updateProgressFromMouse(int mx, const grect_t& r) {
        int relX = mx - r.x;
        float newProgress = (float)relX / r.w;
        if (newProgress < 0.0f) newProgress = 0.0f;
        if (newProgress > 1.0f) newProgress = 1.0f;
        progress = newProgress;
        update();
        if (seekCb) {
            seekCb(seekUserData, progress);
        }
    }
};

class SoundPlayerWin : public WidgetWin {
    FileDialog fdialog;
    AudioPlayer* player;
    SpectrumView* spectrum;
    LabelButton* playBtn;
    Label* timeLabel;
    ProgressBar* progressBar;

protected:
    void onDialoged(XWin* from, int res, void* arg) {
        if (res == Dialog::RES_OK && from == &fdialog) {
            string path = fdialog.getResult();
            if (path.length() > 0) {
                loadAndPlay(path.c_str());
            }
        }
    }

public:
    SoundPlayerWin() {
        player = new AudioPlayer();
        spectrum = NULL;
        playBtn = NULL;
        timeLabel = NULL;
        progressBar = NULL;
    }

    ~SoundPlayerWin() {
        delete player;
    }

    void setSpectrum(SpectrumView* s) { spectrum = s; }
    void setPlayBtn(LabelButton* b) { playBtn = b; }
    void setTimeLabel(Label* l) { timeLabel = l; }
    void setProgressBar(ProgressBar* p) { progressBar = p; }

    void updateProgressBar() {
        if (progressBar && player->isLoaded() && player->getTotalMs() > 0) {
            float progress = (float)player->getCurrentMs() / player->getTotalMs();
            progressBar->setProgress(progress);
        }
    }

    void loadAndPlay(const char* path) {
        if (player->load(path, "/dev/sound0")) {
            player->play();
            updatePlayBtn();
            updateTimeLabel();
        }
    }

    void updatePlayBtn() {
        if (playBtn && player->isLoaded()) {
            if (player->isPlaying()) {
                playBtn->setLabel("||");
            } else {
                playBtn->setLabel(">");
            }
        }
    }

    void updateTimeLabel() {
        if (timeLabel) {
            uint32_t cur = player->getCurrentMs();
            uint32_t tot = player->getTotalMs();
            char buf[64];
            snprintf(buf, sizeof(buf)-1, "%02d:%02d / %02d:%02d",
                cur/60000, (cur/1000)%60,
                tot/60000, (tot/1000)%60);
            timeLabel->setLabel(buf);
        }
    }

    void togglePlay() {
        if (!player->isLoaded()) return;

        if (player->isPlaying()) {
            player->pause();
        } else if (player->isEof()) {
            player->replay("/dev/sound0");
        } else {
            player->play();
        }
        updatePlayBtn();
    }

    AudioPlayer* getPlayer() { return player; }
    FileDialog* getFileDialog() { return &fdialog; }
};

static void onOpenFunc(MenuItem* it, void* p) {
    SoundPlayerWin* win = (SoundPlayerWin*)p;
    win->getFileDialog()->popup(win, 0, 0, "files", XWIN_STYLE_NORMAL);
}

static void onPlayStateChange(void* userData) {
    SoundPlayerWin* win = (SoundPlayerWin*)userData;
    win->updatePlayBtn();
}

static void onTimeUpdate(void* userData) {
    SoundPlayerWin* win = (SoundPlayerWin*)userData;
    win->updateTimeLabel();
    win->updateProgressBar();
}

static void onPlayBtnClick(Widget* wd, xevent_t* evt, void* arg) {
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    SoundPlayerWin* win = (SoundPlayerWin*)arg;
    win->togglePlay();
}

int main(int argc, char** argv) {
    X x;
    SoundPlayerWin win;

    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::VERTICLE);

    Menubar* menubar = new Menubar();
    root->add(menubar);
    menubar->fix(0, 24);
    menubar->setItemSize(50);
    menubar->add(0, "Open", NULL, NULL, onOpenFunc, &win);

    SpectrumView* spectrum = new SpectrumView();
    spectrum->setPlayer(win.getPlayer());
    spectrum->setStateChangeCallback(onPlayStateChange, &win);
    spectrum->setTimeUpdateCallback(onTimeUpdate, &win);
    root->add(spectrum);
    win.setSpectrum(spectrum);

    // Progress bar
    ProgressBar* progressBar = new ProgressBar();
    progressBar->fix(0, 8);
    root->add(progressBar);
    win.setProgressBar(progressBar);

    Container* controls = new Container();
    controls->setType(Container::HORIZONTAL);
    root->add(controls);
    controls->fix(0, 24);

    LabelButton* playBtn = new LabelButton(">");
    playBtn->fix(40, 0);
    playBtn->setEventFunc(onPlayBtnClick, &win);
    controls->add(playBtn);
    win.setPlayBtn(playBtn);

    Label* timeLabel = new Label("00:00 / 00:00");
    timeLabel->fix(140, 0);
    controls->add(timeLabel);
    win.setTimeLabel(timeLabel);

    win.open(&x, -1, -1, -1, 300, 140, "SndPlayer", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT, true);
    win.setTimer(120);

    if (argc >= 2) {
        win.loadAndPlay(argv[1]);
        win.getPlayer()->play();
    }

    widgetXRun(&x, &win);

    win.getPlayer()->stop();
    return 0;
}