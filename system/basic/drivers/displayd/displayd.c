#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <display/display.h>

typedef struct display_st {
	const char *fb_dev;
} display_t;

typedef struct DISP_st {
	uint32_t display_num;
	display_t   displays[DISP_MAX];
} display_man_t;

static int DISP_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	display_man_t* display_man = (display_man_t*)p;
	uint32_t display_index = 0;
	if(in != NULL)
		display_index = proto_read_int(in);
	if(display_index >= display_man->display_num)
		return -1;

	if(cmd == DISP_GET_DISP_NUM) {
		PF->init(ret)->addi(ret, display_man->display_num);
	}
	else if(cmd == DISP_GET_DISP_DEV) {
		PF->init(ret)->adds(ret, display_man->displays[display_index].fb_dev);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/display";

	display_man_t display_man;
	memset(&display_man, 0, sizeof(display_man_t));

	if(argc <= 2) {
		display_man.displays[0].fb_dev =  "/dev/fb0";
		display_man.display_num = 1;
	}
	else {
		for(int i=0; i<(argc-2) && i<DISP_MAX; i++) {
			display_man.displays[i].fb_dev = argv[i+2];
			display_man.display_num++;
		}
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "display");
	dev.dev_cntl = DISP_dev_cntl;
	dev.extra_data = &display_man;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
