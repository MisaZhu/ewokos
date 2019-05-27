#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <kstring.h>
#include <sconf.h>
#include <x/xclient.h>

typedef struct {
	uint32_t fg_color, bg_color;
	font_t* font;
	graph_t* g;
} launcher_t;

static int32_t read_config(launcher_t* launcher, const char* fname, int32_t *w, int32_t *h) {
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;
	
	const char* v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		launcher->bg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		launcher->fg_color = argb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		launcher->font = get_font_by_name(v);

	v = sconf_get(conf, "width");
	if(v[0] != 0) 
		*w = atoi_base(v, 10);

	v = sconf_get(conf, "height");
	if(v[0] != 0) 
		*h = atoi_base(v, 10);
	sconf_free(conf, MFS);
	return 0;
}

static void run(const char* cmd) {
	int pid = fork();
	if(pid == 0) { //child proc
		exec(cmd);
	}
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	gsize_t scr_size;
	int style = X_STYLE_NO_FRAME | X_STYLE_ALPHA;
	x_t x;
	launcher_t launcher;
	launcher.fg_color = 0xff111111;
	launcher.bg_color = 0xff888888;
	launcher.font = get_font_by_name("8x16");
	x_get_scr_size("/dev/X0", &scr_size);
	int32_t w = scr_size.w, h = 32;

	read_config(&launcher, "/etc/x/xlauncher.conf", &w, &h);

	if(x_open("/dev/X0", &x) != 0)
		return -1;

	x_move_to(&x, 0, scr_size.h-h);
	x_resize_to(&x, w, h);
	launcher.g = x_get_graph(&x);
	x_set_style(&x, style);
	x_set_state(&x, X_STATE_NORMAL);
	clear(launcher.g, launcher.bg_color);
	fill(launcher.g, 4, 4, h-8, h-8, launcher.fg_color);

	while(true) {
		x_ev_t ev;
		if(x_get_event(&x, &ev) == 0) {
			if(ev.type == X_EV_MOUSE) {
				if(ev.state == X_EV_MOUSE_DOWN) {
					run("/bin/x/xconsole");
				}
			}
			//x_flush(&x);
		}
		else {
			sleep(0);
		}
	}
	graph_free(launcher.g);
	x_close(&x);
	return 0;
}
