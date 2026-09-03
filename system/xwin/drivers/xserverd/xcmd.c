#include "xserver.h"
#include "xevtpool.h"
#include <ewoksys/mstr.h>
#include <ewoksys/kernel_tic.h>

char* xserver_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    x_t* x = (x_t*)p;

    if(strcmp(argv[0], "help") == 0) {
        return str_detach(str_new(
            "commands:\n"
            "  stat    show server statistics (uptime, windows, event pools)\n"
            "  list    list all windows\n"
            "  help    show this help\n"));
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
