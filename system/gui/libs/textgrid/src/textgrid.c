#include "textgrid/textgrid.h"
#include "ewoksys/keydef.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROWS 1024
/* Shift batch granularity once the scrollback is full: every batch costs one
 * near-full-grid memmove, so a larger batch amortizes that move over more
 * produced lines (16 -> 128 cuts the total bytes moved ~8x). The scrollback
 * capacity is unchanged; only the granularity at which the oldest rows are
 * dropped grows. */
#define EXPAND_ROWS  128

void textgrid_mark_dirty(textgrid_t* textgrid, int32_t start, int32_t end) {
    if(!textgrid)
        return;
    if(start < 0)
        start = 0;
    if(start >= end)
        return;
    if(textgrid->dirty_start >= textgrid->dirty_end) {
        textgrid->dirty_start = start;
        textgrid->dirty_end = end;
        return;
    }
    if(start < textgrid->dirty_start)
        textgrid->dirty_start = start;
    if(end > textgrid->dirty_end)
        textgrid->dirty_end = end;
}

void textgrid_clean_dirty(textgrid_t* textgrid) {
    if(!textgrid)
        return;
    textgrid->dirty_start = 0;
    textgrid->dirty_end = 0;
}

static void textgrid_invalidate(textgrid_t* textgrid) {
    textgrid->dirty_start = 0;
    textgrid->dirty_end = INT32_MAX;
}

textgrid_t* textgrid_new(void) {
    textgrid_t* textgrid = (textgrid_t*)malloc(sizeof(textgrid_t));
    if(!textgrid)
        return NULL;
    memset(textgrid, 0, sizeof(textgrid_t));
    textgrid->max_rows = MAX_ROWS;
    textgrid_invalidate(textgrid);
    return textgrid;
}

void textgrid_free(textgrid_t* textgrid) {
    if(!textgrid)
        return;
    if(textgrid->grid)
        free(textgrid->grid);
    if(textgrid->wrapped)
        free(textgrid->wrapped);
    free(textgrid);
}

void textgrid_clear(textgrid_t* textgrid) {
    if(!textgrid)
        return;
    if(textgrid->grid)
        free(textgrid->grid);
    if(textgrid->wrapped)
        free(textgrid->wrapped);
    uint32_t cols = textgrid->cols;
    uint32_t max_rows = textgrid->max_rows;
    memset(textgrid, 0, sizeof(textgrid_t));
    textgrid->cols = cols;
    textgrid->max_rows = max_rows;
    textgrid_invalidate(textgrid);
}

int textgrid_set_max_rows(textgrid_t* textgrid, uint32_t max_rows) {
    if(!textgrid || max_rows == 0)
        return -1;
    textgrid->max_rows = max_rows < MAX_ROWS ? MAX_ROWS : max_rows;
    textgrid_clear(textgrid);
    return 0;
}

/* Produced length of a row for the reflow: a row that soft-wraps into the
 * next contributes its full width; the last row of a logical line only
 * contributes up to its last non-empty cell (trailing blanks are padding,
 * not content). */
static uint32_t row_content_len(const textgrid_t* tg, uint32_t r, bool continues) {
    if(continues)
        return tg->cols;
    uint32_t len = 0;
    for(uint32_t c = tg->cols; c > 0; c--) {
        if(tg->grid[(size_t)r * tg->cols + (c - 1)].c != 0) {
            len = c;
            break;
        }
    }
    return len;
}

