#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/vdevice.h>
#include <sys/klog.h>
#include <sys/keydef.h>
#include <x/xwin.h>
#include <string.h>

typedef struct {
	uint8_t keys[6];
	uint8_t num;
}KeyState;

class XIM {
	int x_pid;
	int keybFD;
	bool escHome;
	bool release;
	KeyState keyState;

	void input(char c, int state = XIM_STATE_PRESS) {
		klog("k: %d, st: %d\n", c, state);
		xevent_t ev;
		ev.type = XEVT_IM;
		if((c == KEY_ESC || c == KEY_BUTTON_SELECT) && escHome)
			c = KEY_HOME;
		ev.value.im.value = c;
		ev.state = state;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

	void checkRelease(char* rd, uint8_t num) {
		bool released = true;
		for(int i=0;i <keyState.num; i++) {
			release = true;
			char key = keyState.keys[i];
			for(int j=0;j <num; j++) {
				if(key == rd[j]) {// still pressed
					released = false;
					break;
				}
			}
			if(released)
				input(key, XIM_STATE_RELEASE);
			keyState.keys[i] = 0;
		}
		keyState.num = 0;
	}

public:
	inline XIM(const char* keyb_dev, bool escHome) {
		keyState.num = 0;
		x_pid = -1;
		keybFD = -1;
		this->escHome = escHome;
		release = false;
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
			for(int i = 0; i < rd; i++)
				input(v[i], XIM_STATE_PRESS);
		}
		if(rd < 0)
			rd = 0;
		checkRelease(v, rd);
		keyState.num = rd;
		for(int i = 0; i < rd; i++)
			keyState.keys[i] = v[i];
		usleep(60000);
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
