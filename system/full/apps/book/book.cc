#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sconf/sconf.h>
#include <font/font.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <string.h>
#include <sys/utf8unicode.h>

#define HISTORY_PAGE_SIZE 128

using namespace Ewok;

class Book: public XWin {
	font_t font;
	uint32_t bgColor;
	uint32_t fgColor;
	char text[1280];
	int  history_page[HISTORY_PAGE_SIZE];
	int  current_page;
	int  next_page;
	int  read_len;
	FILE  *fp;

protected:
	void onRepaint(graph_t* g) {
		printf("onRepaint\n");
		graph_clear(g, bgColor);
		
		int x = 0;
		int y = 0;
		TTY_U16 w = 0;
		TTY_U16 h = font.max_size.y;
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

			font_char_size(unicode, &font, &w, &tmp);
			if((x + w) >= g->w){
				x = 0;
				y += h;
				if((y + h) >= g->h){
					break;
				}
			}
			graph_draw_char_font(g, x, y, unicode, &font, fgColor, &w, NULL);
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
					if(current_page == history_page[0])
						return;

					current_page = history_page[0];
					for(int i = 0; i < HISTORY_PAGE_SIZE - 1; i++){
						history_page[i]	= history_page[i+1];
					}
					readPage();
					repaint();
				}else if(key == KEY_DOWN){
					if(current_page == next_page)
						return;

					for(int i = HISTORY_PAGE_SIZE - 1; i > 0; i--){
						history_page[i] = history_page[i - 1];
					}
					history_page[0] = current_page;
					current_page = next_page;
					readPage();
					repaint();
				}else
					return;
			}
		}
	}
public:
	inline Book() {
		bgColor = 0xffDDC090;
		fgColor = 0xff3E3422;
		read_len = 0;
	}

	inline ~Book() {
		font_close(&font);
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
		if(conf == NULL){
			font_load("/user/fonts/system.ttf", 14, &font);
			printf("%08x\n", font);
			return false;
		}

		uint32_t font_size = 14;
		const char* v = sconf_get(conf, "font_size");
		if(v[0] != 0)
			font_size = atoi(v);

		v = sconf_get(conf, "font");
		if(v[0] == 0)
			v = "/user/fonts/system.ttf";
		font_load(v, font_size, &font);

		v = sconf_get(conf, "bg_color");
		if(v[0] != 0)
			bgColor = atoi_base(v, 16);
		v = sconf_get(conf, "fg_color");
		if(v[0] != 0)
			fgColor = atoi_base(v, 16);

		sconf_free(conf);
		return true;
	}
};

static void loop(void* p) {
	(void)p;
	//Book* xwin = (Book*)p;
	// xwin->readPage();
	// xwin->repaint();
	usleep(100000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	Book xwin;
	xwin.readConfig(x_get_theme_fname("/user/x/themes", "book", "theme.conf"));
	if(argc == 2){
		xwin.openBook(argv[1]);
	}else{
		xwin.openBook("/data/books/tb.txt");
	}
	X x;
	x.open(&xwin, 32,
			32,
			300,
			200,
			"book",
			X_STYLE_NORMAL);

	xwin.setVisible(true);
	x.run(loop, &xwin);
	return 0;
}
