#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <vprintf.h>
#include <sys/shm.h>
#include <sconf.h>
#include <fbinfo.h>
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

static void console_focus(x_t* x) {
	console_t* console = (console_t*)x->data;
	console->fg_color = _conf.fg_color;
	console->bg_color = _conf.bg_color;
	x_repaint(x);
}

static void console_unfocus(x_t* x) {
	console_t* console = (console_t*)x->data;
	console->fg_color = _conf.unfocus_fg_color;
	console->bg_color = _conf.unfocus_bg_color;
	x_repaint(x);
}

static void console_resize(x_t* x) {
	x_repaint(x);
}

static void repaint(x_t* x, graph_t* g) {
	console_t* console = (console_t*)x->data;
	if(console->w != g->w || console->h != g->h) {
		console_reset(console, g->w, g->h);
	}
	console_refresh(console, g);
}

static void event_handle(x_t* x, xevent_t* ev) {
	(void)x;
	if(ev->type == XEVT_KEYB) {
		int c = ev->value.keyboard.value;
		if(c != 0)
			write_nblock(1, &c, 1);
	}
}

static void loop(x_t* x) {
	console_t* console = (console_t*)x->data;
	char buf[256];
	int32_t size = read_nblock(0, buf, 255);
	if(size > 0) {
		buf[size] = 0;
		const char* p = (const char*)buf;
		for(int32_t i=0; i<size; i++) {
			char c = p[i];
			console_put_char(console, c);
		}
		x_repaint(x);
		return;
	}

	if(errno != EAGAIN) 
		x->closed = true;
}

static int run(int argc, char* argv[]) {
	(void)argc;
	(void)argv;
	memset(&_conf, 0, sizeof(conf_t));
	read_config(&_conf, "/etc/x/xconsole.conf");

	x_t* xp = NULL;
	if(_conf.w > 0 && _conf.h > 0) {
		xp  = x_open(0, 0, _conf.w, _conf.h, "xconsole", 0);
	}
	else {
		xscreen_t scr;
 		x_screen_info(&scr);
		xp = x_open(0, 0, scr.size.w*3/4, scr.size.h*3/4, "xconsole", 0);
	}

	if(xp == NULL) {
		return -1;
	}

	console_t console;
	console_init(&console);
	console.font = _conf.font;
	console.fg_color = _conf.fg_color;
	console.bg_color = _conf.bg_color;

	xp->data = &console;
	xp->on_resize = console_resize;
	xp->on_focus = console_focus;
	xp->on_unfocus = console_unfocus;
	xp->on_repaint = repaint;
	xp->on_loop = loop;
	xp->on_event = event_handle;

	x_set_visible(xp, true);

	x_run(xp);

	console_close(&console);
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
