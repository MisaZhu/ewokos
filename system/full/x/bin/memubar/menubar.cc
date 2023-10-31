#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keydef.h>
#include <upng/upng.h>
#include <x++/X.h>

using namespace Ewok;

class Menubar : public XWin {
protected:
	void onRepaint(graph_t* g) {
		graph_fill(g, 0, 0, g->w, g->h, 0xffffffff);
	}

public:
	Menubar() {
	}

	~Menubar() {
	}
};

int main(int argc, char* argv[]) {
	X x;
	xscreen_t scr;
	x.getScreenInfo(scr, 0);
	Menubar xwin;
	x.open(&xwin, 0, 0, scr.size.w, 32, "menubar",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS);
	xwin.setVisible(true);

	grect_t desk;
	x.getDesktopSpace(desk, 0);
	desk.y += 32;
	desk.h -= 32;
	x.setDesktopSpace(desk);

	x.run(NULL);
	return 0;
}

