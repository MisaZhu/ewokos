#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gterminal/gterminal.h>
#include <ewoksys/keydef.h>
#include <font/font.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TERM_STATE_UNDERLINE  0x01
#define TERM_STATE_REVERSE    0x02
#define TERM_STATE_FLASH      0x04
#define TERM_STATE_HIDE       0x08
#define TERM_STATE_HIGH_LIGHT 0x10

#define  ESC_CMD 033

static gpos_t get_pos(gterminal_t* terminal, int x, int y, int w, int h) {
    int row = terminal->textgrid->curs_y - terminal->textgrid_start_row;
    if(row < 0)
        row = 0;

    uint32_t at = terminal->textgrid->curs_x + (terminal->textgrid->curs_y*terminal->textgrid->cols);
    //int col = terminal->textgrid->curs_x + 1;
    int col = terminal->textgrid->curs_x;
    if(terminal->textgrid->grid[at].c == '\n') {
        col = 0;
        row++;
    }

    gpos_t ret = { col*terminal->char_w, row*terminal->char_h};
    return ret;
}


static void draw_curs(gterminal_t* terminal, graph_t* g, int x, int y, int w, int h) {
    if(!terminal->flash_show || !terminal->show_curs || terminal->scroll_offset != 0)
        return;
    gpos_t pos = get_pos(terminal, x, y, w, h);
    graph_fill_rect(g, x+ pos.x, y + pos.y+4, 4, terminal->char_h-4, terminal->fg_color);
}

static uint32_t g_color(gterminal_t* terminal, uint32_t esc_color, uint8_t fg) {
    if(fg != 0)
        esc_color += 10;
    if(esc_color < 40 || esc_color > 47)
        return 0;

    uint32_t colors[8] = {
            0xff000000, //BLACK
            0xffbb0000, //RED
            0xff00bb00, //GREEN
            0xffbbbb00, //YELLOW
            0xff0000bb, //BLUE
            0xffbb0066, //PURPLE
            0xff00bbbb, //CYAN
            0xffbbbbbb  //WHITE
        };

    uint32_t colorsHi[8] = {
            0xff000000, //BLACK
            0xffff0000, //RED
            0xff00ff00, //GREEN
            0xffffff00, //YELLOW
            0xff0000ff, //BLUE
            0xffff0088, //PURPLE
            0xff00ffff, //CYAN
            0xffffffff  //WHITE
        };

    if((terminal->term_conf.state & TERM_STATE_HIGH_LIGHT) != 0)
        return colorsHi[esc_color-40];
    return colors[esc_color-40];
}

static uint32_t color256(uint8_t idx) {
    if(idx < 8) {
        uint32_t colors[8] = {
            0xff000000, 0xffbb0000, 0xff00bb00, 0xffbbbb00,
            0xff0000bb, 0xffbb0066, 0xff00bbbb, 0xffbbbbbb
        };
        return colors[idx];
    }
    else if(idx < 16) {
        uint32_t colors[8] = {
            0xff000000, 0xffff0000, 0xff00ff00, 0xffffff00,
            0xff0000ff, 0xffff0088, 0xff00ffff, 0xffffffff
        };
        return colors[idx - 8];
    }
    else if(idx < 232) {
        idx -= 16;
        uint8_t r = (idx / 36) * 51;
        uint8_t g = ((idx % 36) / 6) * 51;
        uint8_t b = (idx % 6) * 51;
        return 0xff000000 | (r << 16) | (g << 8) | b;
    }
    else {
        uint8_t gray = (idx - 232) * 10 + 8;
        return 0xff000000 | (gray << 16) | (gray << 8) | gray;
    }
}

