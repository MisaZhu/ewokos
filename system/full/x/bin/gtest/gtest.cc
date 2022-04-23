#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <upng/upng.h>
#include <sys/basic_math.h>
#include <x++/X.h>

using namespace Ewok;

class TestX : public XWin {
	int count, imgX, imgY;
	int mode;
	graph_t* img_big;
	graph_t* img_small;
	font_t* font;

	const int CIRCLE = 0;
	const int RECT   = 1;
	const int ROUND  = 2;

	void drawImage(Graph& g, graph_t* img) {
		if(img == NULL)
			return;
		g.blt(img, 0, 0, img->w, img->h,
				imgX, imgY, img->w, img->h, 0xff);
	}
public:
	inline TestX() {
		count = 0;
		mode = CIRCLE;
        imgX = imgY = 0;
		img_big = png_image_new("/data/images/rokid.png");	
		img_small = png_image_new("/data/images/rokid_small.png");	
		font = font_by_name("8x16");
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

	void onRepaint(Graph& g) {
		int gW = g.getW();
		int gH = g.getH();
		char str[32];
		graph_t* img = gW > (img_big->w*2) ? img_big: img_small;

		int x = random_to(gW);
		int y = random_to(gH);
		int w = random_to(gW/4);
		int h = random_to(gH/4);
		int c = random();

		w = w < 32 ? 32 : w;
		h = h < 32 ? 32 : h;

		if((count++ % 100) == 0) {
			mode++;
			if(mode > ROUND)
				mode = 0;

			g.fill(0, 0, gW, gH, 0xff000000);
			imgX = random_to(gW - img->w);
			imgY = random_to(gH - img->h - font->h);
		}

		if(mode == CIRCLE) {
			g.fillCircle(x, y, h, c);
			g.circle(x, y, h+10, c);
		}
		else if(mode == ROUND) {
			g.fillRound(x, y, w, h, 12, c);
			g.round(x-4, y-4, w+8, h+8, 16, c);
		}
		else if(mode == RECT) {
			g.fill(x, y, w, h, c);
			g.box(x-4, y-4, w+8, h+8, c);
		}

		snprintf(str, 31, "EwokOS %d", count);
		get_text_size(str, font, (int32_t*)&w, NULL);
		g.fill(imgX, imgY+img->h+2, img->w, font->h, 0xffffffff);
		g.drawText(imgX+4, imgY+img->h+2, str, font, 0xff000000);
		drawImage(g, img);
	}
};

static void loop(void* p) {
	XWin* xwin = (XWin*)p;
	xwin->repaint();
	usleep(3000);
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
