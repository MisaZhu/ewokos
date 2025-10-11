#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/proto.h>
#include <ewoksys/core.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <ewoksys/vfs.h>

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
	ev.state = MOUSE_STATE_MOVE;
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
		ev.state = MOUSE_STATE_DOWN;
	else if(state == 0) //up
		ev.state = MOUSE_STATE_UP;

	/*if(ev.state == MOUSE_STATE_UP &&
			ev.value.mouse.x < 32 && (_scr_h - ev.value.mouse.y) < 32) {
		core_next_ux(0);
		return;
	}
	*/

	if(core_get_active_ux(0) == UX_X_DEFAULT) {
		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(_x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}
}

static int32_t read_config(const char* fname) {
	memset(&_xtouch, 0, sizeof(xtouch_t));
	_xtouch.x_max = 100;
	_xtouch.y_max = 100;

	json_var_t *conf_var = json_parse_file(fname);	

	_xtouch.x_rev = json_get_bool_def(conf_var, "x_rev", false);
	_xtouch.y_rev = json_get_bool_def(conf_var, "y_rev", false);
	_xtouch.xy_switch = json_get_bool_def(conf_var, "xy_switch", false);

	_xtouch.x_max = json_get_int_def(conf_var, "x_max", 0);
	_xtouch.y_max = json_get_int_def(conf_var, "y_max", 0);
	_xtouch.x_offset = json_get_int_def(conf_var, "x_offset", 0);
	_xtouch.y_offset = json_get_int_def(conf_var, "y_offset", 0);

	if(conf_var != NULL)
		json_var_unref(conf_var);
	return 0;
}

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	_x_pid = -1;
	read_config("/etc/x/xtouch.json");

	const char* touch_dev = argc > 1 ? argv[1]:"/dev/touch0";
	int fd = -1;
	while(true) {
		//fd = open(touch_dev, O_RDONLY | O_NONBLOCK);
		fd = open(touch_dev, O_RDONLY);
		if(fd > 0)
			break;
		proc_usleep(100000);
	}

	while(true) {
		_x_pid = dev_get_pid("/dev/x");
		if(_x_pid > 0) {
			xscreen_info_t scr;
			x_screen_info(&scr, 0);
			_scr_w = scr.size.w;
			_scr_h = scr.size.h;
			break;
		}
		proc_usleep(100000);
	}

	uint16_t prev_ev = 0;
	while(true) {
		int8_t buf[6];
		memset(buf, 0, 6);
		if(read(fd, buf, 6) == 6) {
			uint16_t* mv = (uint16_t*)buf;
			if(mv[0] == 0 && prev_ev == 0) {
				proc_usleep(20000);
				continue;
			}
			prev_ev = mv[0];

			uint16_t xraw = mv[1];
			uint16_t yraw = mv[2];
			if(_xtouch.xy_switch) {
				xraw = mv[2];
				yraw = mv[1];
			}

			int16_t tx = xraw - _xtouch.x_offset;
			int16_t ty = yraw - _xtouch.y_offset;
			//slog("xtouch: tx:%d rawx:%d xoff:%d xmax:%d\n", tx, xraw, _xtouch.x_offset, _xtouch.x_max);
			//slog("xtouch: ty:%d rawy:%d yoff:%d ymax:%d\n\n", ty, yraw, _xtouch.y_offset, _xtouch.y_max);
			tx = tx < 0 ? 0 : tx;
			ty = ty < 0 ? 0 : ty;

			input(mv[0], tx, ty);
			proc_usleep(20000);
		}
		else
			proc_usleep(20000);
	}

	close(fd);
	return 0;
}
