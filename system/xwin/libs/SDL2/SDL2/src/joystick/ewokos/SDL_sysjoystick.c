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

#if defined(SDL_JOYSTICK_EWOKOS)

#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#include "SDL_events.h"
#include "SDL_endian.h"
#include "SDL_assert.h"

#define MAX_AXES    6
#define MAX_HATS    6

/* The private structure used to keep track of a joystick */
struct joystick_hwdata
{
    struct SDL_joylist_item *item;
    SDL_JoystickGUID guid;
    int axes[MAX_AXES];
    int hats[MAX_HATS];
    unsigned int buttons;
};

/* A linked list of available joysticks */
typedef struct SDL_joylist_item
{
    int device_instance;
    char *name;
    SDL_JoystickGUID guid;
    unsigned int devnum;
    struct joystick_hwdata *hwdata;
    struct SDL_joylist_item *next;

    void (*open) (SDL_Joystick * joystick, int device_index);
    void (*update) (SDL_Joystick * joystick);
    void (*close) (SDL_Joystick * joystick);
} SDL_joylist_item;

static SDL_joylist_item *SDL_joylist = NULL;
static SDL_joylist_item *SDL_joylist_tail = NULL;
static int numjoysticks = 0;
static int instance_counter = 0;

#ifdef HAVE_NES

#define JOY_CLK         24
#define JOY_LCH         23
#define JOY_DATAOUT0    25
#define JOY_DATAOUT1    22

#define NES0_RIGHT      0x0001
#define NES0_LEFT       0x0002
#define NES0_DOWN       0x0004
#define NES0_UP         0x0008
#define NES0_START      0x0010
#define NES0_SELECT     0x0020
#define NES0_B          0x0040
#define NES0_A          0x0080

#define NES1_RIGHT      0x0100
#define NES1_LEFT       0x0200
#define NES1_DOWN       0x0400
#define NES1_UP         0x0800
#define NES1_START      0x1000
#define NES1_SELECT     0x2000
#define NES1_B          0x4000
#define NES1_A          0x8000

static void
NES_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
    Raspberry_pinMode(JOY_CLK, 0);
    Raspberry_pinMode(JOY_LCH, 0);
    Raspberry_pinMode(JOY_DATAOUT0, 1);
    Raspberry_pinMode(JOY_DATAOUT1, 1);

    joystick->hwdata->buttons = 0xFF;

    joystick->nbuttons = 8;
    joystick->naxes = 0;
    joystick->nhats = 0;
}

static void
NES_JoystickUpdate(SDL_Joystick * joystick)
{
    uint32_t nes_bits;

#ifdef HAVE_NES_SN74165 /* Experimental controller with SN74165 shift register */

    /* set clock and latch to 1 */
    Raspberry_digitalWrite(JOY_LCH, 1);
    Raspberry_digitalWrite(JOY_CLK, 1);
    usleep(50);

    /* set latch to 0 */
    Raspberry_digitalWrite(JOY_LCH, 0);
    usleep(50);

    /* set latch to 1 and clock to 0 */
    Raspberry_digitalWrite(JOY_LCH, 1);
    Raspberry_digitalWrite(JOY_CLK, 0);
    usleep(50);

#else

    /* set clock and latch to 0 */
    Raspberry_digitalWrite(JOY_LCH, 0);
    Raspberry_digitalWrite(JOY_CLK, 0);
    usleep(50);

    /* set latch to 1 */
    Raspberry_digitalWrite(JOY_LCH, 1);
    usleep(50);

    /* set latch to 0 */
    Raspberry_digitalWrite(JOY_LCH, 0);
    usleep(50);

#endif // HAVE_NES_SN74165

    /* data is now ready to shift out, clear storage */
    nes_bits = 0;

    /* read 8 bits, 1st bits are already latched and ready, simply save and clock remaining bits */
    for (int i = 0; i < 8; i++) {
        nes_bits <<= 1;
        if (Raspberry_digitalRead(JOY_DATAOUT0))
            nes_bits |= 0x00000001;
        if (Raspberry_digitalRead(JOY_DATAOUT1))
            nes_bits |= 0x00000100;

        Raspberry_digitalWrite(JOY_CLK, 1);
        usleep(50);
        Raspberry_digitalWrite(JOY_CLK, 0);
        usleep(50);
    }

    uint32_t mask = 1;
    for (int i = 0; i < 8; i++, mask <<= 1) {
        if ((nes_bits & mask) != (joystick->hwdata->buttons & mask)) {
            SDL_PrivateJoystickButton(joystick, i, !(nes_bits & mask) ? SDL_PRESSED : SDL_RELEASED);
        }
    }

    joystick->hwdata->buttons = nes_bits;
}

