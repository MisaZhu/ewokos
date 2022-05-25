#ifdef KCONSOLE
#include <kernel/kconsole.h>
#include <dev/fb.h>
#include <basic_math.h>
#include <kprintf.h>
#include <stddef.h>
#include <console/console.h>

static console_t _console;
static uint32_t _line;
static graph_t* _fb_g = NULL;
static uint16_t* _g16 = NULL;

void kconsole_init(void) {
	_fb_g = NULL;
	_g16 = NULL;
	fbinfo_t fbinfo;
	_line = 0;

	console_init(&_console);
	printf("kernel: init framebuffer ... ");
	if(fb_init(640, 480, &fbinfo) != 0) {
		printf("[failed]\n");
		return;
	}
	printf("[ok] base: 0x%x, w:%d, h:%d, dep:%d\n", fbinfo.pointer, fbinfo.width, fbinfo.height, fbinfo.depth);
	if(fbinfo.depth == 32)
		_fb_g = graph_new((uint32_t*)fbinfo.pointer, fbinfo.width, fbinfo.height);
	else {
		_fb_g = graph_new(NULL, fbinfo.width, fbinfo.height);
		_g16 = (uint16_t*)fbinfo.pointer;
	}
	_console.font = get_font();
	_console.fg_color = 0xff000000;
	_console.bg_color = 0xff888888;
	console_reset(&_console, _fb_g->w, _fb_g->h);
}

void kconsole_input(const char* s) {
	if(_fb_g == NULL)
		return;
	console_put_string(&_console, s);
	console_refresh(&_console, _fb_g);
	if(_g16 != NULL)
		blt16(_fb_g->buffer, _g16, _fb_g->w, _fb_g->h);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
}

#endif