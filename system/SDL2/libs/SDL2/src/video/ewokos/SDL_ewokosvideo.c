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

/* RaspberryPi Baremetal SDL video driver implementation.
 *
 * Initial work by Marco Maccaferri (macca@maccasoft.com).
 */

#include "SDL_video.h"
#include "SDL_mouse.h"
#include "../SDL_sysvideo.h"
#include "../SDL_pixels_c.h"
#include "../../events/SDL_events_c.h"
#include "../../events/SDL_mouse_c.h"

#include "SDL_ewokosvideo.h"
#include "SDL_ewokosevents_c.h"
#include "SDL_ewokosframebuffer_c.h"
#include "ewoksys/keydef.h"
#include "x/xwin.h"
#include "x/x.h"
#include "x/xcntl.h"
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <pthread.h>

static x_t _x_;

#ifdef HAVE_USPI
#include "uspi.h"

#define MAX_KEYS        6

static unsigned char keydown[MAX_KEYS] = { 0, 0, 0, 0, 0, 0 };
static unsigned char old_modifiers;
#endif // HAVE_USPI

#define EWOKOSVID_DRIVER_NAME "ewokos"

/* Initialization/Query functions */
static int EWOKOS_VideoInit(_THIS);
static int EWOKOS_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode);
static void EWOKOS_VideoQuit(_THIS);
static int EWOKOS_GetDisplayBounds (_THIS, SDL_VideoDisplay * display, SDL_Rect * rect);

static int phys_width;
static int phys_height;

static bool on_close(xwin_t* xwin) {
    if(xwin == NULL || xwin->xinfo == NULL)
        return false;
    SDL_Window * window = SDL_GetWindowFromID(xwin->xinfo->win);
    if(window == NULL)
        return false;

    SDL_SendWindowEvent(window, SDL_WINDOWEVENT_CLOSE, 0, 0);
    return false;
}

