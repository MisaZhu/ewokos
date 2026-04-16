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

#ifdef MP3_MUSIC

#include "SDL_mixer.h"
#include "dynamic_mp3.h"

smpeg_loader smpeg = {
    0, NULL
};

/* For EwokOS, we use minimp3 instead of SMPEG */
/* These are stub functions for compatibility */

int Mix_InitMP3()
{
    if ( smpeg.loaded == 0 ) {
        /* minimp3 is statically linked, no initialization needed */
    }
    ++smpeg.loaded;
    return 0;
}

void Mix_QuitMP3()
{
    if ( smpeg.loaded == 0 ) {
        return;
    }
    --smpeg.loaded;
}

#endif /* MP3_MUSIC */
