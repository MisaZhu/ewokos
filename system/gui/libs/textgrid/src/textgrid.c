#include "textgrid/textgrid.h"
#include "ewoksys/keydef.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROWS 1024
textgrid_t* textgrid_new(void) {
	textgrid_t* textgrid = (textgrid_t*)malloc(sizeof(textgrid_t));
	if(!textgrid)
		return NULL;
	memset(textgrid, 0, sizeof(textgrid_t));
	textgrid->max_rows = MAX_ROWS;
	return textgrid;
}

void textgrid_free(textgrid_t* textgrid) {
	if(!textgrid)
		return;
	if(textgrid->grid)
		free(textgrid->grid);
	free(textgrid);
}

void textgrid_clear(textgrid_t* textgrid) {
	if(!textgrid)
		return;
	if(textgrid->grid)
		free(textgrid->grid);
	uint32_t cols = textgrid->cols;
	uint32_t max_rows = textgrid->max_rows;
	memset(textgrid, 0, sizeof(textgrid_t));
	textgrid->cols = cols;
	textgrid->max_rows = max_rows;
}

int textgrid_set_max_rows(textgrid_t* textgrid, uint32_t max_rows) {
	if(!textgrid || max_rows == 0)
		return -1;
	textgrid->max_rows = max_rows < MAX_ROWS ? MAX_ROWS : max_rows;
	textgrid_clear(textgrid);
	return 0;
}

int textgrid_reset(textgrid_t* textgrid, uint32_t cols) {
	if(!textgrid || cols == 0)
		return -1;
	textchar_t* p = textgrid->grid;
	uint32_t max_rows = textgrid->max_rows;
	uint32_t size = 0;
	if(textgrid->rows > 0) {
		size = textgrid->cols * (textgrid->rows-1) + textgrid->tail_col+1;
	}
	memset(textgrid, 0, sizeof(textgrid_t));
	textgrid->cols = cols;
	textgrid->max_rows = max_rows;

	if(p == NULL || size == 0) {
		return 0;
	}

	for(uint32_t i=0; i<size; i++) 
		textgrid_push(textgrid, &p[i]);
	
	if(p != NULL)
		free(p);
	return 0;
}

#define EXPAND_ROWS  16

static int textgrid_expand(textgrid_t* grid, uint32_t rows) {
	if(grid == NULL || grid->cols == 0)
		return -1;

	uint32_t rows_new = grid->rows + rows;
	if(rows_new < grid->max_rows || grid->rows == 0) {
		grid->grid = (textchar_t*)realloc(grid->grid,
			(grid->cols*(rows_new))* sizeof(textchar_t));
		if(grid->grid == NULL)
			return -1;
		grid->total_rows = rows_new;
	}
	else {
		if(rows_new > grid->total_rows)
			rows_new = grid->total_rows;
		if(rows > grid->rows)
			rows = grid->rows;
		
		for(int r = rows; r<grid->rows; r++) {
			for(int c=0; c<grid->cols; c++) {
				uint32_t at0 = ((r-rows) * grid->cols) + c;
				uint32_t at1 = (r * grid->cols) + c;
				memcpy(&grid->grid[at0], &grid->grid[at1], sizeof(textchar_t));
			}
		}
		grid->rows -= rows;
		if(grid->rows == 0)
			grid->curs_y = 0;
		else
			grid->curs_y = grid->rows-1;
		grid->curs_x = 0;
	}

	for(int r=grid->rows; r<grid->total_rows; r++) {
		for(int c=0; c<grid->cols; c++) {
			uint32_t at = (r * grid->cols) + c;
			grid->grid[at].c = 0;
		}
	}
	return 0;
}

int textgrid_put(textgrid_t* textgrid, int x, int y, textchar_t* tc) {
	if(!textgrid || !textgrid->grid)
		return -1;
	if(x < 0 || x >= textgrid->cols)
		return -1;
	if(y < 0)
		return -1;
	if(y >= textgrid->total_rows) {
		uint32_t expand = y-textgrid->total_rows+1;
		if(textgrid_expand(textgrid, expand) != 0)
			return -1;
	}

	if((y+1) >= textgrid->rows) {
		textgrid->tail_col = x;
		textgrid->rows = y+1;
	}

	textgrid->grid[y * textgrid->cols + x] = *tc;
	return 0;
}

