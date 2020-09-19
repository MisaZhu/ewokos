#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console/console.h>
#include <graph/graph.h>
#include <fonts/fonts.h>
#include <sys/shm.h>
#include <sys/vfs.h>

typedef struct {
	const char* id;
	int fb_fd;
	int shm_id;
	graph_t* g;
	console_t console;
} fb_console_t;

static void init_console(fb_console_t* console) {
	int fb_fd = open("/dev/fb0", O_RDONLY);
	if(fb_fd < 0) {
		return;
	}

	int id = vfs_dma(fb_fd, NULL);
	if(id <= 0) {
		close(fb_fd);
		return;
	}

	void* gbuf = shm_map(id);
	if(gbuf == NULL) {
		close(fb_fd);
		return;
	}

	proto_t out;
	PF->init(&out, NULL, 0);

	if(vfs_fcntl(fb_fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(fb_fd);
		return;
	}

	int w = proto_read_int(&out);
	int h = proto_read_int(&out);
	graph_t* g = graph_new(gbuf, w, h);
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

static int read_input(int fd, int8_t* c, int rd) {
	//read keyb
	if(rd != 1) {
		rd = read(fd, c, 1);
	}
	else {
		if(write(1, c, 1) == 1)
			rd = 0;
	}
	return rd;
}

static int run(void) {
	int fd = open("/dev/keyb0", O_RDONLY | O_NONBLOCK);
	if(fd < 0)
		return -1;

	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	flags = fcntl(1, F_GETFL, 0);
	fcntl(1, F_SETFL, flags | O_NONBLOCK);

	fb_console_t console;
	init_console(&console);
	
	int rd = 0;
	int8_t c = 0;
	while(1) {
		rd = read_input(fd, &c, rd);

		char buf[256];
		int32_t size = read(0, buf, 255);
		if(size == 0) {
			break;
		}
		else if(size < 0) {
			if(errno == EAGAIN) {
				usleep(10000);
				continue;
			}
			else 
				break;
		}

		buf[size] = 0;
		const char* p = (const char*)buf;
		for(int32_t i=0; i<size; i++) {
			char c = p[i];
			console_put_char(&console.console, c);
		}

		console_refresh(&console.console, console.g);
		vfs_flush(console.fb_fd);
	}
	close(fd);
	close_console(&console);
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
		return run();
	}
	//child proc for p1 writer
	dup2(fds1[1], 1);
	dup2(fds2[0], 0);
	close(fds1[0]);
	close(fds1[1]);
	close(fds2[0]);
	close(fds2[1]);
	setenv("CONSOLE", "console");

	exec("/bin/session");
	return 0;
}
