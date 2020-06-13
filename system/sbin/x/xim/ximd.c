#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>

static int _x_pid = -1;

static void input(char c) {
	xevent_t ev;
	ev.type = XEVT_KEYB;
	ev.value.keyboard.value = c;
	proto_t in;

	PF->init(&in, NULL, 0)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_x_pid = -1;

	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return 1;

	while(true) {
		if(_x_pid > 0) {
			char v;
			int rd = read(fd, &v, 1);
			if(rd == 1) {
				input(v);
			}
		}
		else {
			_x_pid = dev_get_pid("/dev/x");
			usleep(30000);
		}
	}

	close(fd);
	return 0;
}
