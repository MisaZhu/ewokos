#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <syscall.h>
#include <graph/graph.h>

static graph_t* _graph = NULL;
static int32_t _cx = 0;
static int32_t _cy = 0;

static int32_t console_mount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;
	_graph = graph_open("/dev/fb0");
	if(_graph == NULL)
		return -1;

	_cx = 0;
	_cy = 0;	
	clear(_graph, 0x222222);
	return 0;
}

static int32_t console_unmount(uint32_t node) {
	(void)node;
	if(_graph == NULL)
		return -1;

	graph_close(_graph);
	_graph = NULL;
	_cx = 0;
	_cy = 0;	
	return 0;
}

static inline void new_line() {
	_cx = 0;
	_cy ++;
}

static inline int32_t get_x() {
	return _cx * font_big.w;
}

static inline int32_t get_y() {
	return _cy * font_big.h;
}

static void check_board() {
	int32_t x, y;

	/*check board*/
	x = get_x();
	y = get_y();

	if(x > (int32_t)(_graph->w-font_big.w))
		new_line();

	if(y > (int32_t)(_graph->h-font_big.h)) {
		clear(_graph, 0x222222);
		_cy = 0;
	}
}

static void put_char(char c) {
	check_board();

	if(c == '\n') { //new line.
		new_line();
	}
	else {
		draw_char(_graph, get_x(), get_y(), c, &font_big, 0xFFFFFF);
		_cx++;
	}
}

int32_t console_write(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)seek;
	(void)node;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = p[i];
		put_char(c);
	}
	graph_flush(_graph);
	return size;
}

int main() {
	device_t dev = {0};
	dev.mount = console_mount;
	dev.unmount = console_unmount;
	dev.write = console_write;

	dev_run(&dev, "dev.console", 0, "/dev/console0", true);
	return 0;
}
