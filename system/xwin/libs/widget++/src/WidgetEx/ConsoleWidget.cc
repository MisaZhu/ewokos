#include <Widget/RootWidget.h>
#include <Widget/WidgetWin.h>
#include <WidgetEx/ConsoleWidget.h>
#include <gterminal/gterminal.h>
#include <x++/X.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void ConsoleWidget::drawBG(graph_t* g, const grect_t& r) {
	graph_set(g, r.x, r.y, r.w, r.h, terminal.bg_color);
	RootWidget* root = getRoot();
	if(color_a(terminal.bg_color) != 0xff) {
		setAlpha(true);
	}
	else {
		setAlpha(false);
	}
}

void ConsoleWidget::drawScrollbar(graph_t* g, const grect_t& r) {
	graph_fill_3d(g, r.x + r.w - scrollW, r.y, scrollW, r.h, 0x88000000, true);
	int sh = r.h * ((float) terminal.rows / terminal.textgrid->rows);
	int start_row = (int32_t)terminal.textgrid->rows - (int32_t)terminal.rows;
	if(start_row < 0)
		start_row = 0;
	start_row += terminal.scroll_offset;

	int sy = r.h * ((float) start_row / terminal.textgrid->rows);
	graph_fill_3d(g, r.x + r.w-scrollW+1, r.y + sy, scrollW-2, sh, 0x88aaaaaa, false);
}

bool ConsoleWidget::config(const char* fontName,
		uint32_t fontSize,
		int32_t charSpace,
		int32_t lineSpace,
		uint32_t fgColor,
		uint32_t bgColor,
		uint8_t transparent) {
	terminal.font_size = fontSize;
	terminal.char_space = charSpace;
	terminal.line_space = lineSpace;
	terminal.fg_color = fgColor;
	terminal.bg_color = (bgColor & 0x00ffffff) | ((transparent & 0xff) << 24);
	terminal.transparent = transparent;

	if(fontName != NULL && fontName[0] != 0) {
		if(terminal.font != NULL)
			font_free(terminal.font);
		terminal.font = font_new(fontName, true);
	}
	return true;
}

ConsoleWidget::ConsoleWidget() {
	gterminal_init(&terminal);
	scrollW = 8;
}

ConsoleWidget::~ConsoleWidget() {
	gterminal_close(&terminal);
}

void ConsoleWidget::push(const char* buf, int size) {
	gterminal_put(&terminal, buf, size);
}

void ConsoleWidget::flash() {
	gterminal_flash(&terminal);
	update();
}

void ConsoleWidget::setFont(const string& fontName) {
	if(terminal.font != NULL)
		font_free(terminal.font);
	terminal.font = font_new(fontName.c_str(), true);
	gterminal_resize(&terminal, area.w-scrollW, area.h);
	update();
}

void ConsoleWidget::onFocus(void) {
	update();
}

void ConsoleWidget::onUnfocus(void) {
	update();
}

void ConsoleWidget::onResize() {
	gterminal_resize(&terminal, area.w-scrollW, area.h);
}

void ConsoleWidget::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(terminal.font == NULL) {
		terminal.font_size = theme->basic.fontSize;
		terminal.bg_color = theme->basic.bgColor;
		terminal.fg_color = theme->basic.fgColor;
		setFont(theme->basic.fontName);
	}

	drawBG(g, r);
	int gw = r.w-scrollW;
	if(terminal.textgrid->rows > terminal.rows) {
		drawScrollbar(g, r);
	}
	gterminal_paint(&terminal, g, r.x, r.y, gw, r.h);
}

bool ConsoleWidget::onMouse(xevent_t* ev) {
	if(ev->state == MOUSE_STATE_DOWN) {
		mouse_last_y = ev->value.mouse.y;
		return true;
	}
	else if(ev->state == MOUSE_STATE_DRAG) {
		if(ev->value.mouse.y > mouse_last_y) {
			if(gterminal_scroll(&terminal, -1))
				update();
		}
		else if(ev->value.mouse.y < mouse_last_y) {
			if(gterminal_scroll(&terminal, 1))
				update();
		}
		mouse_last_y = ev->value.mouse.y;
		return true;
	}
	else if(ev->state == MOUSE_STATE_MOVE) {
		if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_UP) {
			if(gterminal_scroll(&terminal, 1))
				update();
			return true;
		}
		else if(ev->value.mouse.button == MOUSE_BUTTON_SCROLL_DOWN) {
			if(gterminal_scroll(&terminal, -1))
				update();
			return true;
		}
	}
	return false;
}

bool ConsoleWidget::onIM(xevent_t* ev) {
	if(ev->state == XIM_STATE_PRESS) {
		int c = ev->value.im.value;
		if(c != 0) {
			if(c == KEY_UP) {
				gterminal_scroll(&terminal, -1);
				update();
			}
			else if(c == KEY_DOWN) {
				gterminal_scroll(&terminal, 1);
				update();
			}
			else if(c == KEY_LEFT) {
				if(terminal.font_size > 5)
					terminal.font_size--;
				gterminal_resize(&terminal, area.w-scrollW, area.h);
				update();
			}
			else if(c == KEY_RIGHT) {
				if(terminal.font_size < 99)
					terminal.font_size++;
				gterminal_resize(&terminal, area.w-scrollW, area.h);
				update();
			}
			else {
				input(c);
				gterminal_scroll(&terminal, 0);
				update();
			}
		}
		return true;	
	}
	return false;	
}

}
