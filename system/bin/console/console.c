#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <sys/shm.h>
#include <sconf.h>
#include <sys/global.h>
#include <dev/fbinfo.h>
#include <sys/thread.h>

typedef struct {
	const char* id;
	int fb_fd;
	int shm_id;
	console_t console;
} fb_console_t;

typedef struct {
	font_t* font;
	uint32_t fg_color;
	uint32_t bg_color;
} conf_t;

static conf_t _conf;

static int32_t read_config(conf_t* conf, const char* fname) {
	conf->font = font_by_name("8x16");
	conf->bg_color = 0xffffffff;
	conf->fg_color = 0xff000000;

	sconf_t *sconf = sconf_load(fname);	
	if(sconf == NULL)
		return -1;

	const char* v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		conf->bg_color = argb_int(atoi_base(v, 16));
	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		conf->fg_color = argb_int(atoi_base(v, 16));
	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
		conf->font = font_by_name(v);
	sconf_free(sconf);
	return 0;
}

static void init_console(fb_console_t* console) {
	int fb_fd = open("/dev/fb0", O_RDONLY);
	if(fb_fd < 0) {
		return;
	}

	int id = dma(fb_fd, NULL);
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
	proto_init(&out, NULL, 0);

	if(fcntl_raw(fb_fd, CNTL_INFO, NULL, &out) != 0) {
		shm_unmap(id);
		close(fb_fd);
		return;
	}

	int w = proto_read_int(&out);
	int h = proto_read_int(&out);
	graph_t* g = graph_new(gbuf, w, h);
	proto_clear(&out);

	console->fb_fd = fb_fd;
	console->shm_id = id;
	console_init(&console->console);
	console->console.g = g;
	console->console.font = _conf.font;
	console->console.fg_color = _conf.fg_color;
	console->console.bg_color = _conf.bg_color;
	console_reset(&console->console);
}

static void close_console(fb_console_t* console) {
	graph_free(console->console.g);
	shm_unmap(console->shm_id);
	close(console->fb_fd);
}

static int actived = 0;

static int read_input(int fd, int8_t* c, int rd) {
	//read keyb
	if(actived == 1) {
		if(rd != 1) {
			rd = read_nblock(fd, c, 1);
		}
		else {
			if(write_nblock(1, c, 1) == 1)
				rd = 0;
		}
	}
	return rd;
}

static int run(int argc, char* argv[]) {
	int fd = open("/dev/keyb0", O_RDONLY);
	if(fd < 0)
		return -1;

	read_config(&_conf, "/etc/console.conf");
	fb_console_t console;
	init_console(&console);
	if(argc <= 1)
		console.id = "0";
	else
		console.id = argv[1];
	
	int rd = 0;
	int8_t c = 0;
	while(1) {
		const char* cc = get_global("current_console");
		if(cc[0] == console.id[0]) {
			if(actived == 0) {
				console_refresh(&console.console);
				flush(console.fb_fd);
			}
			actived = 1;
		}
		else {
			actived = 0;
			usleep(10000);
			continue;
		}

		rd = read_input(fd, &c, rd);

		char buf[256];
		int32_t size = read_nblock(0, buf, 255);
		if(size == 0) {
			break;
		}
		else if(size < 0) {
			if(errno == EAGAIN) {
				sleep(0);
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

		if(actived == 1)
			flush(console.fb_fd);
	}
	close(fd);
	close_console(&console);
	return 0;
}

int main(int argc, char* argv[]) {
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
		return run(argc, argv);
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
