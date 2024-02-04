#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <ewoksys/vfs.h>

static int _x_pid = -1;
static int8_t _mouse_down = 0;

static void input(int8_t state, int8_t rx, int8_t ry) {
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
	ev.value.mouse.relative = 1;
	ev.value.mouse.rx = rx;
	ev.value.mouse.ry = ry;

	if(state == 2 || (state == 0 && _mouse_down == 1)) {//down
		ev.state = XEVT_MOUSE_DOWN;
		_mouse_down = 1;
	}
	else if(state == 1) {//up
		ev.state = XEVT_MOUSE_UP;
		_mouse_down = 0;
	}

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	const char* dev_name = argc < 2 ? "/dev/mouse0":argv[1];
	_x_pid = -1;
	_mouse_down = 0;

	int fd = open(dev_name, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "xmoused error: open [%s] failed!\n", dev_name);
		return -1;
	}

	while(_x_pid < 0) {
		_x_pid = dev_get_pid("/dev/x");
		proc_usleep(100000);
	}

	while(true) {
		int8_t mv[4];
		if(read(fd, mv, 4) == 4) {
			if(mv[0] != 0) 
				input(mv[1], mv[2], mv[3]);
		}
		proc_usleep(10000);
	}

	close(fd);
	return 0;
}
