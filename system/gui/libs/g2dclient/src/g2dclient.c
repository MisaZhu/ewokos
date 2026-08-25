#include <g2dclient/g2dclient.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>

#define G2D_DEFAULT_DEV "/dev/g2d"
static char _g2d_dev[FS_FULL_NAME_MAX] = { 0 };
static int  _dev_pid = -1;

static int g2d_dev_pid(void) {
    if(_dev_pid > 0)
        return _dev_pid;

    if(_g2d_dev[0] != 0)
        _dev_pid = dev_get_pid(_g2d_dev);

    if(_dev_pid <= 0)
        _dev_pid = dev_get_pid(G2D_DEFAULT_DEV);
    return _dev_pid;
}

static int g2d_send_struct(int cmd, const void* data, uint32_t size) {
    proto_t in;
    proto_t out;
    int ret;

    int pid = g2d_dev_pid();
    if(pid <= 0 || data == NULL || size == 0)
        return -1;

    PF->init(&in)->add(&in, data, size);
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, cmd, &in, &out);
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

int g2d_set_dev(const char* dev) {
    if(dev == NULL || dev[0] == 0)
        return -1;

    memset(_g2d_dev, 0, FS_FULL_NAME_MAX);
    strncpy(_g2d_dev, dev, FS_FULL_NAME_MAX-1);
    return 0;
}

int g2d_info(g2d_info_t* info) {
    proto_t out;
    int ret;

    int pid = g2d_dev_pid();
    if(pid <= 0)
        return -1;

    memset(info, 0, sizeof(*info));
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, G2D_DEV_CNTL_GET_INFO, NULL, &out);
    if(ret == 0) {
        if(proto_read_to(&out, info, sizeof(*info)) != sizeof(*info))
            ret = -1;
    }
    PF->clear(&out);
    return ret;
}

int g2d_clear(uint32_t color) {
    return g2d_send_struct(G2D_DEV_CNTL_CLEAR, &color, sizeof(color));
}

int g2d_fill_rect(const g2d_fill_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_FILL_RECT, req, sizeof(*req));
}

int g2d_blit_shm(const g2d_blit_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_BLIT, req, sizeof(*req));
}

int g2d_blit_alpha_shm(const g2d_blit_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_BLIT_ALPHA, req, sizeof(*req));
}

int g2d_rotate(const g2d_rotate_req_t* req) {
    if(req == NULL)
        return -1;
    return g2d_send_struct(G2D_DEV_CNTL_ROTATE, req, sizeof(*req));
}

int g2d_scale_to(const g2d_scale_to_req_t* req) {
    if(req == NULL)
        return -1;
    return g2d_send_struct(G2D_DEV_CNTL_SCALE_TO, req, sizeof(*req));
}
