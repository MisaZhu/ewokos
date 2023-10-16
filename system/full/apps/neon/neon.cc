#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <font/font.h>
#include <x++/X.h>
#include <sys/timer.h>

using namespace Ewok;

class TestX : public XWin {
	int count, fps, imgX, imgY;
	int mode;
	uint32_t tic;
	graph_t* img;
	font_t font;

	static const int CIRCLE = 0;
	static const int RECT   = 1;
	static const int ROUND  = 2;

	void drawImage(graph_t* g, graph_t* img) {
		if(img == NULL)
			return;

		switch(mode){
			case 0:
				graph_blt_alpha_cpu(img, 0, 0, img->w, img->h, g, imgX, imgY, img->w, img->h, 0xff);
				break;
			case 1:
				graph_blt_alpha(img, 0, 0, img->w, img->h, g, imgX, imgY, img->w, img->h, 0xff);
				break;
			case 2:
				graph_blt_cpu(img, 0, 0, img->w, img->h, g, imgX, imgY, img->w, img->h);
				break;
			case 3:
				graph_blt(img, 0, 0, img->w, img->h, g, imgX, imgY, img->w, img->h);
				break;
			case 4:
				graph_fill_cpu(g, imgX, imgY, img->w, img->h, 0xff7F0000 + count);
				break;
			case 5:
				graph_fill(g, imgX, imgY, img->w, img->h, 0xff7F0000 + count);
				break;
			case 6:
				graph_fill_cpu(g, imgX, imgY, img->w, img->h, 0x7f7F0000 + count);
				break;
			case 7:
				graph_fill(g, imgX, imgY, img->w, img->h, 0x7F7EFFFF + count);
				break;
			default:
				mode %=8;
				break;
		}
	}
public:
	inline TestX() {
		tic = 0;
		fps = 0;
		count = 0;
		mode = CIRCLE;
        imgX = imgY = 0;
		img = png_image_new(X::getResName("icon.png"));	
		font_load("/data/fonts/system.ttf", 13, &font);
	}
	
	inline ~TestX() {
		if(img != NULL)
			graph_free(img);
		if(font.id >= 0)
			font_close(&font);
	}
protected:
	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_RELEASE) {
			int key = ev->value.im.value;
			if(key == 96){
				mode++;
				mode%=8;
				count = 0;
				kernel_tic32(NULL, NULL, &tic);
			}
		}
	}

	void onRepaint(graph_t* g) {
		int gW = g->w;
		int gH = g->h;
		uint32_t font_h = font.max_size.y;

		count++;


		uint32_t low;
		kernel_tic32(NULL, NULL, &low); 
		if(tic == 0 || (low - tic) >= 3000000) //3 second
			tic = low;

		if(tic == low) { //3 second
			fps = count/3;
			count = 0;
			graph_fill(g, 0, 0, g->w, g->h, 0xff000000);
			char str[8];
			snprintf(str, 7, "%d:%d", mode, fps);
			graph_draw_text_font(g, 0, 0, str, &font, 0xffffffff);

		}

		for(int i = 0; i < 100 ;i ++){
			imgX = random_to(gW - img->w);
			imgY = random_to(gH - img->h - font_h) + font_h;
			drawImage(g, img);
		}
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	usleep(50);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;

	X x;
	x.screenInfo(scr, 0);

	TestX xwin;
	x.open(&scr, &xwin, 0, 0, "neon", X_STYLE_NORMAL);
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
