/*the vdevice interface of the x server: fcntl/open/close/dev_cntl
  callbacks dispatching into the modules, and the main loop step*/
#include <stdlib.h>
#include <string.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/ipc.h>
#include <ewoksys/proc.h>
#include "xserver.h"
#include "xwin.h"
#include "xrepaint.h"
#include "xinput.h"
#include "xwin_cmd.h"

int xserver_fcntl(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        int cmd, proto_t* in, proto_t* out, void* p) {
    (void)dev;
    (void)info;
    x_t* x = (x_t*)p;

    int res = -1;
    if(cmd == XWIN_CNTL_UPDATE) {
        res = x_update(fd, from_pid, x);
    }	
    else if(cmd == XWIN_CNTL_UPDATE_INFO) {
        res = xwin_update_info(fd, from_pid, in, out, x);
    }
    else if(cmd == XWIN_CNTL_WORK_SPACE) {
        res = x_win_space(x, in, out);
    }
    else if(cmd == XWIN_CNTL_CALL_XIM) {
        res = xwin_call_xim(x, in, out);
    }
    else if(cmd == XWIN_CNTL_TRY_FOCUS) {
        res = do_xwin_try_focus(fd, from_pid, x);
    }
    else if(cmd == XWIN_CNTL_TOP) {
        res = do_xwin_top(fd, from_pid, x);
    }
    else if(cmd == XWIN_CNTL_SET_BUSY) {
        res = do_xwin_set_busy(fd, from_pid, in, x);
    }
    return res;
}

int xserver_win_open(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, int oflag, void* p) {
    (void)dev;
    (void)oflag;
    (void)info;
    if(fd < 0)
        return -1;

    x_t* x = (x_t*)p;
    xwin_t* win = (xwin_t*)malloc(sizeof(xwin_t));
    if(win == NULL)
        return -1;

    memset(win, 0, sizeof(xwin_t));
    win->fd = fd;
    win->from_pid = from_pid;
    win->from_main_pid = proc_getpid(from_pid);
    win->from_main_pid_uuid = proc_get_uuid(win->from_main_pid);
    push_win(x, win);
    return 0;
}

static void xwin_close(x_t* x, xwin_t* win) {
    if(win == NULL || win == x->win_launcher)
        return;

    xevent_t ev;
    ev.type = XEVT_WIN;
    ev.value.window.event = XEVT_WIN_CLOSE;
    x_push_event(x, win, &ev);
}
 
static void xwin_launcher(x_t* x, xwin_t* win) {
    if(win == NULL)
        return;

    if(x->win_focus != x->win_launcher) {
        x->win_last = x->win_focus;
        xwin_top(x, x->win_launcher);
    }
}

static int x_set_top(x_t* x, const char* name, proto_t* out) {
    xwin_t* win = x_get_win_by_name(x, name);
    PF->clear(out)->addi(out, -1);
    if(win != NULL) {
        xwin_top(x, win);
        PF->clear(out)->addi(out, 0);
    }
    return 0;
}

