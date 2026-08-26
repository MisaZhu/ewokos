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

int display_flush(display_t* display, bool waiting) {
    if(display == NULL || display->fd < 0)
        return -1;
    return vfs_flush(display->fd, waiting);
}

int display_close(display_t* display) {
    if(display == NULL || display->fd < 0)
        return -1;
    if(display->g != NULL) {
        graph_free(display->g);
        shmdt(display->shm);
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
    uint8_t* shm;
    graph_t* g;

    if(display_info(display, &w, &h, &bpp) != 0 ||
            w <= 0 || h <= 0)
        return NULL;

    if(display->g != NULL && display->g->w == w && display->g->h == h)
        return display->g;

    if(display->g != NULL) {
        graph_free(display->g);
        shmdt(display->shm);
        display->g = NULL;
        display->shm = NULL;
    }

    shm_id = vfs_shm(display->fd, NULL);
    if(shm_id == -1) 
        return NULL;
    
    shm = shmat(shm_id, 0, 0);
    if(shm == (void*)-1) 
        return NULL;

    attach_ctrl(display); //best effort: without it flushes just go full-frame
    
    g = graph_new((uint32_t*)shm, w, h);
    g->shm_id = shm_id;
    g->shm_contig = true;
    display->shm = shm;
    display->shm_id = shm_id;
    display->g = g;
    return g;
}

#ifdef __cplusplus
}
#endif
