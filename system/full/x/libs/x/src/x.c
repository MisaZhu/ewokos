#include <x/xwin.h>
#include <x/x.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/thread.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <sconf/sconf.h>
#include <ewoksys/cmain.h>
#include <ewoksys/basic_math.h>
#include <font/font.h>
#include <ewoksys/proc.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

static int x_get_event(int xserv_pid, xevent_t* ev, bool block) {
	proto_t out;
	PF->init(&out);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_GET_EVT, NULL, &out) != 0) {
		return -1;
	}	

	int res =  proto_read_int(&out);
	if(res == 0) {
		proto_read_to(&out, ev, sizeof(xevent_t));
	}
	PF->clear(&out);

	if(res != 0 && block) {
		proc_block_by(xserv_pid, X_EVT_BLOCK_EVT);
	}

	return res;
}

int x_screen_info(xscreen_t* scr, uint32_t index) {
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->addi(&in, index);

	int ret = dev_cntl("/dev/x", X_DCNTL_GET_INFO, &in, &out);
	if(ret == 0)
		proto_read_to(&out, scr, sizeof(xscreen_t));

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

int x_get_display_num(void) {
	proto_t out;
	PF->init(&out);

	int ret = 0;
	if(dev_cntl("/dev/x", X_DCNTL_GET_DISP_NUM, NULL, &out) == 0)
		ret = proto_read_int(&out);
	return ret;
}

/*
static void sig_stop(int sig_no, void* p) {
	(void)sig_no;
	x_t* x = (x_t*)p;
	x->terminated = true;
}
*/

void x_terminate(x_t* x) {
	x->terminated = true;
	proc_wakeup_pid(getpid(), X_EVT_BLOCK_EVT);
}

const char* x_get_work_dir(void) {
	return cmain_get_work_dir();
}

static x_theme_t _x_theme;
static bool _x_theme_loaded = false;
static int32_t x_read_theme_config(const char* theme_name) {
	_x_theme.bgColor = 0xff000000;
	_x_theme.fgColor = 0xffffffff;
	_x_theme.docFGColor = 0xff000000;
	_x_theme.docBGColor = 0xffffffff;
	_x_theme.widgetFGColor = 0xff000000;
	_x_theme.widgetBGColor = 0xffffffff;
	_x_theme.fgDisableColor = 0xff444444;
	_x_theme.bgDisableColor = 0xff888888;

	strncpy(_x_theme.fontName, DEFAULT_SYSTEM_FONT,THEME_NAME_MAX-1);
	_x_theme.fontSize = DEFAULT_SYSTEM_FONT_SIZE;
	_x_theme.fontFixedSize = DEFAULT_SYSTEM_FONT_SIZE;
	strncpy(_x_theme.name, theme_name, THEME_NAME_MAX-1);

	char fname[FS_FULL_NAME_MAX];
	snprintf(fname, FS_FULL_NAME_MAX-1, "%s/%s/x/theme.conf", X_THEME_ROOT, theme_name);
	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "font_size");
	if(v[0] != 0) 
		_x_theme.fontSize = _x_theme.fontFixedSize = atoi(v);

	v = sconf_get(sconf, "font");
	if(v[0] != 0) {
		memset(_x_theme.fontName, 0, FONT_NAME_MAX);
		strncpy(_x_theme.fontName, v, FONT_NAME_MAX-1);
	}

	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		_x_theme.fgColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		_x_theme.bgColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "doc_fg_color");
	if(v[0] != 0) 
		_x_theme.docFGColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "doc_bg_color");
	if(v[0] != 0) 
		_x_theme.docBGColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "fg_unfocus_color");
	if(v[0] != 0) 
		_x_theme.fgUnfocusColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_unfocus_color");
	if(v[0] != 0) 
		_x_theme.bgUnfocusColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "fg_disable_color");
	if(v[0] != 0) 
		_x_theme.fgDisableColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_disable_color");
	if(v[0] != 0) 
		_x_theme.bgDisableColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "hide_color");
	if(v[0] != 0) 
		_x_theme.hideColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "select_color");
	if(v[0] != 0) 
		_x_theme.selectColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "select_bg_color");
	if(v[0] != 0) 
		_x_theme.selectBGColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "title_color");
	if(v[0] != 0) 
		_x_theme.titleColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "title_bg_color");
	if(v[0] != 0) 
		_x_theme.titleBGColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "widget_color");
	if(v[0] != 0) 
		_x_theme.widgetFGColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "widget_bg_color");
	if(v[0] != 0) 
		_x_theme.widgetBGColor = strtoul(v, NULL, 16);

	sconf_free(sconf);
	return 0;
}

