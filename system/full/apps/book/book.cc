#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sconf/sconf.h>
#include <x++/XTheme.h>
#include <x++/X.h>
#include <ewoksys/keydef.h>
#include <ewoksys/proc.h>
#include <string.h>
#include <ewoksys/utf8unicode.h>

#define HISTORY_PAGE_SIZE 128

using namespace Ewok;

class Book: public XWin {
	char text[4096];
	int  history_page[HISTORY_PAGE_SIZE];
	int  current_page;
	int  next_page;
	int  read_len;
	FILE  *fp;

	void pageUp() {
		if(current_page == history_page[0])
			return;

		current_page = history_page[0];
		for (int i = 0; i < HISTORY_PAGE_SIZE - 1; i++) {
			history_page[i] = history_page[i + 1];
		}
		readPage();
		repaint();
	}

	void pageDown() {
		if(current_page == next_page)
			return;

		for (int i = HISTORY_PAGE_SIZE - 1; i > 0; i--) {
			history_page[i] = history_page[i - 1];
		}
		history_page[0] = current_page;
		current_page = next_page;
		readPage();
		repaint();
	}

protected:
	void onRepaint(graph_t* g) {
		//printf("onRepaint\n");
		graph_clear(g, theme.basic.bgColor);
		
		int x = 0;
		int y = 0;
		TTY_U16 w = 0;
		TTY_U16 h = theme.getFont()->max_size.y;
		TTY_U16 tmp;
		int i = 0;
		while(i < read_len) {
			int unicode;
			int n = utf82unicode_char((uint8_t*)text + i, &unicode);
			if( n < 0){
				break;
			}
			i+=n;
			if(unicode == 0xd || unicode == 0xa){
				y+=h;
				x = 0;
				if((y + h) >= g->h){
					break;
				}
				continue;
			}

			font_char_size(unicode, theme.getFont(), &w, &tmp);
			if((x + w) >= g->w){
				x = 0;
				y += h;
				if((y + h) >= g->h){
					break;
				}
			}
			graph_draw_char_font(g, x, y, unicode, theme.getFont(), theme.basic.fgColor, &w, NULL);
			x += w;
		}
		next_page = current_page + i;
	}


	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(ev->state == XIM_STATE_PRESS) {
				if(key == KEY_UP){
					pageUp();	
				}else if(key == KEY_DOWN){
					pageDown();	
				}else
					return;
			}
		}
		else if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_CLICK) {
			gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
			if(pos.y > xinfo.wsr.h/2)
				pageDown();
			else
				pageUp();
		}
	}
public:
	inline Book() {
		read_len = 0;
	}

	inline ~Book() {
	}

	void readPage(void){
		if(fp){
			fseek(fp, current_page, SEEK_SET);  
			memset(text, 0, sizeof(text));
			read_len = fread(text, 1, sizeof(text), fp);
		}
	}
	
	void openBook(const char* path){
		fp = fopen(path, "r");
		memset(history_page, 0, sizeof(history_page));
		current_page = 0;
		readPage();
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		theme.loadConfig(conf);
		sconf_free(conf);
		return true;
	}
};

static void loop(void* p) {
	(void)p;
	//Book* xwin = (Book*)p;
	// xwin->readPage();
	// xwin->repaint();
	proc_usleep(100000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	Book xwin;
	xwin.readConfig(x_get_theme_fname(X_THEME_ROOT, "book", "theme.conf"));
	if(argc == 2){
		xwin.openBook(argv[1]);
	}else{
		xwin.openBook("/data/books/tb.txt");
	}
	X x;
	x.open(0, &xwin, 32,
			32,
			300,
			200,
			"book",
			XWIN_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(loop, &xwin);
	return 0;
}
