#include <displayman/displayman.h>
#include <ewoksys/vdevice.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

const char* displayman_get_dev(const char* display_man_dev, uint32_t display_index) {
	static char ret[128] = {0};
	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->addi(&in, display_index);

	if(dev_cntl(display_man_dev, DISP_GET_DISP_DEV, &in, &out) == 0)
		strncpy(ret, proto_read_str(&out), 127);
	else if(display_index == 0)
		strncpy(ret, "/dev/disp0", 127);

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

int displayman_open(const char* display_man_dev, uint32_t display_index, display_t* display) {
	const char* dev = displayman_get_dev(display_man_dev, display_index); 
	if(dev == NULL || dev[0] == 0)
		return -1;

	if(display_open(dev, display_index, display) != 0)
		return -1;
	return 0;
}

uint32_t displayman_get_num(const char* display_man_dev) {
	proto_t out;
	PF->init(&out);
	uint32_t ret = 0;

	if(dev_cntl(display_man_dev, DISP_GET_DISP_NUM, NULL, &out) == 0)
		ret = proto_read_int(&out);
	PF->clear(&out);
	return ret;
}

int32_t displayman_add_dev(const char* display_man_dev, const char* dev, uint32_t display_index) {
	proto_t in, out;
	PF->init(&in)->adds(&in, dev)->addi(&in, display_index);
	PF->init(&out);
	int32_t ret = -1;

	if(dev_cntl(display_man_dev, DISP_ADD_DISP_DEV, &in, &out) == 0)
		ret = (int32_t)proto_read_int(&out);
	PF->clear(&in);
	PF->clear(&out);
	return ret;
}
