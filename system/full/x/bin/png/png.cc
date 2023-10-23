#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keydef.h>
#include <upng/upng.h>
#include <x++/X.h>

using namespace Ewok;

class Png : public XWin {
	graph_t* img;

	void background(graph_t* g, int sz) {
		int x = 0;
		int y = 0;
		uint32_t c1;
		uint32_t c2;
		for(int i=0; ;i++) {
			if((i%2) == 0) {
				c1 = 0x44888888;
				c2 = 0x44444444;
			}
			else {
				c2 = 0x44888888;
				c1 = 0x44444444;
			}

			for(int j=0; ;j++) {
				graph_fill(g, x, y, sz, sz, (j%2)==0? c1:c2);
				x += sz;
				if(x >= g->w)
					break;
			}
			x = 0;
			y += sz;
			if(y >= g->h)
				break;
		}
	}
protected:
	void onRepaint(graph_t* g) {
		if(img == NULL)
			return;

		int sz = 32;
		if(img->w <= sz)
			sz = img->w/2;

		background(g, sz);
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, (g->w - img->w)/2, (g->h - img->h)/2, img->w, img->h, 0xff);
	}

public:
	Png() {
		img = NULL;
	}

	~Png() {
		if(img == NULL)
			return;
		graph_free(img);
	}

	inline void setImage(graph_t* img) {
		this->img = img;
	}
};

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: png <png filename>\n");
		return -1;
	}

	graph_t* img = png_image_new(argv[1]);
	if(img == NULL) {
		printf("load '%s' failed!\n", argv[1]);
		return -1;
	}

	X x;
	xscreen_t scr;
	x.screenInfo(scr, 0);
	Png xwin;
	x.open(&scr, &xwin, img->w, img->h, "png",
			X_STYLE_NORMAL | X_STYLE_NO_RESIZE);
			//X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS);
	xwin.setImage(img);
	xwin.setAlpha(true);
	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}

