#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <font/font.h>
#include <x++/X.h>

using namespace Ewok;

class XIMX : public XWin {
 	int xPid;

	static const int TYPE_KEYB = 0;
	static const int TYPE_OTHER = 1;
	static const int FONT_SIZE = 12;
	const char* keytable[2];
	int keytableType;

	font_t* font;

	gsize_t scrSize;
	gsize_t panelSize;

	static const int INPUT_MAX = 128;
	char inputS[INPUT_MAX];

	int col, row, keyw, keyh, keySelect;
	bool hideMode, capMode;
protected:
	int get_at(int x, int y) {
		int input_h = FONT_SIZE + 8;
		y -= input_h;

		/*if(x > col * keyw) 
			return -1;
		if(y > row * keyh) 
			return -1;
			*/

		int i = (x / keyw);
		if(i >= col)
			i = col - 1;

		int j = (y / keyh);
		if(j >= row)
			j = row - 1;

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
		repaint();
	}

	void changeMode(bool hide) {
		hideMode = hide;
		if(hide) {
			int kw = 24;
			if(col > 0) {
				xinfo_t info;
				getInfo(info);
				kw = info.wsr.w / col;
			}
			resizeTo(kw, kw);
			moveTo(scrSize.w - kw, scrSize.h - kw);
			return;
		}

		int w = scrSize.w;
		int h = scrSize.h / 2;

		if(panelSize.w > 0)
			w = panelSize.w;
		if(panelSize.h > 0)
			h = panelSize.h;

		resizeTo(w, h);
		moveTo(scrSize.w-w, scrSize.h-h);
	}

	void changeCapMode(void) {
		capMode = !capMode;
		repaint();
	}

	void doKeyIn(char c) {
		if(c == '\3')
			c = keytable[keytableType][keySelect-1];
		
		if(c == '\4')
			c = KEY_BACKSPACE;
		else if(c == '\2') {
			changeKeyTable();
			return;
		}
		else if(c == '\5') {
			changeCapMode();
			return;
		}

		if(c >= 'a' && c <= 'z') {
			if(capMode)
				c += ('A' - 'a');
		}
		input(c);
	}

	void doMouseEvent(xevent_t* ev) {
		if(hideMode) {
			if(ev->state == MOUSE_STATE_CLICK) {
				changeMode(false);
			}
			return;
		}

		gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
		int x = pos.x;
		int y = pos.y;
		int at = get_at(x, y);
		if(at < 0)
			return;

		if(ev->state == MOUSE_STATE_DOWN) {
			keySelect = at;
			repaint();
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			if(keySelect >= 0 && keySelect != at) {
				keySelect = at;
				repaint();
			}
		}
		else if(ev->state == MOUSE_STATE_UP) {
			char c = keytable[keytableType][keySelect];
			doKeyIn(c);	
			keySelect = -1;
			repaint();
		}
	}

	bool doHideKey(char c) {
		if(c == JOYSTICK_START)
			c = KEY_ENTER;

		if(c == KEY_UP ||
				c == KEY_DOWN ||
				c == KEY_LEFT ||
				c == KEY_RIGHT ||
				c == KEY_ENTER) {
			inputX(c);
			return true;
		}
		return false;
	}

	void doIMEvent(xevent_t* ev) {
		uint8_t c = ev->value.im.value;
		int32_t keyNum = strlen(keytable[keytableType]);

		if(ev->state == XIM_STATE_PRESS) {
			if(hideMode)
				return;

			if(c == KEY_LEFT) {
				keySelect--;
			}
			else if(c == KEY_RIGHT) {
				keySelect++;
			}
			else if(c == KEY_UP) {
				if(keySelect > col)
					keySelect -= col;
			}
			else if(c == KEY_DOWN) {
				if((keySelect+col) < keyNum)
					keySelect += col;
			}
			else
				return;
		}
		else { //RELEASE
			if(hideMode) {
				if(doHideKey(c))
					return;
				changeMode(false);
				return;
			}

			if(c == JOYSTICK_Y) {
				doKeyIn('\4');
				repaint();
				return;
			}
			else if(c == JOYSTICK_L1) {
				changeMode(true);
				return;
			}
			else if(c == JOYSTICK_START) {
				doKeyIn('\r');
				return;
			}
			else if(c == JOYSTICK_X) {
				doKeyIn('-');
				repaint();
				return;
			}
			else if(c == JOYSTICK_B) {
				doKeyIn(' ');
				repaint();
				return;
			}
			else if(c == KEY_ENTER || c == JOYSTICK_A) {
				c = keytable[keytableType][keySelect];
				doKeyIn(c);
				repaint();
				return;
			}
		}

		if(keySelect < 0)
			keySelect = 0;
		else if(keySelect >= keyNum)
			keySelect = keyNum - 1;
		repaint();
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
		uint32_t w;
		graph_fill(g, 0, 0, g->w, input_h, 0xffffffff);
		font_text_size(inputS, font, FONT_SIZE, &w, NULL);
		graph_draw_text_font(g, (g->w-w)/2, 2, inputS, font, FONT_SIZE, 0xff000000);
		graph_box(g, 0, 0, g->w, input_h, 0xff000000);
	}

