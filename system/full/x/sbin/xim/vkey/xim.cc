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
 	int xPid;
	const char* keytable;
	font_t* font;
	gsize_t scrSize;
	bool hideMode;

	int col, row, keyw, keyh, keySelect;
protected:
	int get_at(int x, int y) {
		int i = div_u32(x, keyw);
		int j = div_u32(y, keyh);
		int at = i+j*col;
		if(at >= (int)strlen(keytable))
			return -1;
		return at;
	}

	void changeMode(bool hide) {
		hideMode = hide;
		if(hideMode) {
			int w = 42;
			resizeTo(w, w);
			moveTo(scrSize.w - w, scrSize.h - w);
		}
		else {
			resizeTo(scrSize.w, scrSize.h/2);
			moveTo(0, scrSize.h/2);
		}
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
				keySelect = at;
				repaint();
			}
			else if(ev->state == XEVT_MOUSE_MOVE) {
				if(keySelect >= 0 && keySelect != at) {
					keySelect = at;
					repaint();
				}
			}
			else if(ev->state == XEVT_MOUSE_UP) {
				keySelect = -1;
				if(hideMode) {
					changeMode(false);
					return;
				}

				char c = keytable[at];
				if(c == '\1') {
					changeMode(true);
					return;
				}
				if(c == '\b')
					c = KEY_BACKSPACE;
				input(c);
				repaint();
			}
		}
	}

	void onRepaint(graph_t* g) {
		if(hideMode) {
			graph_clear(g, 0xffaaaaaa);
			graph_draw_text(g,  2, 
					(g->h - font->h)/2,
					"^^", font, 0xff000000);
			graph_box(g, 0, 0, g->w, g->h, 0xffdddddd);
			return;
		}

		keyh = g->h / row;
		keyw = g->w / col;

		graph_fill(g, 0, 0, g->w, g->w, 0xffaaaaaa);
		for(int j=0; j<row; j++) {
			for(int i=0; i<col; i++) {
				int at = i+j*col;
				if(at >= (int)strlen(keytable))
					break;
				char c = keytable[at];
				if(c >= 'a' && c <= 'z') {
					c += ('A' - 'a');
					graph_fill(g, i*keyw, j*keyh, keyw, keyh, 0xffcccccc);
				}

				if(keySelect == at)
					graph_fill(g, i*keyw, j*keyh, keyw, keyh, 0xffffffff);

				if(c == '\n')
					graph_draw_text(g, i*keyw + 2, 
							j*keyh + (keyh - font->h)/2,
							"En", font, 0xff000000);
				else if(c == '\b')
					graph_draw_text(g, i*keyw + 2, 
							j*keyh + (keyh - font->h)/2,
							"<-", font, 0xff000000);
				else if(c == '\1')
					graph_draw_text(g, i*keyw + 2, 
							j*keyh + (keyh - font->h)/2,
							"VV", font, 0xff000000);
				else 
					graph_draw_char(g, i*keyw + (keyw - font->w)/2,
							j*keyh + (keyh - font->h)/2,
							c, font, 0xff000000);

				graph_box(g, i*keyw, j*keyh, keyw, keyh, 0xffdddddd);
			}
		}
	}

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(xPid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

public:
	inline XIMX(int fw, int fh) {
		scrSize.w = fw;
		scrSize.h = fh;
		font = font_by_name("9x16");
		keytable = ""
			"`~1234567890-+%@\b"
			"'\"qwertyuiop{}[]\n"
			"*_|asdfghjkl()=/\\"
			"#$^zxcvbnm &<>.,\1";
		col = 17;
		row = 4;
		keyh = font->h + 12;
		keyw = font->w*2 + 12;
		xPid = dev_get_pid("/dev/x");
		keySelect = -1;
		hideMode = false;
	}

	inline ~XIMX() {
		::close(xPid);
	}

	int getFixH(void) {
		return keyh * row;
	}

	int getFixW(void) {
		return keyw * col;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	X x;
	xscreen_t scr;
	x.screenInfo(scr);

	XIMX xwin(scr.size.w, scr.size.h);
	//x.open(&xwin, scr.size.w - xwin.getFixW(), scr.size.h-xwin.getFixH(), xwin.getFixW(), xwin.getFixH(), "xim",
	//x.open(&xwin, 0, scr.size.h-xwin.getFixH(), scr.size.w, xwin.getFixH(), "xim",
	x.open(&xwin, 0, scr.size.h/2, scr.size.w, scr.size.h/2, "xim",
			X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS | X_STYLE_SYSTOP | X_STYLE_XIM);
	x.run(NULL, &xwin);
	return 0;
}
