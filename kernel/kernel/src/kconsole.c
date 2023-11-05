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
static uint8_t _rotate = G_ROTATE_NONE;

static void read_config(uint32_t *w, uint32_t *h, uint8_t *dep) {
	sconf_t* sconf = sconf_load("/etc/kernel/framebuffer.conf");
	if(sconf == NULL)
		return;
	const char* v = sconf_get(sconf, "width");
	if(v[0] != 0)
		*w = atoi(v);

	v = sconf_get(sconf, "height");
	if(v[0] != 0)
		*h = atoi(v);

	v = sconf_get(sconf, "depth");
	if(v[0] != 0)
		*dep = atoi(v);

	v = sconf_get(sconf, "rotate");
	if(v[0] != 0)
		_rotate = atoi(v);


	sconf_free(sconf);

	if(*w == 0)
		*w = 640;
	if(*h == 0)
		*h = 480;
	if(*dep == 0)
		*dep = 32;
}

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;
	_line = 0;
	_rotate = G_ROTATE_NONE;
	uint32_t w = 640, h = 480;
	uint8_t dep = 32;

	console_init(&_console);
	read_config(&w, &h, &dep);
	//printf("\nkernel: init framebuffer ... ");
	if(fb_init_bsp(w, h, dep, &fbinfo) != 0) {
		//printf("[failed]\n");
		return;
	}
	//printf("[ok] base: 0x%x, w:%d, h:%d, dep:%d\n", fbinfo.pointer, fbinfo.width, fbinfo.height, fbinfo.depth);
	if(_rotate == G_ROTATE_90 || _rotate == G_ROTATE_N90)
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
	fb_flush32(_fb_g->buffer, _fb_g->w, _fb_g->h, _rotate);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
}

#endif