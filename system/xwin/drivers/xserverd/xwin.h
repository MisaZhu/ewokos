#ifndef XWIN_H
#define XWIN_H

/*window list management, focus handling, client events and the
  dirty bookkeeping*/

#include "xserver.h"

/*event plumbing*/
void x_quit(int from_pid);
void x_get_event(int from_pid, proto_t* out);
void x_get_event_node(int from_pid, proto_t* out);
void x_push_event(x_t* x, xwin_t* win, xevent_t* e);

/*visibility and focus*/
void hide_win(x_t* x, xwin_t* win);
void show_win(x_t* x, xwin_t* win);
void x_unfocus(x_t* x);
void try_focus(x_t* x, xwin_t* win);

/*window list*/
void push_win(x_t* x, xwin_t* win);
xwin_t* x_get_win(x_t* x, int fd, int from_pid);
xwin_t* x_get_win_by_name(x_t* x, const char* name);
bool has_win_by_main_pid(x_t* x, int main_pid);
xwin_t* get_top_focus_win(x_t* x, bool skip_launcher);
xwin_t* get_next_focus_win(x_t* x, bool skip_launcher);
void xwin_top(x_t* x, xwin_t* win);
void x_del_win(x_t* x, xwin_t* win);
void check_wins(x_t* x);

/*dirty/repaint predicates and propagation*/
bool need_repaint_frame(x_t* x, xwin_t* win);
bool need_repaint_desktop(x_t* x, xwin_t* win);
bool covered_by_opaque_win(x_t* x, xwin_t* from, uint32_t display_index, const grect_t* r);

/*client content update: shm-based handshake polled from loop_step
  (replaces the old XWIN_CNTL_UPDATE IPC fast path). A published frame is
  accepted in place (no snapshot copy) and composited out of the client's own
  buffer; x_update_commit hands that buffer back once the compositor is done
  reading it, x_update_release drops it without compositing.*/
void x_poll_updates(x_t* x);
void x_update_commit(x_t* x, xwin_t* win);
void x_update_release(x_t* x, xwin_t* win);

#endif
