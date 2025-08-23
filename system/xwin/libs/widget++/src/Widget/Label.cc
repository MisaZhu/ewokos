#include <Widget/Label.h>
#include <x++/XTheme.h>

using namespace std;
namespace Ewok {

void Label::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	graph_draw_text_font_align(g, r.x+marginH, r.y+marginV, r.w-marginH*2, r.h-marginV*2,
				label.c_str(), font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
}

Label::Label(const string& str) {
	label = str;
}

Label::~Label(void) {
}

void Label::setLabel(const string& str) {
	label = str;
	update();
}

void Label::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "label") {
		setLabel(json_var_get_str(value));
	}
}

}