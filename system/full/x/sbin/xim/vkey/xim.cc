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

	static const int TYPE_KEYB = 0;
	static const int TYPE_OTHER = 1;
	const char* keytable[2];
	int keytableType;

	int kbFD;
	font_t* font;
	gsize_t scrSize;
	bool hideMode;

	static const int INPUT_MAX = 128;
	char inputS[INPUT_MAX];

	int col, row, keyw, keyh, keySelect;
protected:
	int get_at(int x, int y) {
		if(!hideMode) {
			int input_h = font->h + 8;
			y -= input_h;
		}
		int i = div_u32(x, keyw);
		int j = div_u32(y, keyh);
		int at = i+j*col;
		if(at >= (int)strlen(keytable[keytableType]))
			return -1;
		return at;
	}

	void changeKeyTable(void) {
		if(keytableType == 0)
			keytableType = 1;
		else
			keytableType = 0;
		repaint(true);
	}

	void changeMode(bool hide) {
		hideMode = hide;
		if(hideMode) {
			int kw = 42;
			resizeTo(kw, kw);
			moveTo(scrSize.w - kw, scrSize.h - kw);
		}
		else {
			int w = scrSize.w;
			if(scrSize.w > scrSize.h*2)
				w = scrSize.w / 2;

			resizeTo(w, scrSize.h/2);
			moveTo(scrSize.w-w, scrSize.h/2);
		}
	}

	void doKeyIn(char c) {
		if(c == '\1') {
			changeMode(true);
			return;
		}
		else if(c == '\2') {
			changeKeyTable();
			return;
		}
		else if(c == '\3')
			c = keytable[keytableType][keySelect-1];
		else if(c == '\b')
			c = KEY_BACKSPACE;
		input(c);
	}

	void doMouseEvent(xevent_t* ev) {
		int x = ev->value.mouse.winx;
		int y = ev->value.mouse.winy;
		int at = get_at(x, y);
		if(at < 0)
			return;

		if(ev->state == XEVT_MOUSE_DOWN) {
			keySelect = at;
			repaint(true);
		}
		else if(ev->state == XEVT_MOUSE_MOVE) {
			if(keySelect >= 0 && keySelect != at) {
				keySelect = at;
				repaint(true);
			}
		}
		else if(ev->state == XEVT_MOUSE_UP) {
			if(hideMode) {
				changeMode(false);
				return;
			}

			char c = keytable[keytableType][keySelect];
			doKeyIn(c);	
			repaint(true);
		}
	}

	void doIMEvent(xevent_t* ev) {
		if(hideMode) {
			changeMode(false);
			return;
		}

		uint8_t c = ev->value.im.value;
		if(c == KEY_BUTTON_X) {
			changeMode(true);
			return;
		}
		else if(c == KEY_LEFT) {
			keySelect--;
		}
		else if(c == KEY_RIGHT) {
			keySelect++;
		}
		else if(c == KEY_UP) {
			keySelect -= col;
		}
		else if(c == KEY_DOWN) {
			keySelect += col;
		}
		else if(c == KEY_ENTER || c == KEY_BACKSPACE) {
			c = keytable[keytableType][keySelect];
			doKeyIn(c);
			return;
		}

		if(keySelect < 0)
			keySelect = 0;
		else if(keySelect >= strlen(keytable[keytableType]))
			keySelect = strlen(keytable[keytableType]) - 1;
		repaint(true);
	}

	void onEvent(xevent_t* ev) {
		if(keyw == 0 || keyh == 0)
			return;

		if(ev->type == XEVT_MOUSE) {
			doMouseEvent(ev);
		}
		else if(ev->type == XEVT_IM) {
			doIMEvent(ev);
		}
	}

	void draw_input(graph_t* g, int input_h) {
		int32_t w;
		graph_fill(g, 0, 0, g->w, input_h, 0xffaaaa88);
		get_text_size(inputS, font, &w, NULL);
		graph_draw_text(g, (g->w-w)/2, 2, inputS, font, 0xff000000);
	}

	const char* getKeyTitle(char c) {
		static char s[16];
		s[0] = c;
		s[1] = 0;

		if(c == ' ')
			strcpy(s, "SPACE");
		else if(c == '\n')
			strcpy(s, "ENTER");
		else if(c == '\b')
			strcpy(s, "Back");
		else if(c == '\2')
			strcpy(s, "C/#");
		else if(c == '\1')
			strcpy(s, "|||");

		return s;
	}

	void onRepaint(graph_t* g) {
		if(hideMode) {
			graph_clear(g, 0xffaaaaaa);
			graph_draw_text(g,  2, 
					(g->h - font->h)/2,
					"|||", font, 0xff000000);
			graph_box(g, 0, 0, g->w, g->h, 0xffdddddd);
			return;
		}

		graph_fill(g, 0, 0, g->w, g->w, 0xffaaaaaa);

		int input_h = font->h + 8;
		keyh = (g->h - input_h) / row;
		keyw = g->w / col;

		draw_input(g, input_h);
		const char* t = "";
		for(int j=0; j<row; j++) {
			for(int i=0; i<col; i++) {
				int at = i+j*col;
				if(at >= (int)strlen(keytable[keytableType]))
					break;
				char c = keytable[keytableType][at];
				int kx, ky, kw, kh;
				kx = i * keyw;
				ky = j * keyh+input_h;
				kw = keyw;
				kh = keyh;

				if(c >= 'a' && c <= 'z') {
					c += ('A' - 'a');
					graph_fill(g, kx, ky, kw, keyh, 0xffcccccc);
				}
				
				if(c == '\3') //two key size
					kx -= keyw;
				if(c == ' ' || c == '\n' || c == '\3') //two key size
					kw = keyw * 2;

				if(keySelect == at) { //hot key
					ky -= (j == 0 ? input_h : keyh/2);
					kh = keyh + keyh/2;
					graph_fill(g, kx, ky, kw, kh, 0xffffffff);
				}

				if(c != '\3')
					t = getKeyTitle(c);
				else if(keySelect != at)
					continue;

				int32_t tw;
				get_text_size(t, font, &tw, NULL);
				if(keySelect == at) //hot key
					graph_draw_text(g, kx + (kw-tw)/2, 
							ky + 2,
							t, font, 0xff000000);
				else
					graph_draw_text(g, kx + (kw-tw)/2, 
							ky + (kh - font->h)/2,
							t, font, 0xff000000);
				graph_box(g, kx, ky, kw, kh, 0xffdddddd);
			}
		}
	}

	void input(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;

		int len = strlen(inputS);
		if(c == KEY_BACKSPACE) {
			if(len > 0)
				inputS[len-1] = 0;
		}
		else if(c == '\n') {
			inputS[0] = 0;
		}
		else if(len < INPUT_MAX-1) {
			inputS[len] = c;
			inputS[len+1] = 0;
		}

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(xPid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

	void onFocus(void) {
		changeMode(false);
	}

public:
	inline XIMX(int fw, int fh) {
		scrSize.w = fw;
		scrSize.h = fh;
		font = font_by_name("10x20");
		keytable[1] = ""
			"1234567890%-+\b"
			"\\#$&*(){}[]!\n\3"
			"\2:;\"'<>. \3`|^\1";
		keytable[0] = ""
			"qwertyuiop-/?\b"
			"~asdfghjkl@_\n\3"
			"\2zxcvbnm \3!,.\1";
		keytableType = 0;

		col = 14;
		row = 3;
		keyh = font->h + 12;
		keyw = font->w*2 + 12;
		xPid = dev_get_pid("/dev/x");
		keySelect = -1;
		hideMode = false;
		inputS[0] = 0;
	}

	inline ~XIMX() {
		if(kbFD > 0)
			::close(kbFD);
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
	x.screenInfo(scr, 0);

	XIMX xwin(scr.size.w, scr.size.h);
	//x.open(&xwin, scr.size.w - xwin.getFixW(), scr.size.h-xwin.getFixH(), xwin.getFixW(), xwin.getFixH(), "xim",
	//x.open(&xwin, 0, scr.size.h-xwin.getFixH(), scr.size.w, xwin.getFixH(), "xim",
	x.open(&xwin, 0, scr.size.h/2, scr.size.w, scr.size.h/2, "xim",
			X_STYLE_NO_FRAME | X_STYLE_NO_FOCUS | X_STYLE_SYSTOP | X_STYLE_XIM);
	x.run(NULL, &xwin);
	return 0;
}
