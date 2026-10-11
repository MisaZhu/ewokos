#include <display/display.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <ewoksys/syscall.h>
#include <ewoksys/vdevice.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/vfs.h>
#include <ewoksys/core.h>

#ifdef __cplusplus
extern "C" {
#endif

int display_open(const char *dev, int32_t disp_index, display_t* display) {
    if(display == NULL || dev == NULL || dev[0] == 0)
        return -1;

    memset(display, 0, sizeof(display_t));
    display->shm_id = -1;
    display->fd = open(dev, O_RDWR);
    if(display->fd < 0)
        return -1;
    display->display_index = disp_index;
    return 0;
}

int display_set(const char *dev, int w, int h, int bpp) {
    if(bpp != 16 && bpp != 32) {
        bpp = 32;
    }

    proto_t in;
    PF->format(&in, "i,i,i", (ewokos_addr_t)w, (ewokos_addr_t)h,
            (ewokos_addr_t)bpp);

    int res = dev_cntl(dev, DISPLAY_DEV_CNTL_SET_INFO, &in, NULL);
    PF->clear(&in);
    return res;
}

int display_dev_info(const char *dev, int *w, int *h, int *bpp) {
    proto_t out;
    PF->init(&out);
    if(dev_cntl(dev, DISPLAY_DEV_CNTL_GET_INFO, NULL, &out) != 0)
        return -1;

    if(w != NULL)
        *w = proto_read_int(&out);
    if(h != NULL)
        *h = proto_read_int(&out);
    if(bpp != NULL)
        *bpp = proto_read_int(&out);
    PF->clear(&out);
    return 0;
}

int display_info(display_t* display, int* w, int* h, int* bpp) {
    if(display == NULL || display->fd < 0)
        return -1;

    proto_t out;
    PF->init(&out);
    if(vfs_fcntl(display->fd, DISPLAY_CNTL_GET_INFO, NULL, &out) != 0) { //get display info
        PF->clear(&out);
        return -1;
    }
    *w = proto_read_int(&out);
    *h = proto_read_int(&out);
    *bpp = proto_read_int(&out);
    PF->clear(&out);
    return 0;
}

/*upper bound (in ~1ms parks) a blocking flush waits for the device to finish
  the previous presentation before pushing. Bounded so a daemon that died with
  busy latched cannot hang the compositor forever.*/
#define DISPLAY_FLUSH_BUSY_WAIT_MAX 200

int display_flush(display_t* display, bool waiting) {
    if(display == NULL || display->fd < 0)
        return -1;

    /*presentation-completion gate. ctrl->busy is held by the daemon for the
      whole time it pushes the previous frame to the physical panel (reading
      this framebuffer shm for scan-out, or streaming it over SPI). Flushing
      now would either race that read (a half-synced shm reaches the panel) or
      start a new presentation on top of an unfinished one, so we never push
      cache_g while the device is still drawing:
        - opportunistic flush (waiting==false, e.g. the cursor fast path):
          skip it. The frame is already complete in cache_g and is presented by
          the next flush, so intermediate frames simply coalesce (nothing lost).
        - blocking flush (waiting==true): wait for the device to go idle (with a
          bound, so a dead daemon cannot hang us), then present the full frame.*/
    if(display->ctrl != NULL) {
        volatile uint8_t* busy = &display->ctrl->busy;
        if(*busy != 0) {
            if(!waiting)
                return 0;
            uint32_t guard = DISPLAY_FLUSH_BUSY_WAIT_MAX;
            while(*busy != 0 && guard > 0) {
                usleep(1000);
                guard--;
            }
        }
    }

    /*sync the completed frame from the off-screen back buffer into the
      framebuffer shm, bounded by the declared dirty rects (or the whole frame
      when none). This is the single point where a fully-composited frame
      reaches the daemon/panel, so no partial drawing is ever scanned out.*/
    if(display->cache_g != NULL && display->g != NULL) {
        display_ctrl_t* ctrl = display->ctrl;
        if(ctrl != NULL && ctrl->dirty_num > 0 && ctrl->dirty_num <= DISPLAY_DIRTY_MAX) {
            for(uint8_t i = 0; i < ctrl->dirty_num; i++) {
                grect_t* r = &ctrl->dirty[i];
                graph_blt(display->cache_g, r->x, r->y, r->w, r->h,
                        display->g, r->x, r->y, r->w, r->h);
            }
        }
        else {
            graph_blt(display->cache_g, 0, 0, display->g->w, display->g->h,
                    display->g, 0, 0, display->g->w, display->g->h);
        }
    }

    return vfs_flush(display->fd, waiting);
}

int display_close(display_t* display) {
    if(display == NULL || display->fd < 0)
        return -1;
    if(display->g != NULL) {
        graph_free(display->g);
        shmdt(display->shm);
    }
    if(display->cache_g != NULL) {
        graph_free(display->cache_g); //also detaches its own shm backing
        display->cache_g = NULL;
    }
    if(display->ctrl != NULL)
        shmdt(display->ctrl);
    close(display->fd);
    return 0;
}

display_ctrl_t* display_ctrl(display_t* display) {
    if(display == NULL)
        return NULL;
    return display->ctrl;
}

bool display_busy(display_t* display) {
    if(display == NULL || display->fd < 0)
        return false;
    display_ctrl_t* ctrl = display_ctrl(display);
    if(ctrl == NULL)
        return false;
    return ctrl->busy != 0;
}

/*declare the damaged area of the next flush. rects == NULL or num == 0
  means the whole framebuffer. The daemon consumes and clears it on every
  flush, so it has to be set again for each partial frame.*/
int display_set_dirty(display_t* display, const grect_t* rects, uint32_t num) {
    display_ctrl_t* ctrl = display_ctrl(display);
    if(ctrl == NULL)
        return -1;

    if(rects == NULL || num == 0 || num > DISPLAY_DIRTY_MAX) {
        ctrl->dirty_num = 0; //too many rects, pushing everything is cheaper
        return 0;
    }

    memcpy(ctrl->dirty, rects, num * sizeof(grect_t));
    ctrl->dirty_num = (uint8_t)num;
    return 0;
}

/*ask the daemon for the shm id of the ctrl block and attach it. The ctrl
  segment is independent of the pixel dma, so it survives geometry changes
  without being re-mapped.*/
static int attach_ctrl(display_t* display) {
    if(display->ctrl != NULL)
        return 0;

    proto_t out;
    PF->init(&out);
    int32_t ctrl_id = -1;
    if(vfs_fcntl(display->fd, DISPLAY_CNTL_GET_CTRL, NULL, &out) == 0)
        ctrl_id = proto_read_int(&out);
    PF->clear(&out);
    if(ctrl_id <= 0)
        return -1;

    display_ctrl_t* ctrl = (display_ctrl_t*)shmat(ctrl_id, 0, 0);
    if(ctrl == (void*)-1)
        return -1;
    display->ctrl = ctrl;
    return 0;
}

graph_t* display_fetch_graph(display_t* display) {
    if(display == NULL || display->fd < 0)
        return NULL;

    int w, h, bpp;
    int32_t shm_id;
    uint8_t contig;
    uint8_t* shm;
    graph_t* g;

    if(display_info(display, &w, &h, &bpp) != 0 ||
            w <= 0 || h <= 0)
        return NULL;

    if(display->g != NULL && display->g->w == w && display->g->h == h)
        return (display->cache_g != NULL) ? display->cache_g : display->g;

    if(display->g != NULL) {
        graph_free(display->g);
        shmdt(display->shm);
        display->g = NULL;
        display->shm = NULL;
    }
    if(display->cache_g != NULL) {
        graph_free(display->cache_g); //also detaches its own shm backing
        display->cache_g = NULL;
    }

    shm_id = vfs_shm(display->fd, &contig, NULL);
    if(shm_id == -1) 
        return NULL;
    
    shm = shmat(shm_id, 0, 0);
    if(shm == (void*)-1) 
        return NULL;

    attach_ctrl(display); //best effort: without it flushes just go full-frame
    
    g = graph_new((uint32_t*)shm, w, h);
    g->shm_id = shm_id;
    g->shm_contig = (contig != 0);
    display->shm = shm;
    display->shm_id = shm_id;
    display->g = g;

    /*off-screen back buffer: every fill/alpha/blt of a frame lands here and is
      synced to the framebuffer once per flush, so a partially-composited frame
      is never scanned out. contig when a g2d device is present so the sync blit
      is hardware-accelerated. On allocation failure fall back to drawing
      straight into the framebuffer (today's behavior, no regression).*/
    display->cache_g = graph_new_shm(w, h);
    if(display->cache_g != NULL)
        return display->cache_g;
    return g;
}

#ifdef __cplusplus
}
#endif
