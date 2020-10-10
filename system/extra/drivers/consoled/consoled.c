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
	int fb_fd;
	graph_t* g;
	fb_dma_t fb_dma;
	console_t console;
} fb_console_t;

static int init_console(fb_console_t* console) {
	memset(console, 0, sizeof(fb_console_t));

	int fd = open("/dev/fb0", O_WRONLY);
	if(fd < 0)
		return -1;

	graph_t* g = fb_graph(fd);
	if(g == NULL) {
		close(fd);
		return -1;
	}

	console->g = g;
	console->fb_fd = fd;
	console_init(&console->console);
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffcccccc;
	console->console.bg_color = 0xff000000;
	console_reset(&console->console, console->g->w, console->g->h);
	return 0;
}

static void close_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);
	fb_close_dma(&console->fb_dma);
	close(console->fb_fd);
}

static int reset_console(fb_console_t* console) {
	if(console->g != NULL)
		graph_free(console->g);
	console->g = fb_graph(console->fb_fd);

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

	fb_flush(_console.fb_fd, &_console.fb_dma, _console.g);
	return size;
}

static int console_step(void* p) {
	(void)p;
	int w, h;
	if(fb_size(_console.fb_fd, &w, &h) != 0)
		return -1;

	if(_console.g != NULL && w == _console.g->w && h == _console.g->h)
		return 0;
	if(fb_dma(_console.fb_fd, &_console.fb_dma, w, h) != 0)
		return -1;

	if(reset_console(&_console) == 0 && _console.g != NULL) {
		console_refresh(&_console.console, _console.g);
		fb_flush(_console.fb_fd, &_console.fb_dma, _console.g);
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
