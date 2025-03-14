#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <gterminal/gterminal.h>
#include <fb/fb.h>
#include <ewoksys/vdevice.h>
#include <display/display.h>
#include <upng/upng.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/klog.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/charbuf.h>
#include <sysinfo.h>
#include <font/font.h>

typedef struct {
	const char* id;
	const char* display_dev;
	uint32_t    display_index;
	fb_t        fb;
	graph_t*    g;
	gterminal_t terminal;
	graph_t*    icon;
} fb_console_t;

static int32_t read_config(fb_console_t* console, const char* fname) {
	json_var_t *conf_var = json_parse_file(fname);	

	console->terminal.fg_color = 0xffcccccc;
	console->terminal.bg_color = 0xff000000;

	const char* font_fname = json_get_str_def(conf_var, "font", DEFAULT_SYSTEM_FONT);
	if(console->terminal.font != NULL)
		font_free(console->terminal.font);
	console->terminal.font = font_new(font_fname, true);

	const char* v = json_get_str_def(conf_var, "icon", "");
	if(v[0] != 0) 
		console->icon = png_image_new(v);

	console->terminal.bg_color = json_get_int_def(conf_var, "bg_color", 0xff000000);
	console->terminal.fg_color = json_get_int_def(conf_var, "fg_color", 0xffcccccc);
	console->terminal.font_size = json_get_int_def(conf_var, "font_size", 12);;
	console->terminal.font_fixed = json_get_int_def(conf_var, "font_fixed", 12);;

	if(conf_var != NULL)
		json_var_unref(conf_var);
	return 0;
}

static void init_graph(fb_console_t* console) {
	console->g = fb_fetch_graph(&console->fb);
	graph_clear(console->g, 0xff000000);
}

static int init_console(fb_console_t* console, const char* display_dev, const uint32_t display_index) {
	memset(console, 0, sizeof(fb_console_t));
	font_init();

	console->display_dev = display_dev;
	console->display_index = display_index;
	const char* fb_dev = get_display_fb_dev(display_dev, console->display_index);
	if(fb_open(fb_dev, &console->fb) != 0)
		return -1;
	init_graph(console);
	gterminal_init(&console->terminal);
	read_config(console, "/etc/init_console.json");
	return 0;
}

static void close_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);

	fb_close(&console->fb);
	if(console->icon != NULL)
		graph_free(console->icon);
	gterminal_close(&console->terminal);
}

static uint32_t _t_lines = 4;

static int reset_console(fb_console_t* console) {
	gterminal_resize(&console->terminal, console->g->w, console->terminal.font_size*_t_lines);
	return 0;
}

static void draw_bg(fb_console_t* console) {
	graph_t* g = console->g;
	graph_clear(g, console->terminal.bg_color);
}

static void flush(fb_console_t* console) {
	draw_bg(console);

	graph_t tg;
	graph_init(&tg, NULL, console->g->w, console->terminal.font_size * _t_lines);
	graph_clear(&tg, 0x0);
	gterminal_paint(&console->terminal, &tg);

	//draw cores
	if(console->icon != NULL) {
		graph_blt_alpha(console->icon, 
				0, 0, console->icon->w, console->icon->h,
				console->g,
				(console->g->w - console->icon->w) / 2,
				(console->g->h - console->icon->h - tg.h) / 2,
				console->icon->w, console->icon->h, 
				0xff);
	}

	graph_blt_alpha(&tg, 0, 0, tg.w, tg.h,
			console->g, 0, console->g->h - tg.h, tg.w, tg.h, 0xff);

	fb_flush(&console->fb, true);
}

static bool _flush = true;
static int _ux_index = 0;
static int console_write(int fd, 
		int from_pid,
		fsinfo_t* node,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)fd;
	(void)from_pid;
	(void)node;
	(void)offset;

	_flush = true;
	fb_console_t* console = (fb_console_t*)p;
	if(size <= 0 || console->g == NULL)
		return 0;

	const char* pb = (const char*)buf;
	gterminal_put(&console->terminal, pb, size);

	//if(_ux_index == core_get_active_ux())
		//flush(console);
	return size;
}

static int console_loop(void* p) {
	if(_ux_index != core_get_active_ux()) {
		usleep(200000);
		_flush = true;
		return 0;
	}

	ipc_disable();
	if(_flush) {
		flush((fb_console_t*)p);
		_flush = false;
	}
	ipc_enable();
	usleep(100000);
	return 0;
}

static const char* _mnt_point = "";
static int _disp_index = 0;
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "l:d:u:");
		if(c == -1)
			break;

		switch (c) {
		case 'l':
			_t_lines = atoi(optarg);
			break;
		case 'd':
			_disp_index = atoi(optarg);
			break;
		case 'u':
			_ux_index = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_t_lines = 4;
	_ux_index = core_get_ux();
	int argind = doargs(argc, argv);
	core_set_ux(_ux_index);
	if(_ux_index == 0)
		core_set_active_ux(0);

	char mnt_point[128] = {0};
	if(argind < argc)
		strncpy(mnt_point, argv[argind], 127);
	else
		snprintf(mnt_point, 127, "/dev/console%d", _ux_index);

	const char* display_dev = "/dev/display";

	fb_console_t _console;
	init_console(&_console, display_dev, _disp_index);
	reset_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "init_console");
	dev.write = console_write;
	dev.extra_data = &_console;
	dev.loop_step = console_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	close_console(&_console);
	return 0;
}
