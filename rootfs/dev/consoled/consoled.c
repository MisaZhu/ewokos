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
#include <vfs/fs.h>

#define T_W 2 /*tab width*/

typedef struct {
	uint32_t start_line;
	uint32_t line;
	uint32_t line_num;
	uint32_t line_w;
	uint32_t total;
	char* data;
	uint32_t size;
} content_t;

typedef struct {
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;
	fb_t fb;
	graph_t* graph;
	int32_t keyb_fd;
	bool enabled;
	content_t content;
} console_t;

static console_t _console;

static int32_t read_config(void) {
	sconf_t *conf = sconf_load("/etc/console.conf");	
	if(conf == NULL)
		return -1;
	
	const char* v = sconf_get(conf, "bg_color");
	if(v[0] != 0) 
		_console.bg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "fg_color");
	if(v[0] != 0) 
		_console.fg_color = rgb_int(atoi_base(v, 16));

	v = sconf_get(conf, "font");
	if(v[0] != 0) 
		_console.font = get_font_by_name(v);

	sconf_free(conf, MFS);
	return 0;
}

static void cons_draw_char(int32_t x, int32_t y, char c) {
	if(_console.enabled)
		draw_char(_console.graph, x, y, c, _console.font, _console.fg_color);
}

static void cons_clear(void) {
	if(_console.enabled)
		clear(_console.graph, _console.bg_color);
}

static int32_t reset(void) {
	_console.content.size = 0;
	_console.content.start_line = 0;
	_console.content.line = 0;
	_console.content.line_w = div_u32(_console.graph->w, _console.font->w)-1;
	_console.content.line_num = div_u32(_console.graph->h, _console.font->h)-1;
	_console.content.total = _console.content.line_num * _console.content.line_w;

	uint32_t data_size = _console.content.line_num*_console.content.line_w;
	if(_console.content.data != NULL)
		free(_console.content.data);
	_console.content.data = (char*)malloc(data_size);
	memset(_console.content.data, 0, data_size);

	cons_clear();
	return 0;
}

static int32_t console_mount(const char* fname, int32_t index) {
	if(fname == NULL || fname[0] == 0 || index < 0)
		return -1;

	_console.enabled = false;
	_console.bg_color = rgb(0x22, 0x22, 0x66);
	_console.fg_color = rgb(0xaa, 0xbb, 0xaa);
	_console.font = get_font_by_name("8x16");
	if(_console.font == NULL)
		return -1;
	read_config();

	_console.keyb_fd = open("/dev/keyb0", 0);
	if(_console.keyb_fd < 0)
		return -1;

	if(fb_open("/dev/fb0", &_console.fb) != 0)
		return -1;
	_console.graph = graph_from_fb(&_console.fb);
	memset(&_console.content, 0, sizeof(content_t));
	return reset();
}

static int32_t console_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	if(_console.graph == NULL)
		return -1;
	close(_console.keyb_fd);
	free(_console.content.data);
	graph_free(_console.graph);
	_console.graph = NULL;
	fb_close(&_console.fb);
	_console.content.size = 0;
	_console.content.data = NULL;
	return 0;
}

static uint32_t get_at(uint32_t i) {
	uint32_t at = i + (_console.content.line_w * _console.content.start_line);
	if(at >= _console.content.total)
		at -=  _console.content.total;
	return at;
}

static void flush(void) {
	if(_console.enabled)
		fb_flush(&_console.fb);
}

static void refresh(void) {
	cons_clear();
	uint32_t i=0;
	uint32_t x = 0;
	uint32_t y = 0;
	while(i < _console.content.size) {
		uint32_t at = get_at(i);
		char c = _console.content.data[at];
		if(c != ' ') {
			cons_draw_char(x*_console.font->w, y*_console.font->h, _console.content.data[at]);
		}
		x++;
		if(x >= _console.content.line_w) {
			y++;
			x = 0;
		}
		i++;
	}	
	flush();
}

static void move_line(void) {
	_console.content.line--;
	_console.content.start_line++;
	if(_console.content.start_line >= _console.content.line_num)
		_console.content.start_line = 0;
	_console.content.size -= _console.content.line_w;
	refresh();
}

static void put_char(char c) {
	if(c == '\r')
		c = '\n';

	if(c == 8) { //backspace
		if(_console.content.size > 0) {
			_console.content.size--;
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
		uint32_t x =  _console.content.size - (_console.content.line*_console.content.line_w);
		while(x < _console.content.line_w) {
			uint32_t at = get_at(_console.content.size);
			_console.content.data[at] = ' ';
			_console.content.size++;
			x++;
		}
		_console.content.line++;
	}
	else {
		uint32_t x =  _console.content.size - (_console.content.line*_console.content.line_w) + 1;
		if(x == _console.content.line_w) {
			_console.content.line++;
		}
	}

	if((_console.content.line) >= _console.content.line_num) {
		move_line();
	}
	
	if(c != '\n') {
		uint32_t at = get_at(_console.content.size);
		_console.content.data[at] = c;
		int32_t x = (_console.content.size - (_console.content.line*_console.content.line_w)) * _console.font->w;
		int32_t y = _console.content.line * _console.font->h;
		cons_draw_char(x, y, c);
		_console.content.size++;
	}
}

static int32_t console_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = p[i];
		put_char(c);
	}
	flush();
	return size;
}

static int32_t console_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)size;
	(void)seek;

	if(!_console.enabled) {
		errno = EAGAIN;
		sleep(0);
		return -1;
	}

	if(_console.keyb_fd < 0)
		return -1;
	int32_t res = read(_console.keyb_fd, buf, 1); 
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

static void console_fctrl_raw(int32_t cmd, const char* data) {
	if(cmd == FS_CTRL_CLEAR) { //clear.
		reset();
	}
	else if(cmd == FS_CTRL_SET_FONT) { //set font.
		font_t* fnt = get_font_by_name(data);
		if(fnt != NULL) {
			_console.font = fnt;
			reset();
		}
	}
	else if(cmd == FS_CTRL_SET_FG_COLOR) { //set fg color.
		_console.fg_color = rgb_int(atoi_base(data, 16));
		refresh();
	}
	else if(cmd == FS_CTRL_SET_BG_COLOR) { //set bg color.
		_console.bg_color = rgb_int(atoi_base(data, 16));
		refresh();
	}
	else if(cmd == FS_CTRL_ENABLE) { //enable.
		_console.enabled = true;
		refresh();
	}
	else if(cmd == FS_CTRL_DISABLE) { //disable.
		_console.enabled = false;
	}
}

static int32_t console_fctrl(int32_t pid, const char* fname, int32_t cmd, proto_t* input, proto_t* out) {
	(void)pid;
	(void)fname;
	(void)out;
	console_fctrl_raw(cmd, proto_read_str(input));
	return 0;
}

static int32_t console_ctrl(int32_t pid, int32_t fd, int32_t cmd, proto_t* input, proto_t* out) {
	(void)pid;
	(void)fd;
	(void)out;

	console_fctrl_raw(cmd, proto_read_str(input));
	return 0;
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	device_t dev = {0};
	dev.mount = console_mount;
	dev.unmount = console_unmount;
	dev.write = console_write;
	dev.read = console_read;
	dev.fctrl = console_fctrl;
	dev.ctrl = console_ctrl;

	dev_run(&dev, argc, argv);
	return 0;
}
