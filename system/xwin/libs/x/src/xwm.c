#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/ipc.h>
#include <ewoksys/ipc_serv.h>
#include <sys/shm.h>
#include <ewoksys/proc.h>
#include <ewoksys/vdevice.h>
#include <graph/graph.h>
#include <x/xcntl.h>
#include <x/xwm.h>
#include <setenv.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XWM_MAP_SLOTS 32
#define XWM_MAP_BYTES (64ull * 1024 * 1024)
#define XWM_DRAW_MAPS 3

typedef struct {
    int32_t id;
    void* buffer;
    uint64_t bytes, used;
    bool pinned, retired;
} xwm_mapping_t;

/*Single-task IPC serializes draws and retirement, including nested GPU calls.
  The extra slots hold oversized/over-budget mappings only until draw end.*/
static xwm_mapping_t _maps[XWM_MAP_SLOTS + XWM_DRAW_MAPS];
static uint64_t _map_tick, _map_bytes;
static int _server_pid = -1;
static uint32_t _server_uuid;

static void mapping_drop(xwm_mapping_t* m) {
    if(m->buffer == NULL)
        return;
    shmdt(m->buffer);
    if(m < &_maps[XWM_MAP_SLOTS])
        _map_bytes -= m->bytes;
    memset(m, 0, sizeof(*m));
}

static void mapping_release(int32_t id) {
    for(unsigned i = 0; i < XWM_MAP_SLOTS + XWM_DRAW_MAPS; i++) {
        xwm_mapping_t* m = &_maps[i];
        if(m->buffer != NULL && (id < 0 || m->id == id)) {
            if(m->pinned)
                m->retired = true;
            else
                mapping_drop(m);
        }
    }
}

static void mapping_end_draw(void) {
    for(unsigned i = 0; i < XWM_MAP_SLOTS + XWM_DRAW_MAPS; i++) {
        _maps[i].pinned = false;
        if(i >= XWM_MAP_SLOTS || _maps[i].retired)
            mapping_drop(&_maps[i]);
    }
}

static void* mapping_get(int32_t id, int w, int h) {
    if(id < 0 || w <= 0 || h <= 0)
        return NULL;
    uint64_t bytes = ((uint64_t)w * h * 4 + 4095) & ~4095ull;
    for(unsigned i = 0; i < XWM_MAP_SLOTS + XWM_DRAW_MAPS; i++) {
        xwm_mapping_t* m = &_maps[i];
        if(m->buffer != NULL && m->id == id && !m->retired) {
            /*Aliases describe the same allocation. The largest view owns
              its accounting; display/frame is acquired before workspace.*/
            if(bytes > m->bytes)
                return NULL;
            m->pinned = true;
            m->used = ++_map_tick;
            return m->buffer;
        }
    }
    xwm_mapping_t* slot = NULL;
    if(bytes <= XWM_MAP_BYTES) {
        for(;;) {
            xwm_mapping_t* oldest = NULL;
            for(unsigned i = 0; i < XWM_MAP_SLOTS; i++) {
                xwm_mapping_t* m = &_maps[i];
                if(m->buffer == NULL)
                    slot = m;
                else if(!m->pinned && (oldest == NULL || m->used < oldest->used))
                    oldest = m;
            }
            if(slot != NULL && _map_bytes + bytes <= XWM_MAP_BYTES)
                break;
            slot = NULL;
            if(oldest == NULL)
                break;
            mapping_drop(oldest);
        }
    }
    if(slot == NULL) {
        for(unsigned i = XWM_MAP_SLOTS; i < XWM_MAP_SLOTS + XWM_DRAW_MAPS; i++)
            if(_maps[i].buffer == NULL) { slot = &_maps[i]; break; }
    }
    if(slot == NULL)
        return NULL;
    void* buffer = shmat(id, 0, 0);
    if(buffer == NULL || buffer == (void*)-1)
        return NULL;
    slot->id = id;
    slot->buffer = buffer;
    slot->bytes = bytes;
    slot->used = ++_map_tick;
    slot->pinned = true;
    if(slot < &_maps[XWM_MAP_SLOTS])
        _map_bytes += bytes;
    return buffer;
}

