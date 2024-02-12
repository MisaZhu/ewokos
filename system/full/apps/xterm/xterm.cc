#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/errno.h>

#ifdef __cplusplus
}
#endif

#include <gterminal/gterminal.h>
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


class XTerm : public XWin {
	gterminal_t terminal;
	int32_t rollStepRows;
	int32_t mouse_last_y;

	void drawBG(graph_t* g) {
		graph_clear(g, terminal.bg_color);
		if(color_a(terminal.bg_color) != 0xff)
			setAlpha(true);

		/*uint32_t cw = g->w / terminal.terminal.cols;
		uint32_t ch = g->h / terminal.terminal.rows;
		uint32_t i = 0;
		while(i < g->w) {
			i += cw;
			graph_line(g, i, 0, i, g->h, 0xff222222);
		}

		i = 0;
		while(i < g->h) {
			i += ch;
			graph_line(g, 0, i, g->w, i, 0xff222222);
		}
		*/
	}

	bool readConfigRaw(const char* fname) {
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;
		theme.loadConfig(sconf);
		sconf_free(sconf);
		return true;
	}
public:
	XTerm() {
		gterminal_init(&terminal);
	}

	~XTerm() {
		gterminal_close(&terminal);
	}

	bool readConfig(const char* fname) {
		readConfigRaw(fname);
		terminal.bg_color = theme.basic.bgColor;
		terminal.fg_color = theme.basic.fgColor;
		terminal.font_size = theme.basic.fontSize;
		terminal.font_fixed = theme.basic.fontFixedSize;
		if(terminal.font != NULL)
			font_free(terminal.font);
		terminal.font = font_new(theme.basic.fontName, terminal.font_size, true);
		return true;
	}

	void put(const char* buf, int size) {
		gterminal_put(&terminal, buf, size);
	}

	void flash() {
		gterminal_flash(&terminal);
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
		drawBG(g);
		gterminal_paint(&terminal, g);
	}

	void onResize() {
		xinfo_t xinfo;
		getInfo(xinfo);
		gterminal_resize(&terminal, xinfo.wsr.w, xinfo.wsr.h);
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

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

int main(int argc, char* argv[]) {
	const char* cmd = "/bin/shell";
	if(argc > 1)
		cmd = argv[1];

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
	dup2(fds1[1], 2);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);

	char console[16];
	snprintf(console, 15, "xterm-%d", getpid());
	setenv("CONSOLE_ID", console);

	return proc_exec(cmd);
}