#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Split.h>
#include <Widget/Columns.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph_png.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>

#include <string>
#include <vector>
using namespace std;

using namespace Ewok;

static const char* _states[] = {
	"unu",
	"crt",
	"slp",
	"wat",
	"blk",
	"rdy",
	"run",
	"zmb"
};

static const char* get_state(procinfo_t* proc) {
	static char ret[16];
	if(proc->state == 4) {
		if(proc->block_by < 0)
			strcpy(ret, "blk[kev]");
		else 
			snprintf(ret, 13, "blk[%d]", proc->block_by);
	}
	else if(proc->state == 3)
		snprintf(ret, 13, "wat[%d]", proc->wait_for);
	else  {
		strcpy(ret, _states[proc->state]);
		if((proc->state == 5 || proc->state == 6) && proc->priority > 0)
			snprintf(ret, 13, "%s(%d)", ret, proc->priority);
	}
	return ret;
}

static const char* get_owner(procinfo_t* proc) {
	if(proc->uid < 0)
		return "kernel";

	session_info_t info;
	static char name[SESSION_USER_MAX+1] = {0};

	if(session_get_by_uid(proc->uid, &info) == 0)
		strncpy(name, info.user, SESSION_USER_MAX);
	else
		snprintf(name, SESSION_USER_MAX, "%d", proc->uid);

	return name;
}

static const char* get_core_loading(procinfo_t* proc) {
	static char ret[8];
	snprintf(ret, 7, "%d:%d%%", proc->core, proc->run_usec/10000);
	return ret;
}

static procinfo_t* ps(int &num) {
	num = syscall0(SYS_GET_PROCS_NUM);
	procinfo_t* procs = (procinfo_t*)malloc(sizeof(procinfo_t)*num);
	if(num > 0 && procs != NULL && syscall2(SYS_GET_PROCS, (ewokos_addr_t)num, (ewokos_addr_t)(uint64_t)procs) == 0) {
		return procs;
	}
	else {
		num = 0;
		return NULL;
	}
}

class Procs: public Columns {
	int coreIndex;
	procinfo_t* procs;
	int procNum;
protected:
	void drawItem(graph_t* g, XTheme* theme, int row, int col, const grect_t& r) {
		font_t* font = theme->getFont();
		if(font == NULL || procs == NULL || row >= procNum)
			return;

		procinfo_t* proc = &procs[row];
		if(proc == NULL)
			return;

		char s[16] = { 0 };
		string str = "";

		if(col == 0) {
			str = get_owner(proc);
		}
		else if(col == 1) {
			snprintf(s, 15, "%d", proc->pid);
			str = s;
		}
		else if(col == 2) {
			str = get_core_loading(proc);
		}
		else if(col == 3) {
			str = get_state(proc);
		}
		else if(col == 4) {
			snprintf(s, 15, "%d", (proc->heap_size / 1024));
			str = s;
		}
		else if(col == 5) {
			str = proc->cmd;
			if(proc->type != TASK_TYPE_PROC)
				str += "[t]";
		}
		int load = proc->run_usec/10000;
		if(load >= 80)
			graph_fill(g, r.x, r.y, r.w, r.h, 0xffff6666);
		else if(load >= 60)
			graph_fill(g, r.x, r.y, r.w, r.h, 0xffffaaaa);
		else if(load >= 40)
			graph_fill(g, r.x, r.y, r.w, r.h, 0xffaaffaa);
		else if(load >= 20)
			graph_fill(g, r.x, r.y, r.w, r.h, 0xffaaaaff);
		else if(load >= 5)
			graph_fill(g, r.x, r.y, r.w, r.h, 0xffaaaaaa);

		graph_draw_text_font(g, r.x, r.y, str.c_str(),
			font, theme->basic.fontSize, theme->basic.docFGColor);
	}

	void build() {
		add("OWNER", 64);
		add("PID", 32);
		add("CPU", 48);
		add("STATE", 72);
		add("HEAP", 64);
		add("CMD", 0);
	}

	void filter() {
		if(procs == NULL || procNum == 0 || coreIndex < 0)
			return;

		procinfo_t* procsNew = (procinfo_t*)malloc(sizeof(procinfo_t)*procNum);
		int num = 0;
		for(int i=0; i<procNum; i++) {
			procinfo_t* proc = &procs[i];
			if(proc->core != coreIndex)
				continue;
			memcpy(&procsNew[num], &procs[i], sizeof(procinfo_t));
			num++;
		}

		free(procs);
		procs = procsNew;
		procNum = num;
	}

public: 
	Procs() {
		procs = NULL;
		coreIndex = -1;
		procNum = 0;
		build();
	}

	void setCore(uint32_t index) {
		if(index == 0)
			coreIndex = -1;
		else
			coreIndex = index - 1;
		off_y = 0;
		filter();
		update();
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if((timerStep % timerFPS) != 0)
			return;

		if(procs != NULL)
			free(procs);
		procs = ps(procNum);
		rowNum = procNum;
		filter();
		update();
	}
};

