#ifdef KCONSOLE
#include <kernel/kernel.h>
#include <dev/fb.h>
#include <kprintf.h>
#include <stddef.h>
#include <console/console.h>
#include <console/kconsole.h>
#include <atoi.h>
#include <stddef.h>
#include <sconf.h>

static console_t _console;
static uint32_t _line;
static graph_t* _fb_g = NULL;

kconsole_conf_t _kconsole_config;

static void load_kconsole_config_file() {
	sconf_t* sconf = sconf_load("/etc/kernel/kconsole.conf");
	if(sconf == NULL)
		return;

	const char* v = sconf_get(sconf, "fb_width");
	if(v[0] != 0)
		_kconsole_config.fb.width = atoi(v);

	v = sconf_get(sconf, "fb_height");
	if(v[0] != 0)
		_kconsole_config.fb.height = atoi(v);

	v = sconf_get(sconf, "fb_depth");
	if(v[0] != 0)
		_kconsole_config.fb.depth = atoi(v);

	v = sconf_get(sconf, "fb_rotate");
	if(v[0] != 0)
		_kconsole_config.fb.rotate = atoi(v);

	v = sconf_get(sconf, "font_size");
	if(v[0] != 0)
		_kconsole_config.font_size = atoi(v);

	sconf_free(sconf);
}

static void load_kconsole_config(void) {
	load_kconsole_config_file();

	if(_kconsole_config.fb.width == 0)
		_kconsole_config.fb.width = 640;

	if(_kconsole_config.fb.height == 0)
		_kconsole_config.fb.height = 480;

	if(_kconsole_config.fb.depth == 0)
		_kconsole_config.fb.depth = 32;
}

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;
	_line = 0;

	load_kconsole_config();
	console_init(&_console);
	//printf("\nkernel: init framebuffer ... ");
	if(fb_init_bsp(_kconsole_config.fb.width,
			_kconsole_config.fb.height,
			_kconsole_config.fb.depth,
			&fbinfo) != 0) {
		//printf("[failed]\n");
		return;
	}
	//printf("[ok] base: 0x%x, w:%d, h:%d, dep:%d\n", fbinfo.pointer, fbinfo.width, fbinfo.height, fbinfo.depth);
	if(_kconsole_config.fb.rotate == G_ROTATE_90 || _kconsole_config.fb.rotate == G_ROTATE_N90)
		_fb_g = graph_new(NULL, fbinfo.height, fbinfo.width);
	else
		_fb_g = graph_new(NULL, fbinfo.width, fbinfo.height);
	_console.font = get_font(_kconsole_config.font_size);
	_console.fg_color = 0xff000000;
	_console.bg_color = 0xff888888;
	console_reset(&_console, _fb_g->w, _fb_g->h);
}

void kconsole_input(const char* s) {
	if(_fb_g == NULL)
		return;
	console_put_string(&_console, s);
	console_refresh(&_console, _fb_g);
	fb_flush32(_fb_g->buffer, _fb_g->w, _fb_g->h, _kconsole_config.fb.rotate);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
	fb_close();
}

#endif