static void do_esc_color(gterminal_t* terminal, uint16_t* values, uint8_t vnum) {
    for(uint8_t i=0; i<vnum; i++) {
        uint16_t v = values[i];
        if(v == 0) {
            terminal->term_conf.state = 0;
            terminal->term_conf.bg_color = 0;
            terminal->term_conf.fg_color = 0;
            terminal->term_conf.set = 0;
        }
        else if(v == 1) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state |= TERM_STATE_HIGH_LIGHT;
        }
        else if(v == 4) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state |= TERM_STATE_UNDERLINE;
        }
        else if(v == 5) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state |= TERM_STATE_FLASH;
        }
        else if(v == 7) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state |= TERM_STATE_REVERSE;
        }
        else if(v == 27) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state &= ~TERM_STATE_REVERSE;
        }
        else if(v == 8) {
            terminal->term_conf.set = 1;
            terminal->term_conf.state |= TERM_STATE_HIDE;
        }
        else if(v == 38) {
            terminal->term_conf.set = 1;
            if(i + 2 < vnum && values[i+1] == 5) {
                terminal->term_conf.fg_color = color256(values[i+2]);
                i += 2;
            }
            else if(i + 4 < vnum && values[i+1] == 2) {
                terminal->term_conf.fg_color = 0xff000000 |
                    ((values[i+2] & 0xff) << 16) |
                    ((values[i+3] & 0xff) << 8) |
                    (values[i+4] & 0xff);
                i += 4;
            }
            else {
                terminal->term_conf.fg_color = 0;
            }
        }
        else if(v == 39) {
            terminal->term_conf.set = 1;
            terminal->term_conf.fg_color = 0;
        }
        else if(v >= 30 && v <= 37) {
            terminal->term_conf.set = 1;
            terminal->term_conf.fg_color = g_color(terminal, v + 10, 0);
        }
        else if(v == 48) {
            terminal->term_conf.set = 1;
            if(i + 2 < vnum && values[i+1] == 5) {
                terminal->term_conf.bg_color = color256(values[i+2]);
                i += 2;
            }
            else if(i + 4 < vnum && values[i+1] == 2) {
                terminal->term_conf.bg_color = 0xff000000 |
                    ((values[i+2] & 0xff) << 16) |
                    ((values[i+3] & 0xff) << 8) |
                    (values[i+4] & 0xff);
                i += 4;
            }
            else {
                terminal->term_conf.bg_color = 0;
            }
        }
        else if(v == 49) {
            terminal->term_conf.set = 1;
            terminal->term_conf.bg_color = 0;
        }
        else if(v >= 40 && v <= 47) {
            terminal->term_conf.set = 1;
            terminal->term_conf.bg_color = g_color(terminal, v, 0);
        }
    }
}