class Cores : public Widget {
	static const uint32_t HEART_BIT_NUM = 24;
	uint32_t index;
	bool loop;
	sys_info_t sysInfo;
	uint32_t cores[MAX_CORE_NUM][HEART_BIT_NUM];

	int x_off;
	int y_off;
public:
	inline Cores() {
		index = 0;
		loop = false;
		x_off = 4;
		y_off = 4;
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

	static inline uint32_t getColor(int32_t core) {
		static const uint32_t COLOR_NUM = 7;
		static uint32_t colors[COLOR_NUM] = {
			0xff0000ff, 
			0xff00ff00, 
			0xffff0000, 
			0xff8800ff,
			0xff0088ff,
			0xffff8800,
			0xff000000
		};
		return colors[core%COLOR_NUM];
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
		graph_draw_text_font(g, x+12, r.y+4, s, theme->getFont(), theme->basic.fontSize, color);
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
			int y = r.h - y_off - yzoom*(perc); //percentage
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

		graph_fill(g, r.x + x_off, r.y+ r.h - h - y_off,
				w, h, bgColor);

		for(uint32_t i=0; i<=HEART_BIT_NUM; i++) {
			graph_line(g, r.x + x_off + i*xstep, r.y + r.h - y_off,
					r.x + x_off + i*xstep, r.y + r.h - y_off - h, color);
		}

		for(uint32_t i=0; i<=10; i++) {
			graph_line(g, r.x + x_off, r.y + r.h - y_off - i*10*yzoom,
				r.x + w + x_off, r.y + r.h - y_off - i*10*yzoom, color);
		}

	}

	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		if(sysInfo.cores == 0)
			return;

		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor, true);

		float xstep = (r.w - x_off*2)/ (float)HEART_BIT_NUM;
		float yzoom = (r.h - y_off*2)/ 100.0;
		if(yzoom == 0)
			yzoom = 1.0;

		drawBG(g, xstep, yzoom, r);
		for(uint32_t i=0; i<sysInfo.cores; i++) {
			uint32_t color = getColor(i);
			drawChat(g, i, xstep, yzoom, color, r);
		}
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		updateCores();
	}
};

class CoreList: public List {
	Procs* procs;
	uint32_t coreNum;
	sys_info_t sysInfo;

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);

		sys_info_t sys_info;
		sys_state_t sys_state;
		syscall1(SYS_GET_SYS_INFO, (ewokos_addr_t)(uint64_t)&sys_info);
		syscall1(SYS_GET_SYS_STATE, (ewokos_addr_t)(uint64_t)&sys_state);
		uint32_t fr_mem = sys_state.mem.free / (1024*1024);
		uint32_t t_mem = sys_info.total_usable_mem_size / (1024*1024);
		char txt[32] = { 0 };
		snprintf(txt, 31, "%dM/%dM", fr_mem, t_mem);

		graph_draw_text_font(g, r.x , r.y + r.h - 16,
				txt, theme->getFont(), 12, 0xFF000000);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index > coreNum)
			return;

		uint32_t color = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			//color = theme->basic.selectColor;
		}

		char s[16] = {0};
		if(index == 0)
			snprintf(s, 15, "all");
		else {
			color = Cores::getColor(index-1);
			int32_t perc = 100 - (sysInfo.core_idles[index-1]/10000);
			if(perc < 0)
				perc = 0;
			snprintf(s, 15, "core%2d: %d%%", index-1, perc);
		}
		graph_draw_text_font(g, r.x+2, r.y+2, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void onSelect(int index) {
		if(procs == NULL)
			return;
		procs->setCore(index);
	}

	void onTimer(uint32_t timerFPS, uint32_t timerStep) {
		if((timerStep % timerFPS) != 0)
			return;

		loadCores();
		update();
	}
public:
	CoreList() {
		procs = NULL;
		loadCores();
	}

	void loadCores() {
		sys_get_sys_info(&sysInfo);
		coreNum = sysInfo.cores;
		setItemNum(coreNum+1);
	}

	void setprocs(Procs* procs) {
		this->procs = procs;
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);

	Container* c = new Container();
	c->setType(Container::VERTICLE);
	c->fix(120, 0);
	root->add(c);

	CoreList* list = new CoreList();
	list->loadCores();
	list->setItemSize(20);
	c->add(list);

	Cores* cores = new Cores();
	cores->fix(0, 72);
	c->add(cores);

	Procs* procs = new Procs();
	root->add(procs);
	list->setprocs(procs);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	procs->setScrollerV(scrollerV);

	win.open(&x, 0, -1, -1, 600, 400, "xprocs", XWIN_STYLE_NORMAL);
	win.setTimer(2);

	widgetXRun(&x, &win);
	return 0;
}