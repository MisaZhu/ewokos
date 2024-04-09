#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <Widget/Split.h>
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

class Procs: public Scrollable {
	int off_y;
	int coreIndex;
	uint32_t procsH;
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.docBGColor);

		font_t* font = theme->getFont();
		if(font == NULL)
			return;

		int32_t y = 0;
		int32_t x = r.x;
		int num = 0;
		procinfo_t* procs = ps(num);
		if(procs == NULL)
			return;
		
		for(int i=0; i<num; i++) {
			procinfo_t* proc = &procs[i];
			if(proc != NULL) {
				if(coreIndex >= 0 && coreIndex != proc->core)
					continue;

				const char* owner = get_owner(proc);
				graph_draw_text_font(g, x, y - off_y, owner,
						font, theme->basic.fontSize, theme->basic.docFGColor);
				x += 64;

				char s[16] = { 0 };
				snprintf(s, 15, "%d", proc->pid);
				graph_draw_text_font(g, x, y - off_y, s,
						font, theme->basic.fontSize, theme->basic.docFGColor);
				x += 32;

				const char* loading = get_core_loading(proc);
				graph_draw_text_font(g, x, y - off_y, loading,
						font, theme->basic.fontSize, theme->basic.docFGColor);
				x += 48;

				const char* state = get_state(proc);
				graph_draw_text_font(g, x, y - off_y, state,
						font, theme->basic.fontSize, theme->basic.docFGColor);
				x += 72;

				snprintf(s, 15, "%d", (proc->heap_size / 1024));
				graph_draw_text_font(g, x, y - off_y, s,
						font, theme->basic.fontSize, theme->basic.docFGColor);
				x += 64;

				graph_draw_text_font(g, x, y - off_y, proc->cmd,
						font, theme->basic.fontSize, theme->basic.docFGColor);

				x = r.x;
				y += theme->basic.fontSize;
			}
		}

		free(procs);

		procsH = y;
		updateScroller();
	}

	bool onScroll(int step, bool horizontal) {
		int old_off = off_y;
		off_y -= step*16;

		if(step < 0) {
			if((off_y + area.h) >= procsH)
				off_y = procsH - area.h;
		}

		if(off_y < 0)
			off_y = 0;
		
		if(off_y == old_off)
			return false;

		return true;
	}

	void updateScroller() {
		if(procsH <= area.h && off_y > 0)
			setScrollerInfo(area.h + off_y, off_y, area.h, false);
		else
			setScrollerInfo(procsH, off_y, area.h, false);
	}

	bool onMouse(xevent_t* ev) {
		bool ret = Scrollable::onMouse(ev);

		if(ev->state == XEVT_MOUSE_MOVE) {
			if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
				scroll(-1, defaultScrollType == SCROLL_TYPE_H);
				ret = true;
			}
			else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
				scroll(1, defaultScrollType == SCROLL_TYPE_H);
				ret = true;
			}
		}
		return ret;
	}

public: 
	Procs() {
		coreIndex = 0;
		off_y = 0;
		procsH = 0;
		coreIndex = -1;
	}

	void setCore(uint32_t index) {
		if(index == 0)
			coreIndex = -1;
		else
			coreIndex = index - 1;
		off_y = 0;
		update();
	}

	void onTimer(uint32_t timerFPS) {
		update();
	}
};

class CoreList: public List {
	Procs* procs;
	uint32_t coreNum;
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

		char s[8] = {0};
		if(index == 0)
			snprintf(s, 7, "all");
		else
			snprintf(s, 7, "core%d", index-1);
		graph_draw_text_font(g, r.x+2, r.y+2, s, theme->getFont(), theme->basic.fontSize, color);
	}

	void onSelect(int index) {
		if(procs == NULL)
			return;
		procs->setCore(index);
	}
public:
	CoreList() {
		coreNum = 1;
		procs = NULL;
	}

	void loadCores() {
		sys_info_t info;
		sys_get_sys_info(&info);
		coreNum = info.cores;
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
	list->fix(60, 0);
	root->add(list);

	Procs* procs = new Procs();
	root->add(procs);
	list->setprocs(procs);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	procs->setScrollerV(scrollerV);

	win.open(&x, 0, -1, -1, 460, 460, "xfont", XWIN_STYLE_NORMAL);
	win.setTimer(1);
	x.run(NULL, &win);
	return 0;
}