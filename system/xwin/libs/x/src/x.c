#include <x/xwin.h>
#include <x/x.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/thread.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/cmain.h>
#include <ewoksys/basic_math.h>
#include <font/font.h>
#include <ewoksys/proc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <setenv.h>

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

uint32_t x_get_display_id(int32_t index_def) {
	int32_t disp_index = index_def;
	if(index_def < 0) {
		const char* disp = getenv("DISPLAY_ID");
		if(disp != NULL && disp[0] != '\0')
			disp_index = atoi(disp);
	}
	if(disp_index < 0 || disp_index >= x_get_display_num())
		disp_index = 0;
	return disp_index;
}

int x_screen_info(xscreen_info_t* scr, int32_t disp_index) {
	disp_index = x_get_display_id(disp_index);
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->addi(&in, disp_index);

	int ret = dev_cntl("/dev/x", X_DCNTL_GET_INFO, &in, &out);
	if(ret == 0)
		proto_read_to(&out, scr, sizeof(xscreen_info_t));

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

typedef struct {
	xscreen_info_t screen;
	graph_t g;
} xscreen_t;

#define SCREEN_MAX 8
static xscreen_t _x_screens[SCREEN_MAX];

int x_fetch_screen_graph(uint32_t index, graph_t* g) {
	xscreen_info_t scrinfo;
	if(g == NULL || index >= SCREEN_MAX)
		return -1;
	if(x_screen_info(&scrinfo, index) != 0)
		return -1;
	if(scrinfo.g_shm_id == -1)
		return -1;

	xscreen_t* xscr = &_x_screens[index];
	uint32_t* buffer = xscr->g.buffer;
	if(buffer == NULL) {
		buffer = (uint32_t*)shmat(scrinfo.g_shm_id, 0, 0);
		if(buffer == NULL)
			return -1;

		xscr->g.buffer = buffer;
		xscr->g.w = scrinfo.size.w;
		xscr->g.h = scrinfo.size.h;
		xscr->g.need_free = false;
	}

	memcpy(g, &xscr->g, sizeof(graph_t));
	return 0;
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

void x_get_work_dir(char* ret, uint32_t len) {
	cmain_get_work_dir(ret, len);
}

static x_theme_t _x_theme;

static int x_update_theme(void) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0)
		return -1;

	proto_t out;
	PF->init(&out);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_GET_THEME, NULL, &out) != 0) {
		slog("update theme error\n");
		return -1;
	}	

	proto_read_to(&out, &_x_theme, sizeof(x_theme_t));
	PF->clear(&out);
	return 0;
}

int x_get_theme(x_theme_t* theme) {
	if(theme == NULL)
		return -1;
	if(x_update_theme())
		return -1;
	memcpy(theme, &_x_theme, sizeof(x_theme_t));
	return 0;
}

void  x_init(x_t* x, void* data) {
	memset(&_x_theme, 0, sizeof(x_theme_t));
	x_get_theme(&_x_theme);

	memset(_x_screens, 0, sizeof(xscreen_info_t)*SCREEN_MAX);
	memset(x, 0, sizeof(x_t));
	x->data = data;
	//sys_signal(SYS_SIG_STOP, sig_stop, x);
}

int x_get_desktop_space(int disp_index, grect_t* r) {
	disp_index = x_get_display_id(disp_index);
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

int x_set_top_app(const char* fname) {
	int res = -1;
	proto_t in, out;
	PF->init(&in)->adds(&in, fname);

	PF->init(&out);
	if(dev_cntl("/dev/x", X_DCNTL_SET_TOP, &in, &out) == 0)
		res = proto_read_int(&out);
	PF->clear(&in);
	PF->clear(&out);
	return res;
}

int  x_show_cursor(bool show) {
	proto_t in;
	PF->init(&in)->addi(&in, (int32_t)show);

	int res = dev_cntl("/dev/x", X_DCNTL_SHOW_CURSOR, &in, NULL);
	PF->clear(&in);
	return res;
}

int x_set_app_name(x_t* x, const char* fname) {
	if(fname == NULL || fname[0] == 0 ||
			x == NULL || x->main_win == NULL || x->main_win->xinfo == NULL)
		return -1;

	memset(x->main_win->xinfo->name, 0, X_APP_NAME_MAX);
	strncpy(x->main_win->xinfo->name, fname, X_APP_NAME_MAX-1);
	return 0;
}

int x_exec(const char* fname) {
	if(x_set_top_app(fname) == 0)
		return 0;

	setenv("X_APP_NAME", fname);
	int pid = fork();
	if(pid == 0) {
		proc_detach();
		proc_exec(fname); 
	}
	return 0;
}

const char* x_get_theme_fname(const char* prefix,
		const char* app_name,
		const char* fname, 
		char* ret,
		uint32_t len) {
	memset(ret, 0, len);
	if(fname[0] == '/') {
		strncpy(ret, fname, 255);
		return ret;
	}

	x_theme_t theme;
	x_get_theme(&theme);

	if(app_name == NULL || app_name[0] == 0)
		snprintf(ret, 255, "%s/%s/%s", prefix, theme.name, fname);
	else
		snprintf(ret, 255, "%s/%s/%s/%s", prefix, theme.name, app_name, fname);
	return ret;
}

const char* x_get_res_name(const char* name, char* ret, uint32_t len) {
	memset(ret, 0, len);
	if(name == NULL || name[0] == 0)
		return ret;
	
	if(name[0] == '/') {
		strncpy(ret, name, len);
		return ret;
	}

	char wkdir[FS_FULL_NAME_MAX+1] = {0};
	x_get_work_dir(wkdir, FS_FULL_NAME_MAX);
	if(wkdir[1] == 0 && wkdir[0] == '/')
		snprintf(ret, len, "/res/%s", name);
	else
		snprintf(ret, len, "%s/res/%s", wkdir, name);
	return ret;
}

int  x_run(x_t* x, void* loop_data) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0) {
		return -1;
	}

	while(true) {
		if(x->main_win != NULL && x->main_win->xinfo != NULL)
			break;
		usleep(100000);
	}

	x_set_app_name(x, getenv("X_APP_NAME"));

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
