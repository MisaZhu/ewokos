#ifdef KCONSOLE
#include <kernel/kconsole.h>
#include <dev/fb.h>
#include <kprintf.h>
#include <stddef.h>
#include <console/console.h>
#include <ext2read.h>
#include <atoi.h>

static console_t _console;
static uint32_t _line;
static graph_t* _fb_g = NULL;

static void read_config(uint32_t *w, uint32_t *h) {
	*w = 0;
	*h = 0;
	int32_t sz;
	char* line = sd_read_ext2("/etc/kernel/framebuffer.conf", &sz);
	if(line != NULL && sz > 0) {
		line[sz] = 0;
		for(int32_t i=0;i < sz; i++) {
			if(line[i] == 'x') {
				char* p = line + i + 1;
				line[i] = 0;
				*w = atoi(line);
				*h = atoi(p);
				break;
			}
		}
		kfree(line);
	}

	if(*w == 0)
		*w = 640;
	if(*h == 0)
		*h = 480;
}

void kconsole_init(void) {
	_fb_g = NULL;
	fbinfo_t fbinfo;
	_line = 0;
	uint32_t w, h;

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