int textgrid_push(textgrid_t* grid, textchar_t* tch) {
	if(grid == NULL || tch == 0 || grid->cols == 0)
		return -1;

	if(tch->c == 0)
		return 0;
	
	bool is_tail = false;
	if((grid->curs_y+1) >= grid->rows && grid->tail_col == grid->curs_x)
		is_tail = true;
	
	if(grid->grid != NULL && grid->rows > 0) {
		uint32_t at = (grid->curs_y)*grid->cols + grid->curs_x;
		if(tch->c == KEY_BACKSPACE || tch->c == CONSOLE_LEFT) {
			grid->grid[at].c = 0;

			if(grid->curs_x == 0) {
				if(grid->curs_y > 0)
					grid->curs_y--;
				grid->curs_x = grid->cols - 1;
			}
			else
				grid->curs_x--;

			if(is_tail) {
				grid->tail_col = grid->curs_x;
				grid->rows = grid->curs_y+1;
			}
			return 0;
		}

		if(grid->grid[at].c == '\r' || grid->grid[at].c == '\n') {
			int col_index = grid->curs_x + 1;
			while(col_index < grid->cols) {
				at = (grid->curs_y)*grid->cols + col_index;
				grid->grid[at].c = 0;
				col_index++;
			}
			grid->curs_x = grid->cols - 1;
			if(is_tail) {
				grid->tail_col = grid->curs_x;
			}
		}
	}

	grid->curs_x++;
	bool new_line = false;
	if(grid->curs_x >= grid->cols || grid->rows == 0) { //new line needed
		new_line = true;
		grid->curs_x = 0;
	}

	if(new_line) {
		if((grid->curs_y+1) >= grid->total_rows) { //expand grid
			if(textgrid_expand(grid, EXPAND_ROWS) != 0)
				return -1;
		}
		if(grid->rows != 0)
			grid->curs_y++;
		if(grid->curs_y >= grid->rows)
			grid->rows = grid->curs_y+1;
	}

	uint32_t at = (grid->curs_y)*grid->cols + grid->curs_x;
	grid->grid[at] = *tch;
	if(is_tail) {
		grid->tail_col = grid->curs_x;
	}
	return 0;
}

int textgrid_move_to(textgrid_t* textgrid, int32_t x, int32_t y) {
	if(textgrid->rows == 0 || textgrid->cols == 0)
		return -1;

	if(x >= textgrid->cols)
		x = textgrid->cols - 1;
	
	if(y >= textgrid->total_rows) {
		uint32_t expand = y-textgrid->total_rows+1;
		if(textgrid_expand(textgrid, expand) != 0)
			return -1;
	}

	if((y+1) >= textgrid->rows) {
		textgrid->tail_col = x;
		textgrid->rows = y+1;
	}

	textgrid->curs_x = x;	
	textgrid->curs_y = y;	
	return 0;
}

int textgrid_paint(graph_t* g,
		textgrid_t* textgrid,
		textgrid_draw_char_t draw_char_func, void* draw_arg,
		int32_t start_row, uint32_t row_height,
		int x, int y, uint32_t w, uint32_t h) {
	if(start_row < 0)
		start_row = 0;

	if(!textgrid || !textgrid->grid ||
			textgrid->cols == 0 || start_row >= textgrid->rows ||
			w == 0 || h == 0) 
		return -1;
	
	uint32_t chw = w / textgrid->cols;
	uint32_t chh = row_height;
	for(int i=start_row; i<textgrid->rows; i++) {
		int cy = y + ((i-start_row)*chh);
		for(int j=0; j<textgrid->cols; j++) {
			uint32_t at = i*textgrid->cols + j;
			textchar_t* ch = &textgrid->grid[at];
			if(ch->c <= 27)
				continue;
			int cx = x + (j*chw);
			draw_char_func(g, ch, cx, cy, chw, chh, draw_arg);
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
