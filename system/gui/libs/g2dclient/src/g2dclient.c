#include <g2dclient/g2dclient.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>

static int g2d_valid(const g2d_t* g2d) {
    if(g2d == NULL || g2d->dev[0] == 0)
        return 0;
    return 1;
}

static int g2d_send_struct(g2d_t* g2d, int cmd, const void* data, uint32_t size) {
    proto_t in;
    proto_t out;
    int ret;

    if(!g2d_valid(g2d) || data == NULL || size == 0)
        return -1;

    PF->init(&in)->add(&in, data, size);
    /*
     * A reply proto has to be supplied even though these commands return no
     * data: dev_cntl() only propagates the driver's handler status when an
     * output proto is present. With a NULL reply it just returns the IPC
     * transport result, so a driver that rejected the request still looks
     * like success to the caller.
     */
    PF->init(&out);
    ret = dev_cntl(g2d->dev, cmd, &in, &out);
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

int g2d_open(const char* dev, g2d_t* g2d) {
    if(dev == NULL || g2d == NULL)
        return -1;
    memset(g2d, 0, sizeof(*g2d));
    strncpy(g2d->dev, dev, FS_FULL_NAME_MAX - 1);
    return 0;
}

int g2d_close(g2d_t* g2d) {
    if(g2d == NULL)
        return -1;
    memset(g2d, 0, sizeof(*g2d));
    return 0;
}

int g2d_info(g2d_t* g2d, g2d_info_t* info) {
    proto_t out;
    int ret;

    if(!g2d_valid(g2d) || info == NULL)
        return -1;

    memset(info, 0, sizeof(*info));
    PF->init(&out);
    ret = dev_cntl(g2d->dev, G2D_DEV_CNTL_GET_INFO, NULL, &out);
    if(ret == 0) {
        if(proto_read_to(&out, info, sizeof(*info)) != sizeof(*info))
            ret = -1;
    }
    PF->clear(&out);
    return ret;
}

int g2d_clear(g2d_t* g2d, uint32_t color) {
    return g2d_send_struct(g2d, G2D_DEV_CNTL_CLEAR, &color, sizeof(color));
}

int g2d_fill_rect(g2d_t* g2d, const g2d_fill_req_t* req) {
    return g2d_send_struct(g2d, G2D_DEV_CNTL_FILL_RECT, req, sizeof(*req));
}

int g2d_blit_shm(g2d_t* g2d, const g2d_blit_req_t* req) {
    return g2d_send_struct(g2d, G2D_DEV_CNTL_BLIT, req, sizeof(*req));
}

int g2d_blit_alpha_shm(g2d_t* g2d, const g2d_blit_req_t* req) {
    return g2d_send_struct(g2d, G2D_DEV_CNTL_BLIT_ALPHA, req, sizeof(*req));
}

int g2d_rotate(g2d_t* g2d, const g2d_rotate_req_t* req) {
    if(req == NULL)
        return -1;
    return g2d_send_struct(g2d, G2D_DEV_CNTL_ROTATE, req, sizeof(*req));
}

int g2d_scale_to(g2d_t* g2d, const g2d_scale_to_req_t* req) {
    if(req == NULL)
        return -1;
    return g2d_send_struct(g2d, G2D_DEV_CNTL_SCALE_TO, req, sizeof(*req));
}
