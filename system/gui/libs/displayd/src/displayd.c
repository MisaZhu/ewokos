#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <display/display.h>
#include <displayd/displayd.h>
#include <graph/graph_image.h>
#include <tinyjson/tinyjson.h>

typedef struct {
    uint32_t size;
    uint8_t* shm;
    int32_t  shm_id;
    uint8_t  shm_contig;
    display_ctrl_t* ctrl; //independent shm segment for the control block
    int32_t  ctrl_id;
} disp_shm_t;

static disp_info_t _fbinfo = {0};
static int32_t _rotate = 0;
static float _zoom = 1.0;
static int32_t _zwidth;
static int32_t _zheight;
static fbdisplayd_t* _fbdisplayd = NULL;
static char _logo[256] = {0};
static disp_shm_t* _cur_shm = NULL; /* live shm, for fbdisplayd_refresh() */

static int disp_fcntl(vdevice_t* dev, int fd,
        int from_pid,
        fsinfo_t* info,
        int cmd,
        proto_t* in,
        proto_t* out,
        void* p) {

    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)in;
    (void)p;
    if(cmd == DISPLAY_CNTL_GET_INFO) { //get fb size
        if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
            PF->addi(out, _zheight)->addi(out, _zwidth)->addi(out, _fbinfo.depth);
        else
            PF->addi(out, _zwidth)->addi(out, _zheight)->addi(out, _fbinfo.depth);
    }
    else if(cmd == DISPLAY_CNTL_GET_CTRL) { //shm id of the ctrl block
        disp_shm_t* shm = (disp_shm_t*)p;
        if(shm != NULL)
            PF->addi(out, shm->ctrl_id);
    }
    return 0;
}

static void draw_bg(graph_t* g) {
    //graph_gradation(g, 0, 0, g->w, g->h, 0xff8888ff, 0xff000000, true);
    graph_gradation(g, 0, 0, g->w, g->h, 0xff444488, 0xff000000, true);
#if __aarch64__
    graph_t* logo = graph_image_new("/usr/system/icons/64bits.png");
    if(logo != NULL) {
        graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
                g, g->w - logo->w - 10, 10, logo->w, logo->h, 0xff);
        graph_free(logo);
    }
#endif
}

static void default_splash(graph_t* g, const char* logo_fname) {
    draw_bg(g);
    graph_t* logo = graph_image_new(logo_fname);
    if(logo != NULL) {
        graph_blt_alpha(logo, 0, 0, logo->w, logo->h,
                g, (g->w-logo->w)/2, (g->h-logo->h)/2, logo->w, logo->h, 0xff);
        graph_free(logo);
    }
}

static graph_t* _rotate_g = NULL; /* cached rotate buffer, grown as needed */
static graph_t* _rect_g = NULL;   /* cached dirty-rect extraction buffer */

/*
 * Rotation into the scan-out buffer, built on graph_rotate_to.
 *
 * Scan-out mappings are typically Normal Non-Cacheable (NC): CPU stores
 * bypass the cache and go straight to DRAM so the HVS DMA engine sees
 * them without cache-coherency stalls. NC writes are merged into DRAM
 * bursts by the write-combine (WC) buffer, but only when stores arrive
 * sequentially within a single WC stream.
 *
 * graph_rotate_to's arch kernels walk the DESTINATION row-major and fill
 * every row left-to-right with 4x4 micro-tiles, keeping 4 open WC
 * streams at a time. That works, but 4 concurrent strided WC streams are
 * still much slower than a single contiguous one.
 *
 * To maximise throughput we therefore ALWAYS rotate into a cacheable
 * intermediate buffer first (L1/L2 write-back absorbs the strided
 * stores at cache bandwidth) and then do a single sequential memcpy
 * to the NC scan-out buffer (one WC stream, maximum DRAM burst
 * efficiency). The extra copy doubles the total bytes moved, but the
 * per-byte write efficiency of a contiguous stream vs a 4-way strided
 * rotation more than compensates on typical ARM SoCs (BCM2711 etc.).
 */