int textgrid_reset(textgrid_t* textgrid, uint32_t cols) {
    if(!textgrid || cols == 0)
        return -1;
    uint32_t old_cols = textgrid->cols;
    if(old_cols == cols)
        return 0;

    textchar_t* p = textgrid->grid;
    uint32_t rows = textgrid->rows;
    if(p == NULL || old_cols == 0 || rows == 0) {
        textgrid->cols = cols;
        textgrid->shift_total = 0;
        textgrid_invalidate(textgrid);
        return 0;
    }

    /* Reflow: regroup the produced rows into logical lines using the
     * soft-wrap flags, then re-wrap every logical line at the new width.
     * A narrower grid (bigger font) spills the overflow onto continuation
     * rows instead of truncating it; a wider grid joins continuation rows
     * back together. Hard breaks (explicit newlines / cursor addressing)
     * stay breaks, so no content is lost and no rows get glued together. */
    const uint8_t* wrapped = textgrid->wrapped;

    /* pass 1: total reflowed row count, plus the cursor's absolute cell
     * offset so it can follow its text through the reflow. The offset must
     * be measured in the reflowed (padded) stream: pass 2 starts every
     * logical line at column 0 of a fresh row, so short lines contribute
     * their full row width and empty lines a whole blank row - counting
     * only the raw content cells would land the cursor before its text. */
    uint32_t new_rows = 0;
    uint32_t curs_off = 0;
    uint32_t curs_y = (uint32_t)textgrid->curs_y;
    if(curs_y >= rows)
        curs_y = rows - 1;
    uint32_t r = 0;
    while(r < rows) {
        uint32_t line_len = 0;
        uint32_t line_start = new_rows * cols;
        do {
            bool continues = (r + 1 < rows) && wrapped != NULL && wrapped[r + 1] != 0;
            uint32_t len = row_content_len(textgrid, r, continues);
            if(r == curs_y) {
                uint32_t cx = (uint32_t)textgrid->curs_x;
                if(cx > len)
                    cx = len;
                curs_off = line_start + line_len + cx;
            }
            line_len += len;
            r++;
        } while(r < rows && wrapped != NULL && wrapped[r] != 0);
        new_rows += (line_len == 0) ? 1 : (line_len + cols - 1) / cols;
    }

    /* Reflow can grow the row count past the scrollback cap: drop the
     * oldest rows like a normal scrollback overflow, never the newest. */
    uint32_t max_rows = textgrid->max_rows;
    uint32_t drop = (new_rows > max_rows) ? (new_rows - max_rows) : 0;
    uint32_t keep_rows = new_rows - drop;
    uint32_t new_total = keep_rows + EXPAND_ROWS;
    if(new_total > max_rows)
        new_total = max_rows;

    if(cols == 0 || new_total > (UINT32_MAX / sizeof(textchar_t)) / cols)
        return -1;
    textchar_t* ng = (textchar_t*)malloc((size_t)cols * new_total * sizeof(textchar_t));
    uint8_t* nw = (uint8_t*)malloc(new_total);
    if(ng == NULL || nw == NULL) {
        free(ng);
        free(nw);
        return -1;
    }
    memset(ng, 0, (size_t)cols * new_total * sizeof(textchar_t));
    memset(nw, 0, new_total);

    /* pass 2: copy the cells, re-wrapping at the new width; rows below the
     * drop cutoff are skipped (their storage was never allocated). */
    uint32_t out_row = 0;
    uint32_t out_col = 0;
    r = 0;
    while(r < rows) {
        uint32_t group_end = r;
        while(group_end + 1 < rows && wrapped != NULL && wrapped[group_end + 1] != 0)
            group_end++;
        bool any = false;
        for(uint32_t rr = r; rr <= group_end; rr++) {
            uint32_t len = row_content_len(textgrid, rr, rr < group_end);
            for(uint32_t c = 0; c < len; c++) {
                if(out_row >= drop) {
                    uint32_t dr = out_row - drop;
                    /* first cell of a continuation row marks the soft wrap */
                    if(out_col == 0 && (rr > r || c > 0))
                        nw[dr] = 1;
                    ng[(size_t)dr * cols + out_col] = p[(size_t)rr * old_cols + c];
                }
                any = true;
                out_col++;
                if(out_col == cols) {
                    out_col = 0;
                    out_row++;
                }
            }
        }
        if(!any) {
            out_row++;      /* an empty logical line keeps its own blank row */
        }
        else if(out_col != 0) {
            out_row++;
            out_col = 0;
        }
        r = group_end + 1;
    }
    if(drop > 0)
        nw[0] = 0;  /* the oldest kept row starts a logical line */

    /* map the cursor through the same cell offset */
    int32_t ny = (int32_t)(curs_off / cols) - (int32_t)drop;
    int32_t nx = (int32_t)(curs_off % cols);
    if(ny < 0) {
        ny = 0;
        nx = 0;
    }
    else if(ny >= (int32_t)keep_rows) {
        /* the offset landed exactly at the end of the text: keep the
         * pending-wrap state on the last row (curs_x == cols, like push
         * leaves it) so the next char appends instead of overwriting */
        ny = (int32_t)keep_rows - 1;
        nx = (int32_t)cols;
    }

    free(p);
    free(textgrid->wrapped);
    textgrid->grid = ng;
    textgrid->wrapped = nw;
    textgrid->cols = cols;
    textgrid->rows = keep_rows;
    textgrid->total_rows = new_total;
    textgrid->curs_x = nx;
    textgrid->curs_y = ny;

    /* tail_col describes the last produced row, recompute it for the new stride. */
    int32_t tail = 0;
    if(keep_rows > 0) {
        for(uint32_t c = cols; c > 0; c--) {
            if(ng[(size_t)(keep_rows - 1) * cols + (c - 1)].c != 0) {
                tail = (int32_t)(c - 1);
                break;
            }
        }
    }
    textgrid->tail_col = tail;
    textgrid->shift_total = 0;
    textgrid_invalidate(textgrid);
    return 0;
}

