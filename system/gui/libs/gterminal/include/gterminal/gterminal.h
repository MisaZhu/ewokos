#ifndef gterminal_H
#define gterminal_H

#include <textgrid/textgrid.h>
#include <graph/graph.h>
#include <font/font.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
	uint16_t set;
	uint16_t state;
	uint32_t fg_color;
	uint32_t bg_color;
} gterm_conf_t;

typedef void (*gterminal_output_callback_t)(void* p, const char* buf, int size);

typedef struct {
	bool dragging;      /* mouse button held: the selection is still growing */
	bool visible;       /* a selection exists and is highlighted */
	int32_t anchor_row, anchor_col;  /* absolute grid cell where the drag began */
	int32_t cur_row, cur_col;        /* absolute grid cell under the pointer */
} gterm_selection_t;

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint8_t  transparent;
	bool     alt_charset;   /* DEC alternate (line-drawing) charset active */
	font_t* font;
	uint32_t font_size;
	int32_t char_space;
	int32_t line_space;

	gpos_t  curs_pos;
	bool flash_show;
	bool show_curs;

	gterm_conf_t term_conf;
	textgrid_t* textgrid;
	int32_t textgrid_start_row;
	int32_t scroll_offset;
	uint32_t scroll_top;
	uint32_t scroll_bottom;

	uint32_t rows;
	uint32_t cols;
	uint32_t char_w;
	uint32_t char_h;

	bool in_esc;
	uint16_t esc_buf[8];
	uint16_t esc_size;

	gterminal_output_callback_t output_callback;
	void* output_callback_arg;

	gterm_selection_t selection;
} gterminal_t;

void gterminal_init(gterminal_t* terminal);
void gterminal_close(gterminal_t* terminal);
bool gterminal_scroll(gterminal_t* terminal, int direction);
void gterminal_put(gterminal_t* terminal, const char* buf, int size);
void gterminal_flash(gterminal_t* terminal);
void gterminal_set_max_rows(gterminal_t* terminal, uint32_t max_rows);
void gterminal_paint(gterminal_t* terminal, graph_t* g, int x, int y, int w, int h);
void gterminal_resize(gterminal_t* terminal, uint32_t gw, uint32_t gh);

/* Text selection. Pixel coordinates are relative to the paint origin
 * (0,0 = top-left of the visible grid). Grid coordinates are absolute
 * textgrid rows/cols, so the highlight stays put while the view scrolls. */
void gterminal_select_begin_at(gterminal_t* terminal, int32_t px, int32_t py);
void gterminal_select_extend_at(gterminal_t* terminal, int32_t px, int32_t py);
void gterminal_select_end(gterminal_t* terminal);
void gterminal_select_clear(gterminal_t* terminal);
bool gterminal_cell_selected_at(gterminal_t* terminal, int32_t px, int32_t py);
/* Copy the selected text as UTF-8 (rows joined with '\n', trailing blanks
 * trimmed). Returns a malloc'ed NUL-terminated string, or NULL when there
 * is no selection. Caller must free(). */
char* gterminal_select_copy(gterminal_t* terminal);

static inline bool gterminal_has_selection(gterminal_t* terminal) {
	return terminal != NULL && terminal->selection.visible;
}

#ifdef __cplusplus
}
#endif

#endif