static graph_t* ensure_graph(graph_t** cache, int32_t w, int32_t h) {
    if(w <= 0 || h <= 0)
        return NULL;
    /*grow-only: dirty rects and full frames alternate on the same cache,
      shrinking here would free/realloc megabytes every frame. The buffer
      is reused but w/h are always updated to the requested dimensions so
      callers can pass the returned graph_t* directly to graph_rotate_to,
      graph_blt, etc. without a graph_init wrapper. */
    if(*cache == NULL || (*cache)->w * (*cache)->h < w * h) {
        if(*cache != NULL)
            graph_free(*cache);
        *cache = graph_new_shm(w, h);
    }
    else {
        (*cache)->w = w;
        (*cache)->h = h;
    }
    return *cache;
}

static uint32_t disp_pitch32(const disp_info_t* fbi) {
    /*several platforms leave pitch at 0 because their full-frame flush is a
      plain memcpy; a packed scan-out is the only sane meaning of that */
    uint32_t pitch = fbi->pitch;
    if(pitch < fbi->width * 4)
        pitch = fbi->width * 4;
    return pitch;
}

uint32_t fbdisplayd_rotate_to(const disp_info_t* fbinfo, const graph_t* g, int rotate) {
    if (fbinfo == NULL || g == NULL || g->buffer == NULL)
        return 0;
    if (fbinfo->pointer == 0 || fbinfo->depth != 32)
        return 0;

    int32_t dw, dh;
    if (rotate == G_ROTATE_90 || rotate == G_ROTATE_270) {
        dw = g->h; dh = g->w;
    }
    else if (rotate == G_ROTATE_180) {
        dw = g->w; dh = g->h;
    }
    else
        return 0;

    if ((uint32_t)dw != fbinfo->width || (uint32_t)dh != fbinfo->height)
        return 0;

    uint32_t pitch = disp_pitch32(fbinfo);
    uint8_t* base = (uint8_t*)(ewokos_addr_t)fbinfo->pointer +
            fbinfo->yoffset * pitch + fbinfo->xoffset * 4;

    /*
     * Always rotate into a cacheable intermediate buffer, then copy to
     * the NC scan-out buffer. Rotating directly into NC memory (the old
     * packed path) writes through 4 strided WC streams which is far
     * slower than a cache-backed rotation + graph_blt (NEON streaming copy).
     */
    graph_t* tmp = ensure_graph(&_rotate_g, dw, dh);
    if (tmp == NULL)
        return 0;
    graph_rotate_to((graph_t*)g, tmp, rotate);

    graph_t dst_g;
    graph_init(&dst_g, (uint32_t*)base, dw, dh);
    graph_blt(tmp, 0, 0, dw, dh, &dst_g, 0, 0, dw, dh);
    return (uint32_t)dw * (uint32_t)dh * 4;
}

static uint32_t (*_flush_rect)(const disp_info_t*, const graph_t*, const grect_t*) = NULL;
static char* (*_dev_cmd)(int from_pid, int argc, char** argv) = NULL;

void fbdisplayd_set_flush_rect(uint32_t (*flush_rect)(const disp_info_t* fbinfo,
        const graph_t* g, const grect_t* r)) {
    _flush_rect = flush_rect;
}

void fbdisplayd_set_dev_cmd(char* (*dev_cmd)(int from_pid, int argc, char** argv)) {
    _dev_cmd = dev_cmd;
}