static int textgrid_expand(textgrid_t* grid, uint32_t rows, uint32_t* shifted) {
    if(shifted != NULL)
        *shifted = 0;
    if(grid == NULL || grid->cols == 0)
        return -1;

    /* Grow the allocated capacity (total_rows), not the produced-row count.
     * Callers that jump the cursor (textgrid_move_to/textgrid_put) compute
     * rows = y - total_rows + 1 so that total_rows becomes y+1; basing this on
     * grid->rows undershoots whenever produced < allocated, which leaves curs_y
     * and rows pointing past the allocation and corrupts the heap. This is what
     * stopped a full-screen app (vi) from using rows below the ones already
     * produced. */
    uint32_t rows_new = grid->total_rows + rows;
    if(rows_new <= grid->max_rows || grid->rows == 0) {
        /* Grow the capacity in EXPAND_ROWS batches as well: cursor-addressed
         * callers (textgrid_move_to) ask for the exact single row they need,
         * and honoring that literally reallocs + clears the grid once per
         * produced line during the initial fill (measured ~1ms/line). Keep
         * the exact request for the empty-grid bootstrap so a lone blank
         * cell does not allocate a full batch. total_rows stays the
         * allocation bound every caller checks against, so overshooting the
         * request is invisible to them; cap at max_rows. */
        if(rows > 0 && rows < EXPAND_ROWS) {
            rows = EXPAND_ROWS;
            rows_new = grid->total_rows + rows;
            if(rows_new > grid->max_rows)
                rows_new = grid->max_rows;
        }
        // Check for integer overflow.
        if(grid->cols == 0 ||
                rows_new > (UINT32_MAX / sizeof(textchar_t)) / grid->cols)
            return -1;
        /* grow the wrap flags first: if this fails the grid stays untouched */
        uint8_t* new_wrapped = (uint8_t*)realloc(grid->wrapped, rows_new);
        if(new_wrapped == NULL)
            return -1;
        grid->wrapped = new_wrapped;
        textchar_t* new_grid = (textchar_t*)realloc(grid->grid,
            (grid->cols*(rows_new))* sizeof(textchar_t));
        if(new_grid == NULL)
            return -1;
        grid->grid = new_grid;
        grid->total_rows = rows_new;
    }
    else {
        if(grid->grid == NULL)
            return -1;
            
        if(rows_new > grid->total_rows)
            rows_new = grid->total_rows;
        /* Shift in batches, not by the exact request: a newline asks for a
         * single row, and moving the whole grid per line costs a full-grid
         * memmove per produced line once the scrollback is full. Drop
         * EXPAND_ROWS at once so the next lines grow into the freed tail
         * without touching the grid again. */
        if(rows < EXPAND_ROWS)
            rows = EXPAND_ROWS;
        if(rows > grid->rows)
            rows = grid->rows;
        
        /* one bulk move instead of a per-row memcpy loop: this runs every
         * EXPAND_ROWS new lines once the scrollback is full */
        uint32_t keep = grid->rows - rows;
        if(shifted != NULL)
            *shifted = rows;
        memmove(grid->grid, grid->grid + (size_t)rows * grid->cols,
                (size_t)keep * grid->cols * sizeof(textchar_t));
        memmove(grid->wrapped, grid->wrapped + rows, keep);
        grid->rows -= rows;
        if(grid->rows == 0)
            grid->curs_y = 0;
        else
            grid->curs_y = grid->rows-1;
        grid->curs_x = 0;
        /* the oldest kept row lost its predecessor: it starts a logical line */
        if(grid->rows > 0)
            grid->wrapped[0] = 0;
        /* every produced row moved up by `rows`: translate the pending
         * dirty range with the content so painters can scroll the canvas
         * and repaint only the new rows instead of the whole viewport */
        grid->shift_total += rows;
        if(grid->dirty_end != INT32_MAX && grid->dirty_start < grid->dirty_end) {
            grid->dirty_start -= (int32_t)rows;
            grid->dirty_end -= (int32_t)rows;
            if(grid->dirty_end <= 0)
                textgrid_clean_dirty(grid);
            else if(grid->dirty_start < 0)
                grid->dirty_start = 0;
        }
        /* total_rows (the allocation) is unchanged by a shift: no realloc
         * here, the freed tail is simply cleared below */
    }

    /* clear the freed tail in one bulk memset per array: the per-cell loop
     * used to issue cols memsets per row (thousands of tiny calls per shift,
     * measured ~1ms of pure call overhead on a 200-column grid) */
    if(grid->total_rows > grid->rows) {
        uint32_t tail = grid->total_rows - grid->rows;
        memset(grid->grid + (size_t)grid->rows * grid->cols, 0,
                (size_t)tail * grid->cols * sizeof(textchar_t));
        memset(grid->wrapped + grid->rows, 0, tail);
    }
    return 0;
}

