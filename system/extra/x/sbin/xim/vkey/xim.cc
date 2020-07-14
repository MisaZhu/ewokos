#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/basic_math.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <x++/X.h>

using namespace Ewok;

class XIMX : public XWin {
 	int x_pid;
 	int keybFD;
	const char* keytable;
	font_t* font;

	int col, row, keyw, keyh;
protected:
	void onEvent(xevent_t* ev) {
		if(keyw == 0 || keyh == 0)
			return;

		if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_DOWN) {
			int x = ev->value.mouse.winx;
			int y = ev->value.mouse.winy;
			int i = div_u32(x, keyw);
			int j = div_u32(y, keyh);
			int at = i+j*col;
			if(at >= (int)strlen(keytable))
				return;
			char c = keytable[at];
			input(c);
		}
	}

	void onRepaint(Graph& g) {
		keyw = div_u32(g.getW(), col);
		g.fill(0, 0, g.getW(), g.getH(), 0xffffffff);
		g.box(0, 0, g.getW(), g.getH(), 0xff222222);

		for(int j=0; j<row; j++) {
			for(int i=0; i<col; i++) {
				int at = i+j*col;
				if(at >= (int)strlen(keytable))
					break;
				char c = keytable[at];
				if(c == '\n')
					g.drawText(i*keyw+(keyw/2)-font->w, j*keyh+2, "En", font, 0xff000000);
				else
					g.drawChar(i*keyw+(keyw/2)-font->w, j*keyh+2, c, font, 0xff000000);
				g.box(i*keyw, j*keyh, keyw, keyh, 0xffaaaaaa);
			}
		}
	}

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;

		proto_t in;
		PF->init(&in, NULL, 0)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

public:
	inline XIMX() {
		font = font_by_name("8x16");
		keytable = "abcdefghijklmnopqrstuvwxyz1234567890-.\n";
		col = 13;
		row = 3;
		keyh = font->h+4;
		keyw = font->w+4;
		keybFD = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
		x_pid = -1;
	}

	inline ~XIMX() {
		if(keybFD < 0)
			return;
		::close(keybFD);
	}

	int getFixH(void) {
		return keyh * row;
	}

	void doRead(void) {
		if(x_pid < 0)
			x_pid = dev_get_pid("/dev/x");
		if(x_pid <= 0 || keybFD < 0)
			return;

		char v;
		int rd = read(keybFD, &v, 1);
		if(rd == 1) {
			input(v);
		}
	}
};

static void loop(void* p) {
	XIMX* xwin = (XIMX*)p;
	xwin->doRead();
	usleep(10000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	X x;
	xscreen_t scr;
	x.screenInfo(scr);

	XIMX xwin;
	x.open(&xwin, 0, scr.size.h-xwin.getFixH(), 240, xwin.getFixH(), "xim",
			X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS | X_STYLE_SYSTOP | X_STYLE_XIM);
	x.run(loop, &xwin);
	return 0;
}
