#include <Widget/Split.h>
#include <Widget/Container.h>

namespace Ewok {

void Split::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	uint32_t d, b;
	graph_get_3d_color(theme->basic.bgColor, &d, &b);
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

	if(horizontal) {
		int dx = (r.w - 2)/2;
		graph_line(g, r.x+dx, r.y, r.x+dx, r.y+r.h, d);
		dx++;
		graph_line(g, r.x+dx, r.y, r.x+dx, r.y+r.h, b);
	}
	else {
		int dy = (r.h - 2)/2;
		graph_line(g, r.x, r.y+dy, r.x+r.w, r.y+dy, d);
		dy++;
		graph_line(g, r.x, r.y+dy, r.x+r.w, r.y+dy, b);
	}
	return;
}

Split::Split() {
	horizontal = false;
	width = 6;
}

void Split::setWidth(uint32_t w) {
	width = w;
	if(horizontal)
		fix(width, 0);
	else
		fix(0, width);
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

void Split::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "width") {
		setWidth(json_var_get_int(value));
	}
}

}