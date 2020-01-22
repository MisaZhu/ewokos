#include "kprintf.h"
#include "vprintf.h"
#include "dev/uart.h"
#include "dev/framebuffer.h"
#include "dev/actled.h"
#include "kstring.h"
#include "mstr.h"
#include "graph.h"
#include "kernel/system.h"

static console_t _console;

#ifdef WITH_LCDHAT
#include "lcdhat/lcdhat.h"
void init_console(void) {
	console_init(&_console);
	lcd_init();
	graph_t* g = graph_new(NULL, LCD_WIDTH, LCD_HEIGHT);
	_console.g = g;
	console_reset(&_console);
}

static void flush_console(void) {
	lcd_flush(_console.g);
}

void setup_console(void) {
}

#else

void init_console(void) {
	console_init(&_console);
}

void setup_console(void) {
	if(_console.g != NULL) //already setup
		return;

	fbinfo_t* info = fb_get_info();
	if(info->pointer == 0)
		return;

	graph_t* g = graph_new(NULL, info->width, info->height);
	_console.g = g;
	console_reset(&_console);
}

static void flush_console(void) {
	fbinfo_t* info = fb_get_info();
	fb_dev_write(NULL, _console.g->buffer, info->width * info->height * 4);
}
#endif

void uart_out(const char* s) {
	uart_write(NULL, s, strlen(s));
}

#define PRINTF_BUF_MAX 128
static uint32_t _len = 0;
static char _buf[PRINTF_BUF_MAX];
static void outc(char c, void* p) {
	(void)p;
	if((_len+1) >= PRINTF_BUF_MAX)
		return;
	_buf[_len] = c;
	_len++;
	_buf[_len] = 0;
}

void flush_actled(void) {
	act_led(1);
	_delay(1000000);
	act_led(0);
	_delay(1000000);
}

void printf(const char *format, ...) {
	act_led(1);
	_delay_msec(10);

	va_list ap;
	va_start(ap, format);
	_len = 0;
	v_printf(outc, NULL, format, ap);
	uart_out(_buf);
	act_led(0);
	if(_console.g != NULL) {
		console_put_string(&_console, _buf);
		flush_console();
	}
}

void close_console(void) {
	if(_console.g != NULL) {
		graph_free(_console.g);
		console_close(&_console);
	}
}

