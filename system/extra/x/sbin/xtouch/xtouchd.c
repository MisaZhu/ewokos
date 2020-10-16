#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xclient.h>
#include <sys/vfs.h>

static int _x_pid = -1;
static int _scr_w = 0;
static int _scr_h = 0;

static void input(uint16_t state, int16_t x, int16_t y) {
	xevent_t ev;
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
	ev.value.mouse.relative = 0;
	ev.value.mouse.x = _scr_w - x*_scr_w/2000;
	ev.value.mouse.y = _scr_h - y*_scr_h/2000;
	ev.value.mouse.rx = 0;
	ev.value.mouse.ry = 0;

	if(state == 1) //down
		ev.state = XEVT_MOUSE_DOWN;
	else if(state == 0) //up
		ev.state = XEVT_MOUSE_UP;
	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_x_pid = -1;

	int fd = open("/dev/touch0", O_RDONLY | O_NONBLOCK);
	if(fd < 0)
		return 1;

	while(true) {
		if(_x_pid > 0) {
			int8_t buf[6];
			if(read(fd, buf, 6) == 6) {
				uint16_t* mv = (uint16_t*)buf;
				input(mv[0], mv[1], mv[2]);
			}
			else
				usleep(3000);
		}
		else {
			usleep(100000);
			_x_pid = dev_get_pid("/dev/x");
			if(_x_pid > 0) {
				xscreen_t scr;
				x_screen_info(&scr);
				_scr_w = scr.size.w;
				_scr_h = scr.size.h;
			}
		}
	}

	close(fd);
	return 0;
}
