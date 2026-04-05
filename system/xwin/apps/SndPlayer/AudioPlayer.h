#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include <stdbool.h>

#include <minimp3/minimp3.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// OGG Vorbis data source
typedef struct {
    uint8_t *data;
    int size;
    int pos;
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

private:
    bool loadMp3(const char* device);
    bool loadWav(const char* device);
    bool loadOgg(const char* device);
    void replayMp3(const char* device);
    void replayWav(const char* device);
    void replayOgg(const char* device);
    bool decodeMp3Frame();
    bool decodeWavFrame();
    bool decodeOggFrame();
    uint32_t estimateTotalMs();

    // MP3
    mp3dec_t* mp3dec;
    mp3dec_frame_info_t* info;

    // PCM device
    struct pcm_t* pcmDev;

    // File data
    void* fileData;
    uint8_t* streamPos;
    int bytesLeft;
    int totalBytes;

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
