#ifndef TEXT_GRID_H
#define TEXT_GRID_H

#include <graph/graph.h>
#include <font/font.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

typedef struct {
	UNICODE16 c;
	uint16_t state;
	uint32_t color;
	uint32_t bg_color;
} textchar_t;

typedef struct {
	textchar_t* grid;
	uint32_t max_rows;
	uint32_t total_rows;

	uint32_t cols;
	uint32_t rows;

	int32_t curs_x;
	int32_t curs_y;

	int32_t  tail_col;
} textgrid_t;

extern textgrid_t* textgrid_new(void);
extern void textgrid_free(textgrid_t* textgrid);
extern void textgrid_clear(textgrid_t* textgrid);
extern int textgrid_reset(textgrid_t* textgrid, uint32_t cols);
extern int textgrid_set_max_rows(textgrid_t* textgrid, uint32_t max_rows);
//extern int textgrid_expand(textgrid_t* textgrid, uint32_t rows);

extern int textgrid_put(textgrid_t* textgrid, int x, int y, textchar_t* tc);
extern int textgrid_push(textgrid_t* textgrid, textchar_t* tc);

extern int textgrid_move_to(textgrid_t* textgrid, int32_t x, int32_t y);

typedef void (*textgrid_draw_char_t)(graph_t* g,
	textchar_t* tch,
	int chx, int chy,
	uint32_t chw, uint32_t chh,
	void*p);

extern int textgrid_paint(graph_t* g,
		textgrid_t* textgrid,
		textgrid_draw_char_t draw_char_func, void* draw_arg,
		int32_t start_row, uint32_t row_height,
		int x, int y, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif
