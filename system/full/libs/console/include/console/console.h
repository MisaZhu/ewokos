#ifndef CONSOLE_H
#define CONSOLE_H

#include <graph/graph.h>
#include <font/font.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

typedef struct st_line {
	UNICODE16 *line;
	uint32_t size;
	uint32_t last_x;
	struct st_line *next;
	struct st_line *prev;
} line_t;

typedef struct {
	uint32_t cols;
	uint32_t rows;
	line_t*  head;
	line_t*  tail;
	line_t*  start;
} content_t;

typedef struct {
	int32_t  font_size;
	int32_t  font_fixed;
	uint32_t w;
	uint32_t h;
	uint32_t bg_color;
	uint32_t fg_color;
	font_t font;
	content_t content;
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
