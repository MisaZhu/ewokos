#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <syscall.h>
#include <graph/graph.h>

static GraphT* _graph = NULL;
static int32_t _cx = 0;
static int32_t _cy = 0;

static int32_t consoleMount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;
	_graph = graphOpen("/dev/fb0");
	if(_graph == NULL)
		return -1;

	_cx = 0;
	_cy = 0;	
	clear(_graph, 0x222222);
	return 0;
}

static int32_t consoleUnmount(uint32_t node) {
	(void)node;
	if(_graph == NULL)
		return -1;

	graphClose(_graph);
	_graph = NULL;
	_cx = 0;
	_cy = 0;	
	return 0;
}

static inline void newLine() {
	_cx = 0;
	_cy ++;
}

static inline int32_t getX() {
	return _cx * fontBig.w;
}

static inline int32_t getY() {
	return _cy * fontBig.h;
}

static void checkBoard() {
	int32_t x, y;

	/*check board*/
	x = getX();
	y = getY();

	if(x > (int32_t)(_graph->w-fontBig.w))
		newLine();

	if(y > (int32_t)(_graph->h-fontBig.h)) {
		clear(_graph, 0x222222);
		_cy = 0;
	}
}

static void putChar(char c) {
	checkBoard();

	if(c == '\n') { //new line.
		newLine();
	}
	else {
		drawChar(_graph, getX(), getY(), c, &fontBig, 0xFFFFFF);
		_cx++;
	}
}

int32_t consoleWrite(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)seek;
	(void)node;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = p[i];
		putChar(c);
	}
	graphFlush(_graph);
	return size;
}

void _start() {
	DeviceT dev = {0};
	dev.mount = consoleMount;
	dev.unmount = consoleUnmount;
	dev.write = consoleWrite;

	devRun(&dev, "dev.console", 0, "/dev/console0", true);
	exit(0);
}
