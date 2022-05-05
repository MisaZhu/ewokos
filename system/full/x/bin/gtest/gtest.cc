#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/basic_math.h>
#include <sys/kernel_tic.h>
#include <sys/klog.h>
#include <x++/X.h>

using namespace Ewok;

class TestX : public XWin {
	int count, fps, imgX, imgY;
	int mode;
	uint32_t tic;
	graph_t* img_big;
	graph_t* img_small;
	font_t* font_big;
	font_t* font_small;

	static const int CIRCLE = 0;
	static const int RECT   = 1;
	static const int ROUND  = 2;

	void drawImage(graph_t* g, graph_t* img) {
		if(img == NULL)
			return;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, imgX, imgY, img->w, img->h, 0xff);
	}
public:
	inline TestX() {
		tic = 0;
		fps = 0;
		count = 0;
		mode = CIRCLE;
        imgX = imgY = 0;
		img_big = png_image_new("/data/images/rokid.png");	
		img_small = png_image_new("/data/images/rokid_small.png");	
		font_big = font_by_name("12x16");
		font_small = font_by_name("8x16");
	}
	
	inline ~TestX() {
		if(img_big != NULL)
			graph_free(img_big);
		if(img_small != NULL)
			graph_free(img_small);
	}
protected:
	void onEvent(xevent_t* ev) {
		int key = 0;
		if(ev->type == XEVT_IM) {
			key = ev->value.im.value;
			if(key == 27) //esc
				this->close();
		}
	}

	void onRepaint(graph_t* g) {
		int gW = g->w;
		int gH = g->h;
		graph_t* img = gW > (img_big->w*2) ? img_big: img_small;
		font_t* font = gW > (img_big->w*2) ? font_big: font_small;

		count++;

		int x = random_to(gW);
		int y = random_to(gH);
		int w = random_to(gW/4);
		int h = random_to(gH/4);
		int c = random();

		uint32_t low;
		kernel_tic32(NULL, NULL, &low); 
		if(tic == 0 || (low - tic) >= 3000000) //3 second
			tic = low;

		if(w > h*2)
			w = h*2;

		w = w < 32 ? 32 : w;
		h = h < 32 ? 32 : h;

		if(tic == low) { //3 second
			fps = count/3;
			count = 0;

			mode++;
			if(mode > ROUND)
				mode = 0;

			graph_fill(g, 0, 0, gW, gH, 0xff000000);
			imgX = random_to(gW - img->w);
			imgY = random_to(gH - img->h - font->h);
		}

		if(mode == CIRCLE) {
			graph_fill_circle(g, x, y, h/2, c);
			graph_circle(g, x, y, h/2+4, c);
		}
		else if(mode == ROUND) {
			graph_fill_round(g, x, y, w, h, 12, c);
			graph_round(g, x-4, y-4, w+8, h+8, 16, c);
		}
		else if(mode == RECT) {
			graph_fill(g, x, y, w, h, c);
			graph_box(g, x-4, y-4, w+8, h+8, c);
		}

		char str[32];
		snprintf(str, 31, "EwokOS FPS: %d", fps);
		get_text_size(str, font, (int32_t*)&w, NULL);
		graph_fill(g, imgX, imgY+img->h+2, img->w, font->h+4, 0xffffffff);
		graph_draw_text(g, imgX+4, imgY+img->h+4, str, font, 0xff000000);
		drawImage(g, img);
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	//usleep(30000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	xscreen_t scr;

	X x;
	x.screenInfo(scr);

	TestX xwin;
	x.open(&xwin, 60, 40, scr.size.w-120, scr.size.h-80, "gtest", X_STYLE_NORMAL);
	xwin.setVisible(true);

	x.run(loop, &xwin);
	return 0;
} 
