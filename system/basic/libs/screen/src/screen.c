#include <screen/screen.h>
#include <sys/vdevice.h>
#include <string.h>
#include <unistd.h>

const char* get_scr_fb_dev(const char* scr_dev) {
	static char ret[128];
	proto_t out;
	PF->init(&out);

	if(dev_cntl(scr_dev, SCR_GET_FBDEV, NULL, &out) == 0)
		strncpy(ret, proto_read_str(&out), 127);
	else
		strncpy(ret, "/dev/fb0", 127);

	PF->clear(&out);
	return ret;
}

bool is_scr_top(const char* scr_dev) {
	proto_t out;
	PF->init(&out);
	if(dev_cntl(scr_dev, SCR_GET_TOP, NULL, &out) != 0)
		return true; //scr read failed , top by default
	int pid = proto_read_int(&out);
	bool ret = pid == getpid();
	PF->clear(&out);
	return ret;
}
