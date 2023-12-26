#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tcurses/tcurses.h>

#ifdef __cplusplus
extern "C" {
#endif

void tcurses_init(tcurses_t* tc) {
	memset(tc, 0, sizeof(tcurses_t));
}

void tcurses_close(tcurses_t* tc) {
	if(tc->content != NULL) {
		free(tc->content);
	}
	memset(tc, 0, sizeof(tcurses_t));
}

void tcurses_reset(tcurses_t* tc, uint32_t cols, uint32_t rows) {
	if(cols == 0 || rows == 0) {
		return;
	}

	tchar_t* content = tc->content;
	uint32_t old_size = tc->cols * tc->rows;
	uint32_t size = cols * rows;

	tc->content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	if(content != NULL) {
		memcpy(tc->content, content, sizeof(tchar_t)*(size<old_size?size:old_size));
		free(content);
	}

	tc->cols = cols;
	tc->rows = rows;

	if(tc->curs_y >= rows) {
		tc->curs_y = rows - 1;
		tc->curs_x = cols - 1;
	}
	else if(tc->curs_x >= cols)
		tc->curs_x = cols - 1;
}

void tcurses_move_to(tcurses_t* tc, uint32_t x, uint32_t y) {
	tc->curs_x = x;	
	tc->curs_y = y;	

	if(tc->curs_y >= tc->rows) {
		tc->curs_y = tc->rows - 1;
		tc->curs_x = tc->cols - 1;
	}
	else if(tc->curs_x >= tc->cols)
		tc->curs_x = tc->cols - 1;
}

void tcurses_move(tcurses_t* tc, int32_t steps) {
	int32_t at = (int32_t)(tc->curs_y*tc->cols + tc->curs_x) + steps;
	if(at < 0)
		at = 0;

	uint32_t y = at/tc->cols;
	uint32_t x = at%tc->cols;
	tcurses_move_to(tc, x, y);
}

void tcurses_put(tcurses_t* tc, UNICODE16 c, uint32_t color) {
	if(tc->content == NULL)
		return;
	uint32_t at = tc->curs_y*tc->cols + tc->curs_x;
	tc->content[at].c = c;
	tc->content[at].color = color;
}

#ifdef __cplusplus
}
#endif