uint32_t fbdisplayd_flush_rect_to(const disp_info_t* fbinfo, const graph_t* g, const grect_t* r) {
    if(fbinfo == NULL || g == NULL || g->buffer == NULL || r == NULL)
        return 0;
    if(fbinfo->pointer == 0)
        return 0;
    if(fbinfo->depth != 32 && fbinfo->depth != 16)
        return 0;
    /* the rect addressing below only holds when the client frame and the
     * panel share the same geometry (no rotation, no scaling) */
    if((uint32_t)g->w != fbinfo->width || (uint32_t)g->h != fbinfo->height)
        return 0;
    if(r->w <= 0 || r->h <= 0)
        return 0;

    uint32_t bytes_per_pixel = fbinfo->depth / 8;
    if(fbinfo->depth == 32 &&
            (ewokos_addr_t)fbinfo->pointer == (ewokos_addr_t)g->buffer)
        return (uint32_t)r->w * (uint32_t)r->h * 4; //scan-out is the dma itself

    uint32_t pitch = fbinfo->pitch;
    if(pitch < fbinfo->width * bytes_per_pixel)
        pitch = fbinfo->width * bytes_per_pixel;

    if(fbinfo->depth == 32) {
        /*packed scan-out: blit through graph_blt so the arch kernels
            (NEON/SSE streaming copies) do the work */
        graph_t dst;
        uint32_t* base = (uint32_t*)((uint8_t*)(ewokos_addr_t)fbinfo->pointer +
                fbinfo->yoffset * pitch + fbinfo->xoffset * 4);
        graph_init(&dst, base, fbinfo->width, fbinfo->height);
        graph_blt((graph_t*)g, r->x, r->y, r->w, r->h,
                &dst, fbinfo->xoffset + r->x, fbinfo->yoffset + r->y, r->w, r->h);
        return (uint32_t)r->w * (uint32_t)r->h * 4;
    }

    /*16bpp scan-out: graph_blt only handles ARGB, convert pixel by pixel */
    uint8_t* dst = (uint8_t*)(ewokos_addr_t)fbinfo->pointer +
            (fbinfo->yoffset + r->y) * pitch +
            (fbinfo->xoffset + r->x) * bytes_per_pixel;
    const uint32_t* src = g->buffer + r->y * g->w + r->x;
    for(int32_t y = 0; y < r->h; y++) {
        uint16_t* d = (uint16_t*)dst;
        for(int32_t x = 0; x < r->w; x++) {
            uint32_t s = src[x];
            d[x] = (uint16_t)((((s >> 16) & 0xff) >> 3) << 11 |
                    (((s >> 8) & 0xff) >> 2) << 5 |
                    ((s & 0xff) >> 3));
        }
        dst += pitch;
        src += g->w;
    }
    return (uint32_t)r->w * (uint32_t)r->h * 2;
}

/*rotate a single client-space rect straight into the scan-out. Mirrors the
  exact mapping of fbdisplayd_rotate_to, but only for the damaged region:
  the rect is extracted into a packed graph, rotated with graph_rotate_to,
  then copied into the panel-space rect it lands on. g is the client
  (pre-rotation) frame; r is in client coordinates. Returns 0
  (=> full-frame fallback) on any surprise.*/
