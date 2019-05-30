#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <graph/tga.h>

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	tga_image* tga = tga_image_new("/data/res/0.tga");
	tga_image_destroy(tga);
	return 0;
}
