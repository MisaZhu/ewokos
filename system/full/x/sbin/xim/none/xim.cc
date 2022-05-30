#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/vdevice.h>
#include <sys/klog.h>
#include <sys/keydef.h>
#include <x/xwin.h>
#include <string.h>

class XIM {
	int x_pid;
	int keybFD;
	bool escHome;

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		if(c == KEY_ESC && escHome)
			c = KEY_HOME;
		ev.value.im.value = c;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

public:
	inline XIM(const char* keyb_dev, bool escHome) {
		x_pid = -1;
		keybFD = -1;
		this->escHome = escHome;
		while(true) {
			keybFD = open(keyb_dev, O_RDONLY);
			if(keybFD > 0)
				break;
			usleep(300000);
		}
	}

	inline ~XIM() {
		if(keybFD < 0)
			return;
		::close(keybFD);
	}

	void read(void) {
		if(x_pid < 0)
			x_pid = dev_get_pid("/dev/x");
		if(x_pid <= 0 || keybFD < 0)
			return;

		char v[6];
		int rd = ::read(keybFD, &v, 6);
		if(rd > 0) {
			for(int i = 0; i < rd; i++){ 
				input(v[i]);
			}
		}
		else {
			input(0);
		}
		usleep(100000);
	}
};

int main(int argc, char* argv[]) {
	const char* keyb_dev = "/dev/keyb0";
	if(argc > 1)
		keyb_dev = argv[1];

	bool escHome = false;
	if(argc > 2 && strcmp(argv[2], "esc_home") == 0) {
		escHome = true;
	}

	XIM xim(keyb_dev, escHome);
	while(true) {
		xim.read();
	}
	return 0;
}