static bool fetch_desktop_graph(xwm_t* xwm, int32_t shm_id, int w, int h, graph_t* g) {
    (void)xwm;
    uint8_t* g_buf = mapping_get(shm_id, w, h);

    if(g_buf == NULL || g_buf == (void*)-1)
        return false;
    graph_init(g, (const uint32_t*)g_buf, w, h);
    return true;
}

static bool fetch_frame_graph(xwm_t* xwm, xinfo_t* info, graph_t* g) {
    if(info == NULL || info->frame_g_shm_id == -1)
        return false;
    memset(g, 0, sizeof(graph_t));

    uint32_t* frame_g_shm = (uint32_t*)mapping_get(info->frame_g_shm_id, info->winr.w, info->winr.h);
    if(frame_g_shm == NULL || frame_g_shm == (void*)-1)
        return false;
    g->buffer = frame_g_shm;
    g->w = info->winr.w;
    g->h = info->winr.h;
    g->shm_id = info->frame_g_shm_id;
    g->shm_contig = info->frame_g_shm_contig;
    g->need_free = false;
    return true;
}

static bool fetch_ws_graph(xwm_t* xwm, xinfo_t* info, graph_t* g) {
    /*read the server's composite source, which is the canvas the compositor
      blits frame_g and the scan-out from - so the decorations land on exactly
      the pixels that go on screen, in both modes.

      That used to be a private server-owned snapshot (ws_g_buffer) taken on
      every accepted frame. The snapshot is gone: the compositor now reads the
      buffer the client published into, and ws_g_buffer_shm_id names that one
      instead (ws_g in the blocking handshake, ws_g[front_index] in fps_async
      mode). Sampling a client's live render buffer would tear, so the server
      only ever issues DRAW_FRAME from inside draw_win, gated on the client's
      xinfo->painting flag - by the time this runs the painter is parked or
      idle and the buffer holds a complete frame.

      The shmat/shmdt pair here is xwm's own: the segment keeps a reference
      from the server (which allocated it) and from the client (which renders
      into it), so detaching after the draw cannot free it out from under
      either.*/
    if(info == NULL || info->ws_g_buffer_shm_id == -1)
        return false;
    memset(g, 0, sizeof(graph_t));

    uint32_t* ws_g_shm = (uint32_t*)mapping_get(info->ws_g_buffer_shm_id, info->wsr.w, info->wsr.h);
    if(ws_g_shm == NULL || ws_g_shm == (void*)-1)
        return false;
    g->buffer = ws_g_shm;
    g->w = info->wsr.w;
    g->h = info->wsr.h;
    g->shm_id = info->ws_g_buffer_shm_id;
    g->shm_contig = info->ws_g_buffer_shm_contig;
    g->need_free = false;
    return true;
}

/*keep_shm_id is the display shm handed over by DRAW_FRAME: a direct
  window aliases it as its frame/workspace, and it is the very segment
  the cached desktop graph points at. shmat on an already-mapped id is
  a no-op that does not take a ref, so detaching it here would tear the
  desktop mapping out of this process and leave the cache dangling.*/
static void free_win_graph(graph_t* g, int32_t keep_shm_id) {
    (void)g;
    (void)keep_shm_id;
    /*Mapping ownership is draw-scoped; aliases remain pinned until dispatch
      finishes, including all failure paths.*/
}

static void draw_drag_frame(xwm_t* xwm, proto_t* in) {
    grect_t r;
    int32_t shm_id = proto_read_int(in);
    if(shm_id == -1)
        return;

    int xw = proto_read_int(in);
    int xh = proto_read_int(in);
    proto_read_to(in, &r, sizeof(grect_t));

    graph_t g;
    if(fetch_desktop_graph(xwm, shm_id, xw, xh, &g)) {
        if(xwm->draw_drag_frame != NULL)
            xwm->draw_drag_frame(&g, &r, xwm->data);
        else
            graph_rect(&g, r.x, r.y, r.w, r.h, 0xffffffff);
    }
}