#endif // HAVE_NES

#ifdef HAVE_USPI
#include <uspi.h>

SDL_Joystick * uspi_joystick[4];

static void
USPiGamePadStatusHandler (unsigned nDeviceIndex, const USPiGamePadState *pGamePadState)
{
    int i, value, bmMask, center, steps1, steps2;
    SDL_Joystick *joystick = uspi_joystick[nDeviceIndex];

    if (joystick == NULL)
        return;

    struct joystick_hwdata *hwdata = joystick->hwdata;

    for (i = 0; i < pGamePadState->naxes; i++) {
        value = pGamePadState->axes[i].value;
        if (hwdata->axes[i] != value) {
            center = (pGamePadState->axes[i].maximum - pGamePadState->axes[i].minimum + 1) / 2 + pGamePadState->axes[i].minimum;
            steps1 = 32767000 / (center - pGamePadState->axes[i].minimum);
            steps2 = 32767000 / (pGamePadState->axes[i].maximum - center);
            if (value < center)
                SDL_PrivateJoystickAxis(joystick, i, ((value - center) * steps1 - 500) / 1000);
            else
                SDL_PrivateJoystickAxis(joystick, i, ((value - center) * steps2 + 500) / 1000);
            hwdata->axes[i] = value;
        }
    }

    for (i = 0; i < pGamePadState->nhats; i++) {
        value = pGamePadState->hats[i];
        if (hwdata->hats[i] != value) {
            SDL_PrivateJoystickHat(joystick, i, value);
            hwdata->hats[i] = value;
        }
    }

    if (pGamePadState->nbuttons > 0) {
        for (i = 0, bmMask = 1 << (pGamePadState->nbuttons - 1); i < pGamePadState->nbuttons; i++, bmMask >>= 1) {
            if ((pGamePadState->buttons & bmMask) != (hwdata->buttons & bmMask)) {
                SDL_PrivateJoystickButton(joystick, i, (pGamePadState->buttons & bmMask) ? SDL_PRESSED : SDL_RELEASED);
            }
        }
    }
    hwdata->buttons = pGamePadState->buttons;
}

static void
USPi_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
    int i;
    struct joystick_hwdata *hwdata = joystick->hwdata;
    const USPiGamePadState *pGamePadState = USPiGamePadGetStatus(hwdata->item->devnum);

    joystick->naxes = pGamePadState->naxes;
    for (i = 0; i < pGamePadState->naxes; i++) {
        hwdata->axes[i] = pGamePadState->axes[i].value;
    }

    joystick->nhats = pGamePadState->nhats;
    for (i = 0; i < pGamePadState->nhats; i++) {
        hwdata->hats[i] = pGamePadState->hats[i];
    }

    joystick->nbuttons = pGamePadState->nbuttons;
    hwdata->buttons = pGamePadState->buttons;

    joystick->nhats = 0;

    uspi_joystick[hwdata->item->devnum] = joystick;
    USPiGamePadRegisterStatusHandler(USPiGamePadStatusHandler);
}

static void
USPi_JoystickClose(SDL_Joystick * joystick)
{
    USPiGamePadRegisterStatusHandler(NULL);
    uspi_joystick[joystick->hwdata->item->devnum] = NULL;
}

#endif // HAVE_USPI

