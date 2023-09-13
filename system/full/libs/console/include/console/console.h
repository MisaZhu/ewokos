#ifndef CONSOLE_H
#define CONSOLE_H

#include <graph/graph.h>
#include <font/font.h>
#include <textview/textview.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	textview_t textview;
	bool show_cursor;
} console_t;

int32_t console_init(console_t* console);
void console_close(console_t* console);
void console_clear(console_t* console);
void console_refresh_content(console_t* console, graph_t* g);
void console_refresh(console_t* console, graph_t* g);
int32_t console_reset(console_t* console, uint32_t w, uint32_t h);
void console_roll(console_t* console, int32_t rows);
void console_put_char(console_t* console, UNICODE16 c);
void console_put_string(console_t* console, const char* s, int len);

#ifdef __cplusplus
}
#endif

#endif
