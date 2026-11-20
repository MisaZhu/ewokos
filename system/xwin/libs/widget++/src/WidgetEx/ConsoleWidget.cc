#include <Widget/RootWidget.h>
#include <Widget/WidgetWin.h>
#include <WidgetEx/ConsoleWidget.h>
#include <gterminal/gterminal.h>
#include <x++/X.h>
#include <ewoksys/keydef.h>

namespace Ewok {

void ConsoleWidget::drawBG(graph_t* g, const grect_t& r) {
	/* A dirty root repaints its theme background over the whole window
	 * before children (alpha windows propagate every update to it), which
	 * wipes the persistent canvas: force a full terminal repaint then.
	 * Otherwise only full repaints need the background laid; partial
	 * gterminal_paint fills its own dirty band. */
	RootWidget* root = getRoot();
	bool wiped = (root != NULL && (Widget*)root != (Widget*)this && root->isDirty());
	if(wiped || gterminal_full_repaint_pending(&terminal)) {
		gterminal_invalidate(&terminal);
		graph_set(g, r.x, r.y, r.w, r.h, terminal.bg_color);
	}
	if(color_a(terminal.bg_color) != 0xff) {
		setAlpha(true);
	}
	else {
		setAlpha(false);
	}
}

void ConsoleWidget::drawScrollbar(graph_t* g, const grect_t& r) {
	if(terminal.textgrid == NULL || terminal.textgrid->rows == 0)
		return;
	/* the track colours are translucent: reset the strip to the terminal
	 * background first so repeated partial repaints don't stack alpha */
	graph_set(g, r.x + r.w - scrollW, r.y, scrollW, r.h, terminal.bg_color);
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

void ConsoleWidget::update() {
	RootWidget* root = getRoot();
	if(root != NULL && root->isPartialRepaint()) {
		int32_t by = 0, bh = 0;
		if(gterminal_damage_band(&terminal, area.h, &by, &bh)) {
			grect_t sub = {0, by, area.w, bh};
			Widget::update(sub);
			return;
		}
		/* full repaint (or a scroll, which moves every kept row): the
		 * whole console area is damaged */
	}
	Widget::update();
}

void ConsoleWidget::repaint(graph_t* g, XTheme* theme) {
	RootWidget* root = getRoot();
	if(root != NULL && root->isPartialRepaint() && !root->isDmgFull()) {
		if(!dirty || !visible)
			return;
		if(themePrivate != NULL)
			theme = themePrivate;
		grect_t r = getRootArea();
		if(r.w <= 0 || r.h <= 0) {
			dirty = false;
			return;
		}
		grect_t clip = r;
		if(!root->hasDamage() || !grect_insect(&root->getDamage(), &clip))
			return;   /* outside the damage: stay dirty for a later frame */
		/* the terminal lays its own background and a scroll blit moves
		 * pixels above the damage band: clip to the whole widget area and
		 * skip the theme prefill other widgets get */
		graph_set_clip(g, r.x, r.y, r.w, r.h);
		onRepaint(g, theme, r);
		dirty = false;
		return;
	}
	Widget::repaint(g, theme);
}

void ConsoleWidget::flash() {
	gterminal_flash(&terminal);
	update();
}

void ConsoleWidget::setMaxRows(uint32_t maxRows) {
	gterminal_set_max_rows(&terminal, maxRows);
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
	if(terminal.textgrid != NULL && terminal.textgrid->rows > terminal.rows) {
		drawScrollbar(g, r);
	}
	gterminal_paint(&terminal, g, r.x, r.y, gw, r.h);
}

bool ConsoleWidget::onMouse(xevent_t* ev) {
	static bool draging = false;
	if(ev->state == MOUSE_STATE_UP) {
		draging = false;
		return true;
	}

	if(ev->state == MOUSE_STATE_DOWN) {
		if(!draging) {
			mouse_last_y = ev->value.mouse.y;
			draging = true;
			return true;
		}
	}

	if(ev->state == MOUSE_STATE_DRAG || draging) {
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
		if(c > 0 && c < 128) {
			if(c == KEY_UP) {
				gterminal_scroll(&terminal, -1);
				update();
			}
			else if(c == KEY_DOWN) {
				gterminal_scroll(&terminal, 1);
				update();
			}
			else {
				if(c != KEY_LSHIFT && c != KEY_RSHIFT && c != KEY_CTRL) {
					input(c);
					gterminal_scroll(&terminal, 0);
					update();
				}
			}
		}
		return true;	
	}
	return false;	
}

}
