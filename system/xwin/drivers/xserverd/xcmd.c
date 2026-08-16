#include "xserver.h"
#include <ewoksys/mstr.h>

char* xserver_dev_cmd(int from_pid, int argc, char** argv, void* p) {
    x_t* x = (x_t*)p;

    if(strcmp(argv[0], "list") == 0) {
        str_t* str = str_new("index  pid  name  title\n");
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
        char* ret = str_detach(str);
        return ret;
    }
    return NULL;
}
