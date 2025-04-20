#ifndef TEXT_GRID_H
#define TEXT_GRID_H

#include <graph/graph.h>
#include <font/font.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

#define TERM_STATE_UNDERLINE  0x01
#define TERM_STATE_REVERSE    0x02
#define TERM_STATE_FLASH      0x04
#define TERM_STATE_HIDE       0x08
#define TERM_STATE_HIGH_LIGHT 0x10

typedef struct {
	UNICODE16 c;
	uint16_t state;
	uint32_t color;
	uint32_t bg_color;
} textchar_t;

typedef struct {
	textchar_t* grid;
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
extern int textgrid_expand(textgrid_t* textgrid, uint32_t rows);

extern int textgrid_put(textgrid_t* textgrid, int x, int y, textchar_t* tc);
extern int textgrid_push(textgrid_t* textgrid, textchar_t* tc);

extern int textgrid_move_to(textgrid_t* textgrid, int32_t x, int32_t y);

extern int textgrid_paint(textgrid_t* textgrid, int32_t start_row,
	graph_t* g, uint32_t bg_color, 
	font_t* font, uint32_t font_size,
	int x, int y, uint32_t w, uint32_t h);

#ifdef __cplusplus
}
#endif

#endif
