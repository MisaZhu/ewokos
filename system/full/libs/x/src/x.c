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
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

void x_push_event(x_t* x, xevent_t* ev) {
	x_event_t* e = (x_event_t*)malloc(sizeof(x_event_t));
	e->next = NULL;
	memcpy(&e->event, ev, sizeof(xevent_t));

	if(x->event_tail != NULL)
		x->event_tail->next = e;
	else
		x->event_head = e;
	x->event_tail = e;
	//if(x->on_loop == NULL)
	//	proc_wakeup((uint32_t)x);
}

static int x_get_event(x_t* x, xevent_t* ev) {
	x_event_t* e = x->event_head;
	if(e == NULL) {
		//if(x->on_loop == NULL)
		//	proc_block(getpid(), (uint32_t)x);
		return -1;
	}

	ipc_disable();
	x->event_head = x->event_head->next;
	if (x->event_head == NULL)
		x->event_tail = NULL;

	memcpy(ev, &e->event, sizeof(xevent_t));
	free(e);
	ipc_enable();
	return 0;
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

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
	(void)from_pid;
	(void)out;
	x_t* x = (x_t*)p;

	if(cmd == X_CMD_PUSH_EVENT) {
		xevent_t ev;
		proto_read_to(in, &ev, sizeof(xevent_t));
		x_push_event(x, &ev);
	}
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

void  x_run(x_t* x, void* loop_data) {
	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0) {
		klog("Error: xserver not running!\n");
		return;
	}

	ipc_serv_run(handle, NULL, x, IPC_NON_BLOCK);

	int cpid = getpid();
	xevent_t xev;
	while(!x->terminated) {
		int res = x_get_event(x, &xev);
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
		else {
			usleep(20000);
			//proc_block(xserv_pid, cpid);
		}
	}
}

const char* x_get_work_dir(void) {
	return cmain_get_work_dir();
}

#ifdef __cplusplus
}
#endif