static void run_esc_cmd(gterminal_t* terminal, UNICODE16 cmd, uint16_t* values, uint8_t vnum, bool private_mode) {
    textgrid_t* tg = terminal->textgrid;
    int32_t top_row = terminal->textgrid_start_row;
    int32_t bottom_row = top_row + (int32_t)terminal->rows - 1;
    uint32_t eff_scroll_top = terminal->scroll_top;
    uint32_t eff_scroll_bottom = (terminal->scroll_bottom < terminal->rows) ?
        terminal->scroll_bottom : (terminal->rows - 1);

    if(cmd == 'm') {
        do_esc_color(terminal, values, vnum);
    }
    else if(cmd == 'J') {
        textchar_t tch = {0};
        if(terminal->term_conf.set) {
            tch.bg_color = terminal->term_conf.bg_color;
        }
        
        if(vnum == 0 || values[0] == 0) {
            for(uint16_t x = tg->curs_x; x < tg->cols; x++)
                textgrid_put(tg, x, tg->curs_y, &tch);
            for(int32_t y = tg->curs_y + 1; y <= bottom_row && y < (int32_t)tg->rows; y++)
                for(uint16_t x = 0; x < tg->cols; x++)
                    textgrid_put(tg, x, y, &tch);
        }
        else if(values[0] == 1) {
            for(int32_t y = top_row; y < tg->curs_y && y < (int32_t)tg->rows; y++)
                for(uint16_t x = 0; x < tg->cols; x++)
                    textgrid_put(tg, x, y, &tch);
            for(uint16_t x = 0; x <= tg->curs_x; x++)
                textgrid_put(tg, x, tg->curs_y, &tch);
        }
        else if(values[0] == 2) {
            for(int32_t y = top_row; y <= bottom_row && y < (int32_t)tg->rows; y++)
                for(uint16_t x = 0; x < tg->cols; x++)
                    textgrid_put(tg, x, y, &tch);
            textgrid_move_to(tg, 0, top_row);
        }
        else if(values[0] == 3) {
            textgrid_clear(tg);
            textgrid_move_to(tg, 0, top_row);
        }
    }
    else if(cmd == 'K') {
        textchar_t tch = {0};
        if(vnum == 0 || values[0] == 0) {
            for(uint16_t x = tg->curs_x; x < tg->cols; x++)
                textgrid_put(tg, x, tg->curs_y, &tch);
        }
        else if(values[0] == 1) {
            for(uint16_t x = 0; x <= tg->curs_x; x++)
                textgrid_put(tg, x, tg->curs_y, &tch);
        }
        else if(values[0] == 2) {
            for(uint16_t x = 0; x < tg->cols; x++)
                textgrid_put(tg, x, tg->curs_y, &tch);
        }
    }
    else if(cmd == 'H' || cmd == 'f') {
        uint16_t row = (vnum >= 1 && values[0] > 0) ? values[0] - 1 : 0;
        uint16_t col = (vnum >= 2 && values[1] > 0) ? values[1] - 1 : 0;
        if(row >= terminal->rows) row = terminal->rows - 1;
        if(col >= tg->cols) col = tg->cols - 1;
        textgrid_move_to(tg, col, row + top_row);
    }
    else if(cmd == 'A') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y - n;
        if(y < top_row) y = top_row;
        textgrid_move_to(tg, tg->curs_x, y);
    }
    else if(cmd == 'B') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y + n;
        if(y > bottom_row) y = bottom_row;
        textgrid_move_to(tg, tg->curs_x, y);
    }
    else if(cmd == 'D') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t x = tg->curs_x - n;
        if(x < 0) x = 0;
        textgrid_move_to(tg, x, tg->curs_y);
    }
    else if(cmd == 'C') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t x = tg->curs_x + n;
        if(x >= (int32_t)tg->cols) x = tg->cols - 1;
        textgrid_move_to(tg, x, tg->curs_y);
    }
    else if(cmd == 'E') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y + n;
        if(y > bottom_row) y = bottom_row;
        textgrid_move_to(tg, 0, y);
    }
    else if(cmd == 'F') {
        int16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y - n;
        if(y < top_row) y = top_row;
        textgrid_move_to(tg, 0, y);
    }
    else if(cmd == 'G') {
        uint16_t x = (vnum > 0 && values[0] > 0) ? values[0] - 1 : 0;
        if(x >= tg->cols) x = tg->cols - 1;
        textgrid_move_to(tg, x, tg->curs_y);
    }
    else if(cmd == 'd') {
        uint16_t row = (vnum > 0 && values[0] > 0) ? values[0] - 1 : 0;
        if(row >= terminal->rows) row = terminal->rows - 1;
        textgrid_move_to(tg, tg->curs_x, row + top_row);
    }
    else if(cmd == 's') {
        terminal->curs_pos.x = tg->curs_x;
        terminal->curs_pos.y = tg->curs_y;
    }
    else if(cmd == 'u') {
        textgrid_move_to(tg, terminal->curs_pos.x, terminal->curs_pos.y);
    }
    else if(cmd == 'l') {
        if(private_mode) {
            if(vnum > 0 && values[0] == 25)
                terminal->show_curs = false;
        }
    }
    else if(cmd == 'h') {
        if(private_mode) {
            if(vnum > 0 && values[0] == 25)
                terminal->show_curs = true;
        }
    }
    else if(cmd == 'L') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t s_bottom = (int32_t)eff_scroll_bottom + top_row;
        int32_t curs_abs_y = tg->curs_y;
        if(n > (uint16_t)(s_bottom - curs_abs_y + 1))
            n = s_bottom - curs_abs_y + 1;
        for(int32_t y = s_bottom; y >= curs_abs_y + n; y--) {
            for(uint16_t x = 0; x < tg->cols; x++)
                tg->grid[y * tg->cols + x] = tg->grid[(y - n) * tg->cols + x];
        }
        for(uint16_t i = 0; i < n; i++) {
            for(uint16_t x = 0; x < tg->cols; x++) {
                textchar_t tch = {0};
                tg->grid[(curs_abs_y + i) * tg->cols + x] = tch;
            }
        }
    }
    else if(cmd == 'M') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t s_bottom = (int32_t)eff_scroll_bottom + top_row;
        int32_t curs_abs_y = tg->curs_y;
        if(n > (uint16_t)(s_bottom - curs_abs_y + 1))
            n = s_bottom - curs_abs_y + 1;
        for(int32_t y = curs_abs_y; y + n <= s_bottom; y++) {
            for(uint16_t x = 0; x < tg->cols; x++)
                tg->grid[y * tg->cols + x] = tg->grid[(y + n) * tg->cols + x];
        }
        for(int32_t y = s_bottom - n + 1; y <= s_bottom; y++) {
            if(y >= curs_abs_y) {
                for(uint16_t x = 0; x < tg->cols; x++) {
                    textchar_t tch = {0};
                    tg->grid[y * tg->cols + x] = tch;
                }
            }
        }
    }
    else if(cmd == 'P') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y;
        int32_t x = tg->curs_x;
        if(n > tg->cols - x) n = tg->cols - x;
        for(int32_t i = x; i < (int32_t)tg->cols - n; i++)
            tg->grid[y * tg->cols + i] = tg->grid[y * tg->cols + i + n];
        for(int32_t i = (int32_t)tg->cols - n; i < (int32_t)tg->cols; i++) {
            textchar_t tch = {0};
            tg->grid[y * tg->cols + i] = tch;
        }
    }
    else if(cmd == '@') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t y = tg->curs_y;
        int32_t x = tg->curs_x;
        if(n > tg->cols - x) n = tg->cols - x;
        for(int32_t i = (int32_t)tg->cols - 1; i >= x + n; i--)
            tg->grid[y * tg->cols + i] = tg->grid[y * tg->cols + i - n];
        for(int32_t i = x; i < x + (int32_t)n; i++) {
            textchar_t tch = {0};
            tg->grid[y * tg->cols + i] = tch;
        }
    }
    else if(cmd == 'X') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        for(uint16_t i = 0; i < n; i++) {
            uint16_t x = tg->curs_x + i;
            if(x >= tg->cols) break;
            textchar_t tch = {0};
            tg->grid[tg->curs_y * tg->cols + x] = tch;
        }
    }
    else if(cmd == 'S') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t s_top = (int32_t)eff_scroll_top + top_row;
        int32_t s_bottom = (int32_t)eff_scroll_bottom + top_row;
        uint32_t region_size = s_bottom - s_top + 1;
        if(n >= region_size) {
            for(int32_t y = s_top; y <= s_bottom; y++)
                for(uint16_t x = 0; x < tg->cols; x++) {
                    textchar_t tch = {0};
                    tg->grid[y * tg->cols + x] = tch;
                }
        } else {
            for(int32_t y = s_top; y + n <= s_bottom; y++)
                for(uint16_t x = 0; x < tg->cols; x++)
                    tg->grid[y * tg->cols + x] = tg->grid[(y + n) * tg->cols + x];
            for(int32_t y = s_bottom - n + 1; y <= s_bottom; y++)
                for(uint16_t x = 0; x < tg->cols; x++) {
                    textchar_t tch = {0};
                    tg->grid[y * tg->cols + x] = tch;
                }
        }
    }
    else if(cmd == 'T') {
        if(tg->grid == NULL || tg->cols == 0) return;
        uint16_t n = (vnum > 0) ? values[0] : 1;
        int32_t s_top = (int32_t)eff_scroll_top + top_row;
        int32_t s_bottom = (int32_t)eff_scroll_bottom + top_row;
        uint32_t region_size = s_bottom - s_top + 1;
        if(n >= region_size) {
            for(int32_t y = s_top; y <= s_bottom; y++)
                for(uint16_t x = 0; x < tg->cols; x++) {
                    textchar_t tch = {0};
                    tg->grid[y * tg->cols + x] = tch;
                }
        } else {
            for(int32_t y = s_bottom; y >= s_top + (int32_t)n; y--)
                for(uint16_t x = 0; x < tg->cols; x++)
                    tg->grid[y * tg->cols + x] = tg->grid[(y - n) * tg->cols + x];
            for(uint32_t y = s_top; y < s_top + n; y++)
                for(uint16_t x = 0; x < tg->cols; x++) {
                    textchar_t tch = {0};
                    tg->grid[y * tg->cols + x] = tch;
                }
        }
    }
    else if(cmd == 'r') {
        uint32_t top = (vnum >= 1 && values[0] > 0) ? values[0] - 1 : 0;
        uint32_t bottom = (vnum >= 2 && values[1] > 0) ? values[1] - 1 : terminal->rows - 1;
        if(top < bottom && bottom < terminal->rows) {
            terminal->scroll_top = top;
            terminal->scroll_bottom = bottom;
        } else {
            terminal->scroll_top = 0;
            terminal->scroll_bottom = UINT32_MAX;
        }
        textgrid_move_to(tg, 0, top_row);
    }
}

