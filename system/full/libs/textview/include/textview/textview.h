#ifndef textview_H
#define textview_H

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
	bool shadow;
	font_t font;

	uint32_t w;
	uint32_t h;
	int32_t last_x;
	int32_t last_y;

	uint32_t bg_color;
	uint32_t fg_color;
	content_t content;
} textview_t;

int32_t textview_init(textview_t* textview);
void textview_close(textview_t* textview);
void textview_clear(textview_t* textview);
void textview_refresh_content(textview_t* textview, graph_t* g);
void textview_refresh(textview_t* textview, graph_t* g);
int32_t textview_reset(textview_t* textview, uint32_t w, uint32_t h, bool to_last);
void textview_roll(textview_t* textview, int32_t rows);
void textview_put_char(textview_t* textview, UNICODE16 c, bool to_last);
void textview_put_string(textview_t* textview, const char* s, int len, bool to_last);

#ifdef __cplusplus
}
#endif

#endif
