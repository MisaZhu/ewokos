#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/vdevice.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: actled [1/0]\n");
		return -1;
	}

	int on = 0;
	if(argv[1][0] != '0')
		on = 1;

	proto_t in;
	PF->init(&in, NULL, 0)->addi(&in, on);

	dev_cntl("/dev/actled", 0, &in, NULL);
	PF->clear(&in);
	return 0;
}