static uint32_t do_esc_cmd(gterminal_t* terminal, UNICODE16* uni, uint32_t from, uint32_t size) {
    UNICODE16 c = uni[from];
    if(c == 0)
        return from;

    from++;
    if(from >= size || c != '[')
        return from;

    uint16_t values[8] = { 0 };
    uint8_t vnum = 0;
    bool private_mode = false;
    c = uni[from++];
    if(from > size || c == 0)
        return from;
    
    while(true) {
        if(c == '?') {
            private_mode = true;
            c = uni[from++];
            if(from > size || c == 0)
                return from;
            continue;
        }
        else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            run_esc_cmd(terminal, c, values, vnum, private_mode);
            from--;
            break;
        }
        else if(c >= '0' && c <= '9') {
            char vstr[4] = {0};
            memset(vstr, 0, 4);
            vstr[0] = c;
            for(uint8_t i=1; i< 4; i++) {
                c = uni[from++];
                if(from > size || c == 0)
                    return from;
                if(c < '0' || c > '9') {
                    if(vnum < 7) {
                        values[vnum] = atoi(vstr);
                        vnum++;
                    }
                    break;
                }
                vstr[i] = c;
            }

            if(c == ';') {
                c = uni[from++];
                if(from >= size || c == 0)
                    return from;
            }
        }
        else {
            from--;
            break;
        }
    }
    return from;
}

