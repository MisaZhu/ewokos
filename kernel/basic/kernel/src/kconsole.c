#include <dev/fb.h>
#include <basic_math.h>
#include <stddef.h>
#include <graph/graph.h>
#include <console/console.h>

static graph_t* _fb_g = NULL;
static console_t _console;

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;

	console_init(&_console);
	if(fb_init(640, 480, &fbinfo) == 0) {
		_fb_g = graph_new((uint32_t*)fbinfo.pointer, fbinfo.width, fbinfo.height);
		graph_clear(_fb_g, 0xff000000);

		_console.font = get_font();
		_console.fg_color = 0xffcccccc;
		_console.bg_color = 0xff000000;
		console_reset(&_console, _fb_g->w, _fb_g->h);
	}
}

void kconsole_input(const char* s) {
	if(_fb_g == NULL)
		return;
	//console_put_string(&_console, s);
	//console_refresh(&_console, _fb_g);
}

void kconsole_close(void) {
	console_close(&_console);
	if(_fb_g != NULL)
		graph_free(_fb_g);
	_fb_g = NULL;
}