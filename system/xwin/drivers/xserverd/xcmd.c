#include "xserver.h"
#include <ewoksys/mstr.h>

char* xserver_dev_cmd(int from_pid, int argc, char** argv, void* p) {
    x_t* x = (x_t*)p;

	if(strcmp(argv[0], "list") == 0) {
		str_t* str = str_new("index  pid  title\n");
        xwin_t *win = x->win_head;
        uint32_t i = 0;
        while (win != NULL) {
			char item[128];
			snprintf(item, 128, "%4d  %4d  %s\n", i, win->from_pid, win->xinfo->title);
            str_add(str, item);
            win = win->next;
            i++;
        }
		char* ret = str_detach(str);
		return ret;
	}
	return NULL;
}