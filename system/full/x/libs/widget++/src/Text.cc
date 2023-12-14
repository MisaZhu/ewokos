#include <Widget/Text.h>
#include <sys/klog.h>

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
		textview_reset(&textview, r.w, r.h, false);
		if(text.length() > 0) {
			textview_put_string(&textview, text.c_str(), text.length(), false);
			text = "";
		}
		bufferGraph = graph_new(NULL, r.w, r.h);
	}
	textview_refresh(&textview, bufferGraph);
	graph_blt(bufferGraph, 0, 0, r.w, r.h, g, r.x, r.y, r.w, r.h);
}

Text::Text(const string& str) {
	bufferGraph = NULL;
	text = str;
	textview_init(&textview);
	textview.font = font_new(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_SIZE, true);
	textview.font_size = DEFAULT_SYSTEM_FONT_SIZE;
}

Text::~Text(void) {
	textview_close(&textview);
	if(bufferGraph != NULL)
		graph_free(bufferGraph);
}

void Text::setText(const string& str) {
	textview_clear(&textview);
	text = str;
	update();
}

}