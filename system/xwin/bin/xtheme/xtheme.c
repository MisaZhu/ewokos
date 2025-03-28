#include <ewoksys/vdevice.h>
#include <ewoksys/vfs.h>
#include <x/x.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("Usage: xtheme [theme_name].\n");
		return -1;
	}

	int xserv_pid = dev_get_pid("/dev/x");
	if(xserv_pid < 0) {
		printf("error: x not ready!\n");
		return -1;
	}

	proto_t in;
	PF->init(&in)->adds(&in, argv[1]);
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_LOAD_THEME, &in, NULL) != 0) {
		printf("error: xtheme set error!\n");
		return -1;
	}	
	if(dev_cntl_by_pid(xserv_pid, X_DCNTL_LOAD_XWM_THEME, &in, NULL) != 0) {
		printf("error: xtheme set error!\n");
		return -1;
	}
	PF->clear(&in);
    return 0;
}