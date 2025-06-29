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

typedef struct {
	uint32_t fg_color;
	uint32_t bg_color;
	uint8_t  transparent;
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

	uint32_t rows;
	uint32_t cols;
	uint32_t char_w;
	uint32_t char_h;
} gterminal_t;

void gterminal_init(gterminal_t* terminal);
void gterminal_close(gterminal_t* terminal);
bool gterminal_scroll(gterminal_t* terminal, int direction);
void gterminal_put(gterminal_t* terminal, const char* buf, int size);
void gterminal_flash(gterminal_t* terminal);
void gterminal_paint(gterminal_t* terminal, graph_t* g, int x, int y, int w, int h);
void gterminal_resize(gterminal_t* terminal, uint32_t gw, uint32_t gh);

#ifdef __cplusplus
}
#endif

#endif
