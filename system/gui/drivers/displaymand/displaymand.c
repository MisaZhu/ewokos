#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <displayman/displayman.h>

#define DEV_NAME_MAX 64

typedef struct display_st {
	char display_dev[DEV_NAME_MAX];
} display_dev_t;

typedef struct DISP_st {
	display_dev_t displays[DISP_MAX];
} display_man_t;

static uint32_t display_man_get_num(display_man_t* display_man) {
	int32_t index = 0;
	uint32_t display_num = 0;
	for(index = 0; index < DISP_MAX; index++) {
		if(display_man->displays[index].display_dev[0] != 0)
			display_num++;
	}
	return display_num;
}

static int32_t get_valuable_display_index(display_man_t* display_man) {
	int32_t index = 0;
	for(index = 0; index < DISP_MAX; index++) {
		if(display_man->displays[index].display_dev[0] == 0)
			return index;
	}
	return -1;
}

static int32_t add_disp(display_man_t* display_man, const char* dev, int32_t display_index) {
	if(display_index < 0 || display_index >= DISP_MAX ||
			display_man->displays[display_index].display_dev[0] != 0) {
		display_index = get_valuable_display_index(display_man);
		if(display_index == -1)
			return -1;
	}
	strncpy(display_man->displays[display_index].display_dev, dev, DEV_NAME_MAX-1);
	return display_index;
}

static int DISP_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)dev;
	display_man_t* display_man = (display_man_t*)p;

	if(cmd == DISP_GET_DISP_NUM) {
		PF->init(ret)->addi(ret, display_man_get_num(display_man));
	}
	else if(cmd == DISP_GET_DISP_DEV) {
		uint32_t display_index = (uint32_t)proto_read_int(in);
		if(display_index >= DISP_MAX)
			return -1;
		PF->init(ret)->adds(ret, display_man->displays[display_index].display_dev);
	}
	else if(cmd == DISP_ADD_DISP_DEV) {
		const char* dev = proto_read_str(in);
		int32_t index = (int32_t)proto_read_int(in);
		index = add_disp(display_man, dev, index);
		PF->init(ret)->addi(ret, index);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = "/dev/displayman";

	display_man_t display_man;
	memset(&display_man, 0, sizeof(display_man_t));

	/*if(argc < 2) {
		add_disp(&display_man, "/dev/disp0");
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
