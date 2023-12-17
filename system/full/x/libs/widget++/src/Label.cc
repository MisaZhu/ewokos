#include <Widget/Label.h>

using namespace EwokSTL;
namespace Ewok {

void Label::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	graph_draw_text_font_align(g, r.x+marginH, r.y+marginV, r.w-marginH*2, r.h-marginV*2,
				label.c_str(), getFont(theme), theme->fgColor, FONT_ALIGN_CENTER);
}

Label::Label(const string& str) {
	label = str;
	alpha = true;
}

Label::~Label(void) {
}

void Label::setLabel(const string& str) {
	label = str;
	update();
}

}