/* Function to scan the system for joysticks.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int
SDL_SYS_JoystickInit(void)
{
    SDL_JoystickGUID guid;
    SDL_joylist_item *item;

#ifdef HAVE_NES

    item = (SDL_joylist_item *) SDL_malloc(sizeof (SDL_joylist_item));
    if (item == NULL) {
        return -1;
    }

    SDL_zerop(item);

    item->name = SDL_strdup("NES Gamepad");
    SDL_memcpy( &item->guid.data, item->name, SDL_min( sizeof(guid), SDL_strlen( item->name ) ) );

    item->open = NES_JoystickOpen;
    item->update = NES_JoystickUpdate;

    item->device_instance = instance_counter++;
    if (SDL_joylist_tail == NULL) {
        SDL_joylist = SDL_joylist_tail = item;
    } else {
        SDL_joylist_tail->next = item;
        SDL_joylist_tail = item;
    }

    ++numjoysticks;

#endif // HAVE_NES

#ifdef HAVE_USPI

    for (int deviceIndex = 0; deviceIndex < USPiGamePadAvailable(); deviceIndex++) {
        item = (SDL_joylist_item *) SDL_malloc(sizeof (SDL_joylist_item));
        if (item == NULL) {
            return -1;
        }

        TUSPiDeviceInformation info;
        USPiDeviceGetInformation (GAMEPAD_CLASS, deviceIndex, &info);

        SDL_zerop(item);
        item->devnum = deviceIndex;
        item->name = SDL_strdup(info.pProduct);

        Uint16 *guid16 = (Uint16 *) ((char *) &item->guid.data);
        *(guid16++) = SDL_SwapLE16(3);
        *(guid16++) = 0;
        *(guid16++) = SDL_SwapLE16(info.idVendor);
        *(guid16++) = 0;
        *(guid16++) = SDL_SwapLE16(info.idProduct);
        *(guid16++) = 0;
        *(guid16++) = SDL_SwapLE16(info.bcdDevice);
        *(guid16++) = 0;

        item->open = USPi_JoystickOpen;
        item->close = USPi_JoystickClose;

        item->device_instance = instance_counter++;
        if (SDL_joylist_tail == NULL) {
            SDL_joylist = SDL_joylist_tail = item;
        } else {
            SDL_joylist_tail->next = item;
            SDL_joylist_tail = item;
        }

        ++numjoysticks;
    }

#endif // HAVE_USPI

    return 0;
}

int SDL_SYS_NumJoysticks()
{
    return numjoysticks;
}

void SDL_SYS_JoystickDetect()
{

}

SDL_bool SDL_SYS_JoystickNeedsPolling()
{
#ifdef HAVE_NES
    return SDL_TRUE;
#else
    return SDL_FALSE;
#endif
}

static SDL_joylist_item *
JoystickByDevIndex(int device_index)
{
    SDL_joylist_item *item = SDL_joylist;

    if ((device_index < 0) || (device_index >= numjoysticks)) {
        return NULL;
    }

    while (device_index > 0) {
        SDL_assert(item != NULL);
        device_index--;
        item = item->next;
    }

    return item;
}

/* Function to get the device-dependent name of a joystick */
const char *
SDL_SYS_JoystickNameForDeviceIndex(int device_index)
{
    return JoystickByDevIndex(device_index)->name;
}

/* Function to perform the mapping from device index to the instance id for this index */
SDL_JoystickID SDL_SYS_GetInstanceIdOfDeviceIndex(int device_index)
{
    return JoystickByDevIndex(device_index)->device_instance;
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int
SDL_SYS_JoystickOpen(SDL_Joystick * joystick, int device_index)
{
    SDL_joylist_item *item = JoystickByDevIndex(device_index);

    joystick->instance_id = item->device_instance;
    joystick->hwdata = (struct joystick_hwdata *)SDL_malloc(sizeof(*joystick->hwdata));
    if (joystick->hwdata == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_memset(joystick->hwdata, 0, sizeof(*joystick->hwdata));
    joystick->hwdata->item = item;
    joystick->hwdata->guid = item->guid;

    SDL_assert(item->hwdata == NULL);
    item->hwdata = joystick->hwdata;

    if (item->open != NULL) {
        (*item->open) (joystick, device_index);
    }

    return 0;
}

/* Function to determine is this joystick is attached to the system right now */
SDL_bool SDL_SYS_JoystickAttached(SDL_Joystick *joystick)
{
    return !joystick->closed;
}

/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 */
void
SDL_SYS_JoystickUpdate(SDL_Joystick * joystick)
{
    SDL_joylist_item *item = joystick->hwdata->item;
    if (item->update != NULL) {
        (*item->update) (joystick);
    }
}

/* Function to close a joystick after use */
void
SDL_SYS_JoystickClose(SDL_Joystick * joystick)
{
    if (joystick->hwdata) {
        if (joystick->hwdata->item) {
            if (joystick->hwdata->item->close != NULL)
                (*joystick->hwdata->item->close) (joystick);
            joystick->hwdata->item->hwdata = NULL;
        }
        SDL_free(joystick->hwdata);
        joystick->hwdata = NULL;
    }

    joystick->closed = 1;
}

/* Function to perform any system-specific joystick related cleanup */
void
SDL_SYS_JoystickQuit(void)
{
    SDL_joylist_item *item = NULL;
    SDL_joylist_item *next = NULL;

    for (item = SDL_joylist; item; item = next) {
        next = item->next;
        SDL_free(item->name);
        SDL_free(item);
    }

    SDL_joylist = SDL_joylist_tail = NULL;

    numjoysticks = 0;
    instance_counter = 0;
}

SDL_JoystickGUID SDL_SYS_JoystickGetDeviceGUID( int device_index )
{
    return JoystickByDevIndex(device_index)->guid;
}


SDL_JoystickGUID SDL_SYS_JoystickGetGUID(SDL_Joystick * joystick)
{
    return joystick->hwdata->guid;
}

#endif /* SDL_JOYSTICK_EWOKOS */

/* vi: set ts=4 sw=4 expandtab: */