static uint32_t fbdisplayd_rotate_rect_to(const disp_info_t* fbi, const graph_t* g,
        const grect_t* r, int rotate) {
    if(fbi == NULL || g == NULL || g->buffer == NULL || r == NULL)
        return 0;
    if(fbi->pointer == 0 || fbi->depth != 32)
        return 0;
    if(rotate != G_ROTATE_90 && rotate != G_ROTATE_270 && rotate != G_ROTATE_180)
        return 0;

    int32_t sw = g->w, sh = g->h;
    int32_t rx0 = r->x < 0 ? 0 : r->x;
    int32_t ry0 = r->y < 0 ? 0 : r->y;
    int32_t rx1 = r->x + r->w; if(rx1 > sw) rx1 = sw;
    int32_t ry1 = r->y + r->h; if(ry1 > sh) ry1 = sh;
    if(rx0 >= rx1 || ry0 >= ry1)
        return 0;
    int32_t rw = rx1 - rx0, rh = ry1 - ry0;

    /*rotated rect dims */
    int32_t dw = (rotate == G_ROTATE_180) ? rw : rh;
    int32_t dh = (rotate == G_ROTATE_180) ? rh : rw;

    graph_t* src_cache = ensure_graph(&_rect_g, rw, rh);
    graph_t* rot_cache = ensure_graph(&_rotate_g, dw, dh);
    if(src_cache == NULL || rot_cache == NULL)
        return 0;

    /*extract the damaged rect into a packed graph */
    graph_blt((graph_t*)g, rx0, ry0, rw, rh, src_cache, 0, 0, rw, rh);
    graph_rotate_to(src_cache, rot_cache, rotate);

    /*where the rotated rect lands in panel space */
    int32_t dy0, dx0;
    if(rotate == G_ROTATE_90) {         /* fb rows [rx0,rx1), cols [sh-ry1, sh-ry0) */
        dy0 = rx0;      dx0 = sh - ry1;
    }
    else if(rotate == G_ROTATE_270) {   /* fb rows [sw-rx1, sw-rx0), cols [ry0, ry1) */
        dy0 = sw - rx1; dx0 = ry0;
    }
    else {                              /* fb rows [sh-ry1, sh-ry0), cols [sw-rx1, sw-rx0) */
        dy0 = sh - ry1; dx0 = sw - rx1;
    }

    uint32_t pitch = disp_pitch32(fbi);
    uint8_t* base = (uint8_t*)(ewokos_addr_t)fbi->pointer +
            fbi->yoffset * pitch + fbi->xoffset * 4;
    graph_t dst;
    graph_init(&dst, (uint32_t*)base, fbi->width, fbi->height);
    graph_blt(rot_cache, 0, 0, dw, dh, &dst, dx0, dy0, dw, dh);
    return (uint32_t)rw * (uint32_t)rh * 4;
}

static inline int is_zoomed(void) {
    return (_zoom > 0.0 && _zoom != 8.0 && _zoom != 1.0);
}

static uint32_t flush(const disp_info_t* fbinfo, const void* buf, uint32_t size, int rotate) {
    if(fbinfo->depth != 32 && fbinfo->depth != 16)
        return 0;

    int zoomed = is_zoomed();
    graph_t g;
    if(rotate == G_ROTATE_270 || rotate == G_ROTATE_90)
        graph_init(&g, buf, _zheight, _zwidth);
    else
        graph_init(&g, buf, _zwidth, _zheight);

    /* fast path: driver-side rotation via fbdisplayd_rotate_to(), which
     * rotates into a cacheable intermediate and copies to the NC scan-out
     * in one pass. If the driver declines (returns 0), fall through to
     * the generic path below which does the same two-step rotation+flush
     * but through the library's own _rotate_g cache and driver flush(). */
    if(rotate != G_ROTATE_0 && !zoomed && _fbdisplayd->flush_rotate != NULL) {
        uint32_t res = _fbdisplayd->flush_rotate(fbinfo, &g, rotate);
        if(res > 0)
            return res;
    }

    graph_t* tmp_g = &g;
    if(rotate == G_ROTATE_90 || rotate == G_ROTATE_270 || rotate == G_ROTATE_180) {
        graph_t* rg = ensure_graph(&_rotate_g, _zwidth, _zheight);
        if(rg != NULL) {
            graph_rotate_to(&g, rg, rotate);
            tmp_g = rg;
        }
    }

    graph_t* gzoom = NULL;
    if(zoomed) {
        gzoom = graph_new_shm(fbinfo->width, fbinfo->height);
        graph_scale_tof(tmp_g, gzoom, _zoom);
        tmp_g = gzoom;
    }

    uint32_t res = _fbdisplayd->flush(fbinfo, tmp_g);
    if(gzoom != NULL)
        graph_free(gzoom);
    return res;
}

static void init_graph(disp_shm_t* shm) {
    graph_t g;
    if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
        graph_init(&g, (const uint32_t*)shm->shm, _zheight, _zwidth);
    else
        graph_init(&g, (const uint32_t*)shm->shm, _zwidth, _zheight);

    if(_fbdisplayd->splash != NULL)
        _fbdisplayd->splash(&g, _logo);
    else
        default_splash(&g, _logo);
    flush(&_fbinfo, shm->shm, shm->size, _rotate);
}

static uint32_t _disp_shm_seq = 1;

