#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/mmu.h>
#include <usbd/usbd.h>
#include <device/hid/keyboard.h>

void LogPrint(const char* message, uint32_t messageLength) {
  (void)messageLength;
  kprintf(false, message);
	//usleep(600000);
}

void usb_host_init(void) {
  UsbInitialise();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
  usleep(100000);
  UsbCheckForChange();
}

typedef unsigned char (*f_keycode2char_t)(int k, unsigned char shift);
typedef struct _usbkbd_t {
    unsigned int kbd_addr;
    unsigned int keys[6];
    f_keycode2char_t keycode2char;
} usbkbd_t;

static usbkbd_t _usbkeyb;

#define USBKBD_KEYMAPSIZE (104)

unsigned char keymap_us[2][USBKBD_KEYMAPSIZE] = {
    {
        0x0, 0x1, 0x2, 0x3, 'a', 'b', 'c', 'd',
        'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
        'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
        '3', '4', '5', '6', '7', '8', '9', '0',
        0xd, 0x1b, 0x8, 0x9, ' ', '-', '=', '[',
        ']', '\\', 0x0, ';', '\'', '`', ',', '.',
        '/', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x12, 0x0, 0x0, 0x7f, 0x0, 0x0, 0x1c,
        0x1d, 0x1f, 0x1e, 0x0, '/', '*', '-', '+',
        0xd, '1', '2', '3', '4', '5', '6', '7',
        '8', '9', '0', '.', '\\', 0x0, 0x0, '=',
    },
    {
        0x0, 0x0, 0x0, 0x0, 'A', 'B', 'C', 'D',
        'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
        'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
        'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
        '#', '$', '%', '^', '&', '*', '(', ')',
        0xa, 0x1b, '\b', '\t', ' ', '_', '+', '{',
        '}', '|', '~', ':', '"', '~', '<', '>',
        '?', 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
        0x0, 0x0, 0x0, 0x0, '/', '*', '-', '+',
        0xa, '1', '2', '3', '4', '5', '6', '7',
        '8', '9', '0', '.', '|', 0x0, 0x0, '=',
    }
};

static unsigned char keycode2char_us(int k, unsigned char shift) {
    if (k > 103) {
        return 0;
    } else {
        return keymap_us[(shift == 0) ? 0 : 1][k];
    }
}

static void usbkbd_init(usbkbd_t *kbd) {
    kbd->kbd_addr = 0;
    for (int i = 0; i < 6; i++) {
        kbd->keys[i] = 0;
    }
    kbd->keycode2char = keycode2char_us;
}

// usbkbd_getc() returns -1 if no input
static int usbkbd_getc(usbkbd_t *kbd) {
    unsigned int key;
    struct KeyboardModifiers mod;
    int result = -1;

    if (kbd->kbd_addr == 0) {
        // Is there a keyboard ?
        UsbCheckForChange();
        if (KeyboardCount() > 0) {
            kbd->kbd_addr = KeyboardGetAddress(0);
        }
    }

    if (kbd->kbd_addr != 0) {
        for(int i = 0; i < 6; i++) {
            // Read and print each keycode of pressed keys
            key = KeyboardGetKeyDown(kbd->kbd_addr, i);
            if (key != kbd->keys[0] && key != kbd->keys[1] && \
                key != kbd->keys[2] && key != kbd->keys[3] && \
                key != kbd->keys[4] && key != kbd->keys[5] && key) {
                mod = KeyboardGetModifiers(kbd->kbd_addr);
                if (mod.RightControl | mod.LeftControl) {
                    unsigned char c = (*(kbd->keycode2char))(key, 1);
                    switch(c) {
                    case 'A': result = 1; break;
                    case 'B': result = 2; break;
                    case 'C': result = 3; break;
                    case 'D': result = 4; break;
                    case 'E': result = 5; break;
                    case 'F': result = 6; break;
                    case 'K': result = 11; break;
                    case 'N': result = 14; break;
                    case 'P': result = 16; break;
                    case 'U': result = 21; break;
                    default: result = 0;
                    }
                } else {
                    result = (*(kbd->keycode2char))(key, mod.RightShift | mod.LeftShift);
                    // convert arrow keys to EMACS binding controls
                    switch(result) {
                    case 0x1d: result = 2; break;
                    case 0x1c: result = 6; break;
                    case 0x1f: result = 14; break;
                    case 0x1e: result = 16; break;
                    default: break;
                    }
                }
            }
            kbd->keys[i] = key;
        }

        if (KeyboardPoll(kbd->kbd_addr) != 0) {
            kbd->kbd_addr = 0;
        }
    }
    return result;
}

static int usb_step(void* p) {
	(void)p;	
	//int c = usbkbd_getc(&_usbkeyb);
	//kprintf(false, "key: %d\n", c);
  UsbCheckForChange();
	usleep(100000);
	return 0;
}

int main(int argc, char** argv) {
	uint32_t _mmio_base = mmio_map();
	if(_mmio_base == 0)
		return -1;

	usb_host_init();
	usbkbd_init(&_usbkeyb);

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/usb";

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "usbd");
	dev.loop_step = usb_step;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
