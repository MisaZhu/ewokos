#ifdef KCONSOLE
#include <kernel/kconsole.h>
#include <dev/fb.h>
#include <kprintf.h>
#include <stddef.h>
#include <console/console.h>
#include <sconf.h>
#include <atoi.h>

static console_t _console;
static uint32_t _line;
static graph_t* _fb_g = NULL;

static void read_config(uint32_t *w, uint32_t *h) {
	*w = 640;
	*h = 480;

	sconf_t* sconf = sconf_load("/etc/kernel/framebuffer.conf");
	if(sconf == NULL)
		return;
	const char* v = sconf_get(sconf, "width");
	printf("%s\n", v);
	if(v[0] != 0)
		*w = atoi(v);

	v = sconf_get(sconf, "height");
	printf("%s\n", v);
	if(v[0] != 0)
		*h = atoi(v);
	sconf_free(sconf);
}

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;
	_line = 0;
	uint32_t w = 640, h = 480;

	console_init(&_console);
	read_config(&w, &h);
	//printf("\nkernel: init framebuffer ... ");
	if(fb_init(w, h, &fbinfo) != 0) {
		//printf("[failed]\n");
		return;
	}
	//printf("[ok] base: 0x%x, w:%d, h:%d, dep:%d\n", fbinfo.pointer, fbinfo.width, fbinfo.height, fbinfo.depth);
	if(fbinfo.depth == 32)
		_fb_g = graph_new((uint32_t*)fbinfo.pointer, fbinfo.width, fbinfo.height);
	else {
		_fb_g = graph_new(NULL, fbinfo.width, fbinfo.height);
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
	fb_flush32(_fb_g->buffer, _fb_g->w, _fb_g->h);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
}

#endif