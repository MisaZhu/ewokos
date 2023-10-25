#include <x/xwin.h>
#include <x/x.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/vfs.h>
#include <sys/syscall.h>
#include <sys/thread.h>
#include <sys/proc.h>
#include <sys/vdevice.h>
#include <sys/cmain.h>
#include <sys/basic_math.h>
#include <sys/proc.h>
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
		proc_block(xserv_pid, 0);
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

static void sig_stop(int sig_no, void* p) {
	(void)sig_no;
	x_t* x = (x_t*)p;
	x->terminated = true;
}

void  x_init(x_t* x, void* data) {
	memset(x, 0, sizeof(x_t));
	x->data = data;
	sys_signal(SYS_SIG_STOP, sig_stop, x);
}

int  x_run(x_t* x, void* loop_data) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0) {
		return -1;
	}

	//ipc_serv_run(handle, NULL, x, IPC_NON_BLOCK);

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
				if(xwin->on_event != NULL)
					xwin->on_event(xwin, &xev);
			}
		}
		else if(x->on_loop != NULL) {
			x->on_loop(loop_data);
		}
		/*else {
			usleep(10000);
		}
		*/
	}
	dev_cntl_by_pid(xserv_pid, X_DCNTL_QUIT, NULL, NULL);
	return 0;
}

const char* x_get_work_dir(void) {
	return cmain_get_work_dir();
}

const char* x_get_theme(void) {
	static char theme[128];
	theme[0] = 0;
	proto_t out;
	PF->init(&out);

	if(dev_cntl("/dev/x", X_DCNTL_GET_THEME, NULL, &out) == 0) {
		const char* t = proto_read_str(&out);
		if(t != NULL)
			strncpy(theme, t, 127);
	}
	return theme;
}

void x_set_top(int pid) {
	proto_t in;
	PF->init(&in)->addi(&in, pid);

	dev_cntl("/dev/x", X_DCNTL_SET_TOP, &in, NULL);
	PF->clear(&in);
}

const char* x_get_theme_fname(const char* prefix, const char* app_name, const char* fname) {
	static char ret[256];
	const char* theme = getenv("XTHEME");
	if(theme[0] == 0) 
		theme = x_get_theme();
	if(theme[0] == 0) 
		theme = "default";

	if(app_name == NULL || app_name[0] == 0)
		snprintf(ret, 255, "%s/%s/%s", prefix, theme, fname);
	else
		snprintf(ret, 255, "%s/%s/%s/%s", prefix, theme, app_name, fname);
	return ret;
}

#ifdef __cplusplus
}
#endif
