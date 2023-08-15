#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>
#include <sconf/sconf.h>
#include <sys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <sys/vfs.h>

static int _x_pid = -1;
static int _scr_w = 0;
static int _scr_h = 0;

typedef struct {
	bool x_rev;
	bool y_rev;
	bool xy_switch;
	int  x_offset;
	int  y_offset;
	int  x_max;
	int  y_max;
} xtouch_t;

static xtouch_t _xtouch;

static void input(uint16_t state, int16_t tx, int16_t ty) {
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.type = XEVT_MOUSE;
	ev.state = XEVT_MOUSE_MOVE;
	ev.value.mouse.relative = 0;
	if(_xtouch.x_rev)
		ev.value.mouse.x = _scr_w - (tx*_scr_w / _xtouch.x_max);
	else
		ev.value.mouse.x = (tx*_scr_w / _xtouch.x_max);

	if(_xtouch.y_rev)
		ev.value.mouse.y = (ty*_scr_h / _xtouch.y_max);
	else
		ev.value.mouse.y = _scr_h - (ty*_scr_h / _xtouch.y_max);

	if(state == 1) //down
		ev.state = XEVT_MOUSE_DOWN;
	else if(state == 0) //up
		ev.state = XEVT_MOUSE_UP;
	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

static int32_t read_config(const char* fname) {
	memset(&_xtouch, 0, sizeof(xtouch_t));
	_xtouch.x_max = 100;
	_xtouch.y_max = 100;

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;

	const char* v = sconf_get(conf, "x_rev");
	if(v[0] == '1') 
		_xtouch.x_rev = true;

	v = sconf_get(conf, "y_rev");
	if(v[0] == '1') 
		_xtouch.y_rev = true;

	v = sconf_get(conf, "xy_switch");
	if(v[0] == '1') 
		_xtouch.xy_switch = true;

	v = sconf_get(conf, "x_max");
	if(v[0] != 0) 
		_xtouch.x_max = atoi(v);

	v = sconf_get(conf, "y_max");
	if(v[0] != 0) 
		_xtouch.y_max = atoi(v);

	v = sconf_get(conf, "x_offset");
	if(v[0] != 0) 
		_xtouch.x_offset = atoi(v);

	v = sconf_get(conf, "y_offset");
	if(v[0] != 0) 
		_xtouch.y_offset = atoi(v);
	sconf_free(conf);
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_x_pid = -1;
	read_config("/etc/x/xtouch.conf");

	const char* touch_dev = argc > 1 ? argv[1]:"/dev/touch0";
	int fd = -1;
	while(true) {
		//fd = open(touch_dev, O_RDONLY | O_NONBLOCK);
		fd = open(touch_dev, O_RDONLY);
		if(fd > 0)
			break;
		usleep(100000);
	}

	while(true) {
		_x_pid = dev_get_pid("/dev/x");
		if(_x_pid > 0) {
			xscreen_t scr;
			x_screen_info(&scr, 0);
			_scr_w = scr.size.w;
			_scr_h = scr.size.h;
			break;
		}
		usleep(100000);
	}

	uint16_t prev_ev = 0;
	while(true) {
		int8_t buf[6];
		if(read(fd, buf, 6) == 6) {
			uint16_t* mv = (uint16_t*)buf;
			if(mv[0] == 0 && prev_ev == 0)
				continue;
			prev_ev = mv[0];

			uint16_t xraw = mv[1];
			uint16_t yraw = mv[2];
			if(_xtouch.xy_switch) {
				xraw = mv[2];
				yraw = mv[1];
			}

			int16_t tx = xraw - _xtouch.x_offset;
			int16_t ty = yraw - _xtouch.y_offset;
			tx = tx < 0 ? 0 : tx;
			ty = ty < 0 ? 0 : ty;

			input(mv[0], tx, ty);
		}
		else
			usleep(2000);
	}

	close(fd);
	return 0;
}
