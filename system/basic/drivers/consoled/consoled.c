#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <console/console.h>
#include <fb/fb.h>
#include <fonts/fonts.h>
#include <sys/shm.h>
#include <sys/vdevice.h>
#include <display/display.h>
#include <upng/upng.h>
#include <sys/syscall.h>
#include <sysinfo.h>
#include <sconf/sconf.h>

typedef struct {
	const char* id;
	const char* display_dev;
	uint32_t    display_index;
	fb_t        fb;
	graph_t*    g;
	console_t   console;
	graph_t*    icon;
} fb_console_t;

static int32_t read_config(fb_console_t* console, const char* fname) {
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffcccccc;
	console->console.bg_color = 0xff000000;
	const char* icon_fn = "/data/icons/starwars/yoda.png";

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL) {
		console->icon = png_image_new(icon_fn);
		return -1;
	}

	const char* v = sconf_get(conf, "font");
	if(v[0] != 0) 
		console->console.font = font_by_name(v);

	v = sconf_get(conf, "icon");
	if(v[0] != 0) 
		console->icon = png_image_new(v);
	else
		console->icon = png_image_new(icon_fn);

	v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		console->console.bg_color = atoi_base(v, 16);

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		console->console.fg_color = atoi_base(v, 16);

	sconf_free(conf);
	return 0;
}

static int init_console(fb_console_t* console, const char* display_dev, const uint32_t display_index) {
	memset(console, 0, sizeof(fb_console_t));
	console->display_dev = display_dev;
	console->display_index = display_index;
	const char* fb_dev = get_display_fb_dev(display_dev, console->display_index);
	if(fb_open(fb_dev, &console->fb) != 0)
		return -1;

	console_init(&console->console);
	read_config(console, "/etc/console.conf");
	return 0;
}

static void close_console(fb_console_t* console) {
	fb_close(&console->fb);
	if(console->icon != NULL)
		graph_free(console->icon);
}

static int reset_console(fb_console_t* console) {
	console->g = fb_fetch_graph(&console->fb);
	if(console->g == NULL)
		return -1;
	console_reset(&console->console, console->g->w, console->g->h);
	return 0;
}

static void flush(fb_console_t* console) {
	console_refresh(&console->console, console->g);
	if(console->display_index == 0 && console->icon != NULL) {
		sys_info_t sys_info;
		syscall1(SYS_GET_SYS_INFO, (int32_t)&sys_info);
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
	fb_flush(&console->fb);
}

static int console_write(int fd, 
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		int offset,
		void* p) {
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)offset;

	fb_console_t* console = (fb_console_t*)p;
	if(size <= 0 || console->g == NULL)
		return 0;

	const char* pb = (const char*)buf;
	int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&console->console, c);
	}
	flush(console);
	return size;
}

static int console_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)in;
	(void)ret;
	fb_console_t* console = (fb_console_t*)p;

	if(cmd == DEV_CNTL_REFRESH) {
		flush(console);
	}
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/console0";
	const char* display_dev = argc > 2 ? argv[2]: "/dev/displayman";
	const uint32_t display_index = argc > 3 ? atoi(argv[3]): 0;

	fb_console_t _console;
	init_console(&_console, display_dev, display_index);
	reset_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "console");
	dev.write = console_write;
	dev.dev_cntl = console_dev_cntl;
	dev.extra_data = &_console;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	close_console(&_console);
	return 0;
}