	const char* getKeyTitle(char c) {
		static char s[16];
		s[0] = c;
		s[1] = 0;

		if(c == ' ')
			strcpy(s, "SPC");
		else if(c == '\r')
			strcpy(s, "ENT");
		else if(c == '\4')
			strcpy(s, "BK");
		else if(c == '\2')
			strcpy(s, "C/#");
		else if(c == '\5')
			strcpy(s, "Cap");
		else if(c == '\1')
			strcpy(s, "|||");

		return s;
	}

	void onRepaint(graph_t* g) {
		uint32_t font_h = FONT_SIZE;

		graph_fill(g, 0, 0, g->w, g->w, 0xffcccccc);
		if(hideMode) {
			graph_box(g, 0, 0, g->w, g->w, 0xff000000);
			return;
		}

		int input_h = font_h + 8;
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
					if(capMode)
						c += ('A' - 'a');
					graph_fill(g, kx, ky, kw, keyh, 0xffeeeeee);
				}
				else if((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) {
					graph_fill(g, kx, ky, kw, keyh, 0xffeeeeee);
				}
				else if(capMode && c == '\5') {
					graph_fill(g, kx, ky, kw, kh, 0xbb000000);
				}
				
				if(c == '\3') //two key size
					kx -= keyw;
				if(c == ' ' || c == '\r' || c <  '\5') //two key size
					kw = keyw * 2;

				if((i+1) == col ||
						((i+2) == col && (c == '\r' || c < '\5')))
					kw = g->w - kx;
				if((j+1) == row)
					kh = g->h - ky;

				if(keySelect == at) {
					ky -= (j == 0 ? input_h : keyh/2);
					kh = keyh + (j == 0 ? input_h : keyh/2);
					graph_fill(g, kx, ky, kw, kh, 0xbb000000);
				}

				if(c != '\3')
					t = getKeyTitle(c);
				else if(keySelect != at)
					continue;

				uint32_t tw;
				font_text_size(t, font, FONT_SIZE, &tw, NULL);
				if(keySelect == at) //hot key
					graph_draw_text_font(g, kx + (kw-tw)/2, 
							ky + 2,
							t, font, FONT_SIZE, 0xffffffff);
				else {
					uint32_t clr = 0xff000000;
					if(capMode && c == '\5') 
						clr = 0xffffffff;
					graph_draw_text_font(g, kx + (kw-tw)/2, 
							ky + (kh - font_h)/2,
							t, font, FONT_SIZE, clr);
				}
				graph_box(g, kx, ky, kw, kh, 0xffaaaaaa);
			}
		}
	}

	void inputX(char c) {
		xevent_t ev;
		ev.type = XEVT_IM;
		ev.value.im.value = c;
		ev.state = XIM_STATE_PRESS;

		proto_t in;
		PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
		dev_cntl_by_pid(xPid, X_DCNTL_INPUT, &in, NULL);
		PF->clear(&in);
	}

	void input(char c) {
		int len = strlen(inputS);
		if(c == KEY_BACKSPACE) {
			if(len > 0)
				inputS[len-1] = 0;
		}
		else if(c == '\r') {
			inputS[0] = 0;
		}
		else if(c < 0xF0 && len < INPUT_MAX-1) {
			inputS[len] = c;
			inputS[len+1] = 0;
		}
		inputX(c);
	}

	void onShow(void) {
		changeMode(false);
	}

public:
	inline XIMX(int fw, int fh, int pw, int ph) {
		scrSize.w = fw;
		scrSize.h = fh;
		panelSize.w = pw;
		panelSize.h = ph;
		font = font_new(DEFAULT_SYSTEM_FONT, true);
		keytable[1] = ""
			"1234567890\4\3"
			"~ABCDEFx+-._"
			"\\#@*(){}[]\r\3"
			"\2\3:;.,<> \3?|";
		keytable[0] = ""
			"&1234567890\4"
			".qwertyuiop-"
			"\5asdfghjkl\r\3"
			"\2\3zxcvbnm \3/";
		keytableType = 0;

		col = 12;
		row = 4;
		keyh = FONT_SIZE + 12;
		keyw = FONT_SIZE*2 + 12;
		xPid = dev_get_pid("/dev/x");
		keySelect = -1;
		inputS[0] = 0;
		hideMode = false;
		capMode = false;
	}

	inline ~XIMX() {
		if(font == NULL)
			font_free(font);
	}

	static uint32_t getCols() {
		return 14;
	}

	static uint32_t getRows() {
		return 3;
	}
};

static void waitX() {
	while(true) {
		int pid = dev_get_pid("/dev/x");
		if(pid > 0)
			break;
		proc_usleep(300000);
	}
}

static int32_t _panelW = 0;
static int32_t _panelH = 0;
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "w:h:");
		if(c == -1)
			break;

		switch (c) {
		case 'w':
			_panelW = atoi(optarg);
			break;
		case 'h':
			_panelH = atoi(optarg);
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char* argv[]) {
	waitX();

	X x;
	xscreen_info_t scr;
	x.getScreenInfo(scr, 0);

	doargs(argc, argv);
	if(_panelW == 0)
		_panelW = scr.size.w;
	if(_panelH == 0)
		_panelH = scr.size.h/2;

	XIMX xwin(scr.size.w, scr.size.h, _panelW, _panelH);
	xwin.open(&x, -1, scr.size.w - _panelW, scr.size.h - _panelH, _panelW, _panelH, "xim",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSTOP | XWIN_STYLE_XIM | XWIN_STYLE_NO_BG_EFFECT, false);
	x.run(NULL, &xwin);
	return 0;
}
