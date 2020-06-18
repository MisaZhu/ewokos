extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keydef.h>
#include <upng/upng.h>
}
#include <x++/X.h>

class Png : public XWin {
	int alpha;
	graph_t* img;

protected:
	void onRepaint(Graph& g) {
		g.clear(0xffffff | (alpha << 24));
		if(img != NULL) {
			g.blt(img, 0, 0, img->w, img->h,
					0, 0, img->w, img->h, 0xff);
		}

		g.drawText(10, img->h+10,
			"Press Up/Down to\nchange transparency\nof background.", 
			font_by_name("7x9"), 0xff000000);
	}

	void onEvent(xevent_t* ev) {
		int key = 0;
		if(ev->type == XEVT_KEYB) {
			key = ev->value.keyboard.value;
			if(key == KEY_ESC)
				close();
			else if(key == KEY_UP) {
				alpha += 0x22;
				if(alpha > 0xff)
					alpha = 0xff;
				repaint();
			}
			else if(key == KEY_DOWN) {
				alpha -= 0x22;
				if(alpha < 0)
					alpha = 0x0;
				repaint();
			}
		}
	}

public:
	Png() {
		alpha = 0xff;
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

	Png xwin;
	x.open(&xwin, 30, 30, img->w, img->h+60, "png", X_STYLE_NORMAL | X_STYLE_NO_RESIZE | X_STYLE_ALPHA);
	xwin.setImage(img);
	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}

