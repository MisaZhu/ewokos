#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <sys/shm.h>
#include <sconf.h>
#include <dev/fbinfo.h>
#include <x/xclient.h>

typedef struct {
	font_t* font;
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
	uint32_t w;
	uint32_t h;
} conf_t;

static conf_t _conf;

static int32_t read_config(conf_t* conf, const char* fname) {
	conf->w = 800;
	conf->h = 600;

	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		conf->bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		conf->fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(sconf, "unfocus_fg_color");
	if(v[0] != 0) 
		conf->unfocus_fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(sconf, "unfocus_bg_color");
	if(v[0] != 0) 
		conf->unfocus_bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
		conf->font = font_by_name(v);

	v = sconf_get(sconf, "width");
	if(v[0] != 0) 
		conf->w = atoi(v);

	v = sconf_get(sconf, "height");
	if(v[0] != 0) 
		conf->h = atoi(v);

	sconf_free(sconf);
	return 0;
}

static void console_resize(x_t* x, void* p) {
	console_t* console = (console_t*)p;
	console->g = x_get_graph(x);
	console_reset(console);
}

static void console_focus(x_t* x, void* p) {
	(void)x;
	console_t* console = (console_t*)p;
	console->fg_color = _conf.fg_color;
	console->bg_color = _conf.bg_color;
	console_refresh(console);
	x_update(x);
}

static void console_unfocus(x_t* x, void* p) {
	(void)x;
	console_t* console = (console_t*)p;
	console->fg_color = _conf.unfocus_fg_color;
	console->bg_color = _conf.unfocus_bg_color;
	console_refresh(console);
	x_update(x);
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	memset(&_conf, 0, sizeof(conf_t));
	read_config(&_conf, "/etc/x/xconsole.conf");

	x_t* xp = x_open(0, 0, _conf.w, _conf.h, "xconsole", 0);
	if(xp == NULL) {
		return -1;
	}

	graph_t* g = x_get_graph(xp);
	console_t console;
	console_init(&console);
	console.g = g;

	console.font = _conf.font;
	console.fg_color = _conf.fg_color;
	console.bg_color = _conf.bg_color;
	console_reset(&console);

	xp->data = &console;
	xp->on_resize = console_resize;
	xp->on_focus = console_focus;
	xp->on_unfocus = console_unfocus;

	x_set_visible(xp, true);

	int krd = 0;
	xevent_t xev;
	while(xp->closed == 0) {
		if(krd != 1) {
			if(x_get_event(xp, &xev) == 0) {
				if(xev.type == XEVT_KEYB)
					krd = 1;
			}
		}
		else {
			char c = xev.value.keyboard.value;
			if(write_nblock(1, &c, 1) == 1)
				krd = 0;
		}

		char buf[256];
		int32_t size = read_nblock(0, buf, 255);
		if(size == 0) {
			break;
		}
		else if(size < 0) {
			if(errno == EAGAIN) {
				sleep(0);
				continue;
			}
			else 
				break;
		}

		buf[size] = 0;
		const char* p = (const char*)buf;
		for(int32_t i=0; i<size; i++) {
			char c = p[i];
			console_put_char(&console, c);
		}
		x_update(xp);
	}

	console_close(&console);
	x_release_graph(xp, g);
	x_close(xp);
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
