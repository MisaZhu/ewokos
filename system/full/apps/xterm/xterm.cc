#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <terminal/terminal.h>
#include <ewoksys/utf8unicode.h>
#include <sconf/sconf.h>
#include <ewoksys/vfs.h>
#include <ewoksys/keydef.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <ttf/ttf.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/timer.h>
#include <x++/X.h>
#include <pthread.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
	font_t* font;
	uint32_t font_size;
	uint32_t font_fixed;
} conf_t;

typedef struct {
	uint16_t set;
	uint16_t state;
	uint32_t fg_color;
	uint32_t bg_color;
} term_conf_t;

class XTerm : public XWin {
	static const uint16_t ESC_CMD = 033;
	conf_t conf;
	terminal_t* terminal;
	int32_t rollStepRows;
	int32_t mouse_last_y;
	term_conf_t termConf;
	gpos_t  cursPos;
	bool show;

	void drawBG(graph_t* g, uint32_t w, uint32_t h) {
		graph_clear(g, conf.bg_color);
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

	gpos_t getPos(graph_t* g, uint32_t at, int32_t cw, int32_t ch) {
		uint32_t cx = 0, cy = 0;
		terminal_pos_by_at(terminal, at, &cx, &cy);
		gpos_t ret = { cx*cw, cy*ch };
		return ret;
	}

	void drawContent(graph_t* g, uint32_t w, uint32_t h) {
		uint32_t size = terminal_size(terminal);
		uint32_t i = 0;
		while(i < size) {
			tchar_t* c = terminal_get_by_at(terminal, i);
			if(c != NULL && c->c != 0 && c->c != '\n') {
				gpos_t pos = getPos(g, i, w, h);

				uint32_t fg = c->color, bg = c->bg_color;

				if((c->state & TERM_STATE_REVERSE) != 0) {
					fg = c->bg_color;
					if(fg == 0)
						fg = conf.bg_color;
					bg = c->color;
				}
				if(bg != 0) 
					graph_fill(g, pos.x, pos.y, w, h, bg);
				
				
				if((c->state & TERM_STATE_HIDE) == 0 && 
						((c->state & TERM_STATE_FLASH) == 0 || show)) {
					if((c->state & TERM_STATE_UNDERLINE) != 0)
						graph_fill(g, pos.x, pos.y+h-2, w, 2, fg);
					graph_draw_char_font_fixed(g, pos.x, pos.y, c->c, conf.font, fg, w, 0);
				}
			}
			i++;
		}
	}

	void drawCurs(graph_t* g, uint32_t w, uint32_t h) {
		if(!show)
			return;
		gpos_t pos = getPos(g, terminal_at(terminal), w, h);
		graph_fill(g, pos.x, pos.y+h-2, w, 2, conf.fg_color);
	}

	uint32_t gColor(uint32_t escColor, uint8_t fg) {
		if(fg != 0)
			escColor += 10;
		if(escColor < 40 || escColor > 47)
			return 0;

		uint32_t colors[8] = {
				0xff000000, //BLACK
				0xffff0000, //RED
				0xff00ff00, //GREEN
				0xffffff00, //YELLOW
				0xff0000ff, //BLUE
				0xffff0088, //PURPLE
				0xff00ffff, //CYAN
				0xffffffff  //WHITE
			};
		return colors[escColor-40];
	}

	void doEscColor(uint16_t* values, uint8_t vnum) {
		for(uint8_t i=0; i<vnum; i++) {
			uint16_t v = values[i];
			if(v == 0) {
				termConf.state = 0;
				termConf.bg_color = 0;
				termConf.fg_color = 0;
				termConf.set = 0;
			}
			else if(v == 4) {
				termConf.set = 1;
				termConf.state |= TERM_STATE_UNDERLINE;
			}
			else if(v == 5) {
				termConf.set = 1;
				termConf.state |= TERM_STATE_FLASH;
			}
			else if(v == 7) {
				termConf.set = 1;
				termConf.state |= TERM_STATE_REVERSE;
			}
			else if(v == 8) {
				termConf.set = 1;
				termConf.state |= TERM_STATE_HIDE;
			}
			else if(v >= 30 && v <= 39) {
				termConf.set = 1;
				termConf.fg_color = gColor(v, 1);
			}
			else if(v >= 40 && v <= 49) {
				termConf.set = 1;
				termConf.bg_color = gColor(v, 0);
			}
		}

		if(termConf.fg_color == 0)
			termConf.fg_color = conf.fg_color;
	}

	void doEscClear(uint16_t* values, uint8_t vnum) {
		if(values[0] == 2) {
			terminal_clear(terminal);
			terminal_move_to(terminal, 0, terminal->rows-1);
		}
	}

	void doEscXY(uint16_t* values, uint8_t vnum) {
		terminal_move_to(terminal, values[1], values[0]);
	}

	void runEscCmd(UNICODE16 cmd, uint16_t* values, uint8_t vnum) {
		if(cmd == 'm') { //color and state
			doEscColor(values, vnum);
		}
		else if(cmd == 'J') { //clear
			doEscClear(values, vnum);
		}
		else if(cmd == 'H') { //move curs y,x
			doEscXY(values, vnum);
		}
		else if(cmd == 'A') { //move curs up
			int16_t y = (int16_t)terminal->curs_y - values[0];
			if(y < 0)
				y = 0;
			terminal_move_to(terminal, terminal->curs_x, y);
		}
		else if(cmd == 'B') { //move curs down
			uint16_t y = terminal->curs_y + values[0];
			if(y >= terminal->rows)
				y = terminal->rows-1;
			terminal_move_to(terminal, terminal->curs_x, y);
		}
		else if(cmd == 'D') { //move curs left
			int16_t x = (int16_t)terminal->curs_x - values[0];
			if(x < 0)
				x = 0;
			terminal_move_to(terminal, x, terminal->curs_y);
		}
		else if(cmd == 'C') { //move curs right
			uint16_t x = terminal->curs_x + values[0];
			if(x >= terminal->cols)
				x = terminal->cols-1;
			terminal_move_to(terminal, x, terminal->curs_y);
		}
		else if(cmd == 's') { //save curs pos
			cursPos.x = terminal->curs_x;
			cursPos.y = terminal->curs_y;
		}
		else if(cmd == 'u') { //restore curs pos
			terminal_move_to(terminal, cursPos.x, cursPos.y);
		}
	}

	uint32_t doEscCmd(UNICODE16* uni, uint32_t from, uint32_t size) {
		UNICODE16 c = uni[from];
		if(c == 0)
			return from;

		from++;
		if(from >= size || c != '[')
			return from;

		uint16_t values[8];
		uint8_t vnum = 0;
		c = uni[from++];
		if(from > size || c == 0)
			return from;
		
		while(true) {
			if(c == '?') { //TODO hide/show curs
				continue;
			}
			else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
				runEscCmd(c, values, vnum);
				from--;
				break;
			}
			else if(c >= '0' && c <= '9') {
				char vstr[4];
				memset(vstr, 0, 4);
				vstr[0] = c;
				for(uint8_t i=1; i< 4; i++) {
					c = uni[from++];
					if(from > size || c == 0)
						return from;
					if(c < '0' || c > '9') {
						if(vnum < 7) {
							values[vnum] = atoi(vstr);
							vnum++;
						}
						break;
					}
					vstr[i] = c;
				}

				if(c == ';') {
					c = uni[from++];
					if(from >= size || c == 0)
						return from;
				}
			}
			else {
				from--;
				break;
			}
		}
		return from;
	}

public:
	XTerm() {
		terminal = (terminal_t*)malloc(sizeof(terminal_t));
		terminal_init(terminal);
		memset(&termConf, 0, sizeof(term_conf_t));
	}

