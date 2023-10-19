#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <sconf/sconf.h>
#include <sys/vfs.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <ttf/ttf.h>
#include <sys/basic_math.h>
#include <x++/X.h>
#include <pthread.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
} conf_t;

class XConsole : public XWin {
	conf_t conf;
	console_t console;
	int32_t rollStepRows;
	int32_t mouse_last_y;
public:
	XConsole() {
		console_init(&console);
	}

	~XConsole() {
		console_close(&console);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL)
			return false;

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = atoi_base(v, 16);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);
		console.textview.font_size = font_size;
		
		v = sconf_get(sconf, "font_fixed");
		if(v[0] != 0) 
			console.textview.font_fixed = atoi(v);

		v = sconf_get(sconf, "font_shadow");
		if(v[0] != 0) {
			console.textview.shadow = true;
			console.textview.shadow_color = atoi_base(v, 16);
		}

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = DEFAULT_SYSTEM_FONT;
		
		font_load(v, font_size, &console.textview.font, true);

		sconf_free(sconf);

		console.textview.fg_color = conf.fg_color;
		console.textview.bg_color = conf.bg_color;
		return true;
	}

	void put(const char* buf, int size) {
		console_put_string(&console, buf, size);
	}

	void rollEnd(void) {
		console_roll(&console, 0x0fffffff); //roll forward to the last row
	}

protected:
	void onFocus(void) {
		console.textview.fg_color = conf.fg_color;
		console.textview.bg_color = conf.bg_color;
		console.show_cursor = true;
		repaint();
		callXIM();
	}

	void onUnfocus(void) {
		console.textview.fg_color = conf.unfocus_fg_color;
		console.textview.bg_color = conf.unfocus_bg_color;
		console.show_cursor = false;
		repaint();
	}

	void onRepaint(graph_t* g) {
		if(console.textview.w != g->w || console.textview.h != g->h) {
			rollStepRows = (g->h / console.textview.font.max_size.y) / 2;
			console_reset(&console, g->w, g->h);
		}
		console_refresh(&console, g);
		if(color_a(console.textview.bg_color) != 0xff)
			setAlpha(true);
		else
			setAlpha(false);
	}
	
	void mouseHandle(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			return;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int mv = (mouse_last_y - ev->value.mouse.y) / console.textview.font.max_size.y;
			if(abs_32(mv) > 0) {
				mouse_last_y = ev->value.mouse.y;
				//drag release
				console_roll(&console, mv);
				repaint();
			}
		}
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c == KEY_ROLL_BACK) {
				console_roll(&console, -(rollStepRows));
				repaint();
				return;
			}
			else if(c == KEY_ROLL_FORWARD) {
				console_roll(&console, (rollStepRows));
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
	XConsole* console = (XConsole*)p;

	while(!_termniated) {
		char buf[512];
		int size = read(0, buf, 512);
		if(_termniated)
			break;
		if(size > 0) {
			console->put(buf, size);
			console->rollEnd();
			console->repaint();
		}
		else if(errno != EAGAIN) {
			console->close();
			break;
		}
	}
	_thread_done = true;
	return NULL;
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	XConsole xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "xconsole", "theme.conf"));


	X x;
	xscreen_t scr;
 	x.screenInfo(scr, 0);
	x.open(&xwin, 64, 40, scr.size.w*3/4, scr.size.h*3/4, "xconsole", 0);
	xwin.setVisible(true);

	pthread_t tid;
	_termniated = false;
	_thread_done = false;
	pthread_create(&tid, NULL, thread_loop, &xwin);

	x.run(NULL, &xwin);
	_termniated = true;
	close(0);
	while(!_thread_done)
		usleep(2000);
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
	setenv("CONSOLE", "xconsole");
	setenv("CONSOLE_ID", "console-x");

	return exec("/bin/shell");
}
