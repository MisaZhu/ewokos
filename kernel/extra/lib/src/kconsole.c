#include "dev/framebuffer.h"
#include <graph.h>
#include <console.h>
#include <stddef.h>
#include <kprintf.h>

static console_t _console;

#ifdef FRAMEBUFFER

void kconsole_init(void) {
	console_init(&_console);
}

void kconsole_setup(void) {
	if(_console.g != NULL)
		return;
	fbinfo_t* info = fb_get_info();
	if(info->pointer == 0)
		return;

	graph_t* g = graph_new(NULL, info->width, info->height);
	_console.g = g;
	_console.font = font_by_name("8x16");
	console_reset(&_console);
}

static void kconsole_flush(void) {
	fbinfo_t* info = fb_get_info();
	fb_dev_write(_console.g->buffer, info->width * info->height * 4);
}

#else
void kconsole_init(void) {
	_console.g == NULL;
}

static void kconsole_flush(void) {
}

void kconsole_setup(void) {
}

#endif

void kconsole_close(void) {
	if(_console.g == NULL)
		return;

	graph_free(_console.g);
	console_close(&_console);
}

void kconsole_out(const char* s) {
	if(_console.g == NULL)
		return;
	console_put_string(&_console, s);
	kconsole_flush();
}
