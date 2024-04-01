#include <Widget/EditLine.h>
#include <x++/XTheme.h>
#include <ewoksys/keydef.h>

using namespace EwokSTL;
namespace Ewok {

void EditLine::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	font_t* font = theme->getFont();
	if(font == NULL)
		return;

	graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);
	graph_draw_text_font_align(g, r.x+marginH, r.y + (r.h-font->max_size.y)/2,
				r.w-marginH*2, r.h,
				content.c_str(), font, theme->basic.fgColor, FONT_ALIGN_NONE);

	if(focused()) {
		int32_t cw;
		font_text_size(content.c_str(), font, (uint32_t*)&cw, NULL);
		graph_fill(g, r.x+marginH + cw + 2, r.y + (r.h-font->max_size.y)/2, 4, font->max_size.y, theme->basic.fgColor); 
	}
}

void EditLine::setContent(const string& content) {
	this->content = content;
	update();
}

bool EditLine::onIM(xevent_t* ev) {
	if(ev->state != XIM_STATE_PRESS)
		return false;

	uint32_t v = ev->value.im.value;
	if(v == KEY_BACKSPACE || v == CONSOLE_LEFT) {
		if(content.length() > 0)
			content = content.substr(0, content.length()-1);
	}
	else if(v > 27) {
		content = content + v;
	}

	onInput();
	update();
	return true;
}

}