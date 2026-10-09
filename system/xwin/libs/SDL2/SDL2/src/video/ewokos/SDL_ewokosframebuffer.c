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

/* The SDL window surface used to alias the xwin workspace shm (ws_g) directly.
   That is unsafe: the app (and the software renderer behind SDL_Renderer)
   writes the surface at arbitrary times between two presents, with no lock,
   while libx's event thread (x_run in SDL_ewokosvideo.c) may publish ws_g on
   its own at any moment - xwin_retry_pending_presents() flips ws_g -> ws_g2
   on every event-loop tick for an fps_async present that was skipped, and a
   server-pushed XEVT_WIN_REPAINT calls xwin_repaint() too. Whatever is in
   ws_g at that instant goes on screen: with the renderer's clear-then-copy
   sequence that is a black or half-drawn frame, i.e. visible flicker.

   So the surface is now a private buffer and ws_g is written in exactly one
   place: fb_on_repaint, which runs inside xwin_repaint under painting_lock.
   It only copies while frame_ready is set, which the present path raises
   right before xwin_repaint and clears right after: a present from the event
   thread that lands mid-draw finds frame_ready==0, leaves ws_g alone, and
   republishes the previous complete frame instead of a torn one. If it finds
   frame_ready==1 the app thread is parked on painting_lock inside its own
   xwin_repaint and is not drawing, so the surface is complete. */
typedef struct {
    void*   pixels;
    int     w;
    int     h;
    int     pitch;
    volatile int frame_ready;
} ewokos_fb_t;

static void fb_on_repaint(xwin_t* xwin, graph_t* g) {
    ewokos_fb_t* fb = (ewokos_fb_t*)xwin->data;
    int w, h, y;
    const uint8_t* src;
    uint8_t* dst;

    if (fb == NULL || fb->pixels == NULL || g == NULL || g->buffer == NULL)
        return;
    if (!fb->frame_ready)
        return;

    w = fb->w < g->w ? fb->w : g->w;
    h = fb->h < g->h ? fb->h : g->h;
    src = (const uint8_t*)fb->pixels;
    dst = (uint8_t*)g->buffer;
    for (y = 0; y < h; y++) {
        memcpy(dst, src, (size_t)w * 4);
        src += fb->pitch;
        dst += g->w * 4;
    }
    fb->frame_ready = 0;
}

static void fb_release(xwin_t* xwin) {
    ewokos_fb_t* fb = (ewokos_fb_t*)xwin->data;
    if (fb == NULL)
        return;
    /* the event thread may be inside xwin_repaint -> fb_on_repaint right now */
    pthread_mutex_lock(&xwin->painting_lock);
    xwin->on_repaint = NULL;
    xwin->data = NULL;
    pthread_mutex_unlock(&xwin->painting_lock);
    if (fb->pixels != NULL)
        SDL_free(fb->pixels);
    SDL_free(fb);
}

int EWOKOS_CreateWindowFramebuffer(_THIS, SDL_Window * window, Uint32 * format, void ** pixels, int *pitch) {
    xwin_t* xwin = (xwin_t*)window->driverdata;
    ewokos_fb_t* fb;
    graph_t g;

    if(xwin == NULL)
        return -1;

    if (xwin_fetch_graph(xwin, &g) == NULL || g.buffer == NULL) {
        return SDL_SetError("Couldn't map xwin framebuffer");
    }
    if (g.w <= 0 || g.h <= 0) {
        return SDL_SetError("Invalid xwin framebuffer size");
    }

    /* SDL recreates the framebuffer after a resize without calling
       DestroyWindowFramebuffer first, so drop the previous one here */
    fb_release(xwin);

    fb = (ewokos_fb_t*)SDL_calloc(1, sizeof(ewokos_fb_t));
    if (fb == NULL)
        return SDL_OutOfMemory();
    fb->w = g.w;
    fb->h = g.h;
    fb->pitch = g.w * 4;
    fb->pixels = SDL_calloc(1, (size_t)fb->pitch * fb->h);
    if (fb->pixels == NULL) {
        SDL_free(fb);
        return SDL_OutOfMemory();
    }

    pthread_mutex_lock(&xwin->painting_lock);
    xwin->data = fb;
    xwin->on_repaint = fb_on_repaint;
    pthread_mutex_unlock(&xwin->painting_lock);

    *format = SDL_PIXELFORMAT_ARGB8888;
    *pixels = fb->pixels;
    *pitch = fb->pitch;

    return 0;
}

int EWOKOS_UpdateWindowFramebuffer(_THIS, SDL_Window * window, const SDL_Rect * rects, int numrects) {
    SDL_Surface *surface = window->surface;
    xwin_t* xwin = (xwin_t*)window->driverdata;
    ewokos_fb_t* fb;
    (void)rects;
    (void)numrects;

    if (surface == NULL) {
        surface = (SDL_Surface *) SDL_GetWindowSurface(window);
    }
    if (surface == NULL) {
        return SDL_SetError("Couldn't find surface for window");
    }
    if (surface->pixels == NULL) {
        return SDL_SetError("Surface pixels is NULL");
    }

    if (xwin == NULL || xwin->xinfo == NULL) {
        return -1;
    }

    fb = (ewokos_fb_t*)xwin->data;
    if (fb == NULL || surface->pixels != fb->pixels) {
        return -1;
    }

    /* the frame is complete from here until xwin_repaint returns; the copy
       into ws_g happens in fb_on_repaint under painting_lock */
    fb->frame_ready = 1;
    xwin_repaint(xwin);
    /* normally already cleared by fb_on_repaint; make sure the next frame's
       drawing never overlaps a window where a foreign present may copy */
    fb->frame_ready = 0;
    return 0;
}

void EWOKOS_DestroyWindowFramebuffer(_THIS, SDL_Window * window) {
    xwin_t* xwin = (xwin_t*)window->driverdata;
    if (xwin == NULL)
        return;
    fb_release(xwin);
}

#endif /* SDL_VIDEO_DRIVER_EWOKOS */

/* vi: set ts=4 sw=4 expandtab: */
