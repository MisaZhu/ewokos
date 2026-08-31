#include <g2dclient/g2dclient.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define G2D_DEFAULT_DEV "/dev/g2d"
static char _g2d_dev[FS_FULL_NAME_MAX] = { 0 };
static int  _dev_pid = 0;

static int g2d_dev_pid(void) {
    if(_dev_pid > 0)
        return _dev_pid;

    if(_g2d_dev[0] != 0)
        _dev_pid = dev_get_pid(_g2d_dev);

    if(_dev_pid <= 0)
        _dev_pid = dev_get_pid(G2D_DEFAULT_DEV);
    return _dev_pid;
}

int has_g2d(void) {
    if(_dev_pid > 0) // got the dev pid, okay.
        return 0;

    if(_dev_pid < 0) //have already tried to get dev pid and failed!
        return -1;

    //should be the first time the get dev pid
    if(g2d_dev_pid() > 0)
        return 0; //yes
    return -1;
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
    if(ret == 0)
        ret = proto_read_int(&out);
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

/* shm canvases: keyed 0666 segments because the driver is an unrelated
   process and cannot map IPC_PRIVATE (family-only) segments. a fresh key
   per allocation, created with IPC_EXCL: without EXCL shmget() would
   return an existing keyed segment WITHOUT resizing it, and in a long
   lived process the key space wraps after 65536 allocations. the kernel
   frees the segment once both sides (client and driver) have detached. */
static uint32_t _g2d_shm_seq = 0;

int g2d_shm_alloc(uint32_t size, int* shm_id, uint32_t** pixels) {
    key_t key;
    int id;
    void* addr;

    if(size == 0 || shm_id == NULL || pixels == NULL)
        return -1;

    id = -1;
    for(int32_t i = 0; i < 16; i++) {
        key = (key_t)(0x47324430u + (((uint32_t)getpid() & 0xffffu) << 16) +
                (_g2d_shm_seq & 0xffffu));
        _g2d_shm_seq++;

        id = shmget(key, (int)size, 0666 | IPC_CREAT | IPC_EXCL);
        if(id > 0)
            break;
    }
    if(id <= 0)
        return -1;
    addr = shmat(id, 0, 0);
    if(addr == (void*)-1)
        return -1;
    *shm_id = id;
    *pixels = (uint32_t*)addr;
    return 0;
}

void g2d_shm_free(uint32_t* pixels) {
    if(pixels != NULL)
        shmdt(pixels);
}

int g2d_fill_rect(const g2d_fill_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_FILL_RECT, req, sizeof(*req));
}

int g2d_blit(const g2d_blit_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_BLIT, req, sizeof(*req));
}

int g2d_blit_alpha(const g2d_blit_req_t* req) {
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

int g2d_blit_to_phy(const g2d_blit_to_phy_req_t* req) {
    if(req == NULL)
        return -1;
    return g2d_send_struct(G2D_DEV_CNTL_BLIT_TO_PHY, req, sizeof(*req));
}