int x_get_theme(x_theme_t* theme) {
	if(theme == NULL)
		return -1;
	const char* name = getenv("XTHEME");
	if(name == NULL || name[0] == 0) 
		name = X_DEFAULT_XTHEME;
	if(!_x_theme_loaded) {
		x_read_theme_config(name);
		_x_theme_loaded = true;
	}
	memcpy(theme, &_x_theme, sizeof(x_theme_t));
	return 0;
}

void  x_init(x_t* x, void* data) {
	memset(&_x_theme, 0, sizeof(x_theme_t));
	x_get_theme(&_x_theme);
	_x_theme_loaded = false;

	memset(x, 0, sizeof(x_t));
	x->data = data;
	//sys_signal(SYS_SIG_STOP, sig_stop, x);
}

int x_get_desktop_space(int disp_index, grect_t* r) {
	int res = -1;
	proto_t out, in;
	PF->init(&in)->addi(&in, disp_index);
	PF->init(&out);

	if(dev_cntl("/dev/x", X_DCNTL_GET_DESKTOP_SPACE, &in, &out) == 0) {
		if(proto_read_int(&out) == 0) {
			proto_read_to(&out, r, sizeof(grect_t));
			res = 0;
		}
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int x_set_desktop_space(int disp_index, const grect_t* r) {
	int res = -1;
	proto_t out, in;
	PF->format(&in, "i,m", disp_index, r, sizeof(grect_t));
	PF->init(&out);

	if(dev_cntl("/dev/x", X_DCNTL_SET_DESKTOP_SPACE, &in, &out) == 0) {
		res = proto_read_int(&out);
	}
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

void x_set_top(int pid) {
	proto_t in;
	PF->init(&in)->addi(&in, pid);

	dev_cntl("/dev/x", X_DCNTL_SET_TOP, &in, NULL);
	PF->clear(&in);
}

const char* x_get_theme_fname(const char* prefix, const char* app_name, const char* fname) {
	static char ret[256];
	x_theme_t theme;
	x_get_theme(&theme);

	if(app_name == NULL || app_name[0] == 0)
		snprintf(ret, 255, "%s/%s/%s", prefix, theme.name, fname);
	else
		snprintf(ret, 255, "%s/%s/%s/%s", prefix, theme.name, app_name, fname);
	return ret;
}

int  x_run(x_t* x, void* loop_data) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0) {
		return -1;
	}

	bool block = x->on_loop==NULL ? true:false;
	xevent_t xev;
	while(!x->terminated) {
		int res = x_get_event(xserv_pid, &xev, block);
		if(res == 0) {
			xwin_t* xwin = (xwin_t*)xev.win;
			if(xwin != NULL) {
				if(xev.type == XEVT_WIN) {
					xwin_event_handle(xwin, &xev);
				}	
				else if(xwin->on_event != NULL) {
					if(xwin->x->prompt_win == NULL ||
							xwin->x->prompt_win == xwin) //has prompt win, can't response
						xwin->on_event(xwin, &xev);
				}
			}
		}
		else if(x->on_loop != NULL) {
			x->on_loop(loop_data);
		}
	}
	dev_cntl_by_pid(xserv_pid, X_DCNTL_QUIT, NULL, NULL);
	return 0;
}

#ifdef __cplusplus
}
#endif
