#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <tcurses/tcurses.h>
#include <ewoksys/utf8unicode.h>

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

void tcurses_set(tcurses_t* tc, UNICODE16 c, uint32_t color) {
	if(tc->content == NULL)
		return;
	uint32_t at = tcurses_at(tc);
	tc->content[at].c = c;
	tc->content[at].color = color;
}

void tcurses_insert(tcurses_t* tc, UNICODE16 c, uint32_t color) {
	uint32_t size = tc->cols*tc->rows;
	if(tc->content == NULL || size == 0)
		return;

	uint32_t at = tcurses_at(tc);
	uint32_t at2 = at+1;
	if(c == '\n')
		at2 = (tc->curs_y+1)*tc->cols;
	size -= at;

	tchar_t* content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	if(content != NULL) {
		uint32_t i;
		for(i=0; i<size; i++) {
			content[i].c = tc->content[i+at].c;
			content[i].color = tc->content[i+at].color;
			tc->content[i+at].c = 0;
			tc->content[i+at].color = 0;
		}

		tcurses_move_at(tc, at2);
		for(i=0; i<size; i++) {
			if(content[i].c != 0) {
				tcurses_set(tc, content[i].c, content[i].color);
				if(content[i].c != '\n')
					tcurses_move(tc, 1);
				else
					tcurses_move_at(tc, (tc->curs_y+1)*tc->cols);
			}
		}
	}
	tc->content[at].c = c;
	tc->content[at].color = color;
	tcurses_move_at(tc, at);
}

void tcurses_del(tcurses_t* tc) {
	uint32_t size = tc->cols*tc->rows;
	if(tc->content == NULL || size == 0)
		return;

	uint32_t at = tcurses_at(tc);
	size -= (at+1);

	tcurses_set(tc, 0, 0);
	tchar_t* content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	if(content != NULL) {
		uint32_t i;
		for(i=0; i<size; i++) {
			content[i].c = tc->content[i+at+1].c;
			content[i].color = tc->content[i+at+1].color;
			tc->content[i+at+1].c = 0;
			tc->content[i+at+1].color = 0;
		}

		tcurses_move_at(tc, at);
		for(i=0; i<size; i++) {
			if(content[i].c != 0) {
				tcurses_set(tc, content[i].c, content[i].color);
				if(content[i].c != '\n')
					tcurses_move(tc, 1);
				else
					tcurses_move_at(tc, (tc->curs_y+1)*tc->cols);
			}
		}
	}
	tcurses_move_at(tc, at);
}

void tcurses_inserts(tcurses_t* tc, tchar_t* s, uint32_t size) {
	for(uint32_t i=0; i<size; i++) {
		if(s[i].c != 0) {
			tcurses_insert(tc, s[i].c, s[i].color);
			if(s[i].c != '\n')
				tcurses_move(tc, 1);
			else
				tcurses_move_to(tc, 0, tc->curs_y+1);
		}
	}
}

void tcurses_sets(tcurses_t* tc, tchar_t* s, uint32_t size) {
	for(uint32_t i=0; i<size; i++) {
		if(s[i].c != 0) {
			tcurses_set(tc, s[i].c, s[i].color);
			if(s[i].c != '\n')
				tcurses_move(tc, 1);
			else
				tcurses_move_to(tc, 0, tc->curs_y+1);
		}
	}
}

tchar_t* tcurses_from_str(const char* str, uint32_t color) {
	uint32_t len = strlen(str);
	if(len == 0)
		return NULL;
	uint16_t* unicode = (uint16_t*)malloc((len+1)*2);
	utf82unicode(str, len, unicode);
	tchar_t* ret = (tchar_t*)malloc(sizeof(tchar_t)*len);
	for(uint32_t i=0; i<len; i++) {
		ret[i].c = unicode[i];
		ret[i].color = color;
	}
	free(unicode);
	return ret;
}

uint32_t tcurses_tail(tcurses_t* tc) {
	int32_t size = tcurses_size(tc);
	int32_t i = size - 1;
	while(i >= 0) {
		if(tc->content[i].c != 0)
			break;
		i--;
	}
	if(i < (size - 1))
		i++;
	return i;
}

void tcurses_reset(tcurses_t* tc, uint32_t cols, uint32_t rows) {
	if(cols == 0 || rows == 0) {
		return;
	}

	tchar_t* content = tc->content;
	uint32_t old_size = tc->cols * tc->rows;
	uint32_t size = cols * rows;

	tc->cols = cols;
	tc->rows = rows;
	tc->content = (tchar_t*)calloc(size * sizeof(tchar_t), 1);
	if(content != NULL) {
		tcurses_move_at(tc, 0);
		tcurses_sets(tc, content, old_size);	
		free(content);
	}
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

int32_t tcurses_pos_by_at(tcurses_t* tc, uint32_t at, uint32_t* x, uint32_t *y) {
	if(at >= tcurses_size(tc))
		return -1;
	*y = at/tc->cols;
	*x = at%tc->cols;
	return 0;
}

int32_t tcurses_pos(tcurses_t* tc, uint32_t* x, uint32_t *y) {
	*y = tc->cols;
	*x = tc->cols;
	return 0;
}

uint32_t tcurses_at_by_pos(tcurses_t* tc, uint32_t x, uint32_t y) {
	uint32_t size = tcurses_size(tc);
	if(size == 0)
		return 0;

	uint32_t at = y*tc->cols + x;
	if(at >= size)
		at = size-1;
	return at;
}

void tcurses_move_at(tcurses_t* tc, uint32_t at) {
	uint32_t y = at/tc->cols;
	uint32_t x = at%tc->cols;
	tcurses_move_to(tc, x, y);
}

inline uint32_t tcurses_at(tcurses_t* tc) {
	return tc->curs_y*tc->cols + tc->curs_x;
}

UNICODE16 tcurses_get_at(tcurses_t* tc, uint32_t at) {
	if(tc->content == NULL || at >= tcurses_size(tc))
		return 0;
	return  tc->content[at].c;
}

UNICODE16 tcurses_get(tcurses_t* tc) {
	if(tc->content == NULL)
		return 0;
	return  tc->content[tcurses_at(tc)].c;
}

inline uint32_t tcurses_size(tcurses_t* tc) {
	return tc->cols * tc->rows;
}

void tcurses_move(tcurses_t* tc, int32_t steps) {
	int32_t at = (int32_t)tcurses_at(tc) + steps;
	if(at < 0)
		at = 0;
	tcurses_move_at(tc, at);
}

#ifdef __cplusplus
}
#endif