int textgrid_put(textgrid_t* textgrid, int x, int y, textchar_t* tc) {
    if(!textgrid || !textgrid->grid)
        return -1;
    if(x < 0 || (uint32_t)x >= textgrid->cols)
        return -1;
    if(y < 0)
        return -1;
    if((uint32_t)y >= textgrid->total_rows) {
        uint32_t expand = y-textgrid->total_rows+1;
        uint32_t shifted = 0;
        if(textgrid_expand(textgrid, expand, &shifted) != 0)
            return -1;
        /* a full scrollback shifts content up instead of growing: the
         * requested row moved with it, so translate instead of clamping
         * to the tail (which would skip the freed rows as blank lines) */
        y -= (int)shifted;
        /* expand may shift the scrollback instead of growing when the
         * request exceeds max_rows: re-clamp or the write lands past the
         * allocation and corrupts the heap */
        if((uint32_t)y >= textgrid->total_rows)
            y = (int)textgrid->total_rows - 1;
        if(y < 0)
            return -1;
    }

    uint32_t old_rows = textgrid->rows;
    if((uint32_t)(y + 1) >= textgrid->rows) {
        /* cursor-addressed rows are hard breaks, never soft-wrap continuations */
        for(uint32_t r = textgrid->rows; r <= (uint32_t)y; r++)
            textgrid->wrapped[r] = 0;
        textgrid->tail_col = x;
        textgrid->rows = y+1;
    }

    textgrid->grid[y * textgrid->cols + x] = *tc;
    textgrid_mark_dirty(textgrid, y, y+1);
    if(textgrid->rows > old_rows)
        textgrid_mark_dirty(textgrid, (int32_t)old_rows, (int32_t)textgrid->rows);
    return 0;
}

/* push may move the cursor across rows (wrap, newline, backspace, or a
 * scrollback shift resetting it): mark every row between the entry row
 * and the final one so partial repaints cover erased and written cells */
static void push_mark_dirty(textgrid_t* grid, int32_t r0) {
    int32_t r1 = grid->curs_y;
    int32_t lo = r0 < r1 ? r0 : r1;
    int32_t hi = r0 > r1 ? r0 : r1;
    textgrid_mark_dirty(grid, lo, hi+1);
}

int textgrid_push(textgrid_t* grid, textchar_t* tch) {
    if(grid == NULL || tch == 0 || grid->cols == 0)
        return -1;

    if(tch->c == 0)
        return 0;

    int32_t r0 = grid->curs_y;
    
    bool is_tail = false;
    if((uint32_t)(grid->curs_y + 1) >= grid->rows && grid->tail_col == grid->curs_x)
        is_tail = true;
    
    if(grid->grid != NULL && grid->rows > 0) {
        uint32_t at = (grid->curs_y)*grid->cols + (grid->curs_x > 0 ? grid->curs_x-1 : grid->curs_x);
        if(tch->c == KEY_BACKSPACE || tch->c == CONSOLE_LEFT) {
            memset(&grid->grid[at], 0, sizeof(textchar_t));
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
            push_mark_dirty(grid, r0);
            return 0;
        }

        if(grid->grid[at].c == '\r' || grid->grid[at].c == '\n') {
        //if(tch->c == '\r' || tch->c == '\n') {
            int col_index = grid->curs_x + 1;
            //int col_index = grid->curs_x;
            while((uint32_t)col_index < grid->cols) {
                at = (grid->curs_y)*grid->cols + col_index;
                memset(&grid->grid[at], 0, sizeof(textchar_t));
                col_index++;
            }
            grid->curs_x = grid->cols-1;
            if(is_tail) {
                grid->tail_col = grid->curs_x;
            }
        }
    }

    bool new_line = false;
    bool soft_wrap = false;
    /* curs_x is the column about to be written, so it may address the last
     * column (cols-1); wrapping here one column early would spill the final
     * char of every full-width line onto the next row and grow rows by a
     * phantom line, which scrolls the visible window up one row. */
    if((uint32_t)grid->curs_x >= grid->cols || grid->rows == 0) { //new line needed
        new_line = true;
        soft_wrap = (grid->rows != 0);  /* overflow wrap, not the first row */
        grid->curs_x = 0;
    }

    if(new_line) {
        if((uint32_t)(grid->curs_y + 1) >= grid->total_rows) { //expand grid
            if(textgrid_expand(grid, EXPAND_ROWS, NULL) != 0)
                return -1;
        }
        if(grid->rows != 0)
            grid->curs_y++;
        if((uint32_t)grid->curs_y >= grid->rows)
            grid->rows = grid->curs_y+1;
        /* remember soft wraps so a later width change can reflow the text */
        grid->wrapped[grid->curs_y] = soft_wrap ? 1 : 0;
    }

    uint32_t at = (grid->curs_y)*grid->cols + grid->curs_x;
    grid->grid[at] = *tch;
    if(is_tail) {
        grid->tail_col = grid->curs_x;
    }
    grid->curs_x++;

    if(tch->c == '\n' || tch->c == '\r') {
        if((uint32_t)(grid->curs_y + 1) >= grid->total_rows) {
            if(textgrid_expand(grid, EXPAND_ROWS, NULL) != 0)
                return -1;
        }
        if(grid->rows != 0)
            grid->curs_y++;
        if((uint32_t)grid->curs_y >= grid->rows)
            grid->rows = grid->curs_y+1;
        grid->wrapped[grid->curs_y] = 0;  /* explicit break, never a soft wrap */
        grid->curs_x = 0;
        push_mark_dirty(grid, r0);
        return 0;
    }
    push_mark_dirty(grid, r0);
    return 0;
}

