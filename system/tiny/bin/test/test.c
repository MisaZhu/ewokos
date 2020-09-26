#include <stdio.h>
#include <arch/arm/bcm283x/framebuffer.h>

int main(int argc, char* argv[]) {
	bcm283x_fb_init(1024, 768, 32);
	return 0;
}
