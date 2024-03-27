#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <upng/upng.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/sys.h>
#include <ewoksys/ipc.h>
#include <ewoksys/klog.h>
#include <font/font.h>
#include <x++/X.h>
#include <ewoksys/timer.h>
#include <sysinfo.h>

using namespace Ewok;


class GCores : public XWin {
	static const uint32_t HEART_BIT_NUM = 32;
	uint32_t index;
	bool loop;
	sys_info_t sysInfo;
	uint32_t cores[MAX_CORE_NUM][HEART_BIT_NUM];
	const  uint32_t colors[6] = {
		0xff000000, //BLACK
		0xffff0000, //RED
		0xff00ff00, //GREEN
		0xff0000ff, //BLUE
		0xffff0088, //PURPLE
		0xff00ffff //CYAN
	};
public:
	inline GCores() {
		index = 0;
		loop = false;
		memset(cores, 0, sizeof(cores));
		memset(&sysInfo, 0, sizeof(sys_info_t));
	}
	
	inline ~GCores() {
	}

	void update_cores() {
		sys_get_sys_info(&sysInfo);

		for(uint32_t i=0; i<sysInfo.cores; i++) {
			cores[i][index] = sysInfo.core_idles[i];
		}

		index++;
		if(index >= HEART_BIT_NUM) {
			loop = true;
			index = 0;
		}
		repaint();
	}

protected:
	void drawTitle(graph_t* g, uint32_t i, uint32_t color) {
		int x = 10 + i*60;
		graph_fill(g, x, 4, 10, 10, color);
		char s[16];
		snprintf(s, 15, "%d:%d%%", i, 100 - (sysInfo.core_idles[i]/10000));
		graph_draw_text_font(g, x+12, 4, s, theme.getFont(), color);
	}

	void drawChat(graph_t* g, uint32_t i, float xstep, float yzoom, uint32_t color) {
		int last_x = 0;
		int last_y = 0;
		uint32_t num = loop ? HEART_BIT_NUM:index;
		for(uint32_t j=0; j<num; j++) {
			uint32_t k = j;
			if(loop)
				k = j+index;
			if(k >= HEART_BIT_NUM)
				k -= HEART_BIT_NUM;

			int x = j*xstep;
			int y = g->h - 10 - yzoom*(100 - (cores[i][k] / 10000)); //percentage
			if(last_y == 0)
				last_y = y;

			graph_wline(g, 10+last_x, last_y, 10+x, y, color, 3);
			last_x = x;
			last_y = y;
		}
	}

	void drawBG(graph_t* g, float xstep, float yzoom) {
		uint32_t color = 0xffdddddd;
		uint32_t w = xstep*HEART_BIT_NUM;
		uint32_t h = yzoom*100;

		for(uint32_t i=0; i<=HEART_BIT_NUM; i++) {
			graph_line(g, 10+i*xstep, g->h-10, 10+ i*xstep, g->h-10-h, color);
		}

		for(uint32_t i=0; i<=10; i++) {
			graph_line(g, 10, g->h-10-i*10*yzoom, w+10, g->h-10-i*10*yzoom, color);
		}

	}

	void onRepaint(graph_t* g) {
		if(sysInfo.cores == 0)
			return;

		graph_clear(g, 0xffffffff);

		float xstep = (g->w - 10*2)/ (float)HEART_BIT_NUM;
		float yzoom = (g->h - 20 - 10)/ 100.0;
		if(yzoom == 0)
			yzoom = 1.0;

		drawBG(g, xstep, yzoom);
		for(uint32_t i=0; i<sysInfo.cores; i++) {
			uint32_t color = colors[i%6];
			drawTitle(g, i, color);
			drawChat(g, i, xstep, yzoom, color);
		}
	}
};


static GCores* _xwin = NULL;
static void timer_handler(void) {
	_xwin->update_cores();
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	X x;
	GCores xwin;
	x.open(0, &xwin, -1, -1, 300, 210, "gcores", XWIN_STYLE_NORMAL);
	xwin.setVisible(true);

	_xwin = &xwin;
	uint32_t tid = timer_set(500000, timer_handler);
	x.run(NULL, &xwin);
	timer_remove(tid);

	//x.run(loop, &xwin);
	return 0;
} 
