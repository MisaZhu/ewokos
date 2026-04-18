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

#if SDL_VIDEO_DRIVER_EWOKOS

#include <string.h>

#include "../SDL_sysvideo.h"
#include "SDL_ewokosvideo.h"
#include "SDL_ewokosframebuffer_c.h"
#include "x/xwin.h"

static unsigned int fb_width;
static unsigned int fb_height;
static unsigned int fb_addr;
static unsigned int fb_size;
static unsigned int fb_pitch;

static unsigned int fb_buffer_addr[2];
static unsigned int fb_buffer;

int EWOKOS_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch) {
    SDL_VideoDisplay *display = SDL_GetDisplayForWindow(window);
    xwin_t* xwin = (xwin_t*)window->driverdata;
    if(xwin == NULL)
        return -1;

    graph_t g;
    if(xwin_fetch_graph(xwin, &g) == NULL)
        return -1;

    // Allocate a new buffer for SDL to write to
    // The buffer size is width * height * 4 bytes (ARGB8888)
    size_t buffer_size = g.w * g.h * 4;
    void *buffer = SDL_malloc(buffer_size);
    if (buffer == NULL) {
        return SDL_OutOfMemory();
    }

    *format = SDL_PIXELFORMAT_ARGB8888;
    *pixels = buffer;
    *pitch = g.w * 4;

    return 0;
}

int EWOKOS_UpdateWindowFramebuffer(_THIS, SDL_Window * window, const SDL_Rect * rects, int numrects) {
    SDL_Surface *surface = (SDL_Surface *) SDL_GetWindowSurface(window);
    if (surface == NULL) {
        return SDL_SetError("Couldn't find surface for window");
    }
    if (surface->pixels == NULL) {
        return SDL_SetError("Surface pixels is NULL");
    }
    xwin_t* xwin = (xwin_t*)window->driverdata;
    if (xwin == NULL || xwin->xinfo == NULL) {
        return -1;
    }

    if (surface->w != xwin->xinfo->wsr.w || surface->h != xwin->xinfo->wsr.h) {
        return -1;
    }

    graph_t g;
    if (xwin_fetch_graph(xwin, &g) == NULL) {
        return -1;
    }

    if (g.buffer == NULL) {
        return -1;
    }

    size_t copy_size = surface->h * surface->pitch;

    memcpy(g.buffer, surface->pixels, copy_size);

    xwin_repaint(xwin);
    return 0;
}

void EWOKOS_DestroyWindowFramebuffer(_THIS, SDL_Window * window) {
    SDL_Surface *surface = (SDL_Surface *) SDL_GetWindowSurface(window);
    if (surface != NULL && surface->pixels != NULL) {
        void *pixels = surface->pixels;
        surface->pixels = NULL;
        SDL_free(pixels);
    }
}

#endif /* SDL_VIDEO_DRIVER_EWOKOS */

/* vi: set ts=4 sw=4 expandtab: */
