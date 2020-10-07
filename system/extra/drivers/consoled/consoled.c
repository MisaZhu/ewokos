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
	console_t console;
} fb_console_t;

static void init_console(fb_console_t* console) {
	if(fb_open("/dev/fb0", &console->fb) != 0)
		return;

	console_init(&console->console);
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffcccccc;
	console->console.bg_color = 0xff000000;
	console_reset(&console->console, console->fb.g->w, console->fb.g->h);
}

static void close_console(fb_console_t* console) {
	fb_close(&console->fb);
}

static void reset_console(fb_console_t* console) {
	if(fb_fetch_graph(&console->fb) == NULL)
		return;
	console_reset(&console->console, console->fb.g->w, console->fb.g->h);
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

	if(size <= 0 || _console.fb.g == NULL)
		return 0;

	const char* pb = (const char*)buf;
	int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&_console.console, c);
	}

	reset_console(&_console);
	console_refresh(&_console.console, _console.fb.g);
	fb_flush(&_console.fb);
	return size;
}

static int console_step(void* p) {
	(void)p;
	int w, h;
	if(fb_size(&_console.fb, &w, &h) != 0)
		return -1;

	if(w == _console.fb.g->w && h == _console.fb.g->h)
		return 0;

	reset_console(&_console);
	if(_console.fb.g != NULL) {
		console_refresh(&_console.console, _console.fb.g);
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
