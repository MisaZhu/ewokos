#ifndef DEVICES_H
#define DEVICES_H

enum {
	DEV_UART = 0,
	DEV_KEYBOARD,
	DEV_FRAME_BUFFER
};

#define dev_typeid(type, id) ((type) << 16 | (id))

#endif
