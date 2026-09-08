#include <x/xwin.h>
#include <x/x.h>
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
#include <ewoksys/klog.h>

#ifdef __cplusplus
extern "C" {
#endif

static uint32_t _xwin_shm_seq = 1;

static int32_t xwin_alloc_shm(uint32_t salt, int32_t size, int32_t flag, key_t* out_key) {
    for(uint32_t i = 0; i < 16; i++) {
        uint32_t seq = _xwin_shm_seq++;
        key_t key = (key_t)((salt * 2654435761u) ^ (seq * 2246822519u));
        if(key == 0)
            key = (key_t)(seq | 1u);

        int32_t shm_id = shmget(key, size, flag);
        if(shm_id != -1) {
            if(out_key != NULL)
                *out_key = key;
            return shm_id;
        }
    }
    if(out_key != NULL)
        *out_key = 0;
    return -1;
}

static ewokos_addr_t xwin_handle(const xwin_t* xwin) {
    if(xwin == NULL)
        return 0;
    return (ewokos_addr_t)xwin->xinfo_shm_id;
}

/*Process-wide registry of every window this process opened. The server no
  longer echoes a raw client pointer in xevent.win - it sends back only the
  window's xinfo shm handle - so the event loop must be able to resolve that
  handle to the xwin_t of ANY window, not just the main/prompt ones. Menus,
  submenus and dialogs are extra windows in the same process; without this
  registry all of their mouse and focus events would be dropped.*/
static xwin_t* _xwin_registry = NULL;

static void xwin_registry_add(xwin_t* xwin) {
    ipc_disable();
    xwin->reg_next = _xwin_registry;
    _xwin_registry = xwin;
    ipc_enable();
}

static void xwin_registry_remove(xwin_t* xwin) {
    ipc_disable();
    xwin_t** pp = &_xwin_registry;
    while(*pp != NULL) {
        if(*pp == xwin) {
            *pp = xwin->reg_next;
            xwin->reg_next = NULL;
            break;
        }
        pp = &(*pp)->reg_next;
    }
    ipc_enable();
}

/*How many windows of this process still hold a present the server has not
  taken. xwin_retry_pending_presents() reads it on every event-loop tick, so
  the "nothing pending" case has to stay a single load: finding out by walking
  the registry needs ipc_disable(), which is a retry loop that can sleep(0) -
  far too heavy to pay at the widget frame rate in every X client for nothing.
  Always written under the owning window's painting_lock.*/
static volatile uint32_t _xwin_present_pending_num = 0;

/*caller holds xwin->painting_lock*/
static void xwin_present_pending_set(xwin_t* xwin, bool pending) {
    if(xwin->present_pending == pending)
        return;
    xwin->present_pending = pending;
    if(pending)
        __sync_add_and_fetch(&_xwin_present_pending_num, 1);
    else
        __sync_sub_and_fetch(&_xwin_present_pending_num, 1);
}

xwin_t* xwin_find_by_handle(ewokos_addr_t handle) {
    if(handle == 0)
        return NULL;

    ipc_disable();
    xwin_t* xwin = _xwin_registry;
    while(xwin != NULL) {
        if((ewokos_addr_t)xwin->xinfo_shm_id == handle) {
            ipc_enable();
            return xwin;
        }
        xwin = xwin->reg_next;
    }
    ipc_enable();
    return NULL;
}

static int xwin_update_info(xwin_t* xwin, uint8_t type) {
    if(xwin->xinfo == NULL)
        return -1;

    if((xwin->ws_g_shm != NULL || xwin->ws_g2_shm != NULL) &&
            (type & X_UPDATE_REBUILD) != 0) {
        // Wait out any in-flight xwin_repaint() on another thread (it
        // holds painting_lock across the whole blit into the workspace
        // shm): detaching a buffer here while a blit writes into it turns
        // the next store into a data abort on the unmapped page. Both the
        // ws_g and the fps_async ws_g2 buffer are dropped so the next
        // x_get_graph remaps the freshly rebuilt shm ids.
        pthread_mutex_lock(&xwin->painting_lock);
        if(xwin->ws_g_shm != NULL) {
            shmdt(xwin->ws_g_shm);
            xwin->ws_g_shm = NULL;
            xwin->ws_g_shm_id = -1;
        }
        if(xwin->ws_g2_shm != NULL) {
            shmdt(xwin->ws_g2_shm);
            xwin->ws_g2_shm = NULL;
            xwin->ws_g2_shm_id = -1;
        }
        /*the rebuild reallocates and clears ws_g, so a present that was
          skipped for the old canvas has nothing left to publish: drop it.
          Every rebuild is followed by a repaint which renders a fresh frame.*/
        xwin_present_pending_set(xwin, false);
        xwin->present_ws_g_id = -1;
        pthread_mutex_unlock(&xwin->painting_lock);
    }

    proto_t in;
    PF->format(&in, "i,i", (ewokos_addr_t)xwin->xinfo_shm_id,
            (ewokos_addr_t)type);
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
    PF->format(&in, "i", (ewokos_addr_t)show);
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

xwin_t* xwin_open(x_t* xp, int32_t disp_index, int x, int y, int w, int h, const char* title, int style) {
    if(w <= 0 || h <= 0)
        return NULL;
    disp_index = x_get_display_id(disp_index);

    int fd = open("/dev/x", O_RDWR);
    if(fd < 0)
        return NULL;

    grect_t xr;
    x_get_desktop_space(disp_index, &xr);
    if(w > xr.w)
        w = xr.w;
    if(h > xr.h)
        h = xr.h;

    grect_t r;
    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;

    xwin_t* ret = (xwin_t*)malloc(sizeof(xwin_t));
    if(ret == NULL) {
        close(fd);
        return NULL;
    }
    memset(ret, 0, sizeof(xwin_t));
    ret->fd = fd;
    ret->x = xp;
    ret->ws_g_shm_id = -1;
    ret->ws_g2_shm_id = -1;

    key_t key = 0;
    uint32_t uuid = proc_get_uuid(getpid());
    int32_t xinfo_shm_id = xwin_alloc_shm(uuid, sizeof(xinfo_t), 0600 |IPC_CREAT|IPC_EXCL, &key);
    if(xinfo_shm_id == -1) {
        /* #region debug-point B:shmget-xinfo */
        klog("[DEBUG][B] xwin_open shmget failed pid=%d uuid=%u key=%x title=%s ptr=%llx size=%d\n",
                        getpid(), uuid, (uint32_t)key,
                        title == NULL ? "<null>" : title, (unsigned long long)(ewokos_addr_t)ret, (int)sizeof(xinfo_t));
        /* #endregion */
        close(fd);
        free(ret);
        return NULL;
    }

    xinfo_t* xinfo = (xinfo_t*)shmat(xinfo_shm_id, 0, 0);
    if(xinfo == (void*)-1) {
        /* #region debug-point C:shmat-xinfo */
        klog("[DEBUG][C] xwin_open shmat failed pid=%d uuid=%u key=%x shm_id=%d title=%s ptr=%llx\n",
                        getpid(), uuid, (uint32_t)key, xinfo_shm_id,
                        title == NULL ? "<null>" : title, (unsigned long long)(ewokos_addr_t)ret);
        /* #endregion */
        close(fd);
        free(ret);
        return NULL;
    }

    if((style & XWIN_STYLE_PROMPT) != 0)
        xp->prompt_win = ret;

    ret->xinfo_shm_id = xinfo_shm_id;
    ret->xinfo = xinfo;
    // Must be ready before the first xwin_update_info() below: its
    // REBUILD path takes painting_lock to serialize against repaints
    pthread_mutex_init(&ret->painting_lock, NULL);
    memset(ret->xinfo, 0, sizeof(xinfo_t));
    ret->xinfo->ws_g_shm_id = -1;
    ret->xinfo->win = xwin_handle(ret);
    ret->xinfo->style = style;
    ret->xinfo->display_index = disp_index;
    /*update_pid must start at -1, not 0: pid 0 is a valid kernel/idle
      context and the server would fire a bogus proc_wakeup_by(0, ...) if
      it ever observed update_requested before the first repaint published
      a real thread pid.*/
    ret->xinfo->update_requested = 0;
    ret->xinfo->update_pid = -1;
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

    xwin_registry_add(ret);
    xwin_update_info(ret, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
    return ret;
}

int xwin_fullscreen(xwin_t* xwin) {
    xwin->xinfo->style |= XWIN_STYLE_NO_RESIZE | XWIN_STYLE_NO_TITLE | XWIN_STYLE_NO_FRAME;
    return xwin_max(xwin);
}

static graph_t* x_get_graph(xwin_t* xwin, graph_t* g) {
    if(xwin == NULL || xwin->xinfo == NULL || xwin->xinfo->ws_g_shm_id == -1)
        return NULL;

    /*The client render target is ALWAYS ws_g (buffer 0), in both modes. This is
      required by framebuffer-style clients (e.g. the SDL2 ewokos backend) which
      cache the pixel pointer returned at window-create time (surface->pixels)
      and blit every frame into that one fixed buffer, re-checking it still
      matches at present. Returning an alternating buffer here would make that
      check fail after the first swap and freeze the display. fps_async keeps a
      stable render target and instead performs the "flip" as an explicit copy
      ws_g -> ws_g2 inside xwin_repaint (see there), so ws_g2 is the handoff
      buffer the server snapshots while the client keeps painting ws_g.*/
    int32_t shm_id = xwin->xinfo->ws_g_shm_id;
    bool contig = xwin->xinfo->ws_g_shm_contig;
    void** cache = &xwin->ws_g_shm;
    int32_t* cache_id = &xwin->ws_g_shm_id;
    if(shm_id == -1)
        return NULL;

    /*the server may rebuild the workspace shm and publish a new id into xinfo;
      remap only when that id changed, instead of paying a shmat syscall on
      every repaint.*/
    if(*cache == NULL || *cache_id != shm_id) {
        void* p = shmat(shm_id, 0, 0);
        if(p == (void*)-1)
            return NULL;
        if(*cache != NULL)
            shmdt(*cache);
        *cache = p;
        *cache_id = shm_id;
        /*on_resize tracks the workspace geometry, which only changes on a
          server rebuild (a fresh ws_g shm id).*/
        if(xwin->on_resize != NULL) {
            xwin->on_resize(xwin);
        }
    }

    g->buffer = *cache;
    g->w = xwin->xinfo->wsr.w;
    g->h = xwin->xinfo->wsr.h;
    /*carry the canvas identity + backing type so g2d ops can engage on this
      graph; need_free stays false, the shm is owned by the server and the
      detach above, graph_free must not touch it*/
    g->shm_id = shm_id;
    g->shm_contig = contig;
    g->need_free = false;
    return g;
}

/*Attach (and cache) the fps_async handoff buffer ws_g2 and describe it as a
  graph_t so xwin_repaint can blit the freshly rendered ws_g into it. ws_g2 is
  the buffer the server snapshots, so copying into it here - rather than letting
  the server read ws_g directly - is what lets the client keep painting ws_g
  without blocking and without tearing the server's source. Returns NULL when
  fps_async is off or ws_g2 is not (yet) published.*/
static graph_t* x_get_flip_graph(xwin_t* xwin, graph_t* g) {
    if(xwin == NULL || xwin->xinfo == NULL || !xwin->xinfo->fps_async)
        return NULL;

    int32_t shm_id = xwin->xinfo->ws_g2_shm_id;
    if(shm_id <= 0)
        return NULL;

    if(xwin->ws_g2_shm == NULL || xwin->ws_g2_shm_id != shm_id) {
        void* p = shmat(shm_id, 0, 0);
        if(p == (void*)-1)
            return NULL;
        if(xwin->ws_g2_shm != NULL)
            shmdt(xwin->ws_g2_shm);
        xwin->ws_g2_shm = p;
        xwin->ws_g2_shm_id = shm_id;
    }

    g->buffer = xwin->ws_g2_shm;
    g->w = xwin->xinfo->wsr.w;
    g->h = xwin->xinfo->wsr.h;
    g->shm_id = shm_id;
    g->shm_contig = xwin->xinfo->ws_g2_shm_contig;
    g->need_free = false;
    return g;
}

/*Perform the fps_async "flip": copy the rendered workspace ws_g onto the
  handoff buffer ws_g2 and publish it to the server. Returns false when the
  server still owns ws_g2 (update_requested != 0) or the buffers are not
  usable. Caller holds painting_lock.

  The copy is bracketed by the painting flag - it is the only write the
  compositor's source buffer sees from this side, so while it runs the server
  must not composite ws_g2 or hand it to xwm - and front_index is written
  before the barrier with update_requested after it, so the server never reads
  ws_g2 until the copy is fully visible.*/
static bool xwin_flip_locked(xwin_t* xwin) {
    if(xwin->xinfo == NULL || !xwin->xinfo->fps_async)
        return false;
    /*the invariant that keeps ws_g2 stable while the server composites it:
      publish only once the previous submission was consumed*/
    if(xwin->xinfo->update_requested != 0)
        return false;

    graph_t g;
    if(xwin_fetch_graph(xwin, &g) == NULL || g.buffer == NULL)
        return false;

    graph_t front;
    memset(&front, 0, sizeof(graph_t));
    if(x_get_flip_graph(xwin, &front) == NULL || front.buffer == NULL)
        return false;

    xwin->xinfo->painting = 1;
    __sync_synchronize();
    graph_blt(&g, 0, 0, g.w, g.h, &front, 0, 0, front.w, front.h);
    xwin->xinfo->front_index = 1;
    xwin->xinfo->painting = 0;
    __sync_synchronize();
    xwin->xinfo->update_requested = 1;
    return true;
}

/*how many skipped presents one event-loop tick retries at most; the rest are
  picked up on the following tick*/
#define XWIN_FLIP_RETRY_MAX 16

void xwin_retry_pending_presents(void) {
    if(_xwin_present_pending_num == 0)
        return;

    xwin_t* pending[XWIN_FLIP_RETRY_MAX];
    uint32_t num = 0;

    /*snapshot the list with IPC disabled so a concurrent xwin_open/xwin_close
      on a painter thread cannot pull a window out from under the walk. The
      flips themselves run outside that critical section: a full-frame blit
      must not happen with IPC disabled.*/
    ipc_disable();
    xwin_t* w = _xwin_registry;
    while(w != NULL && num < XWIN_FLIP_RETRY_MAX) {
        if(w->present_pending)
            pending[num++] = w;
        w = w->reg_next;
    }
    ipc_enable();

    for(uint32_t i = 0; i < num; i++) {
        xwin_t* xwin = pending[i];
        pthread_mutex_lock(&xwin->painting_lock);
        if(xwin->fd > 0 && xwin->xinfo != NULL &&
                xwin->xinfo->fps_async &&
                xwin->present_ws_g_id == xwin->xinfo->ws_g_shm_id) {
            /*still the same canvas the skipped present rendered into, so ws_g
              holds exactly the picture that was never published*/
            if(xwin_flip_locked(xwin))
                xwin_present_pending_set(xwin, false);
        }
        else {
            /*closed, rebuilt since, or downgraded to the blocking handshake
              (the server turns fps_async off per window when it cannot get
              the shm for ws_g2): that frame is gone, and the repaint which
              follows every rebuild publishes a fresh one*/
            xwin_present_pending_set(xwin, false);
        }
        pthread_mutex_unlock(&xwin->painting_lock);
    }
}

void xwin_destroy(xwin_t* xwin) {
    if(xwin != NULL) {
        xwin_registry_remove(xwin);
        /*the object is going away, so a present still recorded on it can never
          be published: drop it from the count or every later event-loop tick
          walks the registry for nothing*/
        if(xwin->present_pending)
            __sync_sub_and_fetch(&_xwin_present_pending_num, 1);
        free(xwin);
    }
}

void xwin_close(xwin_t* xwin) {
    if(xwin == NULL || xwin->fd <= 0)
        return;

    if(xwin->on_close != NULL) {
        if(!xwin->on_close(xwin))
            return;
    }

    xwin_registry_remove(xwin);

    // Wait out any in-flight xwin_repaint() (it can be blocked in the
    // UPDATE IPC on another thread) before tearing the window down:
    // fd, workspace shm and xinfo must stay valid while a repaint is
    // using them.
    pthread_mutex_lock(&xwin->painting_lock);
    close(xwin->fd);
    xwin->fd = -1;

    if(xwin->ws_g_shm != NULL) {
        shmdt(xwin->ws_g_shm);
        xwin->ws_g_shm = NULL;
    }
    xwin->ws_g_shm_id = -1;
    if(xwin->ws_g2_shm != NULL) {
        shmdt(xwin->ws_g2_shm);
        xwin->ws_g2_shm = NULL;
    }
    xwin->ws_g2_shm_id = -1;

    if(xwin->xinfo != NULL) {
        shmdt(xwin->xinfo);
        xwin->xinfo = NULL;
    }

    xwin->data = NULL;
    pthread_mutex_unlock(&xwin->painting_lock);
    pthread_mutex_destroy(&xwin->painting_lock);

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
    if(xwin == NULL || xwin->fd <= 0 || xwin->xinfo == NULL)
        return;

    pthread_mutex_lock(&xwin->painting_lock);
    if(xwin->xinfo != NULL &&
            xwin->xinfo->update_theme &&
            xwin->on_update_theme != NULL) {
        xwin->on_update_theme(xwin);
    }

    /*Claim the workspace before anything draws into it: the compositor reads this
      very buffer now (there is no private snapshot any more), so it has to
      know the canvas is mid-frame. For an on_repaint app the drawing happens
      inside this call, so this is the exact bracket. For a framebuffer-style
      app (the SDL2 backend draws into the window surface between presents)
      the flag is already set from the release at the end of the previous
      call - this write just re-asserts it. fps_async skips it: there the
      compositor reads ws_g2, which only the handoff copy below writes, and
      that copy brackets itself.*/
    if(xwin->xinfo != NULL && !xwin->xinfo->fps_async)
        xwin->xinfo->painting = 1;

    graph_t g;
    if(xwin_fetch_graph(xwin, &g) != NULL) {
        if(xwin->on_repaint != NULL) {
            xwin->on_repaint(xwin, &g);
        }
    }
    if(xwin->fd <= 0 || xwin->xinfo == NULL) {
        pthread_mutex_unlock(&xwin->painting_lock);
        return;
    }

    if(xwin->xinfo->fps_async) {
        /*Non-blocking, double-buffered submit. The client always renders into
          ws_g (see x_get_graph); xwin_flip_locked "flips" by copying that
          just-rendered ws_g into the handoff buffer ws_g2, which is the buffer
          the server composites. It only publishes while the server has consumed
          the previous submission (update_requested==0), so the copy never races
          the compositor and the client never blocks - the client keeps its own
          fps and the display fps stays the server's.

          A present the server is still busy with must NOT simply be dropped,
          which is what this used to do: the picture exists only in ws_g, and
          the caller above (RootWidget::repaintWin, Widget::repaint) has already
          cleared doRefresh and every dirty flag on the way here, so nothing
          will ever render it again. An event-driven app - which is every widget
          app - then sits idle with front_index still 0, the server keeps
          win_comp_src on the empty ws_g2 and the window never becomes ready:
          the process runs, has painted, and stays invisible. Record the skipped
          frame and let the event loop publish it as soon as the server lets go
          of the handoff buffer (see xwin_retry_pending_presents).*/
        if(xwin_flip_locked(xwin)) {
            xwin_present_pending_set(xwin, false);
        }
        else if(g.buffer != NULL && xwin->xinfo->visible) {
            xwin_present_pending_set(xwin, true);
            xwin->present_ws_g_id = xwin->xinfo->ws_g_shm_id;
        }
        xwin->xinfo->update_theme = false;
        pthread_mutex_unlock(&xwin->painting_lock);
        return;
    }

    /*blocking shm UPDATE handshake (replaces the XWIN_CNTL_UPDATE IPC). Publish
      the CURRENT THREAD pid - not getpid(): proc_wakeup_by targets one specific
      proc entry, and getpid returns the root task pid, so on a painter thread
      (e.g. macemu's present_thread) the server would wake the wrong context and
      this thread would stay blocked forever. The barrier pairs with the server's
      __sync_synchronize so it never observes update_requested=1 with a half-drawn
      ws_g. The while loop absorbs spurious wakeups and the latched-wake race the
      kernel documents in proc_block_by: if the server already cleared the flag
      and fired the wake before we trapped in, wake_pending releases us
      immediately and the re-check exits.*/
    xwin->xinfo->update_pid = thread_get_id();
    /*the frame is complete: hand the canvas over. Clearing before the barrier
      means the server can never observe update_requested=1 while painting is
      still 1, and never composite a buffer we are about to write again.*/
    xwin->xinfo->painting = 0;
    __sync_synchronize();
    xwin->xinfo->update_requested = 1;
    while(xwin->xinfo != NULL && xwin->xinfo->update_requested) {
        proc_block_by(xwin->xinfo->win);
    }
    /*xwin_close may have torn xinfo down while we were parked: never touch
      it unchecked after the block returns.*/
    if(xwin->xinfo != NULL) {
        xwin->xinfo->update_theme = false;
        /*Released: the server is done with this frame and the app is about to
          draw the next one into ws_g, which the compositor reads directly.
          Re-claim it here so a framebuffer-style app - which draws outside
          this library - is covered too, and keep it claimed until the next
          publish above. An app that goes idle instead leaves the flag set;
          the server bounds that wait with X_PAINT_TIMEOUT_MS.*/
        xwin->xinfo->painting = 1;
    }
    pthread_mutex_unlock(&xwin->painting_lock);
}

void xwin_repaint_req(xwin_t* xwin) {
    if(xwin == NULL || xwin->x == NULL)
        return;
    x_t* x = xwin->x;
    xevent_t ev;
    memset(&ev, 0, sizeof(xevent_t));
    ev.win = xwin_handle(xwin);
    ev.value.window.event = XEVT_WIN_REPAINT;
    ev.type = XEVT_WIN;
    x_push_event(x, &ev);
}

int xwin_set_display(xwin_t* xwin, uint32_t display_index) {
    if((int32_t)display_index >= x_get_display_num())
        display_index = 0;

    xwin->xinfo->display_index = display_index;
    xwin_update_info(xwin, X_UPDATE_REFRESH);
    return 0;
}

/*Stage a geometry change into the pending handoff fields and flag it for the
  server, instead of writing the live wsr/state the compositor reads. The server
  applies these under x_server_lock at the start of xwin_update_info, so live
  geometry and the rebuilt buffers only ever change together, atomically w.r.t.
  the compositor. Both fields are seeded by the caller from the current live
  values so changing one axis (or the state) leaves the others intact. The
  pending writes are ordered before geom_pending with a release barrier; the
  synchronous UPDATE_INFO round-trip that follows guarantees the server has
  applied them (and updated live wsr) before this call returns, so a subsequent
  stage seeds from fresh values.*/
static void xwin_stage_geom(xwin_t* xwin, const grect_t* wsr, uint32_t state) {
    xwin->xinfo->wsr_pending = *wsr;
    xwin->xinfo->state_pending = state;
    __sync_synchronize();
    xwin->xinfo->geom_pending = 1;
}

int xwin_resize_to(xwin_t* xwin, int w, int h) {
    grect_t xr;
    x_get_desktop_space(xwin->xinfo->display_index, &xr);
    if(w > xr.w)
        w = xr.w;
    if(h > xr.h)
        h = xr.h;

    grect_t wsr = xwin->xinfo->wsr;
    wsr.w = w;
    wsr.h = h;
    xwin_stage_geom(xwin, &wsr, xwin->xinfo->state);
    xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
    xwin_repaint(xwin);
    return 0;
}

int xwin_max(xwin_t* xwin) {
    memcpy(&xwin->xinfo_prev, xwin->xinfo, sizeof(xinfo_t));
    /*stage the max state instead of writing it live: the server turns
      state_pending into state (and clamps wsr to fullscreen) under the lock*/
    xwin_stage_geom(xwin, &xwin->xinfo->wsr, XWIN_STATE_MAX);
    xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);

    if(xwin->on_resize != NULL)
        xwin->on_resize(xwin);
    xwin_repaint(xwin);
    return 0;
}

int xwin_resize(xwin_t* xwin, int dw, int dh) {
    return xwin_resize_to(xwin, xwin->xinfo->wsr.w+dw, xwin->xinfo->wsr.h+dh);
}

int xwin_move_to(xwin_t* xwin, int x, int y) {
    grect_t wsr = xwin->xinfo->wsr;
    wsr.x = x;
    wsr.y = y;
    xwin_stage_geom(xwin, &wsr, xwin->xinfo->state);
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
        grect_t wsr = xwin->xinfo->wsr;
        wsr.w += ev->value.window.v0;
        wsr.h += ev->value.window.v1;
        xwin_stage_geom(xwin, &wsr, xwin->xinfo->state);
        xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
        if(xwin->on_resize != NULL)
            xwin->on_resize(xwin);
        xwin_repaint(xwin);
    }
    else if(ev->value.window.event == XEVT_WIN_MOVE) {
        grect_t wsr = xwin->xinfo->wsr;
        wsr.x += ev->value.window.v0;
        wsr.y += ev->value.window.v1;
        xwin_stage_geom(xwin, &wsr, xwin->xinfo->state);
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
        if(xwin->on_resize != NULL)
            xwin->on_resize(xwin);

        if(xwin->xinfo->state == XWIN_STATE_MAX) {
            /*restore the pre-max geometry through the pending handoff rather
              than memcpy-ing the whole backed-up xinfo back over the live one:
              a full memcpy wrote wsr/state the compositor reads directly (the
              same race the pending path removes), and dragged stale shm ids
              back with it. Only wsr and state actually need restoring here -
              max never changes style, and update_info rebuilds every buffer.*/
            xwin_stage_geom(xwin, &xwin->xinfo_prev.wsr, xwin->xinfo_prev.state);
            xwin_update_info(xwin, X_UPDATE_REBUILD | X_UPDATE_REFRESH);
            if(xwin->on_resize != NULL)
                xwin->on_resize(xwin);
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

void xwin_hide_cursor(xwin_t* xwin, bool hide) {
    if(xwin->xinfo == NULL || xwin->xinfo->hide_cursor == hide)
        return;
    xwin->xinfo->hide_cursor = hide;
    xwin_update_info(xwin, X_UPDATE_REFRESH);
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

gpos_t xwin_get_inside_pos(xwin_t* xwin, int32_t x, int32_t y) {
    gpos_t pos = {0};
    if(xwin == NULL || xwin->xinfo == NULL)
        return pos;
    pos.x = x - xwin->xinfo->wsr.x;
    pos.y = y - xwin->xinfo->wsr.y;
    return pos;
}

gpos_t xwin_get_screen_pos(xwin_t* xwin, int32_t x, int32_t y) {
    gpos_t pos = {0};
    if(xwin == NULL || xwin->xinfo == NULL)
        return pos;
    pos.x = x + xwin->xinfo->wsr.x;
    pos.y = y + xwin->xinfo->wsr.y;
    return pos;
}

#ifdef __cplusplus
}
#endif
