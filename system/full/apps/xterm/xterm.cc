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

class XTerm : public XWin {
	conf_t conf;
	terminal_t* terminal;
	int32_t rollStepRows;
	int32_t mouse_last_y;

public:
	XTerm() {
		terminal = (terminal_t*)malloc(sizeof(terminal_t));
		terminal_init(terminal);
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
			conf.bg_color = strtol(v, NULL, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = strtol(v, NULL, 16);

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = strtol(v, NULL, 16);

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = strtol(v, NULL, 16);

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
		for(int i=0; i<size; i++) {
			terminal_set(terminal, unicode[i], conf.fg_color);
			if(unicode[i] == '\n')
				terminal_move_next_line(terminal);
			else
				terminal_move(terminal, 1);
		}
	}

	void rollEnd(void) {
	}

	void drawBG(graph_t* g) {
		graph_clear(g, conf.bg_color);
		uint32_t w = g->w / terminal->cols;
		uint32_t h = g->h / terminal->rows;
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

	void drawContent(graph_t* g) {
		uint32_t w = g->w / terminal->cols;
		uint32_t h = g->h / terminal->rows;
		uint32_t size = terminal_size(terminal);
		uint32_t i = 0;
		while(i < size) {
			tchar_t* c = terminal_get_by_at(terminal, i);
			if(c != NULL && c->c != 0 && c->c != '\n') {
				gpos_t pos = getPos(g, i, w, h);
				graph_draw_char_font_fixed(g, pos.x, pos.y, c->c, conf.font, c->color, w, 0);
			}
			i++;
		}
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
		drawBG(g);
		drawContent(g);
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
			xterm->rollEnd();
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

	pthread_t tid;
	_termniated = false;
	_thread_done = false;
	pthread_create(&tid, NULL, thread_loop, &xwin);

	x.run(NULL, &xwin);
	_termniated = true;
	close(0);
	close(1);
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
