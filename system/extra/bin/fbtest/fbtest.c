#include <stdio.h>
#include <stdlib.h>
#include <arch/arm/bcm283x/framebuffer.h>

int main(int argc, char* argv[]) {
	if(argc < 3) {
		printf("Usage: test [w] [h]\n");
		return -1;
	}

	bcm283x_fb_init(atoi(argv[1]), atoi(argv[2]), 32);
	return 0;
}
