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
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;
		ev.state = state;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

public:
	inline XIM(const char* keyb_dev) {
		x_pid = -1;
		keybFD = -1;
		while(true) {
			keybFD = open(keyb_dev, O_RDONLY | O_NONBLOCK);
			//keybFD = open(keyb_dev, O_RDONLY);
			if(keybFD > 0)
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
		if(x_pid < 0)
			x_pid = dev_get_pid("/dev/x");
		if(x_pid <= 0 || keybFD < 0)
			return 0;
		int ux = core_get_active_ux(0);
		if(ux != UX_X_DEFAULT)
			return 0;

		keyb_evt_t evts[KEYB_EVT_MAX] = {0};
		int n = keyb_read(keybFD, evts, KEYB_EVT_MAX);
		for(int i=0; i<n; i++)
			input(evts[i].key, evts[i].state);
		return n;
	}
};

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

	core_enable_ux(-1, UX_X_DEFAULT);
	XIM xim(keyb_dev);
	while(true) {
		//if(xim.read() == 0)
		xim.read();
		proc_usleep(_timer);
	}
	return 0;
}
