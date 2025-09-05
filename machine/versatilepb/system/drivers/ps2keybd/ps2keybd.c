#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/klog.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/mmio.h>
#include <ewoksys/keydef.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proc.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/timer.h>
#include <ewoksys/core.h>

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

#define KEYBOARD_BASE (_mmio_base+0x6000)

static uint8_t _held[128] = {0};
static bool _read = false;

int32_t keyb_init(void) {
	_mmio_base = mmio_map();
  put8(KEYBOARD_BASE + KCNTL, 0x10); // bit4=Enable bit0=INT on
  put8(KEYBOARD_BASE + KCLK, 8);
	memset(_held, 0, 128);
	_read = true;
	return 0;
}

//0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F
const char _ltab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0, '\t', '`',  0,
  0,   0,   0,   0,   0,  'q', '1',  0,    0,   0,  'z', 's', 'a', 'w', '2',  0,
  0,  'c', 'x', 'd', 'e', '4', '3',  0,    0,  ' ', 'v', 'f', 't', 'r', '5',  0,
  0,  'n', 'b', 'h', 'g', 'y', '6',  0,    0,   0,  'm', 'j', 'u', '7', '8',  0,
  0,  ',', 'k', 'i', 'o', '0', '9',  0,    0,  '.', '/', 'l', ';', 'p', '-',  0,
  0,   0,  '\'', 0,  '[', '=',  0,   0,    0,   0, '\r', ']',  0, '\\',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,  19,   0,   0,   0,   0,
  0,   0,  24,   0,   4,   5,  27,   0,    0,   0,   0,  0,    0,   0,   0,   0
};

const char _utab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   '~', 0,
  0,   0,   0,   0,   0,  'Q', '!',  0,    0,   0,  'Z', 'S', 'A', 'W', '@',  0,
  0,  'C', 'X', 'D', 'E', '$', '#',  0,    0,  ' ', 'V', 'F', 'T', 'R', '%',  0,
  0,  'N', 'B', 'H', 'G', 'Y', '^',  0,    0,   0,  'M', 'J', 'U', '&', '*',  0,
  0,  '<', 'K', 'I', 'O', ')', '(',  0,    0,  '>', '?', 'L', ':', 'P', '_',  0,
  0,   0,  '"',  0,  '{', '+',  0,   0,    0,   0,  '\r','}',  0,  '|',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,  19,   0,   0,   0,   0,
  0,   0,  24,   0,   4,   5,  27,   0,    0,   0,   0,   0,   0,   0,   0,   0
};

static inline uint8_t get_scode(void) {
	return  get8(KEYBOARD_BASE+KDATA);
}

static inline void empty(void) {
	put8(KEYBOARD_BASE+KDATA, 0);
}

#define LSHIFT 0x12
#define RSHIFT 0x59
#define CTRL   0x14

#define MAX_KEYS 8
static uint8_t _scodes[MAX_KEYS] = {0};

static int32_t keyb_handle(uint8_t scode) {
	if(scode == 0)
		return -1;

	//handle release event and key value
	if(scode == 0xF0) { //release event
		scode = get_scode(); // scan released code
		empty(); //set empty data
		if(scode <= 127 && _held[scode] == 1 && scode)  
			_held[scode] = 0;
		for(int i=0; i<MAX_KEYS; i++) {
			if(_scodes[i] == scode) {
				_scodes[i] = 0;
			}
		}
		return 0;
	}
	else if(scode == 0xFA) //empty data
		return -1;
	if(scode > 127)
		return -1;

	_held[scode] = 1;

	for(int i=0; i<MAX_KEYS; i++) {
		if(_scodes[i] == scode) {
			return 0;
		}
	}

	for(int i=0; i<MAX_KEYS; i++) {
		if(_scodes[i] == 0) {
			_scodes[i] = scode;
			break;
		}
	}
	return 0;
}

static uint8_t to_key(uint8_t scode) {
	char c = 0;
	if(scode == 0)
		return 0;

	if(scode == CTRL) 
		c = KEY_CTRL;
	else if(scode == LSHIFT) 
		c = KEY_LSHIFT;
	else if(scode == RSHIFT) 
		c = KEY_RSHIFT;
	else if(_held[LSHIFT] == 1 || _held[RSHIFT] == 1) // If shift key held
		c = _utab[scode];
	else 
		c = _ltab[scode];
	return c;
}

static int keyb_read(int fd, int from_pid, fsinfo_t* node, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)node;

	uint8_t* res = (uint8_t*)buf;
	int num = 0;
	for(int i=0; i<MAX_KEYS && i<size; i++) {
		if(_scodes[i] != 0) {
			uint8_t c = to_key(_scodes[i]);
			if(c > 0) {
				res[num] = c;
				num++;
			}
		}
	}

	if(num == 0)
		return VFS_ERR_RETRY;
	return num;
}

static void interrupt_handle(uint32_t interrupt, uint32_t p) {
	(void)p;
	uint8_t key_scode = get_scode();
	if(keyb_handle(key_scode) == 0) {
		proc_wakeup(RW_BLOCK_EVT);
	}
	return;
}

#define IRQ_RAW_KEYB (32+3) //VPB keyb interrupt at SIC bit3

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";

	keyb_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyb");
	dev.read = keyb_read;

	static interrupt_handler_t handler;
	handler.data = 0;
	handler.handler = interrupt_handle;
	sys_interrupt_setup(IRQ_RAW_KEYB, &handler);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
