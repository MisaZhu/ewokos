#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <ewoksys/keydef.h>
#include <ewoksys/vdevice.h>
#include <x/xwin.h>
#include <x/xevent.h>

static void keyb_input(int32_t x_pid, char c, int state) {
	xevent_t ev;
	ev.type = XEVT_IM;
	ev.value.im.value = c;
	ev.state = state;

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static int8_t _mouse_down = 0;
static void mouse_input(int32_t x_pid, int8_t state, int8_t rx, int8_t ry) {
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
	dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static void prompt(void) {
	printf( "arraws as mouse move.\n"
			"'ctrl+x' as mouse left.\n"
			"'ctrl+c' to exit\n");
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int32_t x_pid = dev_get_pid("/dev/x");
	if(x_pid < 0) {
		printf("Error: no X found!\n");
		return -1;
	}
	prompt();

	while(1) {
		uint8_t ks[3];
		memset(ks, 0, 3);
		int32_t sz = read(0, ks, 3);
		if(sz > 0) {
			//klog("%d, %d, %d\n", ks[0], ks[1], ks[2]);
			uint8_t c = ks[0];
			if(sz == 3) {
				c = ks[2];
				if(c == 65)
					mouse_input(x_pid, 0, 0, -16);
				else if(c == 66)
					mouse_input(x_pid, 0, 0, 16);
				else if(c == 68)
					mouse_input(x_pid, 0, -16, 0);
				else if(c == 67)
					mouse_input(x_pid, 0, 16, 0);
				continue;
			}
			else {
				if(c == 24) { //click
					mouse_input(x_pid, 2, 0, 0);
					mouse_input(x_pid, 1, 0, 0);
				}
				else if(c == 3) //ctrl+c
					break;
			}
			
			keyb_input(x_pid, c, XIM_STATE_PRESS);
			keyb_input(x_pid, c, XIM_STATE_RELEASE);
		}
		//proc_usleep(30000);
	}
	close(x_pid);
	return 0;
}
