#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <disp/disp.h>

typedef struct disp_st {
	const char *fb_dev;
} disp_t;

typedef struct DISP_st {
	uint32_t disp_num;
	disp_t   disps[DISP_MAX];
} disp_man_t;

static int DISP_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	disp_man_t* disp_man = (disp_man_t*)p;
	uint32_t disp_index = 0;
	if(in != NULL)
		disp_index = proto_read_int(in);
	if(disp_index >= disp_man->disp_num)
		return -1;

	if(cmd == DISP_GET_DISP_NUM) {
		PF->init(ret)->addi(ret, disp_man->disp_num);
	}
	else if(cmd == DISP_GET_DISP_DEV) {
		PF->init(ret)->adds(ret, disp_man->disps[disp_index].fb_dev);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/dispman";

	disp_man_t disp_man;
	memset(&disp_man, 0, sizeof(disp_man_t));

	if(argc <= 2) {
		disp_man.disps[0].fb_dev =  "/dev/fb0";
		disp_man.disp_num = 1;
	}
	else {
		for(int i=0; i<(argc-2) && i<DISP_MAX; i++) {
			disp_man.disps[i].fb_dev = argv[i+2];
			disp_man.disp_num++;
		}
	}

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "dispman");
	dev.dev_cntl = DISP_dev_cntl;
	dev.extra_data = &disp_man;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
