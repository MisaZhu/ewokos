#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <console/console.h>
#include <fb/fb.h>
#include <ttf/ttf.h>
#include <sys/shm.h>
#include <sys/vdevice.h>
#include <display/display.h>
#include <upng/upng.h>
#include <sys/syscall.h>
#include <sysinfo.h>
#include <sconf/sconf.h>
#include <sys/klog.h>

typedef struct {
	const char* id;
	const char* display_dev;
	uint32_t    display_index;
	fb_t        fb;
	graph_t*    g;
	console_t   console;
	graph_t*    icon;
	//graph_t*    bg_image;
} fb_console_t;

static int32_t read_config(fb_console_t* console, const char* fname) {
	const char* font_fname =  "/usr/system/fonts/system.ttf";
	uint32_t font_size = 16;
	console->console.textview.fg_color = 0xffcccccc;
	console->console.textview.bg_color = 0xff000000;
	//const char* icon_fn = "/data/icons/starwars/ewok.png";
	//const char* bg_fn = "/data/images/ewok.png";

	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL) {
		return -1;
	}

	const char* v = sconf_get(conf, "icon");
	if(v[0] != 0) 
		console->icon = png_image_new(v);
	//else
		//console->icon = png_image_new(icon_fn);

	/*v = sconf_get(conf, "bg_image");
	if(v[0] != 0) 
		console->bg_image = png_image_new(v);
	else
		console->bg_image = png_image_new(bg_fn);
		*/

	v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		console->console.textview.bg_color = atoi_base(v, 16);

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		console->console.textview.fg_color = atoi_base(v, 16);
	
	v = sconf_get(conf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);
	console->console.textview.font_size = font_size;

	v = sconf_get(conf, "font_fixed");
	if(v[0] != 0) 
		console->console.textview.font_fixed = atoi(v);

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		font_fname = v;
	font_load(font_fname, font_size, &console->console.textview.font);
	sconf_free(conf);

	return 0;
}

static void init_graph(fb_console_t* console) {
	console->g = fb_fetch_graph(&console->fb);
	graph_clear(console->g, 0xff000000);
	/*
	int w = 32, h = 32;
	int x = (console->g->w - w*2) / 2;
	int y = (console->g->h - h*2) / 2;

	graph_fill_round(console->g, x, y, w-2, h-2, 6, 0xffff0000);
	graph_fill_round(console->g, x+w, y, w-2, h-2, 6, 0xff00ff00);
	graph_fill_round(console->g, x, y+h, w-2, h-2, 6, 0xff0000ff);
	graph_fill_round(console->g, x+w, y+h, w-2, h-2, 6, 0xffffffff);

	fb_flush(&console->fb, true);
	*/
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
	console_init(&console->console);
	read_config(console, "/etc/console.conf");
	return 0;
}

static void close_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);

	fb_close(&console->fb);
	if(console->icon != NULL)
		graph_free(console->icon);
	if(console->console.textview.font.id >= 0)
		font_close(&console->console.textview.font);
	//if(console->bg_image != NULL)
		//graph_free(console->bg_image);
}

static int reset_console(fb_console_t* console) {
	console_reset(&console->console, console->g->w, console->g->h);
	return 0;
}

static void flush(fb_console_t* console) {
	graph_clear(console->g, console->console.textview.bg_color);
	if(console->display_index == 0) {
		//draw bgImage
		/*if(console->bg_image != NULL) {
			graph_blt_alpha(console->bg_image, 
					0, 0, console->bg_image->w, console->bg_image->w,
					console->g,
					(console->g->w - console->bg_image->w) / 2,
					(console->g->h - console->bg_image->h) / 2,
					console->bg_image->w, console->bg_image->h, 
					0xff);
		}
		*/
		//draw cores
		if(console->icon != NULL) {
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
	}

	console_refresh_content(&console->console, console->g);
	fb_flush(&console->fb, true);
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
	/*int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&console->console, c);
	}
	*/
	console_put_string(&console->console, pb, size);
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
	const char* display_dev = argc > 2 ? argv[2]: "/dev/display";
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
