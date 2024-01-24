#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <terminal/terminal.h>
#include <ewoksys/utf8unicode.h>

#ifdef __cplusplus
extern "C" {
#endif

void terminal_init(terminal_t* terminal) {
	memset(terminal, 0, sizeof(terminal_t));
}

void terminal_close(terminal_t* terminal) {
	if(terminal->content != NULL) {
		free(terminal->content);
	}
	memset(terminal, 0, sizeof(terminal_t));
}

void terminal_scroll(terminal_t* terminal, uint32_t lines) {
	uint32_t mlines = terminal->curs_y + 1;
	if(lines > mlines)
		lines = mlines;
	mlines -= lines;

	for(uint32_t i=0; i<mlines; i++) {
		for(uint32_t j=0; j<terminal->cols; j++) {
			uint32_t at = i*terminal->cols + j;
			uint32_t atm = (i+lines)*terminal->cols + j;
			terminal->content[at].c = terminal->content[atm].c;
			terminal->content[at].color = terminal->content[atm].color;
			terminal->content[at].state = terminal->content[atm].state;
			terminal->content[at].bg_color = terminal->content[atm].bg_color;
		}
	}
	for(uint32_t i=0; i<lines; i++) {
		for(uint32_t j=0; j<terminal->cols; j++) {
			uint32_t atm = (i+mlines)*terminal->cols + j;
			terminal->content[atm].c = 0;
			terminal->content[atm].color = 0;
			terminal->content[atm].state = 0;
			terminal->content[atm].bg_color = 0;
		}
	}
}

void terminal_set(terminal_t* terminal, UNICODE16 c, uint16_t state, uint32_t color, uint32_t bg_color) {
	if(terminal->content == NULL)
		return;
	uint32_t at = terminal_at(terminal);
	terminal->content[at].c = c;
	terminal->content[at].color = color;
	terminal->content[at].state = state;
	terminal->content[at].bg_color = bg_color;
}

void terminal_clear(terminal_t* terminal) {
	uint32_t size = terminal_size(terminal);
	for(uint32_t i=0; i<size; i++) {
		terminal->content[i].c = 0;
		terminal->content[i].color = 0;
		terminal->content[i].state = 0;
		terminal->content[i].bg_color = 0;
	}
}

void terminal_reset(terminal_t* terminal, uint32_t cols, uint32_t rows) {
	if(cols == 0 || rows == 0)
		return;

	if(terminal->content != NULL)
		free(terminal->content);

	uint32_t size = cols * rows;
	terminal->content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	terminal->cols = cols;
	terminal->rows = rows;
}

void terminal_move_to(terminal_t* terminal, uint32_t x, uint32_t y) {
	terminal->curs_x = x;	
	terminal->curs_y = y;	

	if(terminal->curs_y >= terminal->rows)
		terminal->curs_y = terminal->rows - 1;
	else if(terminal->curs_x >= terminal->cols)
		terminal->curs_x = terminal->cols - 1;
}

int32_t terminal_pos_by_at(terminal_t* terminal, uint32_t at, uint32_t* x, uint32_t *y) {
	if(at >= terminal_size(terminal))
		return -1;
	*y = at/terminal->cols;
	*x = at%terminal->cols;
	return 0;
}

int32_t terminal_pos(terminal_t* terminal, uint32_t* x, uint32_t *y) {
	*y = terminal->curs_y;
	*x = terminal->curs_x;
	return 0;
}

uint32_t terminal_at_by_pos(terminal_t* terminal, uint32_t x, uint32_t y) {
	uint32_t size = terminal_size(terminal);
	if(size == 0)
		return 0;

	uint32_t at = y*terminal->cols + x;
	if(at >= size)
		at = size-1;
	return at;
}

void terminal_move_at(terminal_t* terminal, uint32_t at) {
	uint32_t y = at/terminal->cols;
	uint32_t x = at%terminal->cols;
	terminal_move_to(terminal, x, y);
}

void terminal_move_next_line(terminal_t* terminal) {
	terminal_move_to(terminal, 0, terminal->curs_y+1);
}

inline uint32_t terminal_at(terminal_t* terminal) {
	return terminal->curs_y*terminal->cols + terminal->curs_x;
}

inline tchar_t* terminal_get_by_at(terminal_t* terminal, uint32_t at) {
	if(terminal->content == NULL || at >= terminal_size(terminal))
		return NULL;
	return  &terminal->content[at];
}

tchar_t* terminal_get_by_pos(terminal_t* terminal, uint32_t x, uint32_t y) {
	uint32_t at = terminal_at_by_pos(terminal, x, y);
	return terminal_get_by_at(terminal, at);
}

tchar_t* terminal_get(terminal_t* terminal) {
	return &terminal->content[terminal_at(terminal)];
}

inline uint32_t terminal_size(terminal_t* terminal) {
	return terminal->cols * terminal->rows;
}

void terminal_move(terminal_t* terminal, int32_t steps) {
	int32_t at = (int32_t)terminal_at(terminal) + steps;
	if(at < 0)
		at = 0;
	terminal_move_at(terminal, at);
}

#ifdef __cplusplus
}
#endif

