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
    int col = terminal->textgrid->curs_x + 1;
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
    graph_fill(g, x+ pos.x, y + pos.y+4, 4, terminal->char_h-4, terminal->fg_color);
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
        else if(v >= 30 && v <= 39) {
            terminal->term_conf.set = 1;
            terminal->term_conf.fg_color = g_color(terminal, v, 1);
        }
        else if(v >= 40 && v <= 49) {
            terminal->term_conf.set = 1;
            terminal->term_conf.bg_color = g_color(terminal, v, 0);
        }
    }
}

static void run_esc_cmd(gterminal_t* terminal, UNICODE16 cmd, uint16_t* values, uint8_t vnum) {
    if(cmd == 'm') { //color and state
        do_esc_color(terminal, values, vnum);
    }
    else if(cmd == 'J') { //clear screen
        if(vnum == 0 || values[0] == 0) {
            // Clear from cursor to end of screen
            textchar_t tch = {0};
            // Clear current line from cursor to end
            for(uint16_t x = terminal->textgrid->curs_x; x < terminal->textgrid->cols; x++) {
                textgrid_put(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row, &tch);
            }
            // Clear all lines below cursor
            for(uint16_t y = terminal->textgrid->curs_y + 1 + terminal->textgrid_start_row; y < terminal->textgrid->rows; y++) {
                for(uint16_t x = 0; x < terminal->textgrid->cols; x++) {
                    textgrid_put(terminal->textgrid, x, y, &tch);
                }
            }
        }
        else if(values[0] == 1) {
            // Clear from beginning of screen to cursor
            textchar_t tch = {0};
            // Clear current line from beginning to cursor
            for(uint16_t x = 0; x <= terminal->textgrid->curs_x; x++) {
                textgrid_put(terminal->textgrid, x, terminal->textgrid->curs_y+terminal->textgrid_start_row, &tch);
            }
            // Clear all lines above cursor
            for(uint16_t y = 0; y < terminal->textgrid->curs_y; y++) {
                for(uint16_t x = 0; x < terminal->textgrid->cols; x++) {
                    textgrid_put(terminal->textgrid, x, y+terminal->textgrid_start_row, &tch);
                }
            }
        }
        else if(values[0] == 2) {
            // Clear entire screen
            textgrid_clear(terminal->textgrid);
            textchar_t tch = {0};
            for(uint16_t y = terminal->textgrid_start_row; y < terminal->textgrid->rows; y++) {
                for(uint16_t x = 0; x < terminal->textgrid->cols; x++) {
                    textgrid_put(terminal->textgrid, x, y, &tch);
                }
            }
            textgrid_move_to(terminal->textgrid, 0, terminal->textgrid_start_row);
        }
    }
    else if(cmd == 'K') { //clear line
        if(vnum == 0 || values[0] == 0) {
            // Clear from cursor to end of line
            for(uint16_t x = terminal->textgrid->curs_x; x < terminal->textgrid->cols; x++) {
                textchar_t tch = {0};
                textgrid_put(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row, &tch);
            }
        }
        else if(values[0] == 1) {
            // Clear from beginning of line to cursor
            for(uint16_t x = 0; x <= terminal->textgrid->curs_x; x++) {
                textchar_t tch = {0};
                textgrid_put(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row, &tch);
            }
        }
        else if(values[0] == 2) {
            // Clear entire line
            for(uint16_t x = 0; x < terminal->textgrid->cols; x++) {
                textchar_t tch = {0};
                textgrid_put(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row, &tch);
            }
        }
    }
    else if(cmd == 'H' || cmd == 'f') { //move curs y,x
        uint16_t row = (vnum >= 1 && values[0] > 0) ? values[0] - 1 : 0;
        uint16_t col = (vnum >= 2 && values[1] > 0) ? values[1] - 1 : 0;
        klog("row: %d, col: %d\n", row, col);
        textgrid_move_to(terminal->textgrid, col, row + terminal->textgrid_start_row);
    }
    else if(cmd == 'A') { //move curs up
        int16_t y = (int16_t)terminal->textgrid->curs_y - (vnum > 0 ? values[0] : 1);
        if(y < 0)
            y = 0;
        textgrid_move_to(terminal->textgrid, terminal->textgrid->curs_x, y + terminal->textgrid_start_row);
    }
    else if(cmd == 'B') { //move curs down
        uint16_t y = terminal->textgrid->curs_y + (vnum > 0 ? values[0] : 1);
        if(y >= terminal->textgrid->rows)
            y = terminal->textgrid->rows - 1;
        textgrid_move_to(terminal->textgrid, terminal->textgrid->curs_x, y + terminal->textgrid_start_row);
    }
    else if(cmd == 'D') { //move curs left
        int16_t x = (int16_t)terminal->textgrid->curs_x - (vnum > 0 ? values[0] : 1);
        if(x < 0)
            x = 0;
        textgrid_move_to(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row);
    }
    else if(cmd == 'C') { //move curs right
        uint16_t x = terminal->textgrid->curs_x + (vnum > 0 ? values[0] : 1);
        if(x >= terminal->textgrid->cols)
            x = terminal->textgrid->cols - 1;
        textgrid_move_to(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row);
    }
    else if(cmd == 'E') { //move to beginning of next line
        uint16_t y = terminal->textgrid->curs_y + (vnum > 0 ? values[0] : 1);
        if(y >= terminal->textgrid->rows)
            y = terminal->textgrid->rows - 1;
        textgrid_move_to(terminal->textgrid, 0, y + terminal->textgrid_start_row);
    }
    else if(cmd == 'F') { //move to beginning of previous line
        int16_t y = (int16_t)terminal->textgrid->curs_y - (vnum > 0 ? values[0] : 1);
        if(y < 0)
            y = 0;
        textgrid_move_to(terminal->textgrid, 0, y + terminal->textgrid_start_row);
    }
    else if(cmd == 'G') { //move to column
        uint16_t x = (vnum > 0 && values[0] > 0) ? values[0] - 1 : 0;
        if(x >= terminal->textgrid->cols)
            x = terminal->textgrid->cols - 1;
        textgrid_move_to(terminal->textgrid, x, terminal->textgrid->curs_y + terminal->textgrid_start_row);
    }
    else if(cmd == 's') { //save curs pos
        terminal->curs_pos.x = terminal->textgrid->curs_x;
        terminal->curs_pos.y = terminal->textgrid->curs_y;
    }
    else if(cmd == 'u') { //restore curs pos
        textgrid_move_to(terminal->textgrid, terminal->curs_pos.x, terminal->curs_pos.y + terminal->textgrid_start_row);
    }
    else if(cmd == 'l') { //hide curs or reset mode
        if(vnum > 0) {
            // Handle mode reset
            if(values[0] == 25) {
                terminal->show_curs = false;
            }
        }
        else {
            terminal->show_curs = false;
        }
    }
    else if(cmd == 'h') { //show curs or set mode
        if(vnum > 0) {
            // Handle mode set
            if(values[0] == 25) {
                terminal->show_curs = true;
            }
        }
        else {
            terminal->show_curs = true;
        }
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
    c = uni[from++];
    if(from > size || c == 0)
        return from;
    
    while(true) {
        if(c == '?') { //TODO hide/flashShow curs
            c = uni[from++];
            if(from > size || c == 0)
                return from;
            continue;
        }
        else if((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
            run_esc_cmd(terminal, c, values, vnum);
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
}

void gterminal_close(gterminal_t* terminal) {
    if(terminal->font != NULL)
        font_free(terminal->font);
    if(terminal->textgrid)
        textgrid_free(terminal->textgrid);
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

    if(bg != 0 && bg != terminal->bg_color) 
        graph_fill(g, chx, chy, chw, chh, bg);
    
    if((tch->state & TERM_STATE_HIDE) == 0) {
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
    int32_t start_row = (int32_t)terminal->textgrid->rows - (int32_t)terminal->rows;
    terminal->textgrid_start_row = start_row < 0 ? 0:start_row;
}

#ifdef __cplusplus
}
#endif