static void draw_frame(xwm_t* xwm, proto_t* in) {
    xinfo_t info;
    int32_t shm_id = proto_read_int(in);
    if(shm_id == -1)
        return;

    int xw = proto_read_int(in);
    int xh = proto_read_int(in);
    proto_read_to(in, &info, sizeof(xinfo_t));
    bool bg_effect = (proto_read_int(in) != 0);
    /*per-frame hints from xserverd. backdrop_unchanged says the clean backdrop
      handed over is byte-identical to the previous DRAW_FRAME's, so a frosted
      xwm can skip its backdrop scan. corners_only says only this window's own
      workspace content changed - the decoration ring still sitting in frame_g
      is valid and only the rounded corners the fresh content overwrote have to
      be re-cut, so the title, buttons, shadow and bg effect are all skipped.*/
    info.backdrop_unchanged = (proto_read_int(in) != 0);
    info.corners_only = (proto_read_int(in) != 0);
    memset(&xwm->draw.frame, 0, sizeof(xwm->draw.frame));
    proto_read_to(in, &xwm->draw.frame, sizeof(xwm->draw.frame));
    xwm->draw.backdrop_id = shm_id;
    xwm->draw.serial++;
    if(xwm->draw.serial == 0)
        xwm->draw.serial++;
    bool keep_decor = info.corners_only ||
            (xwm->retained_decorations && xwm->draw.frame.content_only);
    bool top = info.focused;

    if(xw <= 0 || xh <= 0)
        return;

    graph_t desktop_g;
    graph_t frame_g;
    graph_t ws_g;
    memset(&desktop_g, 0, sizeof(graph_t));
    memset(&frame_g, 0, sizeof(graph_t));
    memset(&ws_g, 0, sizeof(graph_t));
    if(!fetch_desktop_graph(xwm, shm_id, xw, xh, &desktop_g))
        return;
    if(!fetch_frame_graph(xwm, &info, &frame_g))
        return;
    if(!fetch_ws_graph(xwm, &info, &ws_g)) {
        free_win_graph(&frame_g, shm_id);
        return;
    }

    grect_t rframe;
    memset(&rframe, 0, sizeof(grect_t));
    if(xwm->get_frame != NULL) {
        xwm->get_frame(&info, &rframe, xwm->data);
    }
    bool framed = false;
    /*NO_FRAME only drops the border ring and shadow; the title bar and the
      window buttons survive unless NO_TITLE is set. So the two concerns are
      gated separately: title/buttons on hasTitle, border/resize/shadow on
      hasFrame.*/
    bool hasTitle = (info.style & XWIN_STYLE_NO_TITLE) == 0;
    bool hasFrame = (info.style & XWIN_STYLE_NO_FRAME) == 0;

    if(hasTitle || hasFrame) {
        grect_t rtitle, rclose, rmax, rmin, rresize;
        memset(&rtitle, 0, sizeof(grect_t));
        memset(&rclose, 0, sizeof(grect_t));
        memset(&rmax, 0, sizeof(grect_t));
        memset(&rmin, 0, sizeof(grect_t));
        memset(&rresize, 0, sizeof(grect_t));

        if(hasTitle) {
            if(xwm->get_title != NULL)
                xwm->get_title(&info, &rtitle, xwm->data);
            if(xwm->get_close != NULL)
                xwm->get_close(&info, &rclose, xwm->data);
            if(xwm->get_max != NULL)
                xwm->get_max(&info, &rmax, xwm->data);
            if(xwm->get_min != NULL)
                xwm->get_min(&info, &rmin, xwm->data);
        }
        /*the resize corner lives in the border ring, so it only exists on a
          framed window*/
        if(hasFrame && xwm->get_resize != NULL)
            xwm->get_resize(&info, &rresize, xwm->data);

        if(hasTitle && !keep_decor) {
            if(xwm->draw_title != NULL && rtitle.w > 0 && rtitle.h > 0)
                xwm->draw_title(&desktop_g, &frame_g, &info, &rtitle, top, xwm->data);

            if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
                if(xwm->draw_max != NULL && rmax.w > 0 && rmax.h > 0)
                    xwm->draw_max(&frame_g, &info, &rmax, top, xwm->data);
                if(xwm->draw_min != NULL && rmin.w > 0 && rmin.h > 0)
                    xwm->draw_min(&frame_g, &info, &rmin, top, xwm->data);
            }

            if(xwm->draw_close != NULL && rclose.w > 0 && rclose.h > 0)
                xwm->draw_close(&frame_g, &info, &rclose, top, xwm->data);
        }

        if(hasFrame && info.state != XWIN_STATE_MAX) {
            if(xwm->draw_frame != NULL) {
                xwm->draw_frame(&desktop_g, &frame_g, &ws_g, &info, &rframe, top, xwm->data);
                framed = true;
            }

            if(hasTitle && !keep_decor) {
                if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
                    if(xwm->draw_resize != NULL && rresize.w > 0 && rresize.h > 0)
                        xwm->draw_resize(&frame_g, &info, &rresize, top, xwm->data);
                }
            }

            if(xwm->draw_shadow!= NULL && xwm->theme.shadow > 0 && !keep_decor)
                xwm->draw_shadow(&desktop_g, &frame_g, &info, top, xwm->data);
        }
    }

    /*ws_blur only frosts the client area of a translucent window (the frame
      hook itself blends ws_g over the glass for alpha ones only); an opaque
      frameless/maximized window would just pay the backdrop scan for nothing*/
    if(xwm->theme.wsBlur != 0 && info.alpha && !framed) {
        if(xwm->draw_frame != NULL)
            xwm->draw_frame(&desktop_g, &frame_g, &ws_g, &info, &rframe, top, xwm->data);
    }

    if((info.style & XWIN_STYLE_NO_BG_EFFECT) == 0 &&
            !info.focused && !keep_decor &&
            xwm->draw_bg_effect != NULL) {
        xwm->draw_bg_effect(&desktop_g, &frame_g, &ws_g, &info, bg_effect, xwm->data);
    }
    free_win_graph(&frame_g, shm_id);
    free_win_graph(&ws_g, shm_id);
}

