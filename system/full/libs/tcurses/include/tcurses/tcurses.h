#ifndef tcurses_H
#define tcurses_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t UNICODE16;

typedef struct {
	UNICODE16 c;
	uint32_t color;
} tchar_t;

typedef struct {
	tchar_t* content;
	uint32_t cols;
	uint32_t rows;
	uint32_t curs_x;
	uint32_t curs_y;
} tcurses_t;

void tcurses_init(tcurses_t* tcurses);
void tcurses_close(tcurses_t* tcurses);
void tcurses_reset(tcurses_t* tcurses, uint32_t cols, uint32_t rows);
void tcurses_move_to(tcurses_t* tcurses, uint32_t x, uint32_t y);
void tcurses_move_at(tcurses_t* tcurses, uint32_t at);
void tcurses_move(tcurses_t* tcurses, int32_t steps);
void tcurses_put(tcurses_t* tcurses, UNICODE16 ci, uint32_t color);
uint32_t tcurses_at(tcurses_t* tcurses);
uint32_t tcurses_size(tcurses_t* tcurses);

#ifdef __cplusplus
}
#endif

#endif
