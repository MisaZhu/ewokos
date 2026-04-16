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

#if SDL_VIDEO_RENDER_EWOKOS

/* RaspberryPi Baremetal SDL video driver implementation.
 *
 * Initial work by Marco Maccaferri (macca@maccasoft.com).
 */

#include "SDL_video.h"
#include "../../render/SDL_sysrender.h"

SDL_Renderer *
EWOKOS_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    //TODO
    SDL_Surface *surface;

    surface = SDL_GetWindowSurface(window);
    if (!surface) {
        return NULL;
    }
    return SW_CreateRendererForSurface(surface);
}

SDL_RenderDriver EWOKOS_RenderDriver = {
    EWOKOS_CreateRenderer,
    {
     "software",
     SDL_RENDERER_SOFTWARE | SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED,
     8,
     {
      SDL_PIXELFORMAT_RGB555,
      SDL_PIXELFORMAT_RGB565,
      SDL_PIXELFORMAT_RGB888,
      SDL_PIXELFORMAT_BGR888,
      SDL_PIXELFORMAT_ARGB8888,
      SDL_PIXELFORMAT_RGBA8888,
      SDL_PIXELFORMAT_ABGR8888,
      SDL_PIXELFORMAT_BGRA8888
     },
     0,
     0}
};


#endif /* SDL_VIDEO_DRIVER_EWOKOS */

/* vi: set ts=4 sw=4 expandtab: */
