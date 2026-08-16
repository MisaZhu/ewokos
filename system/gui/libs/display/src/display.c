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
    display->dma_id = -1;
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
        shmdt(display->dma);
    }
    close(display->fd);
    return 0;
}

display_ctrl_t* display_ctrl(display_t* display) {
    if(display == NULL || display->dma == NULL || display->g == NULL)
        return NULL;
    uint32_t size = display->g->w * display->g->h * 4;
    return (display_ctrl_t*)(((uint8_t*)display->dma) + size);
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

graph_t* display_fetch_graph(display_t* display) {
    if(display == NULL || display->fd < 0)
        return NULL;

    int w, h, bpp;
    int32_t dma_id;
    uint8_t* dma;
    graph_t* g;

    if(display_info(display, &w, &h, &bpp) != 0 ||
            w <= 0 || h <= 0)
        return NULL;

    if(display->g != NULL && display->g->w == w && display->g->h == h)
        return display->g;

    if(display->g != NULL) {
        graph_free(display->g);
        shmdt(display->dma);
        display->g = NULL;
        display->dma = NULL;
    }

    dma_id = vfs_dma(display->fd, NULL);
    if(dma_id == -1) 
        return NULL;
    
    dma = shmat(dma_id, 0, 0);
    if(dma == (void*)-1) 
        return NULL;
    
    g = graph_new((uint32_t*)dma, w, h);
    display->dma = dma;
    display->dma_id = dma_id;
    display->g = g;
    return g;
}

#ifdef __cplusplus
}
#endif