	~XTerm() {
		terminal_close(terminal);
		free(terminal);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = strtoul(v, NULL, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = strtoul(v, NULL, 16);

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = strtoul(v, NULL, 16);

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = strtoul(v, NULL, 16);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);
		conf.font_size = font_size;
		
		v = sconf_get(sconf, "font_fixed");
		if(v[0] != 0) 
			conf.font_fixed = atoi(v);
		if(conf.font_fixed == 0)
			conf.font_fixed = font_size;

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = DEFAULT_SYSTEM_FONT;
		
		if(conf.font != NULL)
			font_free(conf.font);
		conf.font = font_new(v, font_size, true);

		sconf_free(sconf);
		return true;
	}

	void put(const char* buf, int size) {
		uint16_t* unicode = (uint16_t*)malloc((size+1)*2);
		size = utf82unicode((unsigned char*)buf, size, unicode);

		for(uint32_t i=0; i<size; i++) {
			UNICODE16 c = unicode[i];
			if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
				terminal_move(terminal, -1);
				terminal_set(terminal, 0, 0, 0, 0);
				continue;
			}
			else if(c == ESC_CMD) {
				i = doEscCmd(unicode, i+1, size);
				continue;
			}

			if(termConf.set == 0)
				terminal_set(terminal, c, 0, conf.fg_color, 0);
			else
				terminal_set(terminal, c, termConf.state, termConf.fg_color, termConf.bg_color);

			if(c == '\n') {
				if((terminal->curs_y+1) >= terminal->rows) {
					terminal_scroll(terminal, 1);
					terminal_move_to(terminal, 0, terminal->curs_y);
				}
				else
					terminal_move_next_line(terminal);
			}
			
			else {
				if((terminal_at(terminal)+1) >= terminal_size(terminal)) {//full
					terminal_scroll(terminal, 1);
					terminal_move_to(terminal, 0, terminal->curs_y);
				}
				else
					terminal_move(terminal, 1);
			}
		}
	}

	void flash() {
		show = !show;
		repaint();
	}

