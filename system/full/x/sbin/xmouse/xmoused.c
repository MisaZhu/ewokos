#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <sys/vfs.h>

static int _x_pid = -1;

static void input(int8_t state, int8_t rx, int8_t ry) {
	xevent_t ev;
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
	ev.value.mouse.relative = 1;
	ev.value.mouse.x = 0;
	ev.value.mouse.y = 0;
	ev.value.mouse.rx = rx;
	ev.value.mouse.ry = ry;

	if(state == 2) //down
		ev.state = XEVT_MOUSE_DOWN;
	else if(state == 1) //up
		ev.state = XEVT_MOUSE_UP;

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	const char* dev_name = argc < 2 ? "/dev/mouse0":argv[1];
	_x_pid = -1;

	int fd = open(dev_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "xmoused error: open [%s] failed!\n", dev_name);
		return -1;
	}

	while(_x_pid < 0) {
		_x_pid = dev_get_pid("/dev/x");
		usleep(100000);
	}

	while(true) {
		int8_t mv[4];
		if(read(fd, mv, 4) == 4)
			input(mv[0], mv[1], mv[2]);
	}

	close(fd);
	return 0;
}
