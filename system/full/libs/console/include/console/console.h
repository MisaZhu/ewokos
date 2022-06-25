#ifndef CONSOLE_H
#define CONSOLE_H

#include <graph/graph.h>
#include <ttf/ttf.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

typedef struct {
	uint32_t start_row;
	uint32_t current_row;
	uint32_t back_offset_rows;
	uint32_t size;
} state_t;

typedef struct {
	uint32_t cols;
	uint32_t rows;
	UNICODE16* data;
} content_t;

typedef struct {
	uint32_t w;
	uint32_t h;
	uint32_t bg_color;
	uint32_t fg_color;
	ttf_font_t* font;
	content_t content;
	state_t state;
} console_t;

int32_t console_init(console_t* console);
void console_close(console_t* console);
void console_clear(console_t* console);
void console_refresh_content(console_t* console, graph_t* g);
void console_refresh(console_t* console, graph_t* g);
int32_t console_reset(console_t* console, uint32_t w, uint32_t h, uint32_t total_rows);
void console_roll(console_t* console, int32_t rows);
void console_put_char(console_t* console, UNICODE16 c);
void console_put_string(console_t* console, const char* s, int len);

#ifdef __cplusplus
}
#endif

#endif
