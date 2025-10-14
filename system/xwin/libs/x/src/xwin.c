#include <x/xwin.h>
#include <ewoksys/ipc.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/thread.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

static int xwin_update_info(xwin_t* xwin, uint8_t type) {
	if(xwin->xinfo == NULL)
		return -1;

	if(xwin->ws_g_shm != NULL && (type & X_UPDATE_REBUILD) != 0) {
		shmdt(xwin->ws_g_shm);
		xwin->ws_g_shm = NULL;
	}

	proto_t in;
	PF->format(&in, "i,i", xwin->xinfo_shm_id, type);
	int ret = vfs_fcntl_wait(xwin->fd, XWIN_CNTL_UPDATE_INFO, &in);
	PF->clear(&in);
	return ret;
}

void xwin_busy(xwin_t* xwin, bool busy) {
	proto_t in;
	PF->init(&in)->addi(&in, busy);
	vfs_fcntl_wait(xwin->fd, XWIN_CNTL_SET_BUSY, &in);
	PF->clear(&in);
}

int xwin_call_xim(xwin_t* xwin, bool show) {
	proto_t in, out;
	PF->format(&in, "i", show);
	PF->init(&out);
	int ret = vfs_fcntl(xwin->fd, XWIN_CNTL_CALL_XIM, &in, &out);
	if(ret == 0)
		ret = proto_read_int(&out);
	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

int xwin_top(xwin_t* xwin) {
	int ret = vfs_fcntl(xwin->fd, XWIN_CNTL_TOP, NULL, NULL);
	return ret;
}

/*
static int  x_get_win_rect(int xfd, int style, grect_t* wsr, grect_t* win_space) {
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,m", style, wsr, sizeof(grect_t));
	int ret = vfs_fcntl(xfd, XWIN_CNTL_WORK_SPACE, &in, &out);
	PF->clear(&in);
	if(ret == 0) 
		proto_read_to(&out, win_space, sizeof(grect_t));
	PF->clear(&out);
	return ret;
}
*/

xwin_t* xwin_open(x_t* xp, int32_t disp_index, int x, int y, int w, int h, const char* title, int style) {
	if(w <= 0 || h <= 0)
		return NULL;
	disp_index = x_get_display_id(disp_index);

	int fd = open("/dev/x", O_RDWR);
	if(fd < 0)
		return NULL;

	grect_t r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
	memset(ret, 0, sizeof(xwin_t));
	ret->fd = fd;
	ret->x = xp;

	key_t key = (((int32_t)ret) << 16) | proc_get_uuid(getpid());
	int32_t xinfo_shm_id = shmget(key, sizeof(xinfo_t), 0600 |IPC_CREAT|IPC_EXCL);
	if(xinfo_shm_id == -1) {
		free(ret);
		return NULL;
	}

	xinfo_t* xinfo = (xinfo_t*)shmat(xinfo_shm_id, 0, 0);
	if(xinfo == NULL) {
		free(ret);
		return NULL;
	}

	if((style & XWIN_STYLE_PROMPT) != 0)
		xp->prompt_win = ret;


	ret->xinfo_shm_id = xinfo_shm_id;
	ret->xinfo = xinfo;
	memset(ret->xinfo, 0, sizeof(xinfo_t));
	ret->xinfo->ws_g_shm_id = -1;
	ret->xinfo->win = (uint32_t)ret;
	ret->xinfo->style = style;
	ret->xinfo->display_index = disp_index;
	if(xp->main_win == NULL) {
		ret->xinfo->is_main = true;
		xp->main_win = ret;
	}

	memcpy(&ret->xinfo->wsr, &r, sizeof(grect_t));
	strncpy(ret->xinfo->title, title, XWIN_TITLE_MAX-1);

	const char* auto_max = getenv("X_AUTO_FULL_SCREEN");
	if(auto_max != NULL &&
			(style & XWIN_STYLE_NO_TITLE) == 0 &&
			(style & XWIN_STYLE_NO_RESIZE) == 0 &&
			(style & XWIN_STYLE_NO_FRAME) == 0) {
		ret->xinfo->style |= XWIN_STYLE_NO_RESIZE | XWIN_STYLE_NO_TITLE;
		ret->xinfo->state = XWIN_STATE_MAX;
	}

	if((style & XWIN_STYLE_MAX) != 0)
		ret->xinfo->state = XWIN_STATE_MAX;

	xwin_update_info(ret, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	pthread_mutex_init(&ret->painting_lock, NULL);
	return ret;
}

int xwin_fullscreen(xwin_t* xwin) {
	xwin->xinfo->style |= XWIN_STYLE_NO_RESIZE | XWIN_STYLE_NO_TITLE;
	return xwin_max(xwin);
}

static graph_t* x_get_graph(xwin_t* xwin, graph_t* g) {
	if(xwin == NULL || xwin->xinfo == NULL || xwin->xinfo->ws_g_shm_id == -1)
		return NULL;

	if(xwin->ws_g_shm == NULL) {
		xwin->ws_g_shm = shmat(xwin->xinfo->ws_g_shm_id, 0, 0);
		if(xwin->ws_g_shm == NULL)
			return NULL;
		if(xwin->on_resize != NULL) {
			xwin->on_resize(xwin);
		}
	}

	g->buffer = xwin->ws_g_shm;
	g->w = xwin->xinfo->wsr.w;
	g->h = xwin->xinfo->wsr.h;
	g->need_free = false;
	return g;
}

void xwin_destroy(xwin_t* xwin) {
	if(xwin != NULL)
		free(xwin);
}

void xwin_close(xwin_t* xwin) {
	if(xwin == NULL || xwin->fd <= 0)
		return;

	if(xwin->on_close != NULL) {
		if(!xwin->on_close(xwin))
			return;
	}
	close(xwin->fd);
	xwin->fd = -1;
	pthread_mutex_destroy(&xwin->painting_lock);

	if(xwin->ws_g_shm != NULL)
		shmdt(xwin->ws_g_shm);

	if(xwin->xinfo != NULL)
		shmdt(xwin->xinfo);

	if(xwin->x->main_win == xwin)
		x_terminate(xwin->x);

	if(xwin->x->prompt_win == xwin)
		xwin->x->prompt_win = NULL;
}

graph_t* xwin_fetch_graph(xwin_t* xwin, graph_t* g) {
	memset(g, 0, sizeof(graph_t));
	return x_get_graph(xwin, g);
}

void xwin_repaint(xwin_t* xwin) {
	pthread_mutex_lock(&xwin->painting_lock);
	if(xwin->xinfo != NULL &&
			xwin->xinfo->update_theme &&
			xwin->on_update_theme != NULL) {
		xwin->on_update_theme(xwin);
	}

	graph_t g;
	if(xwin_fetch_graph(xwin, &g) != NULL) {
		if(xwin->on_repaint != NULL) {
			xwin->on_repaint(xwin, &g);
		}
	}
	vfs_fcntl_wait(xwin->fd, XWIN_CNTL_UPDATE, NULL);
	xwin->xinfo->update_theme = false;	
	pthread_mutex_unlock(&xwin->painting_lock);
}

/*
void xwin_repaint_req(xwin_t* xwin) {
	x_t* x = xwin->x;
	xevent_t ev;
	memset(&ev, 0, sizeof(xevent_t));
	ev.win = (uint32_t)xwin;
	ev.value.window.event = XEVT_WIN_REPAINT;
	ev.type = XEVT_WIN;
	ipc_disable();
	x_push_event(x, &ev);
	ipc_enable();
}
*/

int xwin_set_display(xwin_t* xwin, uint32_t display_index) {
	if((int32_t)display_index >= x_get_display_num())
		display_index = 0;

	xwin->xinfo->display_index = display_index;
	xwin_update_info(xwin, X_UPDATE_REFRESH);
	return 0;
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
	xwin->xinfo->wsr.w = w;
	xwin->xinfo->wsr.h = h;
	xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	xwin_repaint(xwin);
	return 0;
}

int xwin_max(xwin_t* xwin) {
	memcpy(&xwin->xinfo_prev, xwin->xinfo, sizeof(xinfo_t));
	xwin->xinfo->state = XWIN_STATE_MAX;
	xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
	xwin_repaint(xwin);
	return 0;
}

int xwin_resize(xwin_t* xwin, int dw, int dh) {
	return xwin_resize_to(xwin, xwin->xinfo->wsr.w+dw, xwin->xinfo->wsr.h+dh);
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
	xwin->xinfo->wsr.x = x;
	xwin->xinfo->wsr.y = y;
	xwin_update_info(xwin, X_UPDATE_REFRESH);
	xwin->on_move(xwin);
	return 0;
}

int xwin_move(xwin_t* xwin, int dx, int dy) {
	return xwin_move_to(xwin, xwin->xinfo->wsr.x+dx, xwin->xinfo->wsr.y+dy);
}

int xwin_event_handle(xwin_t* xwin, xevent_t* ev) {
	if(xwin->xinfo == NULL)
		return -1;

	if(ev->value.window.event == XEVT_WIN_CLOSE) {
		xwin_close(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_FOCUS) {
		if(xwin->x->prompt_win != NULL && xwin->x->prompt_win != xwin) {
			vfs_fcntl(xwin->x->prompt_win->fd, XWIN_CNTL_TRY_FOCUS, NULL, NULL);
		}
		else {
			if(xwin->on_focus != NULL)
				xwin->on_focus(xwin);
			if(xwin->xinfo != NULL)
				xwin->xinfo->focused = true;
			xwin_update_info(xwin,  X_UPDATE_REFRESH);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_UNFOCUS) {
		if(xwin->on_unfocus) {
			xwin->on_unfocus(xwin);
		}
		xwin->xinfo->focused = false;
		xwin_update_info(xwin, X_UPDATE_REFRESH);
	}
	else if(ev->value.window.event == XEVT_WIN_REORG) {
		if(xwin->on_reorg) {
			xwin->on_reorg(xwin);
		}
	}
	else if(ev->value.window.event == XEVT_WIN_RESIZE) {
		xwin->xinfo->wsr.w += ev->value.window.v0;
		xwin->xinfo->wsr.h += ev->value.window.v1;
		xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MOVE) {
		xwin->xinfo->wsr.x += ev->value.window.v0;
		xwin->xinfo->wsr.y += ev->value.window.v1;
		xwin_update_info(xwin, X_UPDATE_REFRESH);
		if(xwin->on_move != NULL)
			xwin->on_move(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_VISIBLE) {
		xwin_set_visible(xwin, ev->value.window.v0 == 1);
	}
	else if(ev->value.window.event == XEVT_WIN_REPAINT) {
		xwin_repaint(xwin);
	}
	else if(ev->value.window.event == XEVT_WIN_MAX) {
		if(xwin->xinfo->state == XWIN_STATE_MAX) {
			memcpy(xwin->xinfo, &xwin->xinfo_prev, sizeof(xinfo_t));
			xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
			xwin_repaint(xwin);
		}
		else {
			xwin_max(xwin);	
		}
	}
	return 0;
}

void xwin_set_alpha(xwin_t* xwin, bool alpha) {
	if(xwin->xinfo == NULL)
		return;
	xwin->xinfo->alpha = alpha;
}

int xwin_set_visible(xwin_t* xwin, bool visible) {
	if(xwin->xinfo == NULL || xwin->xinfo->visible == visible)
		return 0;

	if(visible) {
		if(xwin->on_show != NULL)
			xwin->on_show(xwin);
	}
	else {
		if(xwin->on_hide != NULL)
			xwin->on_hide(xwin);
	}

	xwin->xinfo->visible = visible;
	int res = xwin_update_info(xwin, X_UPDATE_REFRESH);

	if(visible) {
		vfs_fcntl(xwin->fd, XWIN_CNTL_TRY_FOCUS, NULL, NULL);
		xwin_repaint(xwin);
	}
	return res;
}

#ifdef __cplusplus
}
#endif

