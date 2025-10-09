#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <display/display.h>

#define DEV_NAME_MAX 64
typedef struct display_st {
	char fb_dev[DEV_NAME_MAX];
} display_t;

typedef struct DISP_st {
	uint32_t display_num;
	display_t   displays[DISP_MAX];
} display_man_t;

static void add_disp(display_man_t* display_man, const char* dev) {
	if(display_man->display_num >= DISP_MAX)
		return;
	strncpy(display_man->displays[display_man->display_num].fb_dev, dev, DEV_NAME_MAX-1);
	display_man->display_num++;
}

static int DISP_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	display_man_t* display_man = (display_man_t*)p;

	if(cmd == DISP_GET_DISP_NUM) {
		PF->init(ret)->addi(ret, display_man->display_num);
	}
	else if(cmd == DISP_GET_DISP_DEV) {
		uint32_t display_index = proto_read_int(in);
		if(display_index >= display_man->display_num)
			return -1;
		PF->init(ret)->adds(ret, display_man->displays[display_index].fb_dev);
	}
	else if(cmd == DISP_ADD_DISP_DEV) {
		const char* dev = proto_read_str(in);
		add_disp(display_man, dev);
		PF->init(ret)->addi(ret, display_man->display_num);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/display";

	display_man_t display_man;
	memset(&display_man, 0, sizeof(display_man_t));

	/*if(argc < 2) {
		add_disp(&display_man, "/dev/fb0");
	}
	else {
		for(int i=1; i<argc; i++)
			add_disp(&display_man, argv[i]);
	}
			*/

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "display");
	dev.dev_cntl = DISP_dev_cntl;
	dev.extra_data = &display_man;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	return 0;
}
