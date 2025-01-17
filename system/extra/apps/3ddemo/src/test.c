#include <stdio.h>
#include <stdint.h>
#include "cbmp.h"
#include "engine.h"
BMP* bmp;

void draw_line(int x0, int y0, int x1, int y1, uint32_t color){

}

void draw_pixel(int x, int y, uint32_t color){
	uint8_t r = (color>>16)&0xFF;
	uint8_t g = (color>>8)&0xFF;
	uint8_t b = (color>>0)&0xFF;
	set_pixel_rgb(bmp, x, y, r, g, b);
}

int main(int argc, char* argv[]){
	printf("test\n");

	bmp = bopen("temp.bmp");
	setup(get_width(bmp), get_height(bmp));
	update();
	render();

	 bwrite(bmp, "test.bmp");

	 bclose(bmp);
	return 0;
}