static void get_frame_areas(xwm_t* xwm, proto_t* in, proto_t* out) {
    xinfo_t info;
    proto_read_to(in, &info, sizeof(xinfo_t));

    grect_t rtitle, rclose, rmax, rmin, rresize;
    memset(&rtitle, 0, sizeof(grect_t));
    memset(&rclose, 0, sizeof(grect_t));
    memset(&rmax, 0, sizeof(grect_t));
    memset(&rmin, 0, sizeof(grect_t));
    memset(&rresize, 0, sizeof(grect_t));

    if((info.style & XWIN_STYLE_NO_TITLE) == 0) {

        if(xwm->get_title != NULL)
            xwm->get_title(&info, &rtitle, xwm->data);
        if(xwm->get_close != NULL)
            xwm->get_close(&info, &rclose, xwm->data);
        if(xwm->get_min != NULL)
            xwm->get_min(&info, &rmin, xwm->data);

        if((info.style & XWIN_STYLE_NO_RESIZE) == 0) {
            if(xwm->get_max != NULL)
                xwm->get_max(&info, &rmax, xwm->data);
            /*the resize corner is part of the border ring: a frameless window
              has none, so its hit area stays empty*/
            if((info.style & XWIN_STYLE_NO_FRAME) == 0 && xwm->get_resize != NULL)
                xwm->get_resize(&info, &rresize, xwm->data);
        }
    }

    rtitle.x += info.winr.x;
    rtitle.y += info.winr.y;
    rclose.x += info.winr.x;
    rclose.y += info.winr.y;
    rmin.x += info.winr.x;
    rmin.y += info.winr.y;
    rmax.x += info.winr.x;
    rmax.y += info.winr.y;
    rresize.x += info.winr.x;
    rresize.y += info.winr.y;

    PF->add(out, &rtitle, sizeof(grect_t))->
        add(out, &rclose, sizeof(grect_t))->
        add(out, &rmin, sizeof(grect_t))->
        add(out, &rmax, sizeof(grect_t))->
        add(out, &rresize, sizeof(grect_t));
}

