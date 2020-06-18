extern "C" {
#include <string.h>
#include <sconf.h>
}

#include <x++/X.h>

#define ITEM_MAX 16

typedef struct {
	font_t* font;
} items_t;

static items_t _items;

static int32_t read_config(const char* fname, items_t* items) {
	items->font = font_by_name("8x16");
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;
	items->font = font_by_name(sconf_get(conf, "font"));
	sconf_free(conf);
	return 0;
}

class Panel : public XWin {
protected:
	void onRepaint(Graph& g) {
		g.clear(argb_int(0xffaaaaaa));
		g.drawText(2, 2, "EwokOS", _items.font, 0xff222222);
		g.line(0, g.getH()-1, g.getW(), g.getH()-1, argb_int(0xff222222));
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	memset(&_items, 0, sizeof(items_t));
	read_config("/etc/x/syspanel.conf", &_items);

	X x;

	xscreen_t scr;
	XWin::screenInfo(scr);

	Panel xwin;
	x.open(&xwin, 0,  0, scr.size.w, _items.font->h + 4,
			"syspanel", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_NO_FOCUS);
	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}
