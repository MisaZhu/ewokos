#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <string.h>
#include <console/console.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t console_reset(console_t* console, uint32_t w, uint32_t h) {
	return textview_reset(&console->textview, w, h, true);
}

int32_t console_init(console_t* console) {
	textview_init(&console->textview);
	console->show_cursor = true;
	return 0;
}

void console_close(console_t* console) {
	textview_clear(&console->textview);
}

void console_roll(console_t* console, int32_t rows) {
	textview_roll(&console->textview, rows);
}

void console_refresh_content(console_t* console, graph_t* g) {
	textview_refresh_content(&console->textview, g);
}

void console_refresh(console_t* console, graph_t* g) {
	textview_refresh(&console->textview, g);
	//draw cursor
	if(console->show_cursor) {
		graph_fill(g, console->textview.last_x, console->textview.last_y,
				10, console->textview.font.max_size.y, console->textview.fg_color);
	}
}

void console_clear(console_t* console) {
	textview_clear(&console->textview);
}

void console_put_char(console_t* console, UNICODE16 c) {
	textview_put_char(&console->textview, c, true);
}

void console_put_string(console_t* console, const char* str, int len) {
	textview_put_string(&console->textview, str, len, true);
}

#ifdef __cplusplus
}
#endif