/*xwm draws the desktop and the window frames straight into this buffer,
  but it runs in the user session, not as a child of fbdisplayd: an IPC_PRIVATE
  segment is family-only in the kernel, so xwm could not attach it and
  nothing got drawn. A public key lets everyone who knows the id map it,
  like the other shared graphs.
  The key is derived from the tag mixed with a sequence counter so repeated
  opens never collide with a still-attached segment; IPC_EXCL makes shmget
  fail on reuse, and the loop retries with the next sequence value. Keys of
  0 / IPC_PRIVATE are forced to a non-private value.
  IPC_CONTIG is an EwokOS-specific flag: the segment is backed by the
  kernel's reserved physically-contiguous slab instead of scattered pages,
  because display hardware (and DMA) needs a single contiguous physical
  buffer; creation fails strictly if the slab is unconfigured or exhausted.*/
static int32_t shm_new_segment(uint32_t tag, uint32_t sz, bool contig) {
    for(uint32_t i = 0; i < 4; i++) {
        uint32_t seq = _disp_shm_seq++;
        key_t key = (key_t)(0x47525030u + (((uint32_t)getpid() & 0xffffu) << 16) +
                (seq & 0xffffu));
        if(key == 0 || key == IPC_PRIVATE)
            key = (key_t)(seq | 1u);

        int32_t id = -1;
        if(contig)
            id = shmget(key, sz, 0666 | IPC_CREAT | IPC_EXCL | IPC_CONTIG);
        else
            id = shmget(key, sz, 0666 | IPC_CREAT | IPC_EXCL);
        if(id != -1)
            return id;
    }
    return -1;
}

static int disp_shm_init(disp_shm_t* shm) {
    memset(shm, 0, sizeof(disp_shm_t));
    uint32_t sz = _zwidth * _zheight * 4;

    bool contig = false;
    shm->shm_id = shm_new_segment(0x4642444d, sz, true); //pixels only
    if(shm->shm_id > 0)
        contig = true;
    else
        shm->shm_id = shm_new_segment(0x4642444d, sz, false); //pixels only

    if(shm->shm_id <= 0)
        return -1;

    shm->shm = shmat(shm->shm_id, 0, 0);
    if(shm->shm == (void*)-1) {
        /* never attached: IPC_RMID destroys the segment outright */
        shmctl(shm->shm_id, IPC_RMID, NULL);
        shm->shm = NULL;
        return -1;
    }
    shm->shm_contig = contig;
    memset(shm->shm, 0, sz);

    /*the ctrl block lives in its own small segment so the pixel shm stays
      exactly one framebuffer; clients get the id via DISPLAY_CNTL_GET_CTRL.*/
    shm->ctrl_id = shm_new_segment(0x46424443, sizeof(display_ctrl_t), false);
    if(shm->ctrl_id == -1) {
        /* attached by us only: shmdt drops refs to 0 and the kernel
           frees the pixel segment */
        shmdt(shm->shm);
        shm->shm = NULL;
        return -1;
    }
    shm->ctrl = (display_ctrl_t*)shmat(shm->ctrl_id, 0, 0);
    if(shm->ctrl == (void*)-1) {
        shmctl(shm->ctrl_id, IPC_RMID, NULL); /* ctrl was never attached */
        shmdt(shm->shm);
        shm->shm = NULL;
        shm->ctrl = NULL;
        return -1;
    }
    memset(shm->ctrl, 0, sizeof(display_ctrl_t));

    shm->size = sz;
    init_graph(shm);
    return 0;
}

static void disp_get_info() {
    disp_info_t* info = _fbdisplayd->get_info();
    memcpy(&_fbinfo, info, sizeof(disp_info_t));
    _zwidth = _fbinfo.width / _zoom;
    _zheight = _fbinfo.height / _zoom;
}

