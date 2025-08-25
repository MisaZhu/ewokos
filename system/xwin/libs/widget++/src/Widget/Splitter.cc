#include <Widget/Splitter.h>
#include <Widget/Container.h>
#include <Widget/RootWidget.h>

namespace Ewok {

void Splitter::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	uint32_t d, b;
	graph_get_3d_color(theme->basic.bgColor, &d, &b);
	graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

	if(!horizontal) {
		int dx = (r.w - barSize)/2;
		graph_draw_dot_pattern(g, r.x+dx, r.y, barSize, r.h, theme->basic.bgColor, theme->basic.fgColor, 1, 1);
		graph_box_3d(g, r.x+dx, r.y, barSize, r.h, d, b);
	}
	else {
		int dy = (r.h - barSize)/2;
		graph_draw_dot_pattern(g, r.x, r.y+dy, r.w, barSize, theme->basic.bgColor, theme->basic.fgColor, 1, 1);
		graph_box_3d(g, r.x, r.y+dy, r.w, barSize, d, b);
	}

	graph_box_3d(g, r.x, r.y, r.w, r.h, b, d);
	return;
}

void Splitter::resizeAttach(int d) {
	Container* father = attachedWidget->getFather();
	if(father == NULL)
		return;

	grect_t father_area = father->getRootArea();
	grect_t r = attachedWidget->getRootArea();

	if(horizontal) {
		//if((r.x + r.w - d + width) <= (father_area.x + father_area.w))
			attachedWidget->resize(-d, 0);
	}
	else {
		//if((r.y + r.h - d + width) <= (father_area.y + father_area.h))
			attachedWidget->resize(0, -d);
	}
}

bool Splitter::moveSplit(xevent_t* ev) {
	if(attachedWidget == NULL || father == NULL || !fixed)
		return false;
	attachedWidget->setFixed(true);

	if(horizontal) {
		if(ev->state == MOUSE_STATE_DOWN) {
			last_mouse_down = ev->value.mouse.x;
			return true;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			int dx = last_mouse_down - ev->value.mouse.x;
			if(attachedAfter)
				dx = -dx;
			if(abs(dx) > step) {
				last_mouse_down = ev->value.mouse.x;
				resizeAttach(dx);
			}
			return true;
		}
	}
	else {
		if(ev->state == MOUSE_STATE_DOWN) {
			last_mouse_down = ev->value.mouse.y;
			return true;
		}
		else if(ev->state == MOUSE_STATE_DRAG) {
			int dy = last_mouse_down - ev->value.mouse.y;
			if(attachedAfter)
				dy = -dy;
			if(abs(dy) > step) {
				last_mouse_down = ev->value.mouse.y;
				resizeAttach(dy);
			}
			return true;
		}
	}
	return false;
}

bool Splitter::onMouse(xevent_t* ev) {
	if(attachedWidget == NULL) {
		Container* r = getRootContainer();
		if(r == NULL)
			return false;
		Widget* wd = r->getChild(attachedName);
		if(wd != NULL)
			attach(wd, attachedAfter);
	}
	if(attachedWidget == NULL)
		return false;

	return moveSplit(ev);
}

void Splitter::onAdd() {
	uint8_t type = father->getType();
	if(type == Container::FIXED)
		return;

	if(type == Container::HORIZONTAL)
		horizontal = true;
	else
		horizontal = false;
	
	setWidth(width);
}

void Splitter::setWidth(uint32_t w) {
	width = w;
	if(horizontal)
		fix(width, 0);
	else
		fix(0, width);
}

void Splitter::setBarSize(uint32_t bsize) {
	barSize = bsize;
	update();
}

void Splitter::setStep(uint32_t stp) {
	step = stp;
}

void Splitter::attach(Widget* wd, bool after) {
	attachedWidget = wd;
	attachedAfter = after;
}

Splitter::Splitter() {
	last_mouse_down = 0;
	width = 8;
	barSize = 16;
	step = 8;
	horizontal = false;
	attachedAfter = false;
	attachedWidget = NULL;
}

void Splitter::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "attach") {
		attachedName = json_var_get_str(value);
	}	
	else if(attr == "attachAfter") {
		attachedAfter = json_var_get_bool(value);
	}
	else if(attr == "width") {
		setWidth(json_var_get_int(value));
	}
}

}