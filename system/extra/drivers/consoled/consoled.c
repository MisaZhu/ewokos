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

typedef struct {
	const char* id;
	fb_t fb;
	graph_t* g;
	console_t console;
} fb_console_t;

static int init_console(fb_console_t* console) {
	memset(console, 0, sizeof(fb_console_t));

	if(fb_open("/dev/fb0", &console->fb) != 0)
		return -1;

	console_init(&console->console);
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffcccccc;
	console->console.bg_color = 0xff000000;
	return 0;
}

static void close_console(fb_console_t* console) {
	fb_close(&console->fb);
}

static int reset_console(fb_console_t* console) {
	console->g = fb_fetch_graph(&console->fb);
	if(console->g == NULL)
		return -1;
	console_reset(&console->console, console->g->w, console->g->h);
	return 0;
}

static fb_console_t _console;

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
	(void)p;

	if(size <= 0 || _console.g == NULL)
		return 0;

	console_refresh(&_console.console, _console.g);
	const char* pb = (const char*)buf;
	int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&_console.console, c);
	}

	fb_flush(&_console.fb);
	return size;
}

static int console_step(void* p) {
	(void)p;
	int w, h, bpp;
	if(fb_info(&_console.fb, &w, &h, &bpp) != 0)
		return -1;

	if(_console.g != NULL && w == _console.g->w && h == _console.g->h)
		return 0;

	if(reset_console(&_console) == 0 && _console.g != NULL) {
		console_refresh(&_console.console, _console.g);
		fb_flush(&_console.fb);
	}
	return 0;
}
	
int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/console0";

	init_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "console");
	dev.write = console_write;
	dev.loop_step = console_step;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	close_console(&_console);
	return 0;
}