static int disp_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)dev;
    (void)from_pid;
    (void)ret;
    (void)p;

    if(cmd == DISPLAY_DEV_CNTL_SET_INFO) { //set fb size and bpp
        int w = proto_read_int(in);
        int h = proto_read_int(in);
        int bpp = proto_read_int(in);
        if(_fbdisplayd->init(w, h, bpp) != 0)
            return -1;
        disp_get_info();
    }
    else if(cmd == DISPLAY_DEV_CNTL_GET_INFO) {
        if(_rotate == G_ROTATE_270 || _rotate == G_ROTATE_90)
            PF->addi(ret, _zheight)->addi(ret, _zwidth)->addi(ret, _fbinfo.depth);
        else
            PF->addi(ret, _zwidth)->addi(ret, _zheight)->addi(ret, _fbinfo.depth);
    }	
    return 0;
}

/*push only the rects the client declared dirty. Returns the bytes written,
  or -1 when the damage cannot be honoured and the whole frame is needed.*/

/*dirty-rect coalescing: every surviving rect is one g2d/blit dispatch with
  a fixed overhead (IPC, cache maintenance), so fewer larger rects beat
  many small ones. Two passes: (1) union every overlapping/touching pair
  until stable, (2) while more than max remain, merge the cheapest pair
  (smallest union-area growth), the same policy xserverd's pack uses.*/
static void dirty_union_to(grect_t* dst, const grect_t* src) {
    int32_t x0 = dst->x < src->x ? dst->x : src->x;
    int32_t y0 = dst->y < src->y ? dst->y : src->y;
    int32_t x1 = (dst->x + dst->w) > (src->x + src->w) ? (dst->x + dst->w) : (src->x + src->w);
    int32_t y1 = (dst->y + dst->h) > (src->y + src->h) ? (dst->y + dst->h) : (src->y + src->h);
    dst->x = x0; dst->y = y0; dst->w = x1 - x0; dst->h = y1 - y0;
}

static inline int dirty_overlap_or_touch(const grect_t* a, const grect_t* b) {
    return a->x <= b->x + b->w && b->x <= a->x + a->w &&
           a->y <= b->y + b->h && b->y <= a->y + a->h;
}

static uint32_t merge_dirty_rects(grect_t* rects, uint32_t num, uint32_t max) {
    int changed = 1;
    while(changed && num > 1) {
        changed = 0;
        for(uint32_t i = 0; i < num && !changed; i++) {
            for(uint32_t j = i + 1; j < num && !changed; j++) {
                if(dirty_overlap_or_touch(&rects[i], &rects[j])) {
                    dirty_union_to(&rects[i], &rects[j]);
                    num--;
                    if(j < num)
                        rects[j] = rects[num];
                    changed = 1;
                }
            }
        }
    }

    while(num > max) {
        uint32_t ba = 0, bb = 1;
        int64_t best = -1;
        for(uint32_t i = 0; i < num; i++) {
            for(uint32_t j = i + 1; j < num; j++) {
                grect_t u = rects[i];
                dirty_union_to(&u, &rects[j]);
                int64_t cost = (int64_t)u.w * u.h -
                        (int64_t)rects[i].w * rects[i].h -
                        (int64_t)rects[j].w * rects[j].h;
                if(best < 0 || cost < best) {
                    best = cost; ba = i; bb = j;
                }
            }
        }
        dirty_union_to(&rects[ba], &rects[bb]);
        num--;
        if(bb < num)
            rects[bb] = rects[num];
    }
    return num;
}

