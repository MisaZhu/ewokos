#include <types.h>
#include <mm/mmu.h>
#include <dev/keyboard.h>
#include <printk.h>
#include <system.h>
#include <proc.h>
#include <dev/device.h>

//0    1    2    3    4    5    6    7     8    9    A    B    C    D    E    F
const char _ltab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,  'q', '1',  0,    0,   0,  'z', 's', 'a', 'w', '2',  0,
  0,  'c', 'x', 'd', 'e', '4', '3',  0,    0,  ' ', 'v', 'f', 't', 'r', '5',  0,
  0,  'n', 'b', 'h', 'g', 'y', '6',  0,    0,   0,  'm', 'j', 'u', '7', '8',  0,
  0,  ',', 'k', 'i', 'o', '0', '9',  0,    0,  '.', '/', 'l', ';', 'p', '-',  0,
  0,   0,  '\'',  0,  '[', '=',  0,   0,    0,   0, '\r', ']',  0,  '\\',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

const char _utab[] = {
  0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,
  0,   0,   0,   0,   0,  'Q', '!',  0,    0,   0,  'Z', 'S', 'A', 'W', '@',  0,
  0,  'C', 'X', 'D', 'E', '$', '#',  0,    0,  ' ', 'V', 'F', 'T', 'R', '%',  0,
  0,  'N', 'B', 'H', 'G', 'Y', '^',  0,    0,   0,  'M', 'J', 'U', '&', '*',  0,
  0,  '<', 'K', 'I', 'O', ')', '(',  0,    0,  '>', '?', 'L', ':', 'P', '_',  0,
  0,   0,  '"',  0,  '{', '+',  0,   0,    0,   0,  '\r','}',  0,  '|',  0,   0,
  0,   0,   0,   0,   0,   0,  '\b', 0,    0,   0,   0,   0,   0,   0,   0,   0
};

#define KCNTL 0x00
#define KSTAT 0x04
#define KDATA 0x08
#define KCLK  0x0C
#define KISTA 0x10

#define KEYBOARD_BASE (MMIO_BASE+0x6000)

void keyboard_init() {
  *(uint8_t*)(KEYBOARD_BASE + KCNTL) = 0x10; // bit4=Enable bit0=INT on
  *(uint8_t*)(KEYBOARD_BASE + KCLK)  = 8;
}

static uint8_t _held[128] = {0};

#define KEYB_BUF_SIZE 16

static char _buffer_data[KEYB_BUF_SIZE];
static dev_buffer_t _keyb_buffer = { _buffer_data, KEYB_BUF_SIZE, 0, 0 };
static int32_t _keyb_lock = 0;

void keyboard_handle() {
  uint8_t scode;
	char c = 0;
  scode = *(uint8_t*)(KEYBOARD_BASE + KDATA);
 
	if (scode == 0xF0) // key release 
		return;

	if (_held[scode] == 1 && scode) {    // next scan code following key release
		_held[scode] = 0;
		return;
	}
	_held[scode] = 1;

	if(scode == 0x12 || scode == 0x59 || scode == 0x14)
		return;

	if(_held[0x14] == 1 && _ltab[scode] == 'c') { // if control held and 'c' pressed
		return;
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

	if(c == 0)
		return;

	CRIT_IN(_keyb_lock)
	dev_buffer_push(&_keyb_buffer, c, true);
	CRIT_OUT(_keyb_lock)
	//proc_wake((int32_t)&_keyb_buffer);
}

int32_t dev_keyboard_read(int16_t id, void* buf, uint32_t size) {
	(void)id;

	CRIT_IN(_keyb_lock)
	int32_t sz = size < _keyb_buffer.size ? size:_keyb_buffer.size;
	char* p = (char*)buf;	
	int32_t i = 0;
	while(i < sz) {
		char c;
		if(dev_buffer_pop(&_keyb_buffer, &c) != 0) 
			break;
		p[i] = c;
		i++;
	}
	CRIT_OUT(_keyb_lock)

	//if(i == 0)
	//	proc_sleep((int32_t)&_keyb_buffer);
	return i;	
}