int xserver_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)dev;
    x_t* x = (x_t*)p;

    if(cmd == DEV_CNTL_REFRESH) {
        x_dirty(x, -1);
    }
    else if(cmd == X_DCNTL_GET_INFO) {
        uint32_t i = 0;
        if(in != NULL)
            i = proto_read_int(in);

        x_display_t* display = &x->displays[i]; //TODO
        xscreen_info_t scr;	
        scr.id = 0;
        scr.fps = x->config.fps;
        scr.size.w = display->g->w;
        scr.size.h = display->g->h;
        scr.g_shm_id = display->g_shm_id;
        PF->add(ret, &scr, sizeof(xscreen_info_t));
    }
    else if(cmd == X_DCNTL_GET_DISP_NUM) {
        PF->addi(ret, x->display_num);
    }
    else if(cmd == X_DCNTL_SET_XWM) {
        x->xwm_pid = from_pid;
        x->xwm_uuid = proc_get_uuid(from_pid);
        x->xwm_changed = true;
        x_dirty(x, -1);
    }
    else if(cmd == X_DCNTL_UNSET_XWM) {
        x->xwm_pid = -1;
        x->xwm_changed = false;
        x_dirty(x, -1);
    }
    else if(cmd == X_DCNTL_INPUT) {
        xevent_t ev;
        proto_read_to(in, &ev, sizeof(xevent_t));
        handle_input(x, from_pid, &ev);
    }
    else if(cmd == X_DCNTL_GET_EVT) {
        x_get_event(from_pid, ret);
    }
    else if(cmd == X_DCNTL_GET_EVT_NODE) {
        x_get_event_node(from_pid, ret);
    }
    else if(cmd == X_DCNTL_GET_DESKTOP_SPACE) {
        x_get_desktop_space(x, in, ret);
    }
    else if(cmd == X_DCNTL_SET_DESKTOP_SPACE) {
        x_set_desktop_space(x, in, ret);
    }
    else if(cmd == X_DCNTL_GET_THEME) {
        x_dev_get_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_SET_THEME) {
        x_dev_set_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_LOAD_THEME) {
        x_dev_load_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_GET_XWM_THEME) {
        x_dev_get_xwm_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_SET_XWM_THEME) {
        x_dev_set_xwm_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_LOAD_XWM_THEME) {
        x_dev_load_xwm_theme(x, in, ret);
    }
    else if(cmd == X_DCNTL_SET_TOP) {
        const char* name = proto_read_str(in);
        x_set_top(x, name, ret);
    }
    else if(cmd == X_DCNTL_SHOW_CURSOR) {
        x->show_cursor = (bool)proto_read_int(in);
        x_dirty(x, -1);
    }
    else if(cmd == X_DCNTL_NEXT_FOCUS) {
        xwin_t* win = get_next_focus_win(x, true);
        if(win != NULL)
            xwin_top(x, win);
    }
    else if(cmd == X_DCNTL_CLOSE_FOCUS) {
        xwin_close(x, x->win_focus);
    }
    else if(cmd == X_DCNTL_LAUNCHER) {
        xwin_launcher(x, x->win_focus);
    }
    else if(cmd == X_DCNTL_QUIT) {
        x_quit(from_pid);
    }
    return 0;
}

int xserver_win_close(vdevice_t* dev, int fd, int from_pid, uint32_t node, fsinfo_t* fsinfo, void* p) {
    (void)dev;
    (void)fsinfo;
    x_t* x = (x_t*)p;
    xwin_t* win = x_get_win(x, fd, from_pid);
    if(win == NULL) {
        return -1;
    }

    if(win->busy && get_mouse_owner(x, NULL) == win)
        x_cursor_set_busy(x, false);

    int disp_index = win->xinfo != NULL ? win->xinfo->display_index : -1;
    int main_pid = win->from_main_pid;
    x_del_win(x, win);	
    if(!has_win_by_main_pid(x, main_pid))
        x_quit(main_pid);

    x_dirty(x, disp_index);
    return 0;
}

int xserver_step(vdevice_t* dev, void* p) {
    (void)dev;
    x_t* x = (x_t*)p;

    uint64_t tik = kernel_tic_ms(0);
    uint32_t tm = 1000/x->config.fps;

    /*a new xwm took over: windows sized while the previous one was down
      hold a winr equal to their wsr (the fallback of get_xwm_win_space),
      which no decorated window can have. Re-fetch their geometry before
      anything gets drawn with it*/
    if(x->xwm_changed) {
        if(check_xwm(x)) {
            xwin_t* win = x->win_head;
            while(win != NULL) {
                xwin_revalidate_geometry(x, win);
                win = win->next;
            }
            x->xwm_changed = false;
            x_dirty(x, -1);
        }
    }

    ipc_disable();
    check_wins(x);
    /*release the per-window update slots so a new UPDATE IPC may snapshot
      again. The snapshot itself never runs here: the client renders into
      ws_g freely between its blocking UPDATE IPCs, so it is only safe to
      read inside those (see x_refresh_pending_updates)*/
    x_refresh_pending_updates(x);
    for(uint32_t i=0; i<DISP_MAX; i++) {
        x_display_t* display = &x->displays[i];
        if(!display->active)
            continue;
        x_repaint(x, i);
    }
    ipc_enable();

    /*the flush is a plain outbound IPC to the fb daemon and touches no
      window state, so it runs with inbound IPC re-enabled. It waits for the
      daemon to finish copying: compositing now writes straight into the
      scan-out dma, so the next frame must not overwrite it while the daemon
      is still pushing it to the panel (tearing/flicker, seen on real panels
      like raspix whose framebuffer is scanned out continuously).*/
    for(uint32_t i=0; i<DISP_MAX; i++) {
        x_display_t* display = &x->displays[i];
        if(!display->active)
            continue;
        if(display->pending_flush) {
            display_flush(&display->display, true);
            x->displays[i].pending_flush = false;
        }
    }

    uint32_t gap = (uint32_t)(kernel_tic_ms(0) - tik);
    if(gap < tm) {
        gap = tm - gap;
        proc_usleep(gap*1000);
    }
    return 0;
}
