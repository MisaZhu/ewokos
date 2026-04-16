/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

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

/* $Id$ */

#ifdef MP3_MUSIC

/* This file supports MP3 music streams using minimp3 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_mixer.h"
#include "music_mp3.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3/minimp3.h"

/* This is the format of the audio mixer data */
static SDL_AudioSpec mixer;

/* Initialize the MP3 player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
int MP3_init(SDL_AudioSpec *mixerfmt)
{
    mixer = *mixerfmt;
    return(0);
}

/* Set the volume for an MP3 stream */
void MP3_setvolume(MP3_music *music, int volume)
{
    music->volume = volume;
}

/* Load an MP3 stream from an SDL_RWops object */
MP3_music *MP3_new_RW(SDL_RWops *src, int freesrc)
{
    MP3_music *music;
    Sint64 src_size;
    mp3dec_t *mp3dec;
    mp3dec_frame_info_t *info;

    music = (MP3_music *)SDL_malloc(sizeof(MP3_music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }

    /* Initialize the music structure */
    SDL_memset(music, 0, sizeof(MP3_music));
    music->src = src;
    music->freesrc = freesrc;
    music->playing = 0;
    music->volume = MIX_MAX_VOLUME;
    music->len_available = 0;
    music->snd_available = NULL;

    /* Allocate minimp3 structures */
    mp3dec = (mp3dec_t *)SDL_malloc(sizeof(mp3dec_t));
    info = (mp3dec_frame_info_t *)SDL_malloc(sizeof(mp3dec_frame_info_t));
    if (!mp3dec || !info) {
        SDL_OutOfMemory();
        SDL_free(mp3dec);
        SDL_free(info);
        SDL_free(music);
        return NULL;
    }
    music->mp3dec = mp3dec;
    music->info = info;

    /* Initialize decoder */
    mp3dec_init(mp3dec);

    /* Read entire file into memory */
    src_size = SDL_RWseek(src, 0, RW_SEEK_END);
    if (src_size < 0) {
        SDL_SetError("Could not determine MP3 file size");
        MP3_delete(music);
        return NULL;
    }
    SDL_RWseek(src, 0, RW_SEEK_SET);

    music->stream_size = (int)src_size;
    music->stream_buffer = (Uint8 *)SDL_malloc(music->stream_size);
    if (!music->stream_buffer) {
        SDL_OutOfMemory();
        MP3_delete(music);
        return NULL;
    }

    if (SDL_RWread(src, music->stream_buffer, 1, music->stream_size) != (size_t)music->stream_size) {
        SDL_SetError("Could not read MP3 file");
        MP3_delete(music);
        return NULL;
    }

    music->stream_pos = 0;

    /* Allocate decode buffer */
    music->buffer_size = MINIMP3_MAX_SAMPLES_PER_FRAME * 2; /* 16-bit samples */
    music->buffer = (Uint8 *)SDL_malloc(music->buffer_size);
    if (!music->buffer) {
        SDL_OutOfMemory();
        MP3_delete(music);
        return NULL;
    }

    /* Try to decode first frame to get format info */
    short *pcm = (short *)music->buffer;
    int samples = mp3dec_decode_frame(mp3dec, 
                                       music->stream_buffer, 
                                       music->stream_size, 
                                       pcm, 
                                       info);
    
    if (samples == 0 || info->frame_bytes == 0) {
        SDL_SetError("Not a valid MP3 audio stream");
        MP3_delete(music);
        return NULL;
    }

    music->channels = info->channels;
    music->sample_rate = info->hz;
    music->buffer_fill = samples * info->channels * 2; /* 16-bit */
    music->buffer_pos = 0;

    /* Reset decoder and position for actual playback */
    mp3dec_init(mp3dec);
    music->stream_pos = 0;
    music->buffer_fill = 0;
    music->buffer_pos = 0;

    return music;
}

/* Start playback of a given MP3 stream */
void MP3_play(MP3_music *music)
{
    music->playing = 1;
}

/* Return non-zero if a stream is currently playing */
int MP3_playing(MP3_music *music)
{
    return music->playing;
}

/* Read and decode more MP3 data */
static void MP3_getsome(MP3_music *music)
{
    short *pcm;
    int samples;
    int remaining;
    int to_decode;
    mp3dec_t *mp3dec = (mp3dec_t *)music->mp3dec;
    mp3dec_frame_info_t *info = (mp3dec_frame_info_t *)music->info;

    if (music->stream_pos >= music->stream_size) {
        music->playing = 0;
        return;
    }

    remaining = music->stream_size - music->stream_pos;
    to_decode = remaining;

    pcm = (short *)music->buffer;
    samples = mp3dec_decode_frame(mp3dec,
                                   music->stream_buffer + music->stream_pos,
                                   to_decode,
                                   pcm,
                                   info);

    if (samples == 0) {
        /* No more frames */
        if (info->frame_bytes == 0) {
            music->playing = 0;
        } else {
            /* Skip this frame */
            music->stream_pos += info->frame_bytes;
        }
        return;
    }

    /* Update position */
    music->stream_pos += info->frame_bytes;

    /* Set up audio conversion if needed */
    if (info->channels != mixer.channels || 
        info->hz != mixer.freq) {
        SDL_BuildAudioCVT(&music->cvt,
                          AUDIO_S16SYS, info->channels, info->hz,
                          mixer.format, mixer.channels, mixer.freq);
        
        if (music->cvt.needed) {
            music->cvt.len = samples * info->channels * 2; /* 16-bit */
            if (music->cvt.buf) {
                SDL_free(music->cvt.buf);
            }
            music->cvt.buf = (Uint8 *)SDL_malloc(music->cvt.len * music->cvt.len_mult);
            if (music->cvt.buf) {
                SDL_memcpy(music->cvt.buf, music->buffer, music->cvt.len);
                SDL_ConvertAudio(&music->cvt);
                music->len_available = music->cvt.len_cvt;
                music->snd_available = music->cvt.buf;
            } else {
                SDL_SetError("Out of memory");
                music->playing = 0;
            }
        } else {
            music->len_available = samples * info->channels * 2;
            music->snd_available = music->buffer;
        }
    } else {
        music->len_available = samples * info->channels * 2;
        music->snd_available = music->buffer;
    }
}

/* Play some of a stream previously started with MP3_play() */
int MP3_playAudio(MP3_music *music, Uint8 *snd, int len)
{
    int mixable;

    while ((len > 0) && music->playing) {
        if (!music->len_available) {
            MP3_getsome(music);
        }
        
        if (!music->len_available) {
            break;
        }

        mixable = len;
        if (mixable > music->len_available) {
            mixable = music->len_available;
        }

        if (music->volume == MIX_MAX_VOLUME) {
            SDL_memcpy(snd, music->snd_available, mixable);
        } else {
            SDL_MixAudio(snd, music->snd_available, mixable, music->volume);
        }

        music->len_available -= mixable;
        music->snd_available += mixable;
        len -= mixable;
        snd += mixable;
    }

    return len;
}

/* Stop playback of a stream previously started with MP3_play() */
void MP3_stop(MP3_music *music)
{
    music->playing = 0;
}

/* Close the given MP3 stream */
void MP3_delete(MP3_music *music)
{
    if (music) {
        if (music->cvt.buf) {
            SDL_free(music->cvt.buf);
        }
        if (music->buffer) {
            SDL_free(music->buffer);
        }
        if (music->stream_buffer) {
            SDL_free(music->stream_buffer);
        }
        if (music->mp3dec) {
            SDL_free(music->mp3dec);
        }
        if (music->info) {
            SDL_free(music->info);
        }
        if (music->freesrc) {
            SDL_RWclose(music->src);
        }
        SDL_free(music);
    }
}

/* Jump (seek) to a given position (time is in seconds) */
void MP3_jump_to_time(MP3_music *music, double time)
{
    /* Simple implementation: reset decoder and seek to approximate position */
    /* For better accuracy, would need to parse frame headers */
    int byte_pos;
    mp3dec_t *mp3dec = (mp3dec_t *)music->mp3dec;
    
    mp3dec_init(mp3dec);
    
    /* Rough estimation: assume constant bitrate */
    byte_pos = (int)(time * music->sample_rate * music->channels * 2 / 1000);
    if (byte_pos > music->stream_size) {
        byte_pos = music->stream_size;
    }
    if (byte_pos < 0) {
        byte_pos = 0;
    }
    
    music->stream_pos = byte_pos;
    music->buffer_fill = 0;
    music->buffer_pos = 0;
    music->len_available = 0;
}

#endif /* MP3_MUSIC */
