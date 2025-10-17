#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <ewoksys/core.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <x/xwin.h>
#include <ewoksys/vfs.h>
#include <mouse/mouse.h>


static void input(int pid, mouse_evt_t* mevt) {
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.type = XEVT_MOUSE;
	ev.state = mevt->state;
	if(mevt->type == 1){
		ev.value.mouse.relative = 1;
		ev.value.mouse.rx = mevt->x;
		ev.value.mouse.ry = mevt->y;
	}else if(mevt->type == 2){
		ev.value.mouse.relative = 0;
		ev.value.mouse.x = mevt->x;
		ev.value.mouse.y = mevt->y;
	}
	ev.value.mouse.button = mevt->button;

	proto_t in;
	PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
	dev_cntl_by_pid(pid, X_DCNTL_INPUT, &in, NULL);
	PF->clear(&in);
}

int main(int argc, char** argv) {
	const char* dev_name = argc < 2 ? "/dev/mouse0":argv[1];
	int xpid = -1;
	int width = 0, height = 0;
	int click_detect = 0;
	long long click_time = 0;

	core_enable_ux(-1, UX_X_DEFAULT);

	int fd = open(dev_name, O_RDONLY);
	if(fd < 0) {
		fprintf(stderr, "xmouse error: open [%s] failed!\n", dev_name);
		return -1;
	}

	while(xpid < 0) {
		xpid = dev_get_pid("/dev/x");
		if(xpid > 0) {
			xscreen_info_t scr;
			x_screen_info(&scr, 0);
			width = scr.size.w;
			height = scr.size.h;
			break;
		}	
		proc_usleep(100000);
	}

	while(true) {
		mouse_evt_t mevt;
		if(mouse_read(fd, &mevt) == 1) {
			if(mevt.type == 2){
				mevt.x = mevt.x * width / 32768;
				mevt.y = mevt.y * height / 32768;
			}
			if(mevt.state == MOUSE_STATE_UP && mevt.button == MOUSE_BUTTON_LEFT){
				if(kernel_tic_ms(0) - click_time < 300){
					click_detect ++;
				}
				click_time = kernel_tic_ms(0);
			}
			//report original event
			input(xpid, &mevt);

			//report doubole click event
			if(click_detect > 0){
				click_detect = 0;
				click_time = 0;
				mevt.state = MOUSE_STATE_CLICK;
				input(xpid, &mevt);
			}
		}
		else 
			proc_usleep(5000);
	}

	close(fd);
	return 0;
}

