#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <gterminal/gterminal.h>
#include <fb/fb.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <display/display.h>
#include <graph/graph_png.h>
#include <tinyjson/tinyjson.h>
#include <ewoksys/klog.h>
#include <ewoksys/syscall.h>
#include <ewoksys/core.h>
#include <ewoksys/charbuf.h>
#include <sysinfo.h>
#include <font/font.h>
#include <keyb/keyb.h>

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
	console->terminal.char_space = json_get_int_def(conf_var, "char_space", 0);;
	console->terminal.line_space = json_get_int_def(conf_var, "line_space", 0);;

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
	if(display_fb_open(display_dev, console->display_index, &console->fb) != 0)
		return -1;
	init_graph(console);
	gterminal_init(&console->terminal);
	read_config(console, "/etc/console.json");
	return 0;
}

static void close_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);

	fb_close(&console->fb);
	if(console->icon != NULL)
		graph_free(console->icon);
	gterminal_close(&console->terminal);
	font_quit();
}

static int reset_console(fb_console_t* console) {
	gterminal_resize(&console->terminal, console->g->w, console->g->h);
	return 0;
}

static void draw_bg(fb_console_t* console) {
	graph_t* g = console->g;
	graph_clear(g, console->terminal.bg_color);
}

static void flush(fb_console_t* console) {
	draw_bg(console);

	if(console->display_index == 0) {
		//draw cores
		if(console->icon != NULL) {
			sys_info_t sys_info;
			syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)&sys_info);
			if(sys_info.cores > 1) {
				for(uint32_t i=0; i<sys_info.cores; i++) {
					graph_blt_alpha(console->icon, 
							0, 0, console->icon->w, console->icon->w,
							console->g,
							console->g->w - console->icon->w * (i+1),
							console->g->h - console->icon->h,
							console->icon->w, console->icon->h, 
							0xff);
				}	
			}
		}
	}

	gterminal_paint(&console->terminal, console->g, 0, 0, console->g->w, console->g->h);
	fb_flush(&console->fb, true);
}

static bool _flush = true;
static int _ux_index = 0;
static int _disp_index = 0;
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
	//if(_ux_index == core_get_active_ux(console->display_index))
		//flush(console);
	return size;
}

static int _keyb_fd = -1;
static const char* _keyb_dev = "";
static charbuf_t *_buffer;

static int console_read(int fd,
		int from_pid,
		fsinfo_t* node,
		void* buf,
		int size,
		int offset,
		void* p) {

	char c;
	int res = charbuf_pop(_buffer, &c);

	if(res != 0)
		return VFS_ERR_RETRY;
	((char*)buf)[0] = c;
	return 1;
}

static int console_loop(void* p) {
	fb_console_t* console = (fb_console_t*)p;
	if(_ux_index != core_get_active_ux(console->display_index)) {
		usleep(200000);
		_flush = true;
		return 0;
	}

	ipc_disable();
	if(_flush) {
		flush(console);
		_flush = false;
	}

	if(_keyb_fd < 0) {
		if(_keyb_dev[0] != 0)
			_keyb_fd = open(_keyb_dev, O_RDONLY | O_NONBLOCK);
	}

	if(_keyb_fd > 0) {
		keyb_evt_t evts[KEYB_EVT_MAX];
		int n = keyb_read(_keyb_fd, evts, KEYB_EVT_MAX);
		for(int i=0; i<n; i++) {
			uint8_t c = evts[i].key;
			if(evts[i].state != KEYB_STATE_PRESS || c >= 128)
				continue;

			if(c == KEY_UP) {
				gterminal_scroll(&console->terminal, -1);
				_flush = true;	
			}
			else if(c == KEY_DOWN) {
				gterminal_scroll(&console->terminal, 1);
				_flush = true;	
			}
			else if(c == KEY_LEFT) {
				if(console->terminal.font_size > 5)
					console->terminal.font_size--;
				gterminal_resize(&console->terminal, console->g->w, console->g->h);
				_flush = true;	
			}
			else if(c == KEY_RIGHT) {
				if(console->terminal.font_size < 99)
					console->terminal.font_size++;
				gterminal_resize(&console->terminal, console->g->w, console->g->h);
				_flush = true;	
			}
			else {
				gterminal_scroll(&console->terminal, 0);
				charbuf_push(_buffer, c, true);
			}
		}
		if(n > 0)
			proc_wakeup(RW_BLOCK_EVT);
	}

	ipc_enable();
	usleep(30000);
	return 0;
}

static const char* _mnt_point = "";
static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "i:d:u:");
		if(c == -1)
			break;

		switch (c) {
		case 'i':
			_keyb_dev = optarg;
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
	_disp_index = 0;
	_buffer = charbuf_new(0);
	_ux_index = core_get_ux_env();
	int argind = doargs(argc, argv);
	core_enable_ux(_disp_index, _ux_index);

	char mnt_point[128] = {0};
	if(argind < argc)
		strncpy(mnt_point, argv[argind], 127);
	else
		snprintf(mnt_point, 127, "/dev/console%d", _ux_index);

	_keyb_fd = -1;
	const char* display_dev = "/dev/display";

	fb_console_t _console;
	init_console(&_console, display_dev, _disp_index);
	reset_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "console");
	dev.write = console_write;
	dev.read = console_read;
	dev.extra_data = &_console;
	dev.loop_step = console_loop;

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666);
	close_console(&_console);
	charbuf_free(_buffer);
	return 0;
}
