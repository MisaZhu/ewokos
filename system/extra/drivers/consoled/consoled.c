#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <console/console.h>
#include <graph/graph.h>
#include <fonts/fonts.h>
#include <sys/shm.h>
#include <sys/vdevice.h>

typedef struct {
	const char* id;
	int fb_fd;
	int shm_id;
	graph_t* g;
	console_t console;
} fb_console_t;

static void init_console(fb_console_t* console) {
	int id, w, h;
	void* gbuf;
	proto_t out;
	graph_t* g;
	int fb_fd = open("/dev/fb0", O_RDONLY);
	if(fb_fd < 0) {
		return;
	}

	id = vfs_dma(fb_fd, NULL);
	if(id <= 0) {
		close(fb_fd);
		return;
	}

	gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(fb_fd);
		return;
	}

	PF->init(&out, NULL, 0);

	if(vfs_fcntl(fb_fd, 0, NULL, &out) != 0) { //get fb size
		shm_unmap(id);
		close(fb_fd);
		return;
	}

	w = proto_read_int(&out);
	h = proto_read_int(&out);
	g = graph_new(gbuf, w, h);
	PF->clear(&out);

	console_init(&console->console);
	console->fb_fd = fb_fd;
	console->shm_id = id;
	console->g = g;
	console->console.font = font_by_name("8x16");
	console->console.fg_color = 0xffcccccc;
	console->console.bg_color = 0xff000000;
	console_reset(&console->console, w, h);
}

static void close_console(fb_console_t* console) {
	graph_free(console->g);
	shm_unmap(console->shm_id);
	close(console->fb_fd);
}

static void reset_console(fb_console_t* console) {
	int fb_fd = console->fb_fd;
	int id, w, h;
	void* gbuf;
	graph_t* g;

	graph_free(console->g);
	shm_unmap(console->shm_id);
	console->g = NULL;

	id = vfs_dma(fb_fd, NULL);
	if(id <= 0) {
		return;
	}

	gbuf = shm_map(id);
	if(gbuf == NULL) {
		return;
	}

	proto_t out;
	PF->init(&out, NULL, 0);
	if(vfs_fcntl(fb_fd, 0, NULL, &out) != 0) { //get fb size
		shm_unmap(id);
		return;
	}

	w = proto_read_int(&out);
	h = proto_read_int(&out);
	g = graph_new(gbuf, w, h);
	PF->clear(&out);
	console->shm_id = id;
	console->g = g;
	console_reset(&console->console, w, h);
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

	if(size <= 0)
		return 0;

	const char* pb = (const char*)buf;
	int i;
	for(i=0; i<size; i++) {
		char c = pb[i];
		console_put_char(&_console.console, c);
	}

	console_refresh(&_console.console, _console.g);
	if(vfs_flush(_console.fb_fd) == -1) { //fb resize
		reset_console(&_console);
		console_refresh(&_console.console, _console.g);
	}
	return size;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/console0";

	init_console(&_console);

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "console");
	dev.write = console_write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	close_console(&_console);
	return 0;
}