int textgrid_move_to(textgrid_t* textgrid, int32_t x, int32_t y) {
    if(textgrid == NULL)
        return -1;
    if(textgrid->rows == 0 || textgrid->cols == 0)
        return -1;

    if(x < 0)
        x = 0;
    if(y < 0)
        y = 0;
    if((uint32_t)x >= textgrid->cols)
        x = textgrid->cols - 1;
    
    if((uint32_t)y >= textgrid->total_rows) {
        uint32_t expand = y-textgrid->total_rows+1;
        uint32_t shifted = 0;
        if(textgrid_expand(textgrid, expand, &shifted) != 0)
            return -1;
        /* expand may shift instead of grow past max_rows: the target row
         * moved up with the content, translate before the final clamp */
        y -= (int32_t)shifted;
        if((uint32_t)y >= textgrid->total_rows)
            y = (int32_t)textgrid->total_rows - 1;
        if(y < 0)
            return -1;
    }

    int32_t old_y = textgrid->curs_y;
    uint32_t old_rows = textgrid->rows;
    if((uint32_t)(y + 1) >= textgrid->rows) {
        /* cursor-addressed rows are hard breaks, never soft-wrap continuations */
        for(uint32_t r = textgrid->rows; r <= (uint32_t)y; r++)
            textgrid->wrapped[r] = 0;
        textgrid->tail_col = x;
        textgrid->rows = y+1;
    }

    textgrid->curs_x = x;	
    textgrid->curs_y = y;	
    /* the cursor bar is painted on its row: refresh both the row it left
     * and the row it landed on */
    textgrid_mark_dirty(textgrid, old_y, old_y+1);
    textgrid_mark_dirty(textgrid, y, y+1);
    if(textgrid->rows > old_rows)
        textgrid_mark_dirty(textgrid, (int32_t)old_rows, (int32_t)textgrid->rows);
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
            textgrid->cols == 0 || (uint32_t)start_row >= textgrid->rows ||
            w == 0 || h == 0 || row_height == 0)
        return -1;
    
    uint32_t chw = w / textgrid->cols;
    uint32_t chh = row_height;
    /* only the rows the viewport can show (round up for a partial row);
     * painting the whole scrollback here is wasted work when scrolled back */
    uint32_t end_row = (uint32_t)start_row + (h + chh - 1) / chh;
    if(end_row > textgrid->rows || end_row < (uint32_t)start_row)
        end_row = textgrid->rows;
    for(uint32_t i = (uint32_t)start_row; i < end_row; i++) {
        int cy = y + ((i-start_row)*chh);
        for(uint32_t j = 0; j < textgrid->cols; j++) {
            uint32_t at = i*textgrid->cols + j;
            textchar_t* ch = &textgrid->grid[at];
            int cx = x + (j*chw);
            draw_char_func(g, ch, cx, cy, chw, chh, (int32_t)i, (int32_t)j, draw_arg);
        }
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
