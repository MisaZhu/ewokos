#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/core.h>
#include <x/xwin.h>
#include <string.h>
#include <ewoksys/timer.h>
#include <ewoksys/vfs.h>
#include <keyb/keyb.h>

class XIM {
	int x_pid;
	int keybFD;

	void input(uint8_t c, uint8_t state) {
		static bool shift = false;
		static bool ctrl = false;

		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.shift = 0;
		ev.value.im.ctrl = 0;
		ev.value.im.key_code = c;

		if (state == KEYB_STATE_PRESS) {
			if (c == KEY_LSHIFT) {
				shift = true;
				ev.value.im.shift = KEY_LSHIFT;
			} else if (c == KEY_RSHIFT) {
				shift = true;
				ev.value.im.shift = KEY_RSHIFT;
			} else if (c == KEY_CTRL) {
				ctrl = true;
			}

			if (shift) {
				ev.value.im.value = keyb_shift_value(c);
			} else if (ctrl) {
				ev.value.im.value = keyb_ctrl_value(c);
			} else {
				ev.value.im.value = c;
			}
		} else {
			if (c == KEY_LSHIFT || c == KEY_RSHIFT) {
				shift = false;
			} else if (c == KEY_CTRL) {
				ctrl = false;
			}
			ev.value.im.value = c;
		}
		ev.state = state;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		if(dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL) != 0)
			x_pid = -1;
		PF->clear(&in);
	}

public:
	inline XIM(const char* keyb_dev) {
		x_pid = -1;
		keybFD = -1;
		while(true) {
			//keybFD = open(keyb_dev, O_RDONLY | O_NONBLOCK);
			keybFD = open(keyb_dev, O_RDONLY);
			if(keybFD >= 0)
				break;
			proc_usleep(300000);
		}
	}

	inline ~XIM() {
		if(keybFD < 0)
			return;
		::close(keybFD);
	}

	int read(void) {
		if(x_pid <= 0)
			x_pid = dev_get_pid("/dev/x");
		if(x_pid <= 0 || keybFD < 0)
			return 0;

		keyb_evt_t evts[KEYB_EVT_MAX] = {0};
		int n = keyb_read(keybFD, evts, KEYB_EVT_MAX);
		for(int i=0; i<n; i++)
			input(evts[i].key, evts[i].state);
		return n;
	}
};

/*
 * Pacing between reads while keys are HELD. keybFD is opened blocking:
 * while no key is down, hid_keybd reports no VFS_EVT_RD and this proc
 * parks inside vfsd's node wait queue with zero IPCs, so an idle
 * keyboard costs nothing in the whole USB HID chain. While keys are
 * held the read keeps returning the live snapshot immediately
 * (level-triggered RD), and this sleep is what keeps that loop at a
 * sane cadence and paces keyb.c's KEY_REPEAT state machine.
 */
static uint32_t _timer = 20000;

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "t:");
		if(c == -1)
			break;

		switch (c) {
		case 't':
			_timer = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char* argv[]) {
	_timer = 20000;
	int argind = doargs(argc, argv);

	const char* keyb_dev = "/dev/keyb0";
	if(argind < argc)
		keyb_dev = argv[argind];

	XIM xim(keyb_dev);
	while(true) {
		/* blocks inside vfsd while no key is down (zero IPCs); returns
		   at once with the live snapshot while keys are held */
		xim.read();
		proc_usleep(_timer);
	}
	return 0;
}
