#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/core.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <ewoksys/vfs.h>
#include <mouse/mouse.h>

static int _x_pid = -1;

static void input(mouse_evt_t* mevt) {
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.type = XEVT_MOUSE;
	ev.state = mevt->state;
	ev.value.mouse.relative = 1;
	ev.value.mouse.rx = mevt->rx;
	ev.value.mouse.ry = mevt->ry;
	ev.value.mouse.button = mevt->button;

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	const char* dev_name = argc < 2 ? "/dev/mouse0":argv[1];
	_x_pid = -1;

	core_set_ux(UX_X_DEFAULT);

	int fd = open(dev_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "xmouse error: open [%s] failed!\n", dev_name);
		return -1;
	}

	while(_x_pid < 0) {
		_x_pid = dev_get_pid("/dev/x");
		proc_usleep(100000);
	}

	while(true) {
		mouse_evt_t mevt;
		if(mouse_read(fd, &mevt) == 1) {
			input(&mevt);
		}
		else 
			proc_usleep(5000);
	}

	close(fd);
	return 0;
}
