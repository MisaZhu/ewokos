#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/basic_math.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <sys/keydef.h>
#include <x++/X.h>

using namespace Ewok;

class XIMX : public XWin {
 	int x_pid;
 	int keybFD;
	const char* keytable;
	font_t* font;

	int col, row, keyw, keyh, down_at;
protected:
	int get_at(int x, int y) {
		int i = div_u32(x, keyw);
		int j = div_u32(y, keyh);
		int at = i+j*col;
		if(at >= (int)strlen(keytable))
			return -1;
		return at;
	}

	void onEvent(xevent_t* ev) {
		if(keyw == 0 || keyh == 0)
			return;

		if(ev->type == XEVT_MOUSE) {
			int x = ev->value.mouse.winx;
			int y = ev->value.mouse.winy;
			int at = get_at(x, y);
			if(at < 0)
				return;

			if(ev->state == XEVT_MOUSE_DOWN) {
				down_at = at;
				repaint();
			}
			else if(ev->state == XEVT_MOUSE_MOVE) {
				if(down_at >= 0 && down_at != at) {
					down_at = at;
					repaint();
				}
			}
			else if(ev->state == XEVT_MOUSE_UP) {
				down_at = -1;
				char c = keytable[at];
				if(c == '\b')
					c = KEY_BACKSPACE;
				input(c);
				repaint();
			}
		}
	}

	void onRepaint(Graph& g) {
		keyw = div_u32(g.getW(), col);
		g.fill(0, 0, g.getW(), g.getH(), 0xffaaaaaa);

		for(int j=0; j<row; j++) {
			for(int i=0; i<col; i++) {
				int at = i+j*col;
				if(at >= (int)strlen(keytable))
					break;

				if(down_at == at)
					g.fill(i*keyw, j*keyh, keyw, keyh, 0xffffffff);

				char c = keytable[at];
				if(c == '\n')
					g.drawText(i*keyw + 2, 
							j*keyh + (keyh - font->h)/2,
							"En", font, 0xff000000);
				else if(c == '\b')
					g.drawText(i*keyw + 2, 
							j*keyh + (keyh - font->h)/2,
							"<-", font, 0xff000000);
				else
					g.drawChar(i*keyw + (keyw - font->w)/2,
							j*keyh + (keyh - font->h)/2,
							c, font, 0xff000000);
				g.box(i*keyw, j*keyh, keyw, keyh, 0xffcccccc);
			}
		}
	}

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(x_pid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

public:
	inline XIMX() {
		font = font_by_name("12x24");
		keytable = "abcdefghijklm/\\{}"
							 "nopqrstuvwxyz<>[]"
							 "1234567890-+.,'\" "
							 "`~!@#$%^&*()|_=\b\n";
		col = 17;
		row = 4;
		keyh = font->h + 8;
		keyw = font->w*2 + 8;
		keybFD = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
		x_pid = -1;
		down_at = -1;
	}

	inline ~XIMX() {
		if(keybFD < 0)
			return;
		::close(keybFD);
	}

	int getFixH(void) {
		return keyh * row;
	}

	int getFixW(void) {
		return keyw * col;
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
	x.open(&xwin, scr.size.w - xwin.getFixW(), scr.size.h-xwin.getFixH(), xwin.getFixW(), xwin.getFixH(), "xim",
			X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS | X_STYLE_SYSTOP | X_STYLE_XIM);
	x.run(loop, &xwin);
	return 0;
}
