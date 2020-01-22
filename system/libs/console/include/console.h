#ifndef CONSOLE_H
#define CONSOLE_H

#include <graph/graph.h>

typedef struct {
	uint32_t start_line;
	uint32_t line;
	uint32_t line_num;
	uint32_t line_w;
	uint32_t total;
	char* data;
	uint32_t size;
} content_t;

typedef struct {
	graph_t* g;
	uint32_t bg_color;
	uint32_t fg_color;
	font_t* font;
	content_t content;
} console_t;

int32_t console_init(console_t* console);
void console_close(console_t* console);
void console_clear(console_t* console);
void console_refresh(console_t* console);
int32_t console_reset(console_t* console);
void console_put_char(console_t* console, char c);
void console_put_string(console_t* console, const char* s);

#endif
