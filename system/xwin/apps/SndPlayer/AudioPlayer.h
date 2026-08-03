#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#include <minimp3/minimp3.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// OGG Vorbis fd-backed data source
typedef struct {
    int fd;
} OggVorbis_DataSource;

// Audio format enum
enum AudioFormat {
    FORMAT_UNKNOWN,
    FORMAT_MP3,
    FORMAT_WAV,
    FORMAT_OGG
};

// AudioPlayer class
class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    bool load(const char* path, const char* device);
    void play();
    void pause();
    void stop();
    void replay(const char* device);
    void reopenDevice(const char* device);
    bool seekToProgress(float progress);
    bool decodeFrame();

    bool isPlaying();
    bool isPaused();
    bool isLoaded();
    bool isEof();
    bool isWriteFailed();
    int16_t* getSampleBuf();
    int getSimples();
    int getChannels();
    int getSampleRate();
    uint32_t getCurrentMs();
    uint32_t getTotalMs();
    bool isOgg();

private:
    bool loadMp3(const char* path, const char* device);
    bool loadWav(const char* path, const char* device);
    bool loadOgg(const char* path, const char* device);
    void replayMp3(const char* device);
    void replayWav(const char* device);
    void replayOgg(const char* device);
    bool seekMp3(uint32_t targetMs);
    bool seekWav(uint32_t targetMs);
    bool seekOgg(uint32_t targetMs);
    bool decodeMp3Frame();
    bool decodeWavFrame();
    bool decodeOggFrame();
    uint32_t estimateTotalMs();
    bool refillMp3Stream();
    int decodeNextMp3Frame(mp3dec_t* dec, mp3dec_frame_info_t* frameInfo, int16_t* outSamples);
    bool resetMp3Stream();
    bool flushMp3Chunk(bool force);
    uint32_t estimateMp3StreamTotalMs();

    // MP3
    mp3dec_t* mp3dec;
    mp3dec_frame_info_t* info;
    int16_t* mp3FrameBuf;
    int16_t* mp3ChunkBuf;
    int mp3ChunkFrames;

    // PCM device
    struct pcm_t* pcmDev;

    // File data
    void* fileData;
    uint8_t* streamPos;
    int bytesLeft;
    int totalBytes;
    int sourceFd;
    char* sourcePath;
    uint8_t* mp3StreamBuf;
    int mp3StreamBufSize;
    bool mp3StreamEof;

    // Playback state
    int simples;
    int16_t* sampleBuf;
    bool playing;
    bool paused;
    bool eof;
    bool writeFailed;
    int channels;
    int sampleRate;
    uint32_t currentMs;
    uint32_t totalMs;
    const char* devicePath;
    AudioFormat format;

    // WAV
    int wavDataOffset;
    int wavDataSize;
    int wavBitDepth;
    int wavBytesPerFrame;
    uint8_t* wavStreamBuf;
    int wavStreamBufSize;

    // OGG
    OggVorbis_File* oggVf;
    OggVorbis_DataSource* oggDs;
    int oggBitstream;
    int oggChannels;
    int zeroReadCount;
    char* oggDecodeBuffer;
    char* oggStereoBuffer;
};

#endif // AUDIO_PLAYER_H