protected:
	void onFocus(void) {
		repaint();
		callXIM();
	}

	void onUnfocus(void) {
		repaint();
	}

	void onRepaint(graph_t* g) {
		uint32_t w = g->w / terminal->cols;
		uint32_t h = g->h / terminal->rows;
		drawBG(g, w, h);
		drawContent(g, w, h);
		drawCurs(g, w, h);
	}

	void onResize() {
		xinfo_t xinfo;
		getInfo(xinfo);
		uint32_t font_w = conf.font_fixed;
		uint32_t font_h = conf.font->max_size.y;
		terminal_reset(terminal, xinfo.wsr.w/font_w, xinfo.wsr.h/font_h);
	}
	
	void mouseHandle(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			return;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			mouse_last_y = ev->value.mouse.y;
			//console_roll(&console, mv);
			repaint();
		}
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c == KEY_ROLL_BACK) {
				//console_roll(&console, -(rollStepRows));
				repaint();
				return;
			}
			else if(c == KEY_ROLL_FORWARD) {
				//console_roll(&console, (rollStepRows));
				repaint();
				return;
			}

			if(c != 0) {
				write(1, &c, 1);
			}
		}
		else if(ev->type == XEVT_MOUSE) {
			mouseHandle(ev);
			return;	
		}
	}
};

static bool _termniated = false;
static bool _thread_done = false;
static void* thread_loop(void* p) {
	XTerm* xterm = (XTerm*)p;

	while(!_termniated) {
		char buf[512];
		int size = read(0, buf, 512);
		if(size > 0) {
			xterm->put(buf, size);
			xterm->repaint();
		}
		else if(errno != EAGAIN) {
			break;
		}
	}
	if(!_termniated)
		xterm->close();
	_thread_done = true;
	return NULL;
}

static XTerm* _xwin = NULL;
static void timer_handler(void) {
	_xwin->flash();
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	XTerm xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "xterm", "theme.conf"));


	X x;
	grect_t desk;
	x.getDesktopSpace(desk, 0);
	x.open(0, &xwin, desk.w*2/3, desk.h*2/3, "xterm", 0);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t timer_id = timer_set(500000, timer_handler);

	pthread_t tid;
	_termniated = false;
	_thread_done = false;
	pthread_create(&tid, NULL, thread_loop, &xwin);

	x.run(NULL, &xwin);
	_termniated = true;
	close(0);
	close(1);
	timer_remove(timer_id);
	while(!_thread_done)
		proc_usleep(2000);
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	int fds1[2];
	int fds2[2];
	pipe(fds1);
	pipe(fds2);

	int pid = fork();
	if(pid != 0) { //father proc for p2 reader.
		dup2(fds1[0], 0);
		dup2(fds2[1], 1);
		close(fds1[0]);
		close(fds1[1]);
		close(fds2[0]);
		close(fds2[1]);
		return run(argc, argv);
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	//setenv("CONSOLE", "xconsole", 1);
	//setenv("CONSOLE_ID", "console-x", 1);

	return execve("/bin/shell", NULL, NULL);
}
