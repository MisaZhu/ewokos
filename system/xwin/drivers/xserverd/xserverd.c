/*xserverd: the X server daemon of EwokOS.

  This file holds the process entry (main), startup/shutdown and the
  configuration loading. The rest of the daemon is split into modules:

  - xserver.h     shared types (x_t, xwin_t, ...) and small helpers
  - xshm.c        shared-memory allocation/teardown helpers
  - xwin.c        window list, focus, client events, damage tracking
  - xrender.c     compositing: desktop, window content and frame drawing
  - xrepaint.c    per-display repaint pipeline and dirty rect collection
  - xinput.c      mouse/touch and IME input handling
  - xwin_cmd.c    X protocol command handlers (geometry, themes, ...)
  - xserver_dev.c vdevice callbacks and the main loop step
  - cursor.c      cursor bitmap drawing
  - xevtpool.c    per-client event queues
  - xtheme.c      theme file loading
  - xcmd.c        debug command handler
*/
#include <string.h>
#include <ewoksys/proc.h>
#include <tinyjson/tinyjson.h>
#include <graph/graph_png.h>
#include <displayman/displayman.h>
#include "xserver.h"
#include "xtheme.h"

bool check_xwm(x_t* x) {
    if(x->xwm_pid < 0) {
        x->xwm_uuid = 0;
        return false;
    }

    if(proc_check_uuid(x->xwm_pid, x->xwm_uuid) == x->xwm_uuid)
        return true;

    x->xwm_pid = -1;
    x->xwm_uuid = 0;
    return false;
}

void x_dirty(x_t* x, int32_t display_index) {
    if(display_index >= 0) {
        x_display_t *display = &x->displays[display_index];
        display->dirty = true;
        display->need_repaint = true;
        return;
    }

    for(uint32_t i=0; i<DISP_MAX; i++) {
        x_display_t *display = &x->displays[i];
        if(!display->active)
            continue;
        display->dirty = true;
        display->need_repaint = true;
    }
}

void x_repaint_req(x_t* x, int32_t display_index) {
    if(display_index >= 0 && display_index < DISP_MAX) {
        x_display_t *display = &x->displays[display_index];
        if(display->active)
            display->need_repaint = true;
        return;
    }

    for(uint32_t i=0; i<DISP_MAX; i++) {
        x_display_t *display = &x->displays[i];
        if(!display->active)
            continue;
        display->need_repaint = true;
    }
}

static int x_init_display(x_t* x) {
    for(uint32_t i=0; i<DISP_MAX; i++) {
        if(!x->displays[i].active)
            continue;
        x_display_t* display = &x->displays[i];

        if(displayman_open(x->display_man, display->display_index, &display->display) != 0)
            continue;
        graph_t *g_display = display_fetch_graph(&display->display);
        if(g_display == NULL)
            continue;
        display->g_display = g_display;
        display->g = g_display;
        display->g_shm_id = display->display.shm_id;
        display->desktop_rect.x = 0;
        display->desktop_rect.y = 0;
        display->desktop_rect.w = g_display->w;
        display->desktop_rect.h = g_display->h;
        //x_dirty(x, i);
    }
    return 0;
}

static uint32_t active_displays(x_t* x, json_var_t* display_arr_var) {
    for(uint32_t i = 0; i < DISP_MAX; i++) {
        x->displays[i].active = false;
    }

    uint32_t num = 0;
    if(display_arr_var != NULL && display_arr_var->json_is_array != 0) {
        uint32_t size = json_var_array_size(display_arr_var);
        for(uint32_t i = 0; i < size && num < DISP_MAX; i++) {
            json_var_t* item = json_var_array_get_var(display_arr_var, i);
            if(item == NULL || item->type != JSON_V_INT)
                continue;

            int32_t index = json_var_get_int(item);
            if(index < 0 || index >= DISP_MAX)
                continue;

            x->displays[num].active = true;
            x->displays[num].display_index = index;
            num++;
        }
    }

    if(num == 0) {
        x->displays[0].active = true;
        x->displays[0].display_index = 0;
        num = 1;
    }

    return num;
}

