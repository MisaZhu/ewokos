#include <Widget/Blank.h>

namespace Ewok {

void Blank::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor, false);
	return;
}

}