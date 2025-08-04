#include <Widget/Blank.h>

namespace Ewok {

void Blank::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
}

Blank::Blank() {
}

}