extern "C" {
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <sconf.h>
#include <sys/vfs.h>
}
#include <x++/X.h>

typedef struct {
	font_t* font;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
} conf_t;

class XConsole : public XWin {
	conf_t conf;
	console_t console;
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
			conf.bg_color = argb_int(atoi_base(v, 16));

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = argb_int(atoi_base(v, 16));

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = argb_int(atoi_base(v, 16));

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = argb_int(atoi_base(v, 16));

		v = sconf_get(sconf, "font");
		if(v[0] != 0) 
			conf.font = font_by_name(v);

		sconf_free(sconf);

		console.font = conf.font;
		console.fg_color = conf.fg_color;
		console.bg_color = conf.bg_color;
		return true;
	}

	void putChar(char c) {
		console_put_char(&console, c);
	}

protected:
	void onFocus(void) {
		console.fg_color = conf.fg_color;
		console.bg_color = conf.bg_color;
		repaint();
		callXIM();
	}

	void onUnfocus(void) {
		console.fg_color = conf.unfocus_fg_color;
		console.bg_color = conf.unfocus_bg_color;
		repaint();
	}

	void onRepaint(Graph& g) {
		if(console.w != g.getW() || console.h != g.getH()) {
			console_reset(&console, g.getW(), g.getH());
		}
		console_refresh(&console, g.cgraph());
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM) {
			int c = ev->value.im.value;
			if(c != 0)
				vfs_write_nblock(1, &c, 1);
		}
	}
};

static void loop(void* p) {
	XConsole* console = (XConsole*)p;

	char buf[256];
	int32_t size = vfs_read_nblock(0, buf, 255);
	if(size > 0) {
		buf[size] = 0;
		for(int32_t i=0; i<size; i++) {
			char c = buf[i];
			console->putChar(c);
		}
		console->repaint();
		return;
	}

	if(errno != EAGAIN) 
		console->close();
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	XConsole xwin;
	xwin.readConfig("/etc/x/xconsole.conf");

	X x;
	xscreen_t scr;
 	x.screenInfo(scr);
	x.open(&xwin, 10, 40, scr.size.w*3/4, scr.size.h*3/4, "xconsole", 0);
	xwin.setVisible(true);

	x.run(loop, &xwin);
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
