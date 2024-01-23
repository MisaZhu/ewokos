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

void terminal_set(terminal_t* terminal, UNICODE16 c, uint32_t color) {
	if(terminal->content == NULL)
		return;
	uint32_t at = terminal_at(terminal);
	terminal->content[at].c = c;
	terminal->content[at].color = color;
}

void terminal_reset(terminal_t* terminal, uint32_t cols, uint32_t rows) {
	if(cols == 0 || rows == 0) {
		return;
	}

	tchar_t* content = terminal->content;
	uint32_t old_size = terminal->cols * terminal->rows;
	uint32_t size = cols * rows;

	terminal->cols = cols;
	terminal->rows = rows;
	terminal->content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	if(content != NULL) {
		terminal_move_at(terminal, 0);
		terminal_sets(terminal, content, old_size);	
		free(content);
	}
}

void terminal_move_to(terminal_t* terminal, uint32_t x, uint32_t y) {
	terminal->curs_x = x;	
	terminal->curs_y = y;	

	if(terminal->curs_y >= terminal->rows) {
		terminal->curs_y = terminal->rows - 1;
		terminal->curs_x = terminal->cols - 1;
	}
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
	*y = terminal->cols;
	*x = terminal->cols;
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

inline uint32_t terminal_at(terminal_t* terminal) {
	return terminal->curs_y*terminal->cols + terminal->curs_x;
}

UNICODE16 terminal_get_at(terminal_t* terminal, uint32_t at) {
	if(terminal->content == NULL || at >= terminal_size(terminal))
		return 0;
	return  terminal->content[at].c;
}

UNICODE16 terminal_get(terminal_t* terminal) {
	if(terminal->content == NULL)
		return 0;
	return  terminal->content[terminal_at(terminal)].c;
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

