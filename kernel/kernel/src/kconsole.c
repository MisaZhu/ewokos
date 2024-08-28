#ifdef KCONSOLE
#include <kernel/kconsole.h>
#include <kernel/kernel.h>
#include <dev/fb.h>
#include <kprintf.h>
#include <stddef.h>
#include <console/console.h>
#include <sconf.h>
#include <atoi.h>

static console_t _console;
static uint32_t _line;
static graph_t* _fb_g = NULL;

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;
	_line = 0;

	console_init(&_console);
	//printf("\nkernel: init framebuffer ... ");
	if(fb_init_bsp(_kernel_config.fb.width,
			_kernel_config.fb.height,
			_kernel_config.fb.depth,
			&fbinfo) != 0) {
		//printf("[failed]\n");
		return;
	}
	//printf("[ok] base: 0x%x, w:%d, h:%d, dep:%d\n", fbinfo.pointer, fbinfo.width, fbinfo.height, fbinfo.depth);
	if(_kernel_config.fb.rotate == G_ROTATE_90 || _kernel_config.fb.rotate == G_ROTATE_N90)
		_fb_g = graph_new(NULL, fbinfo.height, fbinfo.width);
	else
		_fb_g = graph_new(NULL, fbinfo.width, fbinfo.height);
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
	fb_flush32(_fb_g->buffer, _fb_g->w, _fb_g->h, _kernel_config.fb.rotate);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
	fb_close();
}

#endif