#include <Widget/Split.h>
#include <Widget/Container.h>

namespace Ewok {

void Split::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	uint32_t d, b;
	graph_get_3d_color(theme->basic.widgetBGColor, &d, &b);
	graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor);

	if(!horizontal) {
		int dx = (r.w - barSize)/2;
		graph_draw_dot_pattern(g, r.x+dx, r.y, barSize, r.h, theme->basic.widgetBGColor, theme->basic.widgetFGColor, 1);
		graph_box_3d(g, r.x+dx, r.y, barSize, r.h, d, b);
	}
	else {
		int dy = (r.h - barSize)/2;
		graph_draw_dot_pattern(g, r.x, r.y+dy, r.w, barSize, theme->basic.widgetBGColor, theme->basic.widgetFGColor, 1);
		graph_box_3d(g, r.x, r.y+dy, r.w, barSize, d, b);
	}

	graph_box_3d(g, r.x, r.y, r.w, r.h, b, d);
	return;
}

bool Split::moveSplit(xevent_t* ev) {
	if(attachedWidget == NULL || father == NULL || !fixed)
		return false;
	attachedWidget->setFixed(true);

	if(horizontal) {
		if(ev->state == XEVT_MOUSE_DOWN) {
			last_mouse_down = ev->value.mouse.x;
			return true;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int dx = last_mouse_down - ev->value.mouse.x;
			if(abs(dx) > step) {
				last_mouse_down = ev->value.mouse.x;
				attachedWidget->resize(-dx, 0);
			}
			return true;
		}
	}
	else {
		if(ev->state == XEVT_MOUSE_DOWN) {
			last_mouse_down = ev->value.mouse.y;
			return true;
		}
		else if(ev->state == XEVT_MOUSE_DRAG) {
			int dy = last_mouse_down - ev->value.mouse.y;
			if(abs(dy) > step) {
				last_mouse_down = ev->value.mouse.y;
				attachedWidget->resize(0, -dy);
			}
			return true;
		}
	}
	return false;
}

bool Split::onMouse(xevent_t* ev) {
	if(attachedWidget == NULL)
		return false;

	return moveSplit(ev);
}

void Split::onAdd() {
	uint8_t type = father->getType();
	if(type == Container::FIXED)
		return;

	if(type == Container::HORIZONTAL)
		horizontal = true;
	else
		horizontal = false;
	
	setWidth(width);
}

void Split::setWidth(uint32_t w) {
	width = w;
	if(horizontal)
		fix(width, 0);
	else
		fix(0, width);
}

void Split::setBarSize(uint32_t bsize) {
	barSize = bsize;
	update();
}

void Split::setStep(uint32_t stp) {
	step = stp;
}

void Split::attach(Widget* wd) {
	attachedWidget = wd;
}

Split::Split() {
	last_mouse_down = 0;
	width = 8;
	barSize = 16;
	step = 8;
	horizontal = false;
	attachedWidget = NULL;
}

}