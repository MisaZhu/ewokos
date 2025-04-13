#ifndef gterminal_H
#define gterminal_H

#include <terminal/terminal.h>
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
	font_t* font;
	uint32_t font_size;
	int32_t char_space;

	gpos_t  curs_pos;
	bool flash_show;
	bool show_curs;

	gterm_conf_t term_conf;
	terminal_t terminal;
} gterminal_t;

void gterminal_init(gterminal_t* terminal);
void gterminal_close(gterminal_t* terminal);
void gterminal_put(gterminal_t* terminal, const char* buf, int size);
void gterminal_flash(gterminal_t* terminal);
void gterminal_paint(gterminal_t* terminal, graph_t* g);
void gterminal_resize(gterminal_t* terminal, uint32_t gw, uint32_t gh);

#ifdef __cplusplus
}
#endif

#endif
