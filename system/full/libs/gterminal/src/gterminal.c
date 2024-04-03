#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gterminal/gterminal.h>
#include <ewoksys/keydef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define  ESC_CMD 033

static gpos_t get_pos(gterminal_t* terminal, graph_t* g, uint32_t at, int32_t cw, int32_t ch) {
    uint32_t cx = 0, cy = 0;
    terminal_pos_by_at(&terminal->terminal, at, &cx, &cy);
    gpos_t ret = { cx*cw, cy*ch };
    return ret;
}

static void draw_content(gterminal_t* terminal, graph_t* g, uint32_t cw, uint32_t ch) {
    uint32_t size = terminal_size(&terminal->terminal);
    uint32_t i = 0;
    while(i < size) {
        tchar_t* c = terminal_getc_by_at(&terminal->terminal, i);
        if(c != NULL && c->c != 0 && c->c != '\n') {
            gpos_t pos = get_pos(terminal, g, i, cw, ch);

            uint32_t fg = c->color, bg = c->bg_color;

            if((c->state & TERM_STATE_REVERSE) != 0) {
                fg = c->bg_color;
                if(fg == 0)
                    fg = terminal->bg_color;
                bg = c->color;
            }
            if(bg != 0) 
                graph_fill(g, pos.x, pos.y, cw, ch, bg);
            
            
            if((c->state & TERM_STATE_HIDE) == 0 && 
                    ((c->state & TERM_STATE_FLASH) == 0 || terminal->flash_show)) {
                if((c->state & TERM_STATE_UNDERLINE) != 0)
                    graph_fill(g, pos.x, pos.y+ch-2, cw, 2, fg);

                graph_draw_char_font_fixed(g, pos.x, pos.y, c->c, terminal->font, terminal->font_size, fg, cw, 0);
                if((c->state & TERM_STATE_HIGH_LIGHT) != 0)
                    graph_draw_char_font_fixed(g, pos.x, pos.y+1, c->c, terminal->font, terminal->font_size, fg, cw, 0);
            }
        }
        i++;
    }
}

static void draw_curs(gterminal_t* terminal, graph_t* g, uint32_t cw, uint32_t ch) {
    if(!terminal->flash_show || !terminal->show_curs)
        return;
    gpos_t pos = get_pos(terminal, g, terminal_at(&terminal->terminal), cw, ch);
    graph_fill(g, pos.x+2, pos.y+2, cw-4, ch-4, terminal->fg_color);
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

    if(terminal->term_conf.fg_color == 0)
        terminal->term_conf.fg_color = terminal->fg_color;
}

static void do_esc_clear(gterminal_t* terminal, uint16_t* values, uint8_t vnum) {
    if(values[0] == 2) {
        terminal_clear(&terminal->terminal);
    }
}

static void do_esc_xy(gterminal_t* terminal, uint16_t* values, uint8_t vnum) {
    terminal_move_to(&terminal->terminal, values[1], values[0]);
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
        int16_t y = (int16_t)terminal->terminal.curs_y - values[0];
        if(y < 0)
            y = 0;
        terminal_move_to(&terminal->terminal, terminal->terminal.curs_x, y);
    }
    else if(cmd == 'B') { //move curs down
        uint16_t y = terminal->terminal.curs_y + values[0];
        if(y >= terminal->terminal.rows)
            y = terminal->terminal.rows-1;
        terminal_move_to(&terminal->terminal, terminal->terminal.curs_x, y);
    }
    else if(cmd == 'D') { //move curs left
        int16_t x = (int16_t)terminal->terminal.curs_x - values[0];
        if(x < 0)
            x = 0;
        terminal_move_to(&terminal->terminal, x, terminal->terminal.curs_y);
    }
    else if(cmd == 'C') { //move curs right
        uint16_t x = terminal->terminal.curs_x + values[0];
        if(x >= terminal->terminal.cols)
            x = terminal->terminal.cols-1;
        terminal_move_to(&terminal->terminal, x, terminal->terminal.curs_y);
    }
    else if(cmd == 's') { //save curs pos
        terminal->curs_pos.x = terminal->terminal.curs_x;
        terminal->curs_pos.y = terminal->terminal.curs_y;
    }
    else if(cmd == 'u') { //restore curs pos
        terminal_move_to(&terminal->terminal, terminal->curs_pos.x, terminal->curs_pos.y);
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
    terminal->show_curs = true;
    terminal->flash_show = true;
}

void gterminal_close(gterminal_t* terminal) {
    if(terminal->font != NULL)
        font_free(terminal->font);
}

void gterminal_paint(gterminal_t* terminal, graph_t* g) {
    if(terminal->terminal.cols == 0 || terminal->terminal.rows == 0)
        return;
	uint32_t cw = g->w / terminal->terminal.cols;
	uint32_t ch = g->h / terminal->terminal.rows;
	draw_content(terminal, g, cw, ch);
	draw_curs(terminal, g, cw, ch);
}

void gterminal_put(gterminal_t* terminal, const char* buf, int size) {
    uint16_t* unicode = (uint16_t*)malloc((size+1)*2);
    size = utf82unicode((unsigned char*)buf, size, unicode);

    for(uint32_t i=0; i<size; i++) {
        UNICODE16 c = unicode[i];
        if(c == KEY_BACKSPACE || c == CONSOLE_LEFT) {
            terminal_move(&terminal->terminal, -1);
            terminal_set(&terminal->terminal, 0, 0, 0, 0);
            continue;
        }
        else if(c == ESC_CMD) {
            i = do_esc_cmd(terminal, unicode, i+1, size);
            continue;
        }

        if(terminal->term_conf.set == 0)
            terminal_push(&terminal->terminal, c, 0, terminal->fg_color, 0);
        else
            terminal_push(&terminal->terminal, c, terminal->term_conf.state, terminal->term_conf.fg_color, terminal->term_conf.bg_color);
    }
}

void gterminal_flash(gterminal_t* terminal) {
    terminal->flash_show = !terminal->flash_show;
}

void gterminal_resize(gterminal_t* terminal, uint32_t gw, uint32_t gh) {
    uint32_t font_w = terminal->font_fixed > 0 ? terminal->font_fixed : terminal->font_size;
    uint32_t font_h = terminal->font_size;
    if(font_w == 0 || font_h == 0)
        return;
    uint32_t size;
    tchar_t* content = terminal_gets(&terminal->terminal, &size);
    terminal_reset(&terminal->terminal, gw/font_w, gh/font_h);
    if(content == NULL)
        return;
    for(uint32_t i=0; i<size; i++) {
        terminal_push(&terminal->terminal, content[i].c, content[i].state, content[i].color, content[i].bg_color);
    }
    free(content);
}

#ifdef __cplusplus
}
#endif

