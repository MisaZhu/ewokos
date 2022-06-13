#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <fonts/fonts.h>
#include <x++/X.h>
#include <sys/keydef.h>

using namespace Ewok;

class Finder: public XWin {
	font_t* font;
	int     selected;
	bool    enter;

	void runProc(const char* item) {
		exec(item);
	}

protected:
	void onRepaint(graph_t* g) {
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(key == KEY_UP)
				selected--;
			else if(key == KEY_DOWN)
				selected++;
			else if(key == KEY_ENTER) {
				enter = true;
				return;
			}
			else if(key == 0) {
				if(enter) {
					enter = false;
					runProc("");
					return;
				}
			}
			else
				return;
			repaint(true);
		}
	}
public:
	inline Finder() {
		selected = 0;
		enter = false;
		font = font_by_name("8x16");
	}

	inline ~Finder() {
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		font = font_by_name(sconf_get(conf, "font"));
		sconf_free(conf);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	Finder xwin;
	xwin.readConfig("/etc/x/finder.conf");

	X x;
	x.open(&xwin, 0,
			0,
			300,
			200,
			"finder",
			X_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(NULL);
	return 0;
}
