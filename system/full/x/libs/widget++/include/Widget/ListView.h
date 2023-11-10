#ifndef LISTVIEW_HH
#define LISTVIEW_HH

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <graph/graph.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <sys/klog.h>

using namespace Ewok;

typedef struct {
	int rows;
	int cols;
	int marginH;
	int marginV;
	gsize_t itemSize;
} ListViewItemsT;

class ListView {
	void paintItem(graph_t* g, int index, int x, int y, int w, int h) {
		graph_set_clip(g, x, y, w, h);
		drawItem(g, index, x, y, w, h);
	}

protected:
	const uint8_t POS_BOTTOM = 0;
	const uint8_t POS_TOP = 1;
	const uint8_t POS_LEFT = 2;
	const uint8_t POS_RIGHT = 3;

	ListViewItemsT itemsInfo;
	int selected;
	int start;
	bool focused;
	gsize_t size;
	uint8_t position;

	virtual void drawItem(graph_t* g, int index, int x, int y, int w, int h) = 0;
	virtual void drawBG(graph_t* g, const gpos_t& pos) = 0;
	virtual void onClick(uint32_t sel) = 0;
	virtual uint32_t getItemNum() = 0;

	void onRepaint(graph_t* g) {
		grect_t gr = { 0, 0, g->w, g->h };
		gpos_t pos = getPos(gr);
		drawBG(g, pos);
		uint32_t itemNum = getItemNum();

		int i, j, itemH, itemW;
		itemH = (size.h - itemsInfo.marginV) / itemsInfo.rows;
		itemW = (size.w - itemsInfo.marginH) / itemsInfo.cols;
			
		if(position == POS_TOP || position == POS_BOTTOM) {
			for(j=0; j<itemsInfo.rows; j++) {
				for(i=0; i<itemsInfo.cols; i++) {
					int at = j*itemsInfo.cols + i + start;
					if(at >= itemNum)
						return;
					int x = i*itemW + itemsInfo.marginH/2;
					int y = j*itemH + itemsInfo.marginV/2;
					paintItem(g, at, x + pos.x, y + pos.y, itemW, itemH);
				}
			}
		}
		else {
			for(i=0; i<itemsInfo.cols; i++) {
				for(j=0; j<itemsInfo.rows; j++) {
					int at = i*itemsInfo.rows + j + start;
					if(at >= itemNum)
						return;
					int x;
					if(position == POS_RIGHT)
						x  = (itemsInfo.cols-i-1)*itemW + itemsInfo.marginH/2;
					else
						x  = i*itemW + itemsInfo.marginH/2;
					int y = j*itemH + itemsInfo.marginV/2;
					paintItem(g, at, x + pos.x, y + pos.y, itemW, itemH);
				}
			}
		}
	}

