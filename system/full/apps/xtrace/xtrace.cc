#include <Widget/WidgetWin.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <upng/upng.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>

using namespace Ewok;


int traces;

/*int main (int argc, char **argv) {
  sys_info_t sys_info;
  sys_get_sys_info(&sys_info);

  while(true) {
    traces = syscall1(SYS_GET_TRACE, (int)pids);
    printf("%d\n", traces);
    for(uint32_t trace=0; trace<traces; trace++) {
      for(uint32_t core=0; core<sys_info.cores; core++) {
        printf("%8d - %d:%d ", trace, core, pids[core][trace]);
      }
      printf("\n");
    }
    usleep(500000);
  }
  return 0;
}
*/

class Cores : public Widget {
	uint32_t index;
	sys_info_t sysInfo;
	int *pids;
	int fps;
	int traces;

	int x_off;
	int y_off_bottom;
	int y_off;

	static const uint32_t COLOR_NUM = 7;
	const  uint32_t colors[COLOR_NUM] = {
		0xff0000ff, 
		0xff00ff00, 
		0xffff0000, 
		0xff8800ff,
		0xff0088ff,
		0xffff8800,
		0xff000000
	};
public:
	inline Cores() {
		index = 0;
		x_off = 10;
		y_off = 20;
		y_off_bottom = 10;
    	fps = syscall0(SYS_GET_TRACE_FPS);
		sys_get_sys_info(&sysInfo);
		pids = (int*)malloc(sysInfo.cores * fps * 4);
	}
	
	inline ~Cores() {
		if(pids != NULL)
			free(pids);
	}

	void updateCores() {
		sys_get_sys_info(&sysInfo);
    	traces = syscall1(SYS_GET_TRACE, (int)pids);
		update();
	}

protected:
	void drawTitle(graph_t* g, XTheme* theme, uint32_t i, uint32_t color, const grect_t& r) {
		int x = r.x + x_off + i*60;
		graph_fill(g, x, r.y+4, 10, 10, color);
		char s[16];
		snprintf(s, 15, "%d:%d%%", i, 100 - (sysInfo.core_idles[i]/10000));
		graph_draw_text_font(g, x+12, r.y+4, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void drawChat(graph_t* g, uint32_t i, uint32_t color, const grect_t& r) {
	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(sysInfo.cores == 0 || pids == NULL)
			return;

		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor, true);

		for(uint32_t i=0; i<sysInfo.cores; i++) {
			uint32_t color = colors[i%COLOR_NUM];
			drawTitle(g, theme, i, color, r);
			drawChat(g, i, color, r);
		}
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

	win.open(&x, 0, -1, -1, 320, 140, "xtrace", XWIN_STYLE_NORMAL);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}