#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <font/font.h>
#include <x++/X.h>
#include <ewoksys/timer.h>
#include <ewoksys/keydef.h>
#include <tcurses/tcurses.h>
#include <ewoksys/klog.h>

using namespace Ewok;

class TestX : public XWin {
	tcurses_t tc;	
	font_t* font;

	uint32_t getColor(UNICODE16 c) {
		if((c >= 'a' && c <= 'z') ||
				(c >= 'A' && c <= 'Z'))
			return 0xff00ff00; //green 
		else if(c >= '0' && c <= '9')
			return 0xffffff00; //yellow 
		else
			return 0xffaaaaaa; //blue 
	}

	void drawBG(graph_t* g) {
		graph_clear(g, 0xff000000);
		uint32_t w = g->w / tc.cols;
		uint32_t h = g->h / tc.rows;

		uint32_t i = 0;
		while(i < g->w) {
			i += w;
			graph_line(g, i, 0, i, g->h, 0xff222222);
		}

		i = 0;
		while(i < g->h) {
			i += h;
			graph_line(g, 0, i, g->w, i, 0xff222222);
		}
	}

	gpos_t getPos(graph_t* g, uint32_t curs_x, uint32_t curs_y) {
		uint32_t w = g->w / tc.cols;
		uint32_t h = g->h / tc.rows;
		gpos_t ret = { curs_x*w, curs_y*h };
		return ret;
	}

	gpos_t getPos(graph_t* g, uint32_t at) {
		uint32_t cx = at % tc.cols;
		uint32_t cy = at / tc.cols;
		return getPos(g, cx, cy);
	}

	void drawCurs(graph_t* g) {
		static bool show = true;
		show = !show;
		if(!show)
			return;

		uint32_t w = g->w / tc.cols;
		uint32_t h = g->h / tc.rows;
		gpos_t pos = getPos(g, tc.curs_x, tc.curs_y);
		graph_fill(g, pos.x, pos.y+h-2, w, 2, getColor('_'));
	}

	void drawContent(graph_t* g) {
		uint32_t w = g->w / tc.cols;
		uint32_t h = g->h / tc.rows;

		uint32_t i = 0;
		while(i < tc.cols*tc.rows) {
			if(tc.content[i].c != 0 && tc.content[i].c != '\n') {
				gpos_t pos = getPos(g, i);
				graph_draw_char_font_fixed(g, pos.x, pos.y, tc.content[i].c, font, tc.content[i].color, w, 0);
			}
			i++;
		}
	}

	void skip(bool backward) {
		int32_t at = tcurses_at(&tc);
		int32_t o_at = at;
		int32_t size = tcurses_size(&tc);
		while(at >= 0 && at < size) {
			if(tc.content[at].c != 0)
				break;
			if(backward)
				at--;
			else
				at++;
		}	

		if(at < 0)
			at = 0;
		else if(at == size)
			at = tcurses_tail(&tc);
		tcurses_move_at(&tc, at);
	}

protected:
	void onRepaint(graph_t* g) {
		drawBG(g);
		drawContent(g);
		drawCurs(g);
	}

	void onFocus(void) {
		callXIM();
	}

	void onResize() {
		xinfo_t xinfo;
		getInfo(xinfo);
		uint32_t font_w = font->max_size.x*2/3;
		uint32_t font_h = font->max_size.y;
		tcurses_reset(&tc, xinfo.wsr.w/font_w, xinfo.wsr.h/font_h);
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			UNICODE16 c = ev->value.im.value;
			if(c == KEY_LEFT) {
				tcurses_move(&tc, -1);
				skip(true);
			}
			else if(c == KEY_RIGHT) {
				tcurses_move(&tc, 1);
				if(tcurses_get(&tc) == 0)
					skip(false);
			}
			else if(c == KEY_UP) {
				tcurses_move(&tc, -tc.cols);
				skip(true);
			}	
			else if(c == KEY_DOWN) {
				tcurses_move(&tc, tc.cols);
				skip(true);
			}
			else if(c == KEY_ENTER || c == '\n') {
				c = '\n';
				tcurses_insert(&tc, c, getColor(c));
				tcurses_move_to(&tc, 0, tc.curs_y+1);
			}	
			else if(c == KEY_BACKSPACE ||
					c == CONSOLE_LEFT) {
				skip(true);
				tcurses_set(&tc, 0, getColor(c));
			}
			else if(c == '\t') {
				tcurses_insert(&tc, ' ', getColor(c));
				tcurses_move(&tc, 1);
				tcurses_insert(&tc, ' ', getColor(c));
				tcurses_move(&tc, 1);
			}	
			else  {
				tcurses_insert(&tc, c, getColor(c));
				tcurses_move(&tc, 1);
			}	
			repaint();
		}
	}
public:
	inline TestX() {
		tcurses_init(&tc);
		font = font_new(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_SIZE, true);
	}
	
	inline ~TestX() {
		if(font != NULL)
			font_free(font);
		tcurses_close(&tc);
	}
};

static XWin* _xwin = NULL;
static void timer_handler(void) {
	_xwin->repaint();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	X x;
	TestX xwin;
	x.open(0, &xwin, 0, 0, "test_tcurses", XWIN_STYLE_NORMAL);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t tid = timer_set(500000, timer_handler);
	x.run(NULL, &xwin);
	timer_remove(tid);
	return 0;
} 