void gterminal_init(gterminal_t* terminal) {
    memset(terminal, 0, sizeof(gterminal_t));
    terminal->textgrid = textgrid_new();
    terminal->show_curs = true;
    terminal->flash_show = true;
    terminal->scroll_top = 0;
    terminal->scroll_bottom = UINT32_MAX;
}

void gterminal_close(gterminal_t* terminal) {
    if(terminal->font != NULL) {
        font_free(terminal->font);
        terminal->font = NULL;
    }
    if(terminal->textgrid) {
        textgrid_free(terminal->textgrid);
        terminal->textgrid = NULL;
    }
}

bool gterminal_scroll(gterminal_t* terminal, int direction) {
    int32_t old_offset = terminal->scroll_offset;
    if(direction == 0)
        terminal->scroll_offset = 0;
    else if(direction < 0)
        terminal->scroll_offset -= terminal->rows / 4;
    else
        terminal->scroll_offset += terminal->rows / 4;

    if(terminal->scroll_offset > 0)
        terminal->scroll_offset = 0;
    else if(abs(terminal->scroll_offset) >= terminal->textgrid_start_row)
        terminal->scroll_offset = -terminal->textgrid_start_row;

    if(terminal->scroll_offset != old_offset)
        return true;
    return false;
}

static void gterminal_draw_char(graph_t* g,
        textchar_t* tch,
        int chx, int chy,
        uint32_t chw, uint32_t chh,
        void*p) {
    gterminal_t* terminal = (gterminal_t*)p;
    uint32_t fg = tch->color, bg = tch->bg_color;

    if(fg == 0)
        fg = terminal->fg_color;
    if(bg == 0)
        bg = terminal->bg_color;

    if((tch->state & TERM_STATE_REVERSE) != 0) {
        uint32_t tmp_c = fg;
        fg = bg;
        bg = tmp_c;
    }

    bg = (bg & 0x00ffffff) | (terminal->transparent  << 24);

    if(bg != 0 && bg != terminal->bg_color) 
        graph_fill_rect(g, chx, chy, chw, chh, bg);
    
    if((tch->state & TERM_STATE_HIDE) == 0 && tch->c >= 27) {
        if((tch->state & TERM_STATE_FLASH) != 0 && !terminal->flash_show) {
            // Flash mode: hide character when flash_show is false
        } else {
            if((tch->state & TERM_STATE_UNDERLINE) != 0)
                graph_line(g, chx, chy+chh-1, chx+chw,  chy+chh-1, fg);

            graph_draw_char_font_fixed(g, chx, chy, tch->c,
                    terminal->font, terminal->font_size, fg, chw, 0);
            if((tch->state & TERM_STATE_HIGH_LIGHT) != 0)
                graph_draw_char_font_fixed(g, chx+1, chy, tch->c,
                        terminal->font, terminal->font_size, fg, chw, 0);
        }
    }
}

