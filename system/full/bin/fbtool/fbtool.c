#include <stdio.h>
#include <stdlib.h>
#include <sys/proto.h>
#include <fb/fb.h>

int main(int argc, char* argv[]) {
	int w, h, bpp;
	if(argc < 3) {
		if(fb_dev_info("/dev/fb0", &w, &h ,&bpp) == 0)
			printf("width: %d, heigth %d, bpp: %d\n", w, h, bpp);
		else
			printf("get fb info error!\n");
		return 0;
	}

	bpp = 32;
	w = atoi(argv[1]);
	h = atoi(argv[2]);
	if(argc > 3) {
		bpp = atoi(argv[3]);	
	}

	fb_set("/dev/fb0", w, h, bpp);
	return 0;
}
