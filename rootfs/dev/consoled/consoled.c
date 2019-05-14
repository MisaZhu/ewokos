#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <graph/graph.h>
#include <graph/font.h>
#include <basic_math.h>
#include <kstring.h>
#include <sconf.h>

#define T_W 2 /*tab width*/

static uint32_t _bg_color = 0x0;
static uint32_t _fg_color = 0xffffffff;
static font_t* _font = NULL;

static graph_t* _graph = NULL;
static int32_t _keyb_id = -1;

typedef struct {
	uint32_t start_line;
	uint32_t line;
	uint32_t line_num;
	uint32_t line_w;
	uint32_t total;
	char* data;
	uint32_t size;
} content_t;

static content_t _content;

static int32_t read_config(void) {
	sconf_t *conf = sconf_load("/etc/console.conf");	
	if(conf == NULL)
		return -1;
	
	const char* v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		_bg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		_fg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		_font = get_font_by_name(v);

	sconf_free(conf, MFS);
	return 0;
}

static int32_t reset(void) {
	_content.size = 0;
	_content.start_line = 0;
	_content.line = 0;
	_content.line_w = div_u32(_graph->w, _font->w)-1;
	_content.line_num = div_u32(_graph->h, _font->h)-1;
	_content.total = _content.line_num * _content.line_w;

	uint32_t data_size = _content.line_num*_content.line_w;
	if(_content.data != NULL)
		free(_content.data);
	_content.data = (char*)malloc(data_size);
	memset(_content.data, 0, data_size);

	clear(_graph, _bg_color);
	return 0;
}

static int32_t console_mount(const char* fname, int32_t index) {
	(void)fname;
	(void)index;
	_bg_color = rgb(0x22, 0x22, 0x66);
	_fg_color = rgb(0xaa, 0xbb, 0xaa);
	_font = get_font_by_name("8x16");
	if(_font == NULL)
		return -1;
	read_config();

	_keyb_id = open("/dev/keyb0", 0);
	if(_keyb_id < 0)
		return -1;

	_graph = graph_open("/dev/fb0");
	if(_graph == NULL)
		return -1;
	memset(&_content, 0, sizeof(content_t));
	return reset();
}

static int32_t console_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_graph == NULL)
		return -1;
	close(_keyb_id);
	free(_content.data);
	graph_close(_graph);
	_graph = NULL;
	_content.size = 0;
	_content.data = NULL;
	return 0;
}

static uint32_t get_at(uint32_t i) {
	uint32_t at = i + (_content.line_w * _content.start_line);
	if(at >= _content.total)
		at -=  _content.total;
	return at;
}

static void refresh(void) {
	clear(_graph, _bg_color);
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < _content.size) {
		uint32_t at = get_at(i);
		char c = _content.data[at];
		if(c != ' ') {
			draw_char(_graph, x*_font->w, y*_font->h, _content.data[at], _font, _fg_color);
		}
		x++;
		if(x >= _content.line_w) {
			y++;
			x = 0;
		}
		i++;
	}	
	graph_flush(_graph);
}

static void move_line(void) {
	_content.line--;
	_content.start_line++;
	if(_content.start_line >= _content.line_num)
		_content.start_line = 0;
	_content.size -= _content.line_w;
	refresh();
}

static void put_char(char c) {
	if(c == '\r')
		c = '\n';

	if(c == 8) { //backspace
		if(_content.size > 0) {
			_content.size--;
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
		uint32_t x =  _content.size - (_content.line*_content.line_w);
		while(x < _content.line_w) {
			uint32_t at = get_at(_content.size);
			_content.data[at] = ' ';
			_content.size++;
			x++;
		}
		_content.line++;
	}
	else {
		uint32_t x =  _content.size - (_content.line*_content.line_w) + 1;
		if(x == _content.line_w) {
			_content.line++;
		}
	}

	if((_content.line) >= _content.line_num) {
		move_line();
	}
	
	if(c != '\n') {
		uint32_t at = get_at(_content.size);
		_content.data[at] = c;
		int32_t x = (_content.size - (_content.line*_content.line_w)) * _font->w;
		int32_t y = _content.line * _font->h;
		draw_char(_graph, x, y, c, _font, _fg_color);
		_content.size++;
	}
}

int32_t console_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = p[i];
		put_char(c);
	}
	graph_flush(_graph);
	return size;
}

int32_t console_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)size;
	(void)seek;
	if(_keyb_id < 0)
		return -1;
	int32_t res = read(_keyb_id, buf, 1); 
	errno = ENONE;
	if(res == 0) {
		res = -1;
		errno = EAGAIN;
	}
	else if(res < 0) 
		return -1;
	if(((char*)buf)[0] == 0x4)//ctrl+d
		res = 0;
	return res;
}

void* console_ctrl(int32_t pid, int32_t fd, int32_t cmd, void* data, uint32_t size, int32_t* ret) {
	(void)pid;
	(void)fd;
	(void)data;
	(void)size;
	(void)ret;

	if(cmd == 0) { //clear.
		reset();
	}
	else if(cmd == 1) { //set font.
		font_t* fnt = get_font_by_name((char*)data);
		if(fnt != NULL) {
			_font = fnt;
			reset();
		}
	}
	else if(cmd == 2) { //set fg color.
		_fg_color = rgb_int(atoi_base((char*)data, 16));
		refresh();
	}
	else if(cmd == 3) { //set bg color.
		_bg_color = rgb_int(atoi_base((char*)data, 16));
		refresh();
	}
	return NULL;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = console_mount;
	dev.unmount = console_unmount;
	dev.write = console_write;
	dev.read = console_read;
	dev.ctrl = console_ctrl;

	dev_run(&dev, argc, argv);
	return 0;
}
