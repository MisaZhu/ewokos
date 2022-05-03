#include <stdio.h>
#include <stdlib.h>
#include <screen/screen.h>
#include <sys/vdevice.h>
#include <sys/proto.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: screen [scr_dev] {scr_top_pid}\n");
		return -1;
	}

	if(argc == 2) {
		proto_t out;
		PF->init(&out);
		dev_cntl(argv[1], SCR_GET_TOP, NULL, &out);
		printf("%d\n", proto_read_int(&out));
		PF->clear(&out);
	}
	else if(argc == 3) {
		proto_t in;
		PF->init(&in)->addi(&in, atoi(argv[2]));
		dev_cntl(argv[1], SCR_SET_TOP, &in, NULL);
		PF->clear(&in);
	}
	return 0;
}
