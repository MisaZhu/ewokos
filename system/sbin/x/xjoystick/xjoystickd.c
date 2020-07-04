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

static bool _prs_down = false;
static bool _j_mouse = true;

static void joy_2_mouse(int key, int8_t* mv) {
	mv[0] = mv[1] = mv[2] = 0;
	switch(key) {
	case KEY_V_UP:
		mv[2] -= 8;
		return;
	case KEY_V_DOWN:
		mv[2] += 8;
		return;
	case KEY_V_LEFT:
		mv[1] -= 8;
		return;
	case KEY_V_RIGHT:
		mv[1] += 8;
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
		*v = KEY_UP;
		return;
	case KEY_V_DOWN:
		*v = KEY_DOWN;
		return;
	case KEY_V_LEFT:
		*v = KEY_LEFT;
		return;
	case KEY_V_RIGHT:
		*v = KEY_RIGHT;
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
	PF->init(&in, NULL, 0)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static void im_input(char c) {
	xevent_t ev;
	ev.type = XEVT_IM;
	ev.value.im.value = c;
	proto_t in;

	PF->init(&in, NULL, 0)->add(&in, &ev, sizeof(xevent_t));
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
	(void)argc;
	(void)argv;

	_prs_down = false;
	_j_mouse = true;
	_x_pid = -1;

	int fd = open("/dev/joystick", O_RDONLY);
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
		}
		usleep(50000);
	}

	close(fd);
	return 0;
}