static int32_t flush_dirty(disp_shm_t* shm, const grect_t* rects, uint32_t num) {
    /*client (pre-rotation) geometry: for 90/270 the frame is transposed,
      matching flush()/GET_INFO. For rotate 0 this is _zwidth x _zheight,
      identical to the non-rotated path.*/
    uint32_t gw, gh;
    if(_rotate == G_ROTATE_90 || _rotate == G_ROTATE_270) {
        gw = _zheight; gh = _zwidth;
    }
    else {
        gw = _zwidth; gh = _zheight;
    }
    grect_t bounds = {0, 0, gw, gh};
    /*+1 slack: num <= DISPLAY_DIRTY_MAX (guaranteed by do_flush) and only
      decreases, but GCC's range analysis cannot see that through the
      merge_dirty_rects call and warns on merged[num - 1] without it */
    grect_t merged[DISPLAY_DIRTY_MAX + 1];
    uint32_t mnum = 0;
    graph_t g;
    memset(&g, 0, sizeof(graph_t));
    graph_init(&g, (const uint32_t*)shm->shm, gw, gh);

    /*clip to the frame first, then coalesce into at most DISPLAY_DIRTY_MAX
      dispatches (do_flush already guarantees num <= DISPLAY_DIRTY_MAX) */
    for(uint32_t i = 0; i < num; i++) {
        grect_t r = rects[i];
        if(grect_insect(&bounds, &r) && mnum < DISPLAY_DIRTY_MAX)
            merged[mnum++] = r;
    }
    if(mnum == 0)
        return 0;
    mnum = merge_dirty_rects(merged, mnum, DISPLAY_DIRTY_MAX);

    /*the merged region is most of the frame anyway: one sequential
      full-frame push beats N separate dispatches with their fixed
      per-request overhead */
    {
        int64_t area = 0;
        for(uint32_t i = 0; i < mnum; i++)
            area += (int64_t)merged[i].w * merged[i].h;
        if(area * 2 >= (int64_t)gw * gh)
            return -1;
    }

    int32_t res = 0;
    for(uint32_t i = 0; i < mnum; i++) {
        grect_t r = merged[i];
        uint32_t n = (_rotate == G_ROTATE_0)
                ? _flush_rect(&_fbinfo, &g, &r)
                : fbdisplayd_rotate_rect_to(&_fbinfo, &g, &r, _rotate);
        if(n == 0) //hook refused this geometry
            return -1;
        res += (int32_t)n;
    }
    return res;
}

static int32_t do_flush(disp_shm_t* shm) {
    uint8_t* buf = (uint8_t*)shm->shm;
    if(buf == NULL || shm->ctrl == NULL)
        return -1;

    uint32_t size = shm->size;
    display_ctrl_t* ctrl = shm->ctrl;
    uint32_t num = ctrl->dirty_num;
    grect_t rects[DISPLAY_DIRTY_MAX];
    if(num > DISPLAY_DIRTY_MAX)
        num = 0;
    if(num > 0)
        memcpy(rects, ctrl->dirty, num * sizeof(grect_t));
    ctrl->dirty_num = 0; //consumed, next flush is full unless declared again

    ctrl->busy = 1;
    int32_t res = -1;
    /*dirty-rect flushing: rotate 0 needs the driver's rect hook; a rotated
      panel is handled in-library, but only when it uses the generic
      fbdisplayd_rotate_to (so the rect rotate matches its full-frame model).*/
    int dirty_ok = (num > 0) && !is_zoomed();
    if(dirty_ok) {
        if(_rotate == G_ROTATE_0)
            dirty_ok = (_flush_rect != NULL);
        else
            dirty_ok = (_fbdisplayd->flush_rotate == fbdisplayd_rotate_to) &&
                    (_fbinfo.depth == 32);
    }
    if(dirty_ok)
        res = flush_dirty(shm, rects, num);
    if(res < 0)
        res = flush(&_fbinfo, buf, size, _rotate);
    ctrl->busy = 0;
    return res;
}

/*return
0: error;
-1: resized;
>0: size flushed*/
static int do_disp_flush(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;

    disp_shm_t* shm = (disp_shm_t*)p;
    return do_flush(shm);
}

/*re-push the frame the client last drew. A driver that changes how pixels
  are pushed (e.g. a contrast LUT) needs this: without it the new setting
  only shows up whenever the client happens to redraw, which on a static
  screen may be never. Always a FULL frame: honouring the pending dirty
  rects would repaint a few patches with the new pixel pipeline and leave
  the rest of the screen as the old one drew it.*/
