#ifndef XWIN_CMD_H
#define XWIN_CMD_H

/*handlers for the X protocol requests (fcntl/dev_cntl commands):
  window geometry, themes, desktop space and xwm interplay*/

#include "xserver.h"

int xwin_update_info(int fd, int from_pid, proto_t* in, proto_t* out, x_t* x);
int do_xwin_top(int fd, int from_pid, x_t* x);
int do_xwin_try_focus(int fd, int from_pid, x_t* x);
int do_xwin_set_busy(int fd, int from_pid, proto_t* in, x_t* x);
int x_win_space(x_t* x, proto_t* in, proto_t* out);
int xwin_call_xim(x_t* x, proto_t* in, proto_t* out);

/*theme handling*/
int x_dev_load_theme(x_t* x, proto_t* in, proto_t* out);
int x_dev_get_theme(x_t* x, proto_t* in, proto_t* out);
int x_dev_set_theme(x_t* x, proto_t* in, proto_t* out);
int x_dev_load_xwm_theme(x_t* x, proto_t* in, proto_t* out);
int x_dev_get_xwm_theme(x_t* x, proto_t* in, proto_t* out);
int x_dev_set_xwm_theme(x_t* x, proto_t* in, proto_t* out);

/*desktop space queries*/
int x_get_desktop_space(x_t* x, proto_t* in, proto_t* out);
int x_set_desktop_space(x_t* x, proto_t* in, proto_t* out);

/*refetch window geometry from xwm and rebuild the frame buffer when
  winr went stale (xwm restart/theme change fallback)*/
void xwin_revalidate_geometry(x_t* x, xwin_t* win);

#endif
