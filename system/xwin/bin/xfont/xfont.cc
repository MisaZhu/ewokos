#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <unistd.h>
#include <string>
#include <font/font.h>
#include <ewoksys/kernel_tic.h>
#include <pthread.h>

using namespace Ewok;

class FontWidget: public Widget {
public:
	font_t* font;
	bool fontLoaded;
	std::string fontName;
	std::string fontFileName;

	static void* loadThread(void* p) {
		FontWidget* fw = (FontWidget*)p;
		fw->getWin()->busy(true);
		if(font_load(fw->fontName.c_str(), fw->fontFileName.c_str()) >= 0) {
			fw->font = font_new(fw->fontName.c_str(), false);
			if(fw->font != NULL)
				fw->fontLoaded = true;
		}
		return NULL;
	}

private:
	void drawLoading(graph_t* g, XTheme* theme, const grect_t& r) {
		font_t* ft = theme->getFont();
		std::string text = "[ ";
		text += fontName + " ... ]";
		
		graph_fill(g, r.x, r.y, r.w, font_get_height(ft, theme->basic.fontSize), theme->basic.titleBGColor);
		graph_draw_text_font(g, r.x+10, r.y, text.c_str(),
			ft, 12, theme->basic.docFGColor);
	}

	const char*  font_name_by_fname(const char* fname) {
		static char ret[128];
		memset(ret, 0, 128);

		char sname[FS_FULL_NAME_MAX];
		vfs_file_name(fname, sname, FS_FULL_NAME_MAX);
		strncpy(ret, sname, 127);
		int i = strlen(sname)-1;
		while(i >= 0) {
			if(ret[i] == '.') {
				ret[i] = 0;
				break;
			}
			--i;
		}
		return ret;
	}

protected:

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor);
		if(!fontLoaded) {
			drawLoading(g, theme, r);
			return;
		}

		uint16_t margin = 2;
		uint32_t y = 0;

		graph_fill(g, r.x, y, r.w, font_get_height(font, theme->basic.fontSize), theme->basic.titleBGColor);
		std::string text = "[ font: ";
		text += fontName + " ]";
		graph_draw_text_font(g, r.x+10, y, text.c_str(), font, theme->basic.fontSize, theme->basic.docFGColor);
		y += theme->basic.fontSize + margin*2;

		for(int i=0; i<3; i++) {
			uint16_t size = (i+1) * 8;
			graph_draw_text_font(g, r.x+10, y, "abcdefghijklmn", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y, "opqrstuvwxyz", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y, "0123456789.+-", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y, "~!@#$%%^&*()", font, size, theme->basic.docFGColor);
			y += size + margin;
			graph_draw_text_font(g, r.x+10, y, "中文字体演示", font, size, theme->basic.docFGColor);
			y += (size + margin)*2;
		}
		getWin()->busy(false);
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if(timerStep == 0) {
			pthread_t tid;
			pthread_create(&tid, NULL, loadThread, this);
		}
		update();
	}
public: 
	FontWidget(const char* fname) {
		font = NULL;
		fontLoaded = false;
		fontFileName = fname;
		fontName = font_name_by_fname(fontFileName.c_str());
	}
};

int main(int argc, char** argv) {
	if(argc < 2) {
		return -1;
	}

	X x;
	WidgetWin win;


	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);

	FontWidget* fontW = new FontWidget(argv[1]);
	root->add(fontW);

	win.open(&x, -1, -1, -1, 0, 0, "xfont", XWIN_STYLE_NORMAL);
	win.setTimer(8);

	widgetXRun(&x, &win);
	return 0;
}