static void on_resize(xwin_t* xwin) {
    if(xwin == NULL || xwin->xinfo == NULL)
        return;
    SDL_Window * window = SDL_GetWindowFromID(xwin->xinfo->win);
    if(window == NULL)
        return;

    if(xwin->xinfo->wsr.w != window->w || xwin->xinfo->wsr.h != window->h) {
        SDL_SendWindowEvent(window, SDL_WINDOWEVENT_RESIZED, xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
        //SDL_SetWindowSize(window, xwin->xinfo->wsr.w, xwin->xinfo->wsr.h);
    }
}

static int sdl_key(int v) {
    switch(v) {
        case KEY_UP:
            return SDLK_UP;
        case KEY_DOWN:
            return SDLK_DOWN;
        case KEY_LEFT:
            return SDLK_LEFT;
        case KEY_RIGHT:
            return SDLK_RIGHT;
    }
    return v;
}

static void on_event(xwin_t* xw, xevent_t* ev) {
	if(xw == NULL)
		return;

    SDL_Event sdlEvent;
    SDL_zero(sdlEvent);

    if(ev->type == XEVT_IM) {
        if(ev->state == XIM_STATE_PRESS)
            sdlEvent.type = SDL_KEYDOWN;
        else if(ev->state == XIM_STATE_RELEASE)
            sdlEvent.type = SDL_KEYUP;
        sdlEvent.key.keysym.sym = sdl_key(ev->value.im.value);
        SDL_PushEvent(&sdlEvent);
    }
    else if(ev->type == XEVT_MOUSE) {
        int mousex =  ev->value.mouse.x - xw->xinfo->wsr.x;
        int mousey =  ev->value.mouse.y - xw->xinfo->wsr.y;
        
        if(ev->state == MOUSE_STATE_MOVE || ev->state == MOUSE_STATE_DRAG) {
            sdlEvent.type = SDL_MOUSEMOTION;
            sdlEvent.motion.windowID = SDL_GetWindowID(SDL_GetWindowFromID(xw->xinfo->win));
            sdlEvent.motion.x = mousex;
            sdlEvent.motion.y = mousey;
            sdlEvent.motion.xrel = ev->value.mouse.rx;
            sdlEvent.motion.yrel = ev->value.mouse.ry;
            sdlEvent.motion.state = (ev->state == MOUSE_STATE_DRAG) ? SDL_PRESSED : SDL_RELEASED;
            SDL_PushEvent(&sdlEvent);
        }
        else if(ev->state == MOUSE_STATE_DOWN) {
            sdlEvent.type = SDL_MOUSEBUTTONDOWN;
            sdlEvent.button.windowID = SDL_GetWindowID(SDL_GetWindowFromID(xw->xinfo->win));
            sdlEvent.button.x = mousex;
            sdlEvent.button.y = mousey;
            sdlEvent.motion.xrel = ev->value.mouse.rx;
            sdlEvent.motion.yrel = ev->value.mouse.ry;
            sdlEvent.button.button = SDL_BUTTON_LEFT;
            sdlEvent.button.state = SDL_PRESSED;
            SDL_PushEvent(&sdlEvent);
        }
        else if(ev->state == MOUSE_STATE_UP) {
            sdlEvent.type = SDL_MOUSEBUTTONUP;
            sdlEvent.button.windowID = SDL_GetWindowID(SDL_GetWindowFromID(xw->xinfo->win));
            sdlEvent.button.x = mousex;
            sdlEvent.button.y = mousey;
            sdlEvent.motion.xrel = ev->value.mouse.rx;
            sdlEvent.motion.yrel = ev->value.mouse.ry;
            sdlEvent.button.button = SDL_BUTTON_LEFT;
            sdlEvent.button.state = SDL_RELEASED;
            SDL_PushEvent(&sdlEvent);
        }
    }
}

static int
EWOKOS_CreateWindow(_THIS, SDL_Window * window)
{
    if(window->w <= 0 || window->h <= 0)
        window->flags |= SDL_WINDOW_FULLSCREEN;

    window->flags &= (~SDL_WINDOW_HIDDEN);
    window->flags |= SDL_WINDOW_SHOWN;          /* only one window */
    window->flags |= SDL_WINDOW_INPUT_FOCUS;    /* always has input focus */

    if((window->x & SDL_WINDOWPOS_CENTERED) != 0 || (window->x & SDL_WINDOWPOS_UNDEFINED) != 0)
        window->x = (phys_width - window->w)/2;
    if((window->y & SDL_WINDOWPOS_CENTERED) != 0 || (window->y & SDL_WINDOWPOS_UNDEFINED) != 0)
        window->y = (phys_height - window->h)/2;

    int flags = (window->flags & SDL_WINDOW_RESIZABLE) ? XWIN_STYLE_NORMAL : XWIN_STYLE_NO_RESIZE;     /* window is NEVER resizeable */
    x_t* x = (x_t*)_this->driverdata;
	xwin_t* xwin = xwin_open(x, 0, 
            window->x, window->y,
            window->w, window->h,
            window->title,
            flags);

    xwin->on_event = on_event;
    xwin->on_close = on_close;
    xwin->on_resize = on_resize;
    window->driverdata = xwin;
    if(xwin->xinfo != NULL)
        window->id = xwin->xinfo->win;
    if((window->flags & SDL_WINDOW_HIDDEN) == 0) {
        if((window->flags & SDL_WINDOW_FULLSCREEN) != 0) {
            xwin_fullscreen(xwin);
            window->w = xwin->xinfo->wsr.w;
            window->h = xwin->xinfo->wsr.h;
        }
        else if((window->flags & SDL_WINDOW_MAXIMIZED) != 0) {
            xwin_max(xwin);
            window->w = xwin->xinfo->wsr.w;
            window->h = xwin->xinfo->wsr.h;
        }
        xwin_set_visible(xwin, true);
    }

    /* Adjust the window data to match the screen */
    if (window->w > 1 && window->h > 1)
    {
        SDL_DisplayMode mode;
        SDL_zero(mode);
        mode.format = SDL_PIXELFORMAT_ABGR8888;
        mode.w = window->w;
        mode.h = window->h;
        mode.refresh_rate = 60;
        mode.driverdata = NULL;
        SDL_AddDisplayMode(&_this->displays[0], &mode);
    }

    /* One window, it always has focus */
    SDL_SetMouseFocus(window);
    SDL_SetKeyboardFocus(window);
}

/* driver bootstrap functions */

static int
EWOKOS_Available(void)
{
    return (1);
}

static void
EWOKOS_DeleteDevice(SDL_VideoDevice * device)
{
    SDL_free(device);
}

#ifdef HAVE_USPI
static void
EWOKOS_USPiKeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6]) {
    int i, index;
    unsigned char key;

    if ((old_modifiers & LSHIFT) != (ucModifiers & LSHIFT)) {
        SDL_SendKeyboardKey((ucModifiers & LSHIFT) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_LSHIFT);
    }
    if ((old_modifiers & RSHIFT) != (ucModifiers & RSHIFT)) {
        SDL_SendKeyboardKey((ucModifiers & RSHIFT) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_RSHIFT);
    }
    if ((old_modifiers & ALT) != (ucModifiers & ALT)) {
        SDL_SendKeyboardKey((ucModifiers & ALT) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_LALT);
    }
    if ((old_modifiers & ALTGR) != (ucModifiers & ALTGR)) {
        SDL_SendKeyboardKey((ucModifiers & ALTGR) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_RALT);
    }
    if ((old_modifiers & LCTRL) != (ucModifiers & LCTRL)) {
        SDL_SendKeyboardKey((ucModifiers & LCTRL) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_LCTRL);
    }
    if ((old_modifiers & RCTRL) != (ucModifiers & RCTRL)) {
        SDL_SendKeyboardKey((ucModifiers & RCTRL) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_RCTRL);
    }
    if ((old_modifiers & LWIN) != (ucModifiers & LWIN)) {
        SDL_SendKeyboardKey((ucModifiers & LWIN) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_LGUI);
    }
    if ((old_modifiers & RWIN) != (ucModifiers & RWIN)) {
        SDL_SendKeyboardKey((ucModifiers & RWIN) != 0 ? SDL_PRESSED : SDL_RELEASED, SDL_SCANCODE_RGUI);
    }

    old_modifiers = ucModifiers;

    for (index = 0; index < MAX_KEYS; index++) {
        key = RawKeys[index];
        if (key < 4) { // Invalid key code
            continue;
        }
        for (i = 0; i < MAX_KEYS; i++) {
            if (key == keydown[i]) {
                break;
            }
        }
        if (i >= MAX_KEYS) {
            for (i = 0; i < MAX_KEYS; i++) {
                if (keydown[i] == 0) {
                    SDL_SendKeyboardKey(SDL_PRESSED, key);
                    keydown[i] = key;
                    break;
                }
            }
        }
    }

    for (i = 0; i < MAX_KEYS; i++) {
        key = keydown[i];
        if (key != 0) {
            for (index = 0; index < MAX_KEYS; index++) {
                if (RawKeys[index] == key)
                    break;
            }
            if (index >= MAX_KEYS) {
                SDL_SendKeyboardKey(SDL_RELEASED, key);
                keydown[i] = 0;
            }
        }
    }
}
#endif // HAVE_USPI

