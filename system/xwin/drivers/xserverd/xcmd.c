#include "xserver.h"
#include "xevtpool.h"
#include <graph/graph_g2d.h>
#include <ewoksys/mstr.h>
#include <ewoksys/kernel_tic.h>

/*describe one compositor canvas: the flags decide whether its blits ride
  /dev/g2d or run a cpu pixel pass inside this process*/
static void add_g(str_t* str, const char* tag, const graph_t* g) {
    char item[96];
    if(g == NULL)
        snprintf(item, sizeof(item), "%s=- ", tag);
    else if(g->shm_id <= 0)
        snprintf(item, sizeof(item), "%s=%dx%d/heap ", tag, g->w, g->h);
    else
        snprintf(item, sizeof(item), "%s=%dx%d/%s ", tag, g->w, g->h,
                g->shm_contig ? "contig" : "NOCONTIG");
    str_add(str, item);
}

char* xserver_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    x_t* x = (x_t*)p;

    if(strcmp(argv[0], "help") == 0) {
        return str_detach(str_new(
            "commands:\n"
            "  stat    show server statistics (uptime, windows, event pools)\n"
            "  list    list all windows\n"
            "  bufs    show every compositor canvas, whether it rides /dev/g2d,\n"
            "          and the per-window publish/paint handshake state\n"
            "  help    show this help\n"));
    }

    /*graph_blt falls back to a full cpu pass in THIS process whenever a
      canvas is not contig backed, with no error and no log anywhere - one
      segment that lost its IPC_CONTIG allocation quietly turns the
      compositor into a software blitter. This makes that visible: the
      per-canvas flags say which segments degraded, and the reject counters
      say how many pixels the cpu had to move because of it.*/
    if(strcmp(argv[0], "bufs") == 0) {
        uint32_t rj_num = 0, rj_noncontig = 0, rj_small = 0;
        uint64_t rj_px = 0;
        graph_g2d_reject_stats(&rj_num, &rj_noncontig, &rj_small, &rj_px);

        str_t* str = str_new("");
        char item[160];
        snprintf(item, sizeof(item),
                "g2d rejects: %u (noncontig %u, small %u), cpu-moved: %uK px\n",
                rj_num, rj_noncontig, rj_small, (uint32_t)(rj_px / 1024));
        str_add(str, item);

        x_server_lock_enter();
        for(uint32_t i = 0; i < DISP_MAX; i++) {
            x_display_t* display = &x->displays[i];
            if(!display->active)
                continue;
            snprintf(item, sizeof(item), "disp%u ", i);
            str_add(str, item);
            add_g(str, "scanout", display->g);
            str_add(str, "\n");
        }

        xwin_t* win = x->win_head;
        while(win != NULL) {
            if(win->xinfo != NULL) {
                snprintf(item, sizeof(item), "win %s[%dx%d] ",
                        win->xinfo->name,
                        win->xinfo->winr.w, win->xinfo->winr.h);
            }
            else {
                snprintf(item, sizeof(item), "win <pending> ");
            }
            str_add(str, item);
            add_g(str, "ws", win->ws_g);
            add_g(str, "ws2", win->ws_g2);
            add_g(str, "frame", win->frame_g);
            /*there is no snapshot canvas any more: src names the buffer the
              compositor actually reads, and pub/paint are the two handshake
              bits win_src_stable() decides on (pub=1 the server owns it, so it
              is readable; paint=1 with pub=0 the client may be mid-frame, so an
              incremental repaint skips the window and a rebuild waits)*/
            if(win->xinfo != NULL) {
                snprintf(item, sizeof(item), "src=%s pub=%u paint=%u ",
                        win_comp_src(win) == win->ws_g2 ? "ws2" : "ws",
                        (unsigned)win->xinfo->update_requested,
                        (unsigned)win->xinfo->painting);
                str_add(str, item);
            }
            str_add(str, "\n");
            win = win->next;
        }
        x_server_lock_leave();
        return str_detach(str);
    }

    if(strcmp(argv[0], "stat") == 0) {
        x_server_lock_enter();
        uint32_t win_num = 0;
        xwin_t* win = x->win_head;
        while(win != NULL) {
            win_num++;
            win = win->next;
        }
        uint32_t pool_num = 0;
        uint32_t evt_num = 0;
        xevent_pool_stats(&pool_num, &evt_num);
        x_server_lock_leave();

        str_t* str = str_new("");
        char item[128];
        snprintf(item, 128, "uptime: %d sec\n", (int)(kernel_tic_ms(0)/1000));
        str_add(str, item);
        snprintf(item, 128, "wins: %d\n", win_num);
        str_add(str, item);
        snprintf(item, 128, "evt pools: %d, queued events: %d\n", pool_num, evt_num);
        str_add(str, item);
        return str_detach(str);
    }

    if(strcmp(argv[0], "list") == 0) {
        str_t* str = str_new("index  pid  name  title\n");
        x_server_lock_enter();
        xwin_t *win = x->win_head;
        uint32_t i = 0;
        while (win != NULL) {
            char item[128];
            if(win->xinfo != NULL) {
                snprintf(item, 128, "%4d  %4d  %s %s (x:%d, y:%d, w:%d, h:%d)\n", i, win->from_pid,
                   win->xinfo->name,
                   win->xinfo->title,
                   win->xinfo->wsr.x, win->xinfo->wsr.y,
                   win->xinfo->wsr.w, win->xinfo->wsr.h);
            }
            else {
                snprintf(item, 128, "%4d  %4d  <pending> <pending> (x:%d, y:%d, w:%d, h:%d)\n",
                        i, win->from_pid, 0, 0, 0, 0);
            }
            str_add(str, item);
            win = win->next;
            i++;
        }
        x_server_lock_leave();
        char* ret = str_detach(str);
        return ret;
    }
    return NULL;
}
