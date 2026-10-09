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

#include "SDL_rwops.h"
#include "SDL_audio.h"
#include "SDL_mixer.h"

typedef struct {
    SDL_RWops *src;
    int freesrc;
    SDL_AudioCVT cvt;
    int playing;
    int volume;
    
    /* minimp3 data */
    void *mp3dec;
    void *info;
    Uint8 *buffer;
    int buffer_size;
    int buffer_pos;
    int buffer_fill;
    
    /* Stream data */
    Uint8 *stream_buffer;
    int stream_size;
    int stream_pos;
    
    /* Audio format */
    int channels;
    int sample_rate;
    
    /* Output buffer */
    Uint8 *snd_available;
    int len_available;
} MP3_music;

/* Initialize the MP3 player, with the given mixer settings
   This function returns 0, or -1 if there was an error.
 */
extern int MP3_init(SDL_AudioSpec *mixerfmt);

/* Set the volume for an MP3 stream */
extern void MP3_setvolume(MP3_music *music, int volume);

/* Load an MP3 stream from an SDL_RWops object */
extern MP3_music *MP3_new_RW(SDL_RWops *src, int freesrc);

/* Start playback of a given MP3 stream */
extern void MP3_play(MP3_music *music);

/* Return non-zero if a stream is currently playing */
extern int MP3_playing(MP3_music *music);

/* Play some of a stream previously started with MP3_play() */
extern int MP3_playAudio(MP3_music *music, Uint8 *snd, int len);

/* Stop playback of a stream previously started with MP3_play() */
extern void MP3_stop(MP3_music *music);

/* Close the given MP3 stream */
extern void MP3_delete(MP3_music *music);

/* Jump (seek) to a given position (time is in seconds) */
extern void MP3_jump_to_time(MP3_music *music, double time);

#endif /* MP3_MUSIC */
