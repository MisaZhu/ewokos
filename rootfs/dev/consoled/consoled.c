#include <dev/devserv.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <console.h>
#include <kstring.h>
#include <vfs/fs.h>
#include <shm.h>

static console_t _console;
static fb_t _fb;
static int32_t _keyb_fd = -1;
static bool _enabled = false;

static int32_t console_mount(const char* fname, int32_t index) {
	if(fname == NULL || fname[0] == 0 || index < 0)
		return -1;

	_enabled = false;
	console_init(&_console, "/etc/console.conf");

	_keyb_fd = open("/dev/keyb0", 0);
	if(_keyb_fd < 0)
		return -1;

	if(fb_open("/dev/fb0", &_fb) != 0)
		return -1;
	_console.g = graph_new(NULL, _fb.w, _fb.h);
	return console_reset(&_console);
}

static int32_t console_unmount(int32_t pid, const char* fname) {
	(void)pid;
	(void)fname;
	close(_keyb_fd);
	console_close(&_console);
	fb_close(&_fb);
	return 0;
}

static void flush(void) {
	if(_enabled && _console.g != NULL) {
		void* p = shm_map(_fb.shm_id);
		memcpy(p, _console.g->buffer, _fb.w*_fb.h*4);
		fb_flush(&_fb);
		shm_unmap(_fb.shm_id);
	}
}

static int32_t console_write(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)seek;

	const char* p = (const char*)buf;
	for(uint32_t i=0; i<size; i++) {
		char c = p[i];
		console_put_char(&_console, c);
	}
	flush();
	return size;
}

static int32_t console_read(int32_t pid, int32_t fd, void* buf, uint32_t size, int32_t seek) {
	(void)pid;
	(void)fd;
	(void)size;
	(void)seek;

	if(!_enabled) {
		errno = EAGAIN;
		sleep(0);
		return -1;
	}

	if(_keyb_fd < 0)
		return -1;
	int32_t res = read(_keyb_fd, buf, 1); 
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
		console_reset(&_console);
	}
	else if(cmd == FS_CTRL_SET_FONT) { //set font.
		font_t* fnt = get_font_by_name(data);
		if(fnt != NULL) {
			_console.font = fnt;
			console_reset(&_console);
			flush();
		}
	}
	else if(cmd == FS_CTRL_SET_FG_COLOR) { //set fg color.
		_console.fg_color = rgb_int(atoi_base(data, 16));
		console_refresh(&_console);
		flush();
	}
	else if(cmd == FS_CTRL_SET_BG_COLOR) { //set bg color.
		_console.bg_color = rgb_int(atoi_base(data, 16));
		console_refresh(&_console);
		flush();
	}
	else if(cmd == FS_CTRL_ENABLE) { //enable.
		_enabled = true;
		console_refresh(&_console);
		flush();
	}
	else if(cmd == FS_CTRL_DISABLE) { //disable.
		_enabled = false;
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
