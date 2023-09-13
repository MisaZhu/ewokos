#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <textview/textview.h>
#include <sconf/sconf.h>
#include <sys/vfs.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <ttf/ttf.h>
#include <sys/basic_math.h>
#include <x++/X.h>

using namespace Ewok;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint32_t unfocus_fg_color;
	uint32_t unfocus_bg_color;
} conf_t;

class XText : public XWin {
	conf_t conf;
	textview_t textview;
	int32_t rollStepRows;
	int32_t mouse_last_y;
public:
	XText() {
		textview_init(&textview);
	}

	~XText() {
		textview_close(&textview);
	}

	bool readConfig(const char* fname) {
		memset(&conf, 0, sizeof(conf_t));
		conf.bg_color = 0xfffffff;
		conf.unfocus_fg_color = 0xffffffff;
		conf.fg_color = 0xff000000;
		conf.unfocus_bg_color = 0xff444444;

		sconf_t *sconf = sconf_load(fname);	
		if(sconf == NULL) {
			font_load("/data/fonts/system.ttf", 12, &textview.font);
			textview.font_size = 12;
			return false;
		}

		const char* v = sconf_get(sconf, "bg_color");
		if(v[0] != 0) 
			conf.bg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "fg_color");
		if(v[0] != 0) 
			conf.fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_fg_color");
		if(v[0] != 0) 
			conf.unfocus_fg_color = atoi_base(v, 16);

		v = sconf_get(sconf, "unfocus_bg_color");
		if(v[0] != 0) 
			conf.unfocus_bg_color = atoi_base(v, 16);

		uint32_t font_size = 16;
		v = sconf_get(sconf, "font_size");
		if(v[0] != 0) 
			font_size = atoi(v);
		textview.font_size = font_size;
		
		v = sconf_get(sconf, "font_fixed");
		if(v[0] != 0) 
			textview.font_fixed = atoi(v);

		v = sconf_get(sconf, "font");
		if(v[0] == 0) 
			v = "/data/fonts/system.ttf";
		
		font_load(v, font_size, &textview.font);

		sconf_free(sconf);

		textview.fg_color = conf.fg_color;
		textview.bg_color = conf.bg_color;
		return true;
	}

	void put(const char* buf, int size) {
		textview_put_string(&textview, buf, size);
		repaint();
	}

	void rollEnd(void) {
		textview_roll(&textview, 0x0fffffff); //roll forward to the last row
	}

protected:
	void onFocus(void) {
		textview.fg_color = conf.fg_color;
		textview.bg_color = conf.bg_color;
		repaint();
	}

	void onUnfocus(void) {
		textview.fg_color = conf.unfocus_fg_color;
		textview.bg_color = conf.unfocus_bg_color;
		repaint();
	}

	void onRepaint(graph_t* g) {
		if(textview.w != g->w || textview.h != g->h) {
			rollStepRows = (g->h / textview.font.max_size.y) / 2;
			textview_reset(&textview, g->w, g->h);
		}
		textview_refresh(&textview, g);
	}
	
	void mouseHandle(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN) {
			mouse_last_y = ev->value.mouse.y;
			return;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int mv = (mouse_last_y - ev->value.mouse.y) / textview.font.max_size.y;
			if(abs_32(mv) > 0) {
				mouse_last_y = ev->value.mouse.y;
				//drag release
				textview_roll(&textview, mv);
				repaint();
			}
		}
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_IM && ev->state == XIM_STATE_PRESS) {
			int c = ev->value.im.value;
			if(c == KEY_ROLL_BACK) {
				textview_roll(&textview, -(rollStepRows));
				repaint();
				return;
			}
			else if(c == KEY_ROLL_FORWARD) {
				textview_roll(&textview, (rollStepRows));
				repaint();
				return;
			}

			if(c != 0) {
				write(1, &c, 1);
			}
		}
		else if(ev->type == XEVT_MOUSE) {
			mouseHandle(ev);
			return;	
		}
	}
};

int main(int argc, char* argv[]) {
	void* text = NULL;
	int size = 0;
	if(argc >= 2) {
		text = vfs_readfile(argv[1], &size);
	}

	XText xwin;
	xwin.readConfig("/etc/x/xtext.conf");

	X x;
	xscreen_t scr;
 	x.screenInfo(scr, 0);
	x.open(&xwin, 64, 40, scr.size.w*3/4, scr.size.h*3/4, "xtext", 0);
	xwin.setVisible(true);

	if(text != NULL && size > 0) {
		xwin.put((const char*)text, size);
		free(text);
	}

	x.run(NULL, &xwin);
	return 0;
}