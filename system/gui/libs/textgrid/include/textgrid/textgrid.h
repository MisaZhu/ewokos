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
	textchar_t* grid;        // The grid buffer storing all text characters
	uint8_t*  wrapped;       // Per-row flag: row is a soft-wrap continuation of the previous row
	uint32_t max_rows;       // Maximum number of rows the grid can hold
	uint32_t total_rows;     // Total allocated rows in the grid

	uint32_t cols;           // Number of columns per row
	uint32_t rows;           // Current number of rows with content

	int32_t curs_x;          // Current cursor column position
	int32_t curs_y;          // Current cursor row position

	int32_t  tail_col;       // Last column with content in the last row

	/* Dirty row range [dirty_start, dirty_end), absolute grid rows.
	 * dirty_end == INT32_MAX means "everything is dirty" (reset, clear).
	 * dirty_start >= dirty_end means clean. A scrollback shift translates
	 * the range with the content instead of invalidating it. */
	int32_t dirty_start;
	int32_t dirty_end;

	/* Cumulative rows dropped by scrollback shifts. Painters compare it
	 * against their own snapshot to scroll the canvas instead of repainting
	 * every row; reset/clear zero it (content identity is lost). */
	uint32_t shift_total;
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

/* Union [start, end) into the dirty row range (start clamped at 0,
 * empty ranges are no-ops); reset the range to clean. */
extern void textgrid_mark_dirty(textgrid_t* textgrid, int32_t start, int32_t end);
extern void textgrid_clean_dirty(textgrid_t* textgrid);

typedef void (*textgrid_draw_char_t)(graph_t* g,
	textchar_t* tch,
	int chx, int chy,
	uint32_t chw, uint32_t chh,
	int32_t row, int32_t col,
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
