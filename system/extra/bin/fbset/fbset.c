#include <stdio.h>
#include <stdlib.h>
#include <sys/proto.h>
#include <sys/vdevice.h>

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: test [w] [h] <bpp>\n");
		return -1;
	}

	int bpp = 32;
	if(argc > 3) {
		bpp = atoi(argv[3]);	
	}

	proto_t in;
	PF->init(&in, NULL, 0)->
		addi(&in, atoi(argv[1]))->
		addi(&in, atoi(argv[2]))->
		addi(&in, bpp);

	dev_cntl("/dev/fb0", 0, &in, NULL);
	PF->clear(&in);
	return 0;
}
