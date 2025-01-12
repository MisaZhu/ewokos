#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/keydef.h>
#include <x/xcntl.h>
#include <x/xevent.h>

static int _x_pid = -1;

#define JOY_STEP         3 

static bool _prs_down = false;
static bool _j_x_rev = false;
static bool _j_y_rev = false;
static uint32_t _j_speed_up = 0;

static void joy_2_mouse(int key, int8_t* mv) {
	uint32_t j_times = 1;
	if(_j_speed_up > 8) {
		/*j_times = _j_speed_up/6;
		if(j_times > 16)
			j_times = 16;
			*/
		j_times = 4;
	}
		
	mv[0] = mv[1] = mv[2] = 0;
	switch(key) {
	case KEY_UP:
		mv[2] -= (_j_y_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_DOWN:
		mv[2] += (_j_y_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_LEFT:
		mv[1] -= (_j_x_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_RIGHT:
		mv[1] += (_j_x_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_BUTTON_A:
	case KEY_BUTTON_START:
		//if(!_prs_down) {
			mv[0] = 2;
			_prs_down = true;
		//}
		return;
	}	
}

static void mouse_input(int8_t state, int8_t rx, int8_t ry) {
	xevent_t ev;
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
	ev.value.mouse.button = MOUSE_BUTTON_LEFT;
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

static void input(char key) {
	int8_t mv[4];
	joy_2_mouse(key, mv);
	if(mv[1] != 0 || mv[2] != 0)
		_j_speed_up++;
	else
		_j_speed_up = 0;

	if(key == 0 && _prs_down) {
		key = 1;
		_prs_down = false;
		mv[0] = 1;
	}
	if(key != 0) {
		mouse_input(mv[0], mv[1], mv[2]);
	}
}

int main(int argc, char** argv) {
	_prs_down = false;
	_j_x_rev = false;
	_j_y_rev = false;
	_j_speed_up = 0;
	_x_pid = -1;

	const char* dev_name = argc < 2 ? "/dev/joystick":argv[1];

	if(argc > 2 && strstr(argv[2], "rev") != NULL) {
		if(strchr(argv[2], 'x') != NULL)
			_j_x_rev = true;
		if(strchr(argv[2], 'y') != NULL)
			_j_y_rev = true;
	}

	int fd = open(dev_name, O_RDONLY | O_NONBLOCK);
	if(fd < 0) {
		fprintf(stderr, "xjoymoused error: open [%s] failed!\n", dev_name);
		return -1;
	}

	while(true) {
		if(_x_pid > 0) {
			char v[4];
			int rd = read(fd, &v, 4);
			if(rd > 0) {
				for(int i=0;i < rd; i++)
					input(v[i]);
			}
			else {
				input(0);
			}
			proc_usleep(10000);
		}
		else {
			proc_usleep(50000);
			_x_pid = dev_get_pid("/dev/x");
		}
	}

	close(fd);
	return 0;
}