int fbdisplayd_refresh(void) {
    if(_cur_shm == NULL || _cur_shm->shm == NULL || _cur_shm->ctrl == NULL)
        return -1;

    display_ctrl_t* ctrl = _cur_shm->ctrl;
    ctrl->dirty_num = 0;
    ctrl->busy = 1;
    uint32_t res = flush(&_fbinfo, _cur_shm->shm, _cur_shm->size, _rotate);
    ctrl->busy = 0;
    return res > 0 ? 0 : -1;
}

static int32_t disp_shm(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info, uint8_t* contig, int* size, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    disp_shm_t* shm = (disp_shm_t*)p;
    *size = shm->size;
    *contig = shm->shm_contig;
    return shm->shm_id;
}

static void read_config(const char* conf_file, uint32_t index, uint32_t* w, uint32_t* h, uint8_t* dep, int32_t* rotate, float* zoom) {
    char cfile[128] = {0};
    if(conf_file == NULL || conf_file[0] == 0) {
        sprintf(cfile, "/etc/display.json");
        if(index > 1)
            sprintf(cfile, "/etc/display.%d.json", index-1);
    }
    else
        sprintf(cfile, "%s", conf_file);
    slog("read_config: %s\n", cfile);

    json_var_t *conf_var = json_parse_file(cfile);	

    const char* v = json_get_str_def(conf_var, "logo", "/usr/system/images/logos/ewokos.png");
    if(v[0] != 0) 
        strncpy(_logo, v, 255);

    *zoom = json_get_float_def(conf_var, "zoom", 1.0);
    *w = json_get_int_def(conf_var, "width", 0);
    *h = json_get_int_def(conf_var, "height", 0);
    *dep = json_get_int_def(conf_var, "depth", 32);
    *rotate = json_get_int_def(conf_var, "rotate", 0);

    if(*zoom <= 0)
        *zoom = 1.0;
    else if(*zoom > 8.0)
        *zoom = 8.0;

    if(conf_var != NULL)
        json_var_unref(conf_var);
}


static int disp_dev_read(vdevice_t* dev, int fd, int from_pid, fsinfo_t* info,
        void* buf, int size, int offset, void* p) {
    (void)dev;
    (void)fd;
    (void)from_pid;
    (void)info;
    (void)offset;
    (void)p;

    if(_fbdisplayd->read == NULL)
        return 0;
    return _fbdisplayd->read(buf, size);
}

static char* disp_dev_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    (void)dev;
    (void)p;

    if(_dev_cmd == NULL)
        return NULL;
    return _dev_cmd(from_pid, argc, argv);
}

int fbdisplayd_run(fbdisplayd_t* fbdisplayd, const char* mnt_name,
        uint32_t def_w, uint32_t def_h, const char* conf_file, uint32_t display_index) {
    _fbdisplayd = fbdisplayd;
    uint32_t w = def_w, h = def_h;
    _zoom = 1.0;
    uint8_t dep = 32;
    _rotate = G_ROTATE_0;

    int32_t index = displayman_add_dev("/dev/displayman", mnt_name, display_index);
    if(index < 0)
        return -1;
    read_config(conf_file, index, &w, &h, &dep, &_rotate, &_zoom);

    disp_shm_t shm;
    shm.shm = NULL;
    if(fbdisplayd->init(w, h, dep) != 0)
        return -1;
    disp_get_info();
    
    if(disp_shm_init(&shm) != 0)
        return -1;

    vdevice_t dev;
    memset(&dev, 0, sizeof(vdevice_t));
    strcpy(dev.desc, "display");
    dev.shm = disp_shm;
    dev.flush = do_disp_flush;
    dev.fcntl = disp_fcntl;
    dev.dev_cntl = disp_dev_cntl;
    dev.read = disp_dev_read;
    dev.cmd = disp_dev_cmd;

    dev.extra_data = &shm;
    _cur_shm = &shm;

    device_run(&dev, mnt_name, FS_TYPE_CHAR, 0666, false);
    shmdt(shm.shm);
    if(shm.ctrl != NULL && shm.ctrl != (void*)-1)
        shmdt(shm.ctrl);
    return 0;
}
