/******************************************************************************
*	device/hid/touch.h
*	 by Alex Chadwick
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	device/hid/touch.h contains definitions relating to touch.
******************************************************************************/

#ifndef TOUCH_H_
#define TOUCH_H_

#ifdef __cplusplus
extern "C"
{
#endif


#include <usbd/device.h>
#include <types.h>

struct TouchEvent{
	u16 event;
	u16 x;
	u16 y;
	u16 size_x;
	u16 size_y;	
};

/** 
	\brief Keyboard specific data.

	The contents of the driver data field for keyboard devices. Placed in
	HidDevice, as this driver is built atop that.
*/
struct TouchDevice {
	/** Standard driver data header. */
	struct UsbDriverDataHeader Header;
};

/**
	\brief Enumerates a device as a touch.

	Checks a device already checked by HidAttach to see if it appears to be a 
	touch, and, if so, builds up necessary information to enable the 
	touch methods.
*/
Result TouchAttach(struct UsbDevice *device, u32 interface);

/**
	\brief Checks a given keyboard.

	Reads back the report from a given keyboard and parses it into the internal
	fields. These can be accessed with KeyboardGet... methods
*/
Result TouchPoll();

Result TouchGetEvent(struct TouchEvent* event);

#ifdef __cplusplus
}
#endif

#endif
