#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <screen/screen.h>

typedef struct screen_st {
	int top_pid;
	const char *fb_dev;
} screen_t;

static int scr_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)in;
	screen_t* scr = (screen_t*)p;

	if(cmd == SCR_SET_TOP) {
		scr->top_pid = from_pid;
	}
	else if(cmd == SCR_GET_TOP) {
		PF->init(ret)->addi(ret, scr->top_pid);
	}
	else if(cmd == SCR_GET_FBDEV) {
		PF->init(ret)->adds(ret, scr->fb_dev);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/screen0";
	const char* fb_dev = argc > 2 ? argv[2]: "/dev/fb0";

	screen_t scr;
	scr.fb_dev = fb_dev;

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "screen");
	dev.dev_cntl = scr_dev_cntl;
	dev.extra_data = &scr;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
