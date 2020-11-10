#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sys/proto.h>
#include <sys/keydef.h>
#include <x/xcntl.h>
#include <x/xevent.h>

static int _x_pid = -1;

#define KEY_V_UP        0x1
#define KEY_V_DOWN      0x2
#define KEY_V_LEFT      0x4
#define KEY_V_RIGHT     0x8
#define KEY_V_PRESS     0x10
#define KEY_V_1         0x20
#define KEY_V_2         0x40
#define KEY_V_3         0x80

#define JOY_STEP          6

static bool _prs_down = false;
static bool _j_mouse = true;
static bool _j_x_rev = false;
static bool _j_y_rev = false;
static uint32_t _j_speed_up = 0;

static void joy_2_mouse(int key, int8_t* mv) {
	uint32_t j_times = 1;
	if(_j_speed_up > 6) {
		j_times = _j_speed_up/6;
		if(j_times > 16)
			j_times = 16;
	}
		
	mv[0] = mv[1] = mv[2] = 0;
	switch(key) {
	case KEY_V_UP:
		mv[2] -= (_j_y_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_V_DOWN:
		mv[2] += (_j_y_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_V_LEFT:
		mv[1] -= (_j_x_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_V_RIGHT:
		mv[1] += (_j_x_rev ? -JOY_STEP:JOY_STEP) * j_times;
		return;
	case KEY_V_PRESS:
		if(!_prs_down) {
			mv[0] = 2;
			_prs_down = true;
		}
		return;
	}	
}

static void joy_2_keyb(int key, int8_t* v) {
	*v = 0;
	switch(key) {
	case KEY_V_UP:
		*v = _j_y_rev ? KEY_DOWN:KEY_UP;
		return;
	case KEY_V_DOWN:
		*v = _j_y_rev ? KEY_UP:KEY_DOWN;
		return;
	case KEY_V_LEFT:
		*v = _j_x_rev ? KEY_LEFT:KEY_RIGHT;
		return;
	case KEY_V_RIGHT:
		*v = _j_x_rev ? KEY_RIGHT:KEY_LEFT;
		return;
	case KEY_V_1:
	case KEY_V_PRESS:
		*v = KEY_ENTER;
		return;
	case KEY_V_2:
		*v = KEY_ESC;
		return;
	}	
}

static void mouse_input(int8_t state, int8_t rx, int8_t ry) {
	xevent_t ev;
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
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

static void im_input(char c) {
	xevent_t ev;
	ev.type = XEVT_IM;
	ev.value.im.value = c;
	proto_t in;

	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static void input(char key) {
	int8_t mv[4];
	if(key == KEY_V_3 && !_prs_down) { //switch joy mouse/im mode
		_j_mouse = !_j_mouse;
		_prs_down = true;
	}

	if(_j_mouse) {
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
	else {
		int8_t v;
		if(key == 0 && _prs_down)
			_prs_down = false;
		else {
			joy_2_keyb(key, &v);
			if(v != 0)
				im_input(v);
		}
	}
}

int main(int argc, char** argv) {
	_prs_down = false;
	_j_mouse = true;
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

	int fd = open(dev_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "xjoystickd error: open [%s] failed!\n", dev_name);
		return -1;
	}

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
		}
		usleep(50000);
	}

	close(fd);
	return 0;
}
