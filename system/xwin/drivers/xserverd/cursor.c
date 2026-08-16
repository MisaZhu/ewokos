#include "cursor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_png.h>
#include <x/x.h>

static inline void draw_cursor_raw(graph_t* g, int mx, int my, int mw, int mh, bool down) {
    (void)down;
    int r = mw/2 <= mh/2 ? mw/2 : mh/2;
    graph_fill_circle(g, mx+r, my+r, r, 0x88ffffff);
    graph_fill_circle(g, mx+r, my+r, r*2/3, 0x88000000);
}

static inline void clamp_cursor_offset(gpos_t* offset, int w, int h) {
    if(offset == NULL || w <= 0 || h <= 0)
        return;
    if(offset->x < 0)
        offset->x = 0;
    else if(offset->x >= w)
        offset->x = w - 1;
    if(offset->y < 0)
        offset->y = 0;
    else if(offset->y >= h)
        offset->y = h - 1;
}

void draw_cursor(graph_t* g, cursor_t* cursor, int mx, int my, bool busy) {
    if(cursor->type == CURSOR_TOUCH && !cursor->down && !busy)
        return;

    graph_t* img = cursor->img;	
    if(busy) {
        img = cursor->img_busy;	
    }

    if(img == NULL)
        draw_cursor_raw(g, mx, my, cursor->size.w, cursor->size.h, cursor->down);
    else
        graph_blt_alpha(img, 0, 0, img->w, img->h,
                g, mx, my, img->w, img->h, 0xff);
}

void cursor_init(const char* theme, cursor_t* cursor) {
    if(theme == NULL || theme[0] == 0 || cursor == NULL)
        return;

    if(cursor->img != NULL)
        graph_free(cursor->img);
    cursor->img = NULL;

    if(cursor->img_busy != NULL)
        graph_free(cursor->img_busy);
    cursor->img_busy = NULL;

    if(cursor->saved != NULL)
        graph_free(cursor->saved);
    cursor->saved = NULL;

    cursor->size.w = 21;
    cursor->size.h = 21;
    cursor->offset.x = cursor->size.w/2;
    cursor->offset.y = cursor->size.h/2;

    char fname[256] = {0};
    snprintf(fname, 255, "%s/%s/xwm/cursors/%s.json", X_THEME_ROOT, theme,
            cursor->type == CURSOR_MOUSE ? "mouse":"touch");
    json_var_t *conf_var = json_parse_file(fname);	
    cursor->offset_normal.x = cursor->offset.x = json_get_int_def(conf_var, "x_offset", 0);
    cursor->offset_normal.y = cursor->offset.y = json_get_int_def(conf_var, "y_offset", 0);

    const char* v = json_get_str_def(conf_var, "cursor", "");
    if(v[0] != 0) {
        if(v[0] != '/')
            snprintf(fname, 255, "%s/%s/xwm/%s", X_THEME_ROOT, theme, v);
        else
            strncpy(fname, v, 255);
        cursor->img = png_image_new(fname);
    }
    if(conf_var != NULL)
        json_var_unref(conf_var);

    if(cursor->img != NULL) { 
        cursor->size.w = cursor->img->w; 
        cursor->size.h = cursor->img->h; 
        clamp_cursor_offset(&cursor->offset, cursor->size.w, cursor->size.h);
        clamp_cursor_offset(&cursor->offset_normal, cursor->size.w, cursor->size.h);
    }

    snprintf(fname, 255, "%s/%s/xwm/cursors/busy.json", X_THEME_ROOT, theme);
    conf_var = json_parse_file(fname);	
    cursor->offset_busy.x = json_get_int_def(conf_var, "x_offset", 0);
    cursor->offset_busy.y = json_get_int_def(conf_var, "y_offset", 0);

    v = json_get_str_def(conf_var, "cursor", "");
    if(v[0] != 0) {
        if(v[0] != '/')
            snprintf(fname, 255, "%s/%s/xwm/%s", X_THEME_ROOT, theme, v);
        else
            strncpy(fname, v, 255);
        cursor->img_busy = png_image_new(fname);
    }
    if(cursor->img_busy != NULL)
        clamp_cursor_offset(&cursor->offset_busy, cursor->img_busy->w, cursor->img_busy->h);
    if(conf_var != NULL)
        json_var_unref(conf_var);
}
