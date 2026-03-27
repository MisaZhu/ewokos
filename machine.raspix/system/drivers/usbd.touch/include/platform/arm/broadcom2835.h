/******************************************************************************
*	platform/arm/broadcom2835.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	platform/arm/broadcom2835.h contains definitions pertaining to the
*	broadcom2835 chip, used in the Raspberry Pi.
******************************************************************************/

#include <configuration.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** The null address. */
#define NULL ((void*)0)
#ifdef MEM_INTERNAL_MANAGER 
// When asked to use internal memory management, we use the default.
#define MEM_INTERNAL_MANAGER_DEFAULT
#endif

#ifdef __cplusplus
}
#endif
