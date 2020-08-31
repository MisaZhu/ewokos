#include "dev/framebuffer.h"
#include <graph.h>
#include <console.h>
#include <stddef.h>
#include <kprintf.h>

static console_t _console;

#ifdef WITH_LCDHAT
#include "lcdhat/lcdhat.h"
void kconsole_init(void) {
	printf("kernel: console with lcdhat out\n");
	console_init(&_console);
	lcd_init();
	graph_t* g = graph_new(NULL, LCD_WIDTH, LCD_HEIGHT);
	_console.g = g;
	_console.font = font_by_name("8x16");
	console_reset(&_console);
}

static void kconsole_flush(void) {
	lcd_flush(_console.g);
}

void kconsole_setup(void) {
}

#else

void kconsole_init(void) {
	printf("kernel: console with framebuffer out\n");
	console_init(&_console);
}

void kconsole_setup(void) {
	_console.g = NULL;
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
#endif

void kconsole_close(void) {
	if(_console.g != NULL) {
		graph_free(_console.g);
		console_close(&_console);
	}
}

void kconsole_out(const char* s) {
	if(_console.g == NULL)
		return;
	console_put_string(&_console, s);
	kconsole_flush();
}
