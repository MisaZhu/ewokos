#include <Widget/Text.h>

using namespace EwokSTL;
namespace Ewok {

void Text::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	textview.bg_color = theme->bgColor;
	textview.fg_color = theme->fgColor;

	if(bufferGraph != NULL && 
			(r.w != bufferGraph->w || r.h != bufferGraph->h)) {
		graph_free(bufferGraph);
		bufferGraph = NULL;
	}

	if(bufferGraph == NULL) {
		bufferGraph = graph_new(NULL, r.w, r.h);
		reset = true;
	}

	if(reset) {
		textview.font = getFont(theme, &textview.font_size);
		textview_reset(&textview, r.w, r.h, false);
		if(resetText) {
			textview_clear(&textview);
			textview_put_string(&textview, text.c_str(), text.length(), false);
			resetText = false;
		}
		reset = false;
	}
	textview_refresh(&textview, bufferGraph);
	graph_blt(bufferGraph, 0, 0, r.w, r.h, g, r.x, r.y, r.w, r.h);
}

Text::Text(const string& str) {
	bufferGraph = NULL;
	text = str;
	reset = true;
	resetText = true;
	textview_init(&textview);
}

Text::~Text(void) {
	textview_close(&textview);
	if(bufferGraph != NULL)
		graph_free(bufferGraph);
}

void Text::onFont() {
	reset = true;
	update();
}

void Text::setText(const string& str) {
	text = str;
	resetText = true;
	update();
}

}