static SDL_VideoDevice *
EWOKOS_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *) SDL_calloc(1, sizeof(SDL_VideoDevice));
    if (!device) {
        SDL_OutOfMemory();
        SDL_free(device);
        return (0);
    }

    /* Set the function pointers */
    device->VideoInit = EWOKOS_VideoInit;
    device->VideoQuit = EWOKOS_VideoQuit;
    device->SetDisplayMode = EWOKOS_SetDisplayMode;
    device->PumpEvents = EWOKOS_PumpEvents;
    device->CreateWindow = EWOKOS_CreateWindow;
    device->CreateWindowFramebuffer = EWOKOS_CreateWindowFramebuffer;
    device->UpdateWindowFramebuffer = EWOKOS_UpdateWindowFramebuffer;
    device->DestroyWindowFramebuffer = EWOKOS_DestroyWindowFramebuffer;
    device->GetDisplayBounds = EWOKOS_GetDisplayBounds;

    device->free = EWOKOS_DeleteDevice;

#ifdef HAVE_USPI
    USPiInitialize ();
    if (USPiKeyboardAvailable()) {
        USPiKeyboardRegisterKeyStatusHandlerRaw (EWOKOS_USPiKeyStatusHandlerRaw);
    }
#endif

    return device;
}

VideoBootStrap EWOKOS_bootstrap = {
    EWOKOSVID_DRIVER_NAME, "SDL EwokOS video driver",
    EWOKOS_Available, EWOKOS_CreateDevice
};