	void onIM(xevent_t* ev) {
		uint32_t itemNum = getItemNum();
		int key = ev->value.im.value;
		if(ev->state == XIM_STATE_PRESS) {
			if(position == POS_TOP || position == POS_BOTTOM) {
				if(key == KEY_LEFT)
					selected--;
				else if(key == KEY_RIGHT)
					selected++;
				else if(key == KEY_UP)
					selected -= itemsInfo.cols;
				else if(key == KEY_DOWN)
					selected += itemsInfo.cols;
				else
					return;
			}
			else if(position == POS_LEFT) {
				if(key == KEY_LEFT)
					selected -= itemsInfo.rows;
				else if(key == KEY_RIGHT)
					selected += itemsInfo.rows;
				else if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
			else {
				if(key == KEY_LEFT)
					selected += itemsInfo.rows;
				else if(key == KEY_RIGHT)
					selected -= itemsInfo.rows;
				else if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
		}
		else {//XIM_STATE_RELEASE
			if(key == KEY_ENTER || key == KEY_BUTTON_START || key == KEY_BUTTON_A) {
				onClick(selected);
			}
			return;
		}

		if(selected >= (itemNum-1))
			selected = itemNum-1;
		if(selected < 0)
			selected = 0;
		
		if(selected < start) {
			start -= itemsInfo.cols*itemsInfo.rows;
			if(start < 0)
				start = 0;
		}
		else if((selected - start) >= itemsInfo.cols*itemsInfo.rows) 
			start += itemsInfo.cols*itemsInfo.rows;
	}

	void onMouse(xevent_t* ev, const grect_t& r) {
		uint32_t itemNum = getItemNum();
		gpos_t pos = getPos(r);
		int itemW = size.w / itemsInfo.cols;
		int itemH = size.h / itemsInfo.rows;
		int col = (ev->value.mouse.x - pos.x) / itemW;
		int row = (ev->value.mouse.y - pos.y) / itemH;
		int at;
		if(position == POS_TOP || position == POS_BOTTOM) 
			at = row*itemsInfo.cols + col + start;
		else if(position == POS_LEFT) 
			at = col*itemsInfo.rows + row + start;
		else
			at = (itemsInfo.cols-col-1)*itemsInfo.rows + row + start;
		if(at >= itemNum)
			return;

		if(ev->state == XEVT_MOUSE_DOWN) {
			if(selected != at) {
				selected = at;
			}
		}
		else if(ev->state == XEVT_MOUSE_CLICK) {
			onClick(at);
			return;
		}
	}

	void onEvent(xevent_t* ev, const grect_t& scr) {
		if(ev->type == XEVT_MOUSE) {
			onMouse(ev, scr);
		}
		else if(ev->type == XEVT_IM) {
			onIM(ev);
		}
	}

public:

	inline ListView() {
		selected = 0;
		start = 0;
		memset(&size, 0, sizeof(gsize_t));
		memset(&itemsInfo, 0, sizeof(ListViewItemsT));
	}

	virtual ~ListView() { }

	inline const gsize_t& getSize(void) {
		return size;
	}

	void layout(const grect_t& r) {
		uint32_t itemNum = getItemNum();
		if(position == POS_TOP || position == POS_BOTTOM) {
			int max = (r.w - itemsInfo.marginH) / (itemsInfo.itemSize.w + itemsInfo.marginH);
			if(itemNum > max)
				itemsInfo.cols = max;
			else
				itemsInfo.cols = itemNum;
			if(itemsInfo.cols > 0)
				itemsInfo.rows = itemNum / itemsInfo.cols;
			if((itemsInfo.cols*itemsInfo.rows) != itemNum)
				itemsInfo.rows++;
		}
		else {
			int max = (r.h - itemsInfo.marginV) / (itemsInfo.itemSize.h + itemsInfo.marginV);
			if(itemNum > max)
				itemsInfo.rows = max;
			else
				itemsInfo.rows = itemNum;
			if(itemsInfo.rows > 0)
				itemsInfo.cols = itemNum / itemsInfo.rows;
			if((itemsInfo.cols*itemsInfo.rows) != itemNum)
				itemsInfo.cols++;
		}

		size.h = (itemsInfo.itemSize.h + itemsInfo.marginV) * itemsInfo.rows + itemsInfo.marginV;
		size.w = (itemsInfo.itemSize.w + itemsInfo.marginH) * itemsInfo.cols + itemsInfo.marginH;
	}

	inline gpos_t getPos(const grect_t& scr) {
		gpos_t pos;
		if(position == POS_BOTTOM) {
			pos.x = scr.x + (scr.w-size.w)/2;
			pos.y = scr.y + scr.h - size.h;
		}
		else if(position == POS_TOP) {
			pos.x = scr.x;
			pos.y = scr.y;
		}
		else if(position == POS_LEFT) {
			pos.x = scr.x;
			pos.y = scr.y;
		}
		else if(position == POS_RIGHT) {
			pos.x = scr.x + scr.w - size.w;
			pos.y = scr.y;
		}
		return pos;
	}

	inline void repaint(graph_t* g) { onRepaint(g); }

	inline void handleEvent(xevent_t* ev, const grect_t& scr) { onEvent(ev, scr); }
};

#endif