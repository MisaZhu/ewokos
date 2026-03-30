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
	sys_state_t sysState;
	uint32_t cores[MAX_CORE_NUM][HEART_BIT_NUM];

	// Memory monitoring data
	static const uint32_t DATA_POINTS = 64;
	uint32_t memData[DATA_POINTS];

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

	// Memory colors
	const  uint32_t memColors[3] = {
		0xff000000, 
		0xff00ff00, // Free memory
		0xffff0000, // Used memory
	};
public:
	inline Cores() {
		index = 0;
		loop = false;
		x_off = 6;
		y_off = 18;
		y_off_bottom = 6;
		memset(cores, 0, sizeof(cores));
		memset(memData, 0, sizeof(memData));
		memset(&sysInfo, 0, sizeof(sys_info_t));
		memset(&sysState, 0, sizeof(sys_state_t));
	}
	
	inline ~Cores() {
	}

	void updateCores() {
		sys_get_sys_info(&sysInfo);
		syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)(uint64_t)&sysState);

		for(uint32_t i=0; i<sysInfo.cores; i++) {
			cores[i][index] = sysInfo.core_idles[i];
		}

		// Update memory data
		uint32_t totalMem = sysInfo.total_usable_mem_size / (1024*1024);
		uint32_t freeMem = sysState.mem.free / (1024*1024);
		uint32_t usedMem = totalMem - freeMem;
		uint32_t usedPercent = (totalMem > 0) ? (usedMem * 100 / totalMem) : 0;

		memData[index] = usedPercent;

		index++;
		if(index >= HEART_BIT_NUM) {
			loop = true;
			index = 0;
		}
		update();
	}

protected:
	void drawTitle(graph_t* g, XTheme* theme, uint32_t i, uint32_t color, const grect_t& r) {
		//int x = r.x + x_off + i*60;
		int x = r.x + i*(theme->basic.fontSize*3+4) + theme->basic.fontSize*5;
		graph_fill(g, x, r.y+4, 10, 10, color);
		char s[16];
		int32_t perc = 100 - (sysInfo.core_idles[i]/10000);
		if(perc < 0)
			perc = 0;
		snprintf(s, 15, "%d%%", perc);
		//snprintf(s, 15, "%d", i);
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

			graph_wline(g, r.x+x_off+last_x, r.y+last_y, r.x+x_off+x, r.y+y, 2, color);
			last_x = x;
			last_y = y;
		}
	}

	void drawBG(graph_t* g, XTheme* theme, float xstep, float yzoom, const grect_t& r, bool isMem = false) {
		uint32_t color = 0xffbbbbbb;
		uint32_t bgColor = 0xff888888;
		uint32_t w = xstep * (isMem ? DATA_POINTS : HEART_BIT_NUM);
		uint32_t h = yzoom * 100;

		graph_fill(g, r.x + x_off, r.y + r.h - h - y_off_bottom,
				w, h, bgColor);

		uint32_t points = isMem ? DATA_POINTS : HEART_BIT_NUM;
		for(uint32_t i=0; i<=points; i++) {
			graph_line(g, r.x + x_off + i*xstep, r.y + r.h - y_off_bottom,
					r.x + x_off + i*xstep, r.y + r.h - y_off_bottom - h, color);
		}

		for(uint32_t i=0; i<=10; i++) {
			graph_line(g, r.x + x_off, r.y + r.h - y_off_bottom - i*10*yzoom,
				r.x + w + x_off, r.y + r.h - y_off_bottom - i*10*yzoom, color);
		}
	}

	void drawMemTitle(graph_t* g, XTheme* theme, const grect_t& r) {
		char s[64];
		uint32_t totalMem = sysInfo.total_usable_mem_size / (1024*1024);
		uint32_t freeMem = sysState.mem.free / (1024*1024);
		uint32_t usedMem = totalMem - freeMem;
		uint32_t usedPercent = (totalMem > 0) ? (usedMem * 100 / totalMem) : 0;

		char usedMemStr[8] = {0};
		get_mem_size_desc(usedMem*1024*1024, usedMemStr);
		char totalMemStr[8] = {0};
		get_mem_size_desc(totalMem*1024*1024, totalMemStr);

		snprintf(s, 63, "Mem: %s/%s (%d%%)", usedMemStr, totalMemStr, usedPercent);
		graph_draw_text_font(g, r.x + x_off, r.y - y_off_bottom + 2, s, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
	}

	void drawMemGraph(graph_t* g, float xstep, float yzoom, const grect_t& r) {
		int last_x = 0;
		int last_y = -1;
		uint32_t num = loop ? DATA_POINTS : index;
		
		for(uint32_t j=0; j<num; j++) {
			uint32_t k = j;
			if(loop)
				k = j + index;
			if(k >= DATA_POINTS)
				k -= DATA_POINTS;

			int x = (j+1) * xstep;
			int usedY = r.h - y_off_bottom - yzoom * memData[k];
			
			if(last_y < 0)
				last_y = usedY;

			// Draw used memory line
			graph_wline(g, r.x+x_off+last_x, r.y+usedY, r.x+x_off+x, r.y+usedY, 2, memColors[2]);
			last_x = x;
			last_y = usedY;
		}
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);

		// Calculate layout: top part for CPU cores, bottom part for memory with 2px gap
		int cpuHeight = r.h * 3 / 5;
		int memHeight = r.h - cpuHeight - 2; // 2px gap between CPU and memory parts

		// CPU cores part
		grect_t cpuRect = { r.x, r.y, r.w, cpuHeight };
		float cpuXStep = (cpuRect.w - x_off*2) / (float)HEART_BIT_NUM;
		float cpuYZoom = (cpuRect.h - y_off - y_off_bottom) / 100.0f;
		if(cpuYZoom <= 0.0)
			cpuYZoom = 1.0;
		if(cpuYZoom > 10.0)
			cpuYZoom = 10.0;

		drawBG(g, theme, cpuXStep, cpuYZoom, cpuRect, false);

		graph_draw_text_font(g, x_off, r.y, "cores:", theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
		if(sysInfo.cores > 0) {
			for(uint32_t i=0; i<sysInfo.cores; i++) {
				uint32_t color = colors[i%COLOR_NUM];
				drawTitle(g, theme, i, color, cpuRect);
				drawChat(g, i, cpuXStep, cpuYZoom, color, cpuRect);
			}
		}

		// Memory part
		grect_t memRect = { r.x, r.y + cpuHeight, r.w, memHeight }; // 2px gap below CPU part
		float memXStep = (memRect.w - x_off*2) / (float)DATA_POINTS;
		float memYZoom = (memRect.h - y_off) / 100.0f;
		if(memYZoom <= 0.0)
			memYZoom = 1.0;
		if(memYZoom > 10.0)
			memYZoom = 10.0;

		drawBG(g, theme, memXStep, memYZoom, memRect, true);
		drawMemGraph(g, memXStep, memYZoom, memRect);
		drawMemTitle(g, theme, memRect);
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

	win.open(&x, -1, -1, -1, 240, 120, "xcores", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(2);

	widgetXRun(&x, &win);	
	return 0;
}