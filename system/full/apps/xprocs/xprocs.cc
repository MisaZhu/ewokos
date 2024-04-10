#include <Widget/WidgetWin.h>
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
#include <upng/upng.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>

#include <string>
#include <vector>
using namespace EwokSTL;

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
	else 
		strcpy(ret, _states[proc->state]);
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
	if(num > 0 && procs != NULL && syscall2(SYS_GET_PROCS, num, (int)procs) == 0) {
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
		}
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
		if(procNum == 0 || procs == NULL || coreIndex < 0)
			return;
		procinfo_t* procs0 = (procinfo_t*)malloc(sizeof(procinfo_t)*procNum);
		int num = 0;
		for(int i=0;i <procNum; i++) {
			if(procs[i].core != coreIndex)
				continue;
			memcpy(&procs0[num], &procs[i], sizeof(procinfo_t));
			num++;
		}
		free(procs);
		procs = procs0;
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

	void onTimer(uint32_t timerFPS) {
		if(procs != NULL)
			free(procs);
		procs = ps(procNum);
		if(procNum == 0)
			return;

		filter();
		rowNum = procNum;
		update();
	}
};

class CoreList: public List {
	Procs* procs;
	uint32_t coreNum;
	sys_info_t sysInfo;
protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index > coreNum)
			return;

		uint32_t color = theme->basic.fgColor;
		if(index == itemSelected) {
			graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.selectBGColor);
			color = theme->basic.selectColor;
		}

		char s[16] = {0};
		if(index == 0)
			snprintf(s, 15, "all");
		else
			snprintf(s, 15, "core%2d: %d%%", index-1, 100 - (sysInfo.core_idles[index-1]/10000));
		graph_draw_text_font(g, r.x+2, r.y+2, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void onSelect(int index) {
		if(procs == NULL)
			return;
		procs->setCore(index);
	}

	void onTimer(uint32_t timerFPS) {
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

	CoreList* list = new CoreList();
	list->loadCores();
	list->setItemSize(20);
	list->fix(100, 0);
	root->add(list);

	Procs* procs = new Procs();
	root->add(procs);
	list->setprocs(procs);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	procs->setScrollerV(scrollerV);

	win.open(&x, 0, -1, -1, 640, 480, "xprocs", XWIN_STYLE_NORMAL);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}