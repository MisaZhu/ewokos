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

static void do_esc_clear(gterminal_t* terminal, uint16_t* values, uint8_t vnum) {
    if(values[0] == 2) {
        textgrid_clear(terminal->textgrid);
        textgrid_move_to(terminal->textgrid, 0, terminal->textgrid_start_row);
    }
}

static void do_esc_xy(gterminal_t* terminal, uint16_t* values, uint8_t vnum) {
    textgrid_move_to(terminal->textgrid, values[1], values[0]+terminal->textgrid_start_row);
}

static void run_esc_cmd(gterminal_t* terminal, UNICODE16 cmd, uint16_t* values, uint8_t vnum) {
    if(cmd == 'm') { //color and state
        do_esc_color(terminal, values, vnum);
    }
    else if(cmd == 'J') { //clear
        do_esc_clear(terminal, values, vnum);
    }
    else if(cmd == 'H') { //move curs y,x
        do_esc_xy(terminal, values, vnum);
    }
    else if(cmd == 'A') { //move curs up
        int16_t y = (int16_t)terminal->textgrid->curs_y - values[0];
        if(y < 0)
            y = 0;
        textgrid_move_to(terminal->textgrid, terminal->textgrid->curs_x, y+terminal->textgrid_start_row);
    }
    else if(cmd == 'B') { //move curs down
        uint16_t y = terminal->textgrid->curs_y + values[0];
        if(y >= terminal->textgrid->rows)
            y = terminal->textgrid->rows-1;
        textgrid_move_to(terminal->textgrid, terminal->textgrid->curs_x, y+terminal->textgrid_start_row);
    }
    else if(cmd == 'D') { //move curs left
        int16_t x = (int16_t)terminal->textgrid->curs_x - values[0];
        if(x < 0)
            x = 0;
        textgrid_move_to(terminal->textgrid, x, terminal->textgrid->curs_y+terminal->textgrid_start_row);
    }
    else if(cmd == 'C') { //move curs right
        uint16_t x = terminal->textgrid->curs_x + values[0];
        if(x >= terminal->textgrid->cols)
            x = terminal->textgrid->cols-1;
        textgrid_move_to(terminal->textgrid, x, terminal->textgrid->curs_y+terminal->textgrid_start_row);
    }
    else if(cmd == 's') { //save curs pos
        terminal->curs_pos.x = terminal->textgrid->curs_x;
        terminal->curs_pos.y = terminal->textgrid->curs_y;
    }
    else if(cmd == 'u') { //restore curs pos
        textgrid_move_to(terminal->textgrid, terminal->curs_pos.x, terminal->curs_pos.y+terminal->textgrid_start_row);
    }
    else if(cmd == 'l') { //hide curs
        terminal->show_curs = false;
    }
    else if(cmd == 'h') { //show curs
        terminal->show_curs = true;
    }
}

static uint32_t do_esc_cmd(gterminal_t* terminal, UNICODE16* uni, uint32_t from, uint32_t size) {
    UNICODE16 c = uni[from];
    if(c == 0)
        return from;

    from++;
    if(from >= size || c != '[')
        return from;

    uint16_t values[8];
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
            char vstr[4];
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
        fg = bg;
        bg = tch->color;
    }
    if(bg != 0 && bg != terminal->bg_color) 
        graph_fill(g, chx, chy, chw, chh, bg);
    
    if((tch->state & TERM_STATE_HIDE) == 0 && 
            ((tch->state & TERM_STATE_FLASH) == 0)) {
        if((tch->state & TERM_STATE_UNDERLINE) != 0)
            graph_line(g, chx, chy+chh-1, chx+chw,  chy+chh-1, fg);

        graph_draw_char_font_fixed(g, chx, chy, tch->c,
                terminal->font, terminal->font_size, fg, chw, 0);
        if((tch->state & TERM_STATE_HIGH_LIGHT) != 0)
            graph_draw_char_font_fixed(g, chx+1, chy, tch->c,
                    terminal->font, terminal->font_size, fg, chw, 0);
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

void gterminal_put(gterminal_t* terminal, const char* buf, int size) {
    uint16_t* unicode = (uint16_t*)malloc((size+1)*2);
    size = utf82unicode((unsigned char*)buf, size, unicode);

    for(uint32_t i=0; i<size; i++) {
        UNICODE16 c = unicode[i];
        if(c == ESC_CMD) {
            i = do_esc_cmd(terminal, unicode, i+1, size);
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

