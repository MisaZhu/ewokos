#include "textgrid/textgrid.h"
#include "ewoksys/keydef.h"

#ifdef __cplusplus
extern "C" {
#endif

textgrid_t* textgrid_new(void) {
	textgrid_t* textgrid = (textgrid_t*)malloc(sizeof(textgrid_t));
	if(!textgrid)
		return NULL;
	memset(textgrid, 0, sizeof(textgrid_t));
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
	memset(textgrid, 0, sizeof(textgrid_t));
	textgrid->cols = cols;
}

int textgrid_reset(textgrid_t* textgrid, uint32_t cols) {
	if(!textgrid || cols == 0)
		return -1;

	textchar_t* p = textgrid->grid;
	uint32_t size = 0;
	if(textgrid->rows > 0) {
		size = textgrid->cols * (textgrid->rows-1) + textgrid->tail_col+1;
	}
	memset(textgrid, 0, sizeof(textgrid_t));
	textgrid->cols = cols;

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

int textgrid_expand(textgrid_t* grid, uint32_t rows) {
	if(grid == NULL || grid->cols == 0)
		return -1;

	grid->grid = (textchar_t*)realloc(grid->grid,
		(grid->cols*(grid->rows+rows))* sizeof(textchar_t));
	if(grid->grid == NULL)
		return -1;
	grid->total_rows = grid->rows + rows;

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

	//klog("%d, %d\n", textgrid->rows, textgrid->tail_col);
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
		//uint32_t at = (grid->rows-1)*grid->cols + grid->tail_col;
		uint32_t at = (grid->curs_y)*grid->cols + grid->curs_x;
		//klog("at: %d, [%c]\n", at, grid->grid[at].c);
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
	//klog("at: %d, %d\n", at, grid->rows);
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
	//klog("x:%d, y:%d\n", x, y);
	return 0;
}

int textgrid_paint(textgrid_t* textgrid, int32_t start_row,
		graph_t* g, uint32_t bg_color,
		font_t* font, uint32_t font_size,
		int x, int y, uint32_t w, uint32_t h) {
	if(start_row < 0)
		start_row = 0;

	if(font == NULL || !textgrid || !textgrid->grid ||
			textgrid->cols == 0 || start_row >= textgrid->rows ||
			w == 0 || h == 0) 
		return -1;
	
	uint32_t chw = w / textgrid->cols;
	uint32_t chh = font_get_height(font, font_size);
	for(int i=start_row; i<textgrid->rows; i++) {
		int cy = y + ((i-start_row)*chh);
		for(int j=0; j<textgrid->cols; j++) {
			uint32_t at = i*textgrid->cols + j;
			textchar_t* ch = &textgrid->grid[at];
			if(ch->c <= 27)
				continue;
			int cx = x + (j*chw);

			uint32_t fg = ch->color, bg = ch->bg_color;
			if(bg == 0)
				bg = bg_color;

            if((ch->state & TERM_STATE_REVERSE) != 0) {
                fg = bg;
                bg = ch->color;
            }
            if(bg != 0) 
                graph_fill(g, cx, cy, chw, chh, bg);
            
            if((ch->state & TERM_STATE_HIDE) == 0 && 
                    ((ch->state & TERM_STATE_FLASH) == 0)) {
                if((ch->state & TERM_STATE_UNDERLINE) != 0)
                    graph_line(g, cx, cy+chh-1, cx+chw,  cy+chh-1, fg);

                graph_draw_char_font_fixed(g, cx, cy, ch->c, font, font_size, fg, chw, 0);
                if((ch->state & TERM_STATE_HIGH_LIGHT) != 0)
                    graph_draw_char_font_fixed(g, cx+1, cy, ch->c, font, font_size, fg, chw, 0);
            }
		}
	}
	return 0;
}

#ifdef __cplusplus
}
#endif
