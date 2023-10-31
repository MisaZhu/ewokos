#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keydef.h>
#include <upng/upng.h>
#include <font/font.h>
#include <x++/X.h>

using namespace Ewok;

class Menubar : public XWin {
	graph_t* logo;
	font_t* font;
protected:
	void onRepaint(graph_t* g) {
		graph_fill(g, 0, 0, g->w, g->h, 0xffffffff);
		graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
				g, 10, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
		graph_draw_text_font(g, 20 + logo->w, (g->h - font->max_size.y)/2, "EwokOS", font, 0xff000000);
	}

public:
	Menubar() {
		logo = png_image_new("/usr/system/icons/os.png");
		font = font_new(DEFAULT_SYSTEM_FONT, 12, true);
	}

	~Menubar() {
		if(logo != NULL)
			graph_free(logo);
		if(font != NULL)
			font_free(font);
	}
};

int main(int argc, char* argv[]) {
	X x;
	xscreen_t scr;
	x.getScreenInfo(scr, 0);
	Menubar xwin;
	x.open(&xwin, 0, 0, scr.size.w, 28, "menubar",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS);
	xwin.setVisible(true);

	grect_t desk;
	x.getDesktopSpace(desk, 0);
	desk.y += 28;
	desk.h -= 28;
	x.setDesktopSpace(desk);

	x.run(NULL);
	return 0;
}

