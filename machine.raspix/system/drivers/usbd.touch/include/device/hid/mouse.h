/******************************************************************************
*	device/hid/mouse.h
*	 by Steve White 
*
*	A light weight implementation of the USB protocol stack fit for a simple
*	driver.
*
*	device/hid/mouse.h contains definitions relating to mouses.
******************************************************************************/

#ifndef MOUSE_H_
#define MOUSE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <usbd/device.h>
#include <types.h>

/** The DeviceDriver field in UsbDriverDataHeader for mouse devices. */
#define DeviceDriverMouse 0x4B424431
/** The maximum number of keys a mouse can report at once. Should be 
	multiple of 2. */
#define MouseMaxKeys 6

enum MouseDeviceButton {
	MouseDeviceButtonLeft,
	MouseDeviceButtonRight,
	MouseDeviceButtonMiddle,
	MouseDeviceButtonSide,
	MouseDeviceButtonExtra,
};

/** 
	\brief Mouse specific data.

	The contents of the driver data field for mouse devices. Placed in
	HidDevice, as this driver is bult atop that.
*/
struct MouseDevice {
	/** Standard driver data header. */
	struct UsbDriverDataHeader Header;
	/** Internal - Index in mouse arrays. */
	u32 Index;

	u8 buttonState;
	s16 mouseX;
	s16 mouseY;
	s16 wheel;

	/** The input report. */
	struct HidParserReport *MouseReport;
};

/**
	\brief Enumerates a device as a mouse.

	Checks a device already checked by HidAttach to see if it appears to be a 
	mouse, and, if so, bulds up necessary information to enable the 
	mouse methods.
*/
Result MouseAttach(struct UsbDevice *device, u32 interface);

/**
	\brief Returns the number of mouses connected to the system.	
*/
u32 MouseCount();

/**
	\brief Checks a given mouse.

	Reads back the report from a given mouse and parses it into the internal
	fields. These can be accessed with MouseGet... methods
*/
Result MousePoll(u32 mouseAddress);

/** 
	\brief Returns the device address of the nth connected mouse.

	Mouses that are connected are stored in an array, and this method 
	retrieves the nth item from that array. Returns 0 on error.
*/
u32 MouseGetAddress(u32 index);

/**
	\brief Returns the current X coordinate of the mouse
*/
s16 MouseGetPositionX(u32 mouseAddress);

/**
	\brief Returns the current Y coordinate of the mouse
*/
s16 MouseGetPositionY(u32 mouseAddress);

/**
	\brief Returns the current wheel value of the mouse
*/
s16 MouseGetWheel(u32 mouseAddress);

/**
	\brief Returns the current X and Y coordinates of the mouse

	X is in the high 16 bits, Y is in the low 16 bits
*/
u32 MouseGetPosition(u32 mouseAddress);

/**
	\brief Returns the current button state of the mouse

	First bit : Left button
	Second bit: Right button
	Third bit : Middle button
	Fourth bit: Side button
	Fifth bit : Extra button
*/
u8 MouseGetButtons(u32 mouseAddress);

/**
	\brief Returns whether or not a particular mouse button is pressed.

	Reads back whether or not a mouse button was pressed on the last 
	successfully received report.
*/
bool MouseGetButtonIsPressed(u32 mouseAddress, enum MouseDeviceButton button);


#ifdef __cplusplus
}
#endif

#endif