static int32_t x_get_first_active_display(x_t* x) {
    if(x->displays[0].active && x->displays[0].g != NULL) {
        return 0;
    }
    return -1;
}

static int32_t read_config(x_t* x, const char* fname) {
    x->config.fps = 60;

    json_var_t *conf_var = json_parse_file(fname);	

    json_var_t* display_arr_var = json_get_obj(conf_var, "displays");
    x->display_num = active_displays(x, display_arr_var);

    x->config.fps = json_get_int_def(conf_var, "fps", 30);
    x->config.bg_proc_priority = json_get_int_def(conf_var, "bg_proc_priority", 2);

    const char* v = json_get_str_def(conf_var, "cursor", "");
    if(strcmp(v, "touch") == 0)
        x->cursor.type = CURSOR_TOUCH;
    else if(strcmp(v, "mouse") == 0)
        x->cursor.type = CURSOR_MOUSE;
    else {
        if(strcmp(v, "none") == 0)
            x->show_cursor = false;
    }

    v = json_get_str_def(conf_var, "logo", "/usr/system/icons/xlogo.png");
    x->config.logo = png_image_new(v);

    if(conf_var != NULL)
        json_var_unref(conf_var);
    return 0;
}

static int x_init(x_t* x, const char* display_man) {
    memset(x, 0, sizeof(x_t));
    x->xwm_pid = -1;
    for(uint32_t i = 0; i < DISP_MAX; i++) {
        x->displays[i].g_shm_id = -1;
    }

    x->display_man = display_man;
    read_config(x, "/etc/x/x.json");

    if(x_init_display(x) != 0)
        return -1;

    int32_t display_index = x_get_first_active_display(x);
    if(display_index == -1)
        return -1;
    x->current_display = display_index;
    x_display_t* display = &x->displays[display_index];

    x->cursor.cpos.x = display->g->w/2;
    x->cursor.cpos.y = display->g->h/2;
    x->mouse_state.last_pos.x = x->cursor.cpos.x;
    x->mouse_state.last_pos.y = x->cursor.cpos.y;
    x->show_cursor = true;

    xevent_pool_init();
    return 0;
}	

static void x_close(x_t* x) {
    for(uint32_t i=0; i<DISP_MAX; i++) {
        x_display_t* display = &x->displays[i];
        if(!display->active)
            continue;
        /* g and g_display both alias the scan-out dma graph here; display_close frees it. */
        display_close(&display->display);
        display->g = NULL;
        display->g_display = NULL;
        display->g_shm_id = -1;
    }
}

char* xserver_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p);

/*
static int doargs(int argc, char* argv[]) {
    int c = 0;
    while (c != -1) {
        c = getopt (argc, argv, "d:");
        if(c == -1)
            break;

        switch (c) {
        case 'd':
            _disp_index = atoi(optarg);
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}
*/

int main(int argc, char** argv) {
    const char* mnt_point = "/dev/x";
    const char* display_man = "/dev/displayman";
    //doargs(argc, argv);

    x_t x;
    if(x_init(&x, display_man) != 0)
        return -1;

    cursor_init("default", &x.cursor);
    x_load_theme("default", &x.config.theme);
    x_dirty(&x, -1);

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "xserver");
    dev.fcntl = xserver_fcntl;
    dev.close = xserver_win_close;
    dev.open = xserver_win_open;
    dev.dev_cntl = xserver_dev_cntl;
    dev.cmd = xserver_dev_cmd;
    dev.loop_step = xserver_step;
    dev.extra_data = &x;
    x.dev = &dev;

    device_run(&dev, mnt_point, FS_TYPE_CHAR | FS_TYPE_ANNOUNIMOUS, 0666, false);
    x_close(&x);
    return 0;
}
