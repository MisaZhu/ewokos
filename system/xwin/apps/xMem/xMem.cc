#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>

using namespace Ewok;

class Memory : public Widget {
static const uint32_t DATA_POINTS = 64;
uint32_t index;
bool loop;
sys_info_t sysInfo;
sys_state_t sysState;
uint32_t memData[DATA_POINTS];

int x_off;
int y_off_bottom;
int y_off;

static const uint32_t COLOR_NUM = 3;
const  uint32_t colors[COLOR_NUM] = {
	0xff000000, 
	0xff00ff00, // Free memory
	0xffff0000, // Used memory
};
public:
inline Memory() {
	index = 0;
	loop = false;
	x_off = 10;
	y_off = 20;
	y_off_bottom = 10;
	memset(memData, 0, sizeof(memData));
	memset(&sysInfo, 0, sizeof(sys_info_t));
	memset(&sysState, 0, sizeof(sys_state_t));
}

inline ~Memory() {
}

void updateMemory() {
	syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)(uint64_t)&sysInfo);
	syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)(uint64_t)&sysState);

	// Calculate used memory percentage
	uint32_t totalMem = sysInfo.total_usable_mem_size / (1024*1024);
	uint32_t freeMem = sysState.mem.free / (1024*1024);
	uint32_t usedMem = totalMem - freeMem;
	uint32_t usedPercent = (totalMem > 0) ? (usedMem * 100 / totalMem) : 0;

	memData[index] = usedPercent;

	index++;
	if(index >= DATA_POINTS) {
		loop = true;
		index = 0;
	}
	update();
}

protected:
void drawTitle(graph_t* g, XTheme* theme, const grect_t& r) {
	char s[64];
	uint32_t totalMem = sysInfo.total_usable_mem_size / (1024*1024);
	uint32_t freeMem = sysState.mem.free / (1024*1024);
	uint32_t usedMem = totalMem - freeMem;
	uint32_t usedPercent = (totalMem > 0) ? (usedMem * 100 / totalMem) : 0;

	snprintf(s, 63, "Mem: %d/%d MB (%d%%)", usedMem, totalMem, usedPercent);
	graph_draw_text_font(g, r.x + x_off, r.y + 2, s, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
}

void drawGraph(graph_t* g, float xstep, float yzoom, const grect_t& r) {
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
		graph_wline(g, r.x+x_off+last_x, r.y+last_y, r.x+x_off+x, r.y+usedY, colors[2], 2);
		last_x = x;
		last_y = usedY;
	}
}

void drawBG(graph_t* g, XTheme* theme, float xstep, float yzoom, const grect_t& r) {
	uint32_t color = 0xffbbbbbb;
	uint32_t bgColor = 0xff888888;
	uint32_t w = xstep * DATA_POINTS;
	uint32_t h = yzoom * 100;

	graph_fill(g, r.x + x_off, r.y + r.h - h - y_off_bottom,
			w, h, bgColor);

	// Draw vertical grid lines
	for(uint32_t i=0; i<=DATA_POINTS; i++) {
		graph_line(g, r.x + x_off + i*xstep, r.y + r.h - y_off_bottom,
			r.x + x_off + i*xstep, r.y + r.h - y_off_bottom - h, color);
	}

	// Draw horizontal grid lines
	for(uint32_t i=0; i<=10; i++) {
		graph_line(g, r.x + x_off, r.y + r.h - y_off_bottom - i*10*yzoom,
			r.x + w + x_off, r.y + r.h - y_off_bottom - i*10*yzoom, color);
		
		// Draw percentage labels
		char s[8];
		snprintf(s, 7, "%d%%", i*10);
		graph_draw_text_font(g, r.x + x_off - 30, r.y + r.h - y_off_bottom - i*10*yzoom - 5, 
			s, theme->getFont(), theme->basic.fontSize, color);
	}
}

void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);
	float xstep = (r.w - x_off*2) / (float)DATA_POINTS;
	float yzoom = (r.h - y_off - y_off_bottom) / 100.0f;
	if(yzoom <= 0.0)
		yzoom = 1.0;
	if(yzoom > 10.0)
		yzoom = 10.0;

	drawBG(g, theme, xstep, yzoom, r);
	drawGraph(g, xstep, yzoom, r);
	drawTitle(g, theme, r);
}

void onTimer(uint32_t timerFPS, uint32_t timerStep) {
	updateMemory();
}
};


int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);


	Memory* memory = new Memory();
	root->add(memory);

	win.open(&x, -1, -1, -1, 240, 80, "xMem", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT);
	win.setTimer(2);

	widgetXRun(&x, &win);
	return 0;
}
