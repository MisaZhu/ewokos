#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <graph/graph.h>

#define T_W 2 /*tab width*/
static graph_t* _graph = NULL;
static int32_t _keyb_id = -1;

typedef struct {
	uint32_t line;
	uint32_t line_num;
	uint32_t line_w;
	char* data;
	uint32_t offset;
} content_t;

static content_t _content;

static inline uint32_t udiv(uint32_t v, uint32_t by) {
	if(by == 0)
		return 0;
	uint32_t ret = 0;
	uint32_t cmp = 0;
	while(cmp <= v) {
		cmp += by;
		ret++;
	}
	return ret;
}

static int32_t console_mount(uint32_t node, int32_t index) {
	(void)node;
	(void)index;
	
	_keyb_id = open("/dev/keyb0", 0);
	if(_keyb_id < 0)
		return -1;
	_graph = graph_open("/dev/fb0");
	if(_graph == NULL)
		return -1;


	_content.offset = 0;
	_content.line = 0;
	_content.line_w = udiv(_graph->w, font_big.w);
	_content.line_num = udiv(_graph->h, font_big.h)-1;
	_content.data = (char*)malloc(_content.line_num*_content.line_w);
	clear(_graph, 0x222222);
	return 0;
}

static int32_t console_unmount(uint32_t node) {
	(void)node;
	if(_graph == NULL)
		return -1;
	close(_keyb_id);
	free(_content.data);
	graph_close(_graph);
	_graph = NULL;
	_content.offset = 0;
	_content.data = NULL;
	return 0;
}

static void refresh() {
	clear(_graph, 0x222222);
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < _content.offset) {
		draw_char(_graph, x*font_big.w, y*font_big.h, _content.data[i], &font_big, 0xFFFFFF);
		
		x++;
		if(x >= _content.line_w) {
			y++;
			x = 0;
		}
		i++;
	}	
}

static void move_line() {
	_content.line--;
	_content.offset -= _content.line_w;
	uint32_t i = 0;
	while(i < _content.offset) {
		_content.data[i] = _content.data[i+_content.line_w];
		i++;
	}
	refresh();
}

static void put_char(char c) {
	if(c == '\r')
		c = '\n';

	if(c == 8) { //backspace
		if(_content.offset > 0) {
			_content.offset--;
			refresh();
		}
		return;
	}
	else if(c == '\t') {
		uint32_t x = 0;
		while(x < T_W) {
			put_char(' ');
			x++;
		}
		return;
	}
	if(c == '\n') { //new line.
		uint32_t x =  _content.offset - (_content.line*_content.line_w);
		while(x < _content.line_w) {
			_content.data[_content.offset++] = ' ';
			x++;
		}
		_content.line++;
	}
	else {
		uint32_t x =  _content.offset - (_content.line*_content.line_w) + 1;
		if(x == _content.line_w) {
			_content.line++;
		}
	}

	if(_content.line >= _content.line_num) {
		move_line();
	}
	if(c != '\n') {
		_content.data[_content.offset] = c;
		int32_t x = (_content.offset - (_content.line*_content.line_w)) * font_big.w;
		int32_t y = _content.line * font_big.h;
		draw_char(_graph, x, y, c, &font_big, 0xFFFFFF);
		_content.offset++;
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

int32_t console_read(uint32_t node, void* buf, uint32_t size, int32_t seek) {
	(void)node;
	(void)size;
	(void)seek;
	if(_keyb_id < 0)
		return -1;

	read(_keyb_id, buf, 1);
	return 1;
}

int main() {
	device_t dev = {0};
	dev.mount = console_mount;
	dev.unmount = console_unmount;
	dev.write = console_write;
	dev.read = console_read;

	dev_run(&dev, "dev.console", 0, "/dev/console0", true);
	return 0;
}
