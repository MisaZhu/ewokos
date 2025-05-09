#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>

int main(int argc, char* argv[]) {
	int on = 0;
	proto_t in;
	if(argc < 2) {
		printf("Usage: actled [1/0]\n");
		return -1;
	}

	if(argv[1][0] != '0')
		on = 1;

	PF->init(&in)->addi(&in, on);

	dev_cntl("/dev/actled", 0, &in, NULL);
	PF->clear(&in);
	return 0;
}
