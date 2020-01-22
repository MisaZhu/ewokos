#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/mmio.h>

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

#define KEYBOARD_BASE (_mmio_base+0x6000)

static uint32_t _mmio_base = 0;
static char _rd;
int32_t keyb_init(void) {
	_rd = 0;
	_mmio_base = mmio_map();
  //put8(KEYBOARD_BASE + KCNTL, 0x10); // bit4=Enable bit0=INT on
  put8(KEYBOARD_BASE + KCLK, 8);
	return 0;
}

//0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F
const char _ltab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0, '\t',  '`',   0,
  0,   0,   0,   0,   0,  'q', '1',  0,    0,   0,  'z', 's', 'a', 'w', '2',  0,
  0,  'c', 'x', 'd', 'e', '4', '3',  0,    0,  ' ', 'v', 'f', 't', 'r', '5',  0,
  0,  'n', 'b', 'h', 'g', 'y', '6',  0,    0,   0,  'm', 'j', 'u', '7', '8',  0,
  0,  ',', 'k', 'i', 'o', '0', '9',  0,    0,  '.', '/', 'l', ';', 'p', '-',  0,
  0,   0,  '\'',  0,  '[', '=',  0,   0,    0,   0, '\r', ']',  0, '\\',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

const char _utab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   '~',   0,
  0,   0,   0,   0,   0,  'Q', '!',  0,    0,   0,  'Z', 'S', 'A', 'W', '@',  0,
  0,  'C', 'X', 'D', 'E', '$', '#',  0,    0,  ' ', 'V', 'F', 'T', 'R', '%',  0,
  0,  'N', 'B', 'H', 'G', 'Y', '^',  0,    0,   0,  'M', 'J', 'U', '&', '*',  0,
  0,  '<', 'K', 'I', 'O', ')', '(',  0,    0,  '>', '?', 'L', ':', 'P', '_',  0,
  0,   0,  '"',  0,  '{', '+',  0,   0,    0,   0,  '\r','}',  0,  '|',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

static uint8_t _held[128] = {0};

static int32_t keyb_handle(void) {
	uint8_t scode;
	char c = 0;
	scode = get8(KEYBOARD_BASE + KDATA);

	if((scode == 0xF0)) {
		_rd = 1;
		return 0;
	}

	if(_rd == 0)
		return 0;

	if (_held[scode] == 1 && scode) {    // next scan code following key release
		_held[scode] = 0;
		return 0;
	}
	_held[scode] = 1;

	if(scode == 0x12 || scode == 0x59 || scode == 0x14)
		return 0;

	if(_held[0x14] == 1 && _ltab[scode] == 'c') { // if control held and 'c' pressed
		//kevent_push(KEV_TERMINATE, NULL);
		return 0;
	}
	else if(_held[0x14] == 1 && _ltab[scode] == '\t') {// if control held and table pressed
		//kevent_push(KEV_CONSOLE_SWITCH, NULL);
		return 0;
	}
	else if(_held[0x14] == 1 && _ltab[scode] == 'd') {// if control held and 'd' pressed
		c = 0x04;
	}
	else if (_held[0x12] == 1 || _held[0x59] == 1) { // If shift key held
		c = _utab[scode];
	}
	else { // No significant keys held               
		c = _ltab[scode];
	}
	return c;
}

static int keyb_read(int fd, int from_pid, fsinfo_t* info, 
		void* buf, int size, int offset, void* p) {
	(void)fd;
	(void)from_pid;
	(void)offset;
	(void)p;
	(void)size;
	(void)info;

	char c = keyb_handle();
	if(c == 0) {
		return ERR_RETRY;
	}
	_rd = 0;
	((char*)buf)[0] = c;
	return 1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/keyb0";

	keyb_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "keyb");
	dev.read = keyb_read;

	device_run(&dev, mnt_point, FS_TYPE_DEV, NULL, 1);
	return 0;
}