void gterminal_paint(gterminal_t* terminal, graph_t* g, int x, int y, int w, int h) {
    if(terminal->textgrid->cols == 0 || terminal->textgrid->rows == 0)
        return;

	textgrid_paint(g, terminal->textgrid,
            gterminal_draw_char, terminal,
            terminal->textgrid_start_row + terminal->scroll_offset, terminal->char_h,
            x, y, w, h);
	draw_curs(terminal, g, x, y, w, h);
}

static bool _in_esc = false;
static uint16_t _esc_buf[8] = {0};
static uint16_t _esc_size = 0;

void gterminal_put(gterminal_t* terminal, const char* buf, int size) {
    uint16_t* unicode = (uint16_t*)malloc((size+1)*2);
    if(!unicode)
        return;
    size = utf82unicode((unsigned char*)buf, size, unicode);

    for(uint32_t i=0; i<size; i++) {
        UNICODE16 c = unicode[i];
        if(c == 0)
            break;
        
        if(c == ESC_CMD) {
            _in_esc = true;
            _esc_size = 0;
            continue;
        }
        //else
            //klog("put %d, %d, (%c)\n", size, i, c);

        if(_in_esc) {
            _esc_buf[_esc_size] = c;
            _esc_size++;
            if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                do_esc_cmd(terminal, _esc_buf, 0, _esc_size);
                _esc_size = 0;
                _in_esc = false;
            }
            continue;
        }

        textchar_t tch;
        tch.c = c;
        if(terminal->term_conf.set == 0) {
            terminal->term_conf.fg_color = 0;
            terminal->term_conf.bg_color = 0;
            terminal->term_conf.state = 0;
        }
        tch.bg_color = terminal->term_conf.bg_color;
        tch.color = terminal->term_conf.fg_color;
        tch.state = terminal->term_conf.state;
        textgrid_push(terminal->textgrid, &tch);
    }

    free(unicode);
    int32_t start_row = (int32_t)terminal->textgrid->rows - (int32_t)terminal->rows;
    terminal->textgrid_start_row = start_row < 0 ? 0:start_row;
}

void gterminal_flash(gterminal_t* terminal) {
    terminal->flash_show = !terminal->flash_show;
}

void gterminal_set_max_rows(gterminal_t* terminal, uint32_t max_rows) {
    textgrid_set_max_rows(terminal->textgrid, max_rows);
}

void gterminal_resize(gterminal_t* terminal, uint32_t gw, uint32_t gh) {
    if(terminal->font == NULL)
        return;
    int32_t font_w = font_get_width(terminal->font, terminal->font_size);
    if(font_w <= 0 || font_w >= terminal->font_size) {
        font_char_size('M', terminal->font, terminal->font_size, &font_w, NULL);
    }
    font_w += terminal->char_space;

    int32_t font_h = font_get_height(terminal->font, terminal->font_size);
    font_h += terminal->line_space;
    if(font_w == 0 || font_h == 0)
        return;

    terminal->char_w = font_w;
    terminal->char_h = font_h;
    terminal->rows = gh / font_h;
    terminal->cols = gw / font_w;

    textgrid_reset(terminal->textgrid, terminal->cols);
    terminal->scroll_top = 0;
    terminal->scroll_bottom = UINT32_MAX;
    int32_t start_row = (int32_t)terminal->textgrid->rows - (int32_t)terminal->rows;
    terminal->textgrid_start_row = start_row < 0 ? 0:start_row;
}

#ifdef __cplusplus
}
#endif

