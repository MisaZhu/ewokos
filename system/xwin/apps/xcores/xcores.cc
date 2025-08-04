#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph_png.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>

using namespace Ewok;

class Cores : public Widget {
	static const uint32_t HEART_BIT_NUM = 64;
	uint32_t index;
	bool loop;
	sys_info_t sysInfo;
	uint32_t cores[MAX_CORE_NUM][HEART_BIT_NUM];

	int x_off;
	int y_off_bottom;
	int y_off;

	static const uint32_t COLOR_NUM = 7;
	const  uint32_t colors[COLOR_NUM] = {
		0xff000000, 
		0xff0000ff,
		0xff00ff00, 
		0xffff0000, 
		0xff8800ff,
		0xff0088ff,
		0xffff8800
	};
public:
	inline Cores() {
		index = 0;
		loop = false;
		x_off = 10;
		y_off = 20;
		y_off_bottom = 10;
		memset(cores, 0, sizeof(cores));
		memset(&sysInfo, 0, sizeof(sys_info_t));
	}
	
	inline ~Cores() {
	}

	void updateCores() {
		sys_get_sys_info(&sysInfo);

		for(uint32_t i=0; i<sysInfo.cores; i++) {
			cores[i][index] = sysInfo.core_idles[i];
		}

		index++;
		if(index >= HEART_BIT_NUM) {
			loop = true;
			index = 0;
		}
		update();
	}

protected:
	void drawTitle(graph_t* g, XTheme* theme, uint32_t i, uint32_t color, const grect_t& r) {
		int x = r.x + x_off + i*60;
		graph_fill(g, x, r.y+4, 10, 10, color);
		char s[16];
		int32_t perc = 100 - (sysInfo.core_idles[i]/10000);
		if(perc < 0)
			perc = 0;
		snprintf(s, 15, "%d:%d%%", i, perc);
		graph_draw_text_font(g, x+12, r.y, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void drawChat(graph_t* g, uint32_t i, float xstep, float yzoom, uint32_t color, const grect_t& r) {
		int last_x = 0;
		int last_y = -1;
		uint32_t num = loop ? HEART_BIT_NUM:index;
		for(uint32_t j=0; j<num; j++) {
			uint32_t k = j;
			if(loop)
				k = j + index;
			if(k >= HEART_BIT_NUM)
				k -= HEART_BIT_NUM;

			int32_t perc = 100 - (cores[i][k] / 10000);
			if(perc < 0)
				perc = 0;

			int x = (j+1) * xstep;
			int y = r.h - y_off_bottom - yzoom*(perc); //percentage
			if(last_y < 0)
				last_y = y;

			graph_wline(g, r.x+x_off+last_x, r.y+last_y, r.x+x_off+x, r.y+y, color, 2);
			last_x = x;
			last_y = y;
		}
	}

	void drawBG(graph_t* g, float xstep, float yzoom, const grect_t& r) {
		uint32_t color = 0xffbbbbbb;
		uint32_t bgColor = 0xff888888;
		uint32_t w = xstep*HEART_BIT_NUM;
		uint32_t h = yzoom*100;

		graph_fill(g, r.x + x_off, r.y+ r.h - h - y_off_bottom,
				w, h, bgColor);

		for(uint32_t i=0; i<=HEART_BIT_NUM; i++) {
			graph_line(g, r.x + x_off + i*xstep, r.y + r.h - y_off_bottom,
					r.x + x_off + i*xstep, r.y + r.h - y_off_bottom - h, color);
		}

		for(uint32_t i=0; i<=10; i++) {
			graph_line(g, r.x + x_off, r.y + r.h - y_off_bottom - i*10*yzoom,
				r.x + w + x_off, r.y + r.h - y_off_bottom - i*10*yzoom, color);
		}

	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(sysInfo.cores == 0)
			return;

		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor, true);

		float xstep = (r.w - x_off*2)/ (float)HEART_BIT_NUM;
		float yzoom = (r.h - y_off - y_off_bottom)/ 100.0f;
		if(yzoom <= 0.0)
			yzoom = 1.0;
		if(yzoom > 10.0)
			yzoom = 10.0;

		drawBG(g, xstep, yzoom, r);
		for(uint32_t i=0; i<sysInfo.cores; i++) {
			uint32_t color = colors[i%COLOR_NUM];
			drawTitle(g, theme, i, color, r);
			drawChat(g, i, xstep, yzoom, color, r);
		}

		sys_info_t sys_info;
		sys_state_t sys_state;
		syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)(uint64_t)&sys_info);
		syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)(uint64_t)&sys_state);
		uint32_t fr_mem = sys_state.mem.free / (1024*1024);
		uint32_t t_mem = sys_info.total_usable_mem_size / (1024*1024);
		char txt[32] = { 0 };
		snprintf(txt, 31, "%d/%d(m)", fr_mem, t_mem);
		font_t* font = theme->getFont();
		uint32_t w;
		font_text_size(txt, font, 10, &w, NULL);
		graph_draw_text_font(g, r.x + r.w - w, r.y, txt, theme->getFont(), 10, theme->basic.widgetFGColor);
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		updateCores();
	}
};


int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);
	

	Cores* cores = new Cores();
	root->add(cores);

	win.open(&x, 0, -1, -1, 360, 140, "xcores", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(1);

	widgetXRun(&x, &win);	
	return 0;
}