static void* x_thread(void* p) {
    SDL_VideoDevice* dev = (SDL_VideoDevice*)p;
    x_t* x = (x_t*)dev->driverdata;
    x_run(x, NULL);
    return NULL;
}

int
EWOKOS_VideoInit(_THIS)
{
    SDL_DisplayMode mode;
    /*unsigned int mb_addr = 0x40007000;      // 0x7000 in L2 cache coherent mode
    volatile unsigned int *mailbuffer = (unsigned int *) mb_addr;

    mailbuffer[0] = 8 * 4;             // size of this message
    mailbuffer[1] = 0;                  // this is a request
    mailbuffer[2] = TAG_GET_PHYS_WH;    // get physical width/height tag
    mailbuffer[3] = 8;                  // value buffer size
    mailbuffer[4] = 0;                  // request/response
    mailbuffer[5] = 0;                  // space to return width
    mailbuffer[6] = 0;                  // space to return height
    mailbuffer[7] = 0;
    Raspberry_MailboxWrite(MAIL_TAGS, mb_addr);

    Raspberry_MailboxRead(MAIL_TAGS);
    if (mailbuffer[1] != MAIL_FULL) {
        return SDL_SetError("Can't get physiscal video size");
    }

    phys_width = mailbuffer[5];
    phys_height = mailbuffer[6];
    */
    xscreen_info_t scr;
    x_screen_info(&scr, 0);
    phys_width = scr.size.w;
    phys_height = scr.size.h;

    x_init(&_x_, NULL);    
    _this->driverdata = &_x_;

    SDL_zero(mode);
    mode.format = SDL_PIXELFORMAT_ABGR8888;
    mode.w = phys_width;
    mode.h = phys_height;
    mode.refresh_rate = 60;

    if (SDL_AddBasicVideoDisplay(&mode) < 0) {
        return -1;
    }

    SDL_AddDisplayMode(&_this->displays[0], &mode);

    pthread_t tid;
    pthread_create(&tid, NULL, x_thread, _this);

    return 0;
}

static int
EWOKOS_SetDisplayMode(_THIS, SDL_VideoDisplay * display, SDL_DisplayMode * mode)
{
    //phys_width = mode->w;
    //phys_height = mode->h;
    return 0;
}

void
EWOKOS_VideoQuit(_THIS)
{
}

static int
EWOKOS_GetDisplayBounds (_THIS, SDL_VideoDisplay * display, SDL_Rect * rect)
{
    rect->x = 0;
    rect->y = 0;
    rect->w = phys_width;
    rect->h = phys_height;
    return 0;
}

#endif /* SDL_VIDEO_DRIVER_EWOKOS */

/* vi: set ts=4 sw=4 expandtab: */