static void draw_desktop(xwm_t* xwm, proto_t* in) {
    int32_t shm_id = proto_read_int(in);
    if(shm_id == -1)
        return;

    int xw = proto_read_int(in);
    int xh = proto_read_int(in);

    graph_t g;
    if(fetch_desktop_graph(xwm, shm_id, xw, xh, &g)) {
        if(xwm->draw_desktop != NULL)
            xwm->draw_desktop(&g, xwm->data);
    }
}

static void get_win_space(xwm_t* xwm, proto_t* in, proto_t* out) {
    grect_t r, winr;
    int style = proto_read_int(in);
    int state = proto_read_int(in);
    proto_read_to(in, &r, sizeof(grect_t));
    memcpy(&winr, &r, sizeof(grect_t));
    
    if(xwm->get_win_space != NULL)
        xwm->get_win_space(style, state, &r, &winr, xwm->data);

    PF->add(out, &winr, sizeof(grect_t));
}

static void get_min_size(xwm_t* xwm, proto_t* in, proto_t* out) {
    xinfo_t info;
    proto_read_to(in, &info, sizeof(xinfo_t));
    
    int w = 0, h = 0;
    if(xwm->get_min_size != NULL)
        xwm->get_min_size(&info, &w, &h, xwm->data);

    PF->addi(out, w)->
        addi(out, h);
}

static void set_theme(xwm_t* xwm, proto_t* in, proto_t* out) {
    proto_read_to(in, &xwm->theme, sizeof(xwm_theme_t));
    if(xwm->update_theme != NULL)
        xwm->update_theme(false, xwm->data);
}

static void handle(int from_pid, int cmd, proto_t* in, proto_t* out, void* p) {
    xwm_t* xwm = (xwm_t*)p;
    uint32_t uuid = proc_get_uuid(from_pid);
    if(_server_pid != from_pid || _server_uuid != uuid) {
        mapping_release(-1);
        if(xwm->release_window != NULL)
            xwm->release_window(0, xwm->data);
        _server_pid = from_pid;
        _server_uuid = uuid;
    }

    if(cmd == XWM_CNTL_DRAW_FRAME) { //draw frame
        draw_frame(xwm, in);
    }
    else if(cmd == XWM_CNTL_DRAW_DRAG_FRAME) {
        draw_drag_frame(xwm, in);
    }
    else if(cmd == XWM_CNTL_DRAW_DESKTOP) { //draw desktop
        draw_desktop(xwm, in);
    }
    else if(cmd == XWM_CNTL_GET_FRAME_AREAS) {
        get_frame_areas(xwm, in, out);
    }
    else if(cmd == XWM_CNTL_GET_WIN_SPACE) { //get workspace
        get_win_space(xwm, in, out);
    }
    else if(cmd == XWM_CNTL_GET_MIN_SIZE) {
        get_min_size(xwm, in, out);
    }
    else if(cmd == XWM_CNTL_SET_THEME) {
        set_theme(xwm, in, out);
    }
    else if(cmd == XWM_CNTL_RELEASE_CANVASES) {
        ewokos_addr_t win = 0;
        proto_read_to(in, &win, sizeof(win));
        int count = proto_read_int(in);
        if(count < 0)
            mapping_release(-1);
        else
            for(int i = 0; i < count; i++) {
                int32_t id = proto_read_int(in);
                if(id >= 0)
                    mapping_release(id);
            }
        if((win != 0 || count < 0) && xwm->release_window != NULL)
            xwm->release_window(win, xwm->data);
    }
    mapping_end_draw();
}

void xwm_run(xwm_t* xwm) {
    mapping_release(-1);
    ipc_serv_run(handle, NULL, xwm, IPC_NON_BLOCK);

    setenv("XWM", "true");
    proto_t in;
    PF->init(&in)->addi(&in, xwm->retained_decorations ? 1 : 0);
    dev_cntl("/dev/x", X_DCNTL_SET_XWM, &in, NULL);
    PF->clear(&in);

    while(true) {
        usleep(100000);
    }
    
    mapping_release(-1);
    if(xwm->release_window != NULL)
        xwm->release_window(0, xwm->data);
}


#ifdef __cplusplus
}
#endif
