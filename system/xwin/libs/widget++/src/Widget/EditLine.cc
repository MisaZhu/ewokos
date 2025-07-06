#include <Widget/EditLine.h>
#include <x++/XTheme.h>
#include <ewoksys/keydef.h>

using namespace std;
namespace Ewok {

void EditLine::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	font_t* font = theme->getFont();
	if(font == NULL)
		return;
	
	uint32_t bgColor = theme->basic.bgColor;
	uint32_t fgColor = theme->basic.fgColor;
	if(!focused()) {
		bgColor = theme->basic.bgDisableColor;
		fgColor = theme->basic.fgDisableColor;
	}

	graph_fill_3d(g, r.x, r.y, r.w, r.h, bgColor, true);
	graph_draw_text_font_align(g, r.x+marginH, r.y + (r.h-theme->basic.fontSize)/2,
				r.w-marginH*2, r.h,
				content.c_str(), font, theme->basic.fontSize, fgColor, FONT_ALIGN_NONE);

	if(focused() && showCur) {
		int32_t cw;
		font_text_size(content.c_str(), font, theme->basic.fontSize, (uint32_t*)&cw, NULL);
		graph_fill(g, r.x+marginH + cw + 2, r.y + (r.h-theme->basic.fontSize)/2, 4, theme->basic.fontSize, fgColor); 
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
		content += (char)v;
	}

	onInput();
	update();
	return true;
}

void EditLine::onTimer(uint32_t timerFPS, uint32_t timerStep) {
	if(curTimerCounter*2 >= timerFPS) {
		curTimerCounter = 0;
		showCur = !showCur;
		update();
	}
	else
		curTimerCounter++;
}

void EditLine::onInput() {
	if(onInputFunc != NULL)
		onInputFunc(this);
}

EditLine::EditLine() {
	showCur = true;
	showXIM = false;
	curTimerCounter = 0;
	onInputFunc = NULL;
}

bool EditLine::onMouse(xevent_t* ev) {
	if(ev->state == MOUSE_STATE_CLICK) {
		showXIM = !showXIM;
		getWin()->callXIM(showXIM);
		return true;
	}
	return Widget::onMouse(ev);
}

}