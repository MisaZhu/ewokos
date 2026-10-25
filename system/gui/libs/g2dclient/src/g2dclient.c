#include <stdio.h>
#include <g2dclient/g2dclient.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ewoksys/shm.h>
#include <ewoksys/proc.h>
#include <ewoksys/klog.h>
#include <unistd.h>
#include <limits.h>

#define G2D_DEFAULT_DEV "/dev/g2d"
#define G2D_SHM_ALIGN 64u
static char _g2d_dev[FS_FULL_NAME_MAX] = { 0 };
static int  _dev_pid = 0;

void g2d_quiesce_on_fault(int result) {
    if(result != G2D_ERR_TIMEOUT && result != G2D_ERR_MMU_FAULT)
        return;
    /* A terminal job may still own GPU-visible buffers.  Never park the client:
       display callers retry on the CPU, and OpenCL retains the failed job's
       staging buffers before its CPU replay. */
    klog("g2d: terminal GPU result %d; using CPU fallback\n", result);
}

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
        return G2D_ERR_FAILED;

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
        return G2D_ERR_FAILED;

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
    return g2d_shm_alloc_phy(size, shm_id, pixels, NULL);
}

int g2d_shm_alloc_phy(uint32_t size, int* shm_id, uint32_t** pixels,
        ewokos_addr_t* phy) {
    key_t key;
    int id;
    void* addr;
    uint64_t alloc64;

    if(size == 0 || shm_id == NULL || pixels == NULL)
        return G2D_ERR_FAILED;

    *shm_id = -1;
    *pixels = NULL;
    if(phy != NULL)
        *phy = 0;

    alloc64 = ((uint64_t)size + G2D_SHM_ALIGN - 1u) &
            ~((uint64_t)G2D_SHM_ALIGN - 1u);
    if(alloc64 > (uint64_t)INT_MAX)
        return G2D_ERR_FAILED;

    id = -1;
    for(int32_t i = 0; i < 16; i++) {
        key = (key_t)(0x47324430u + (((uint32_t)getpid() & 0xffffu) << 16) +
                (_g2d_shm_seq & 0xffffu));
        _g2d_shm_seq++;

        /* GPU canvases must come from the physically-contiguous IPC slab;
           ordinary scattered shm cannot be represented by one VC4 address. */
        id = shmget(key, (int)alloc64,
                0666 | IPC_CREAT | IPC_EXCL | IPC_CONTIG);
        if(id > 0)
            break;
    }
    if(id <= 0)
        return G2D_ERR_FAILED;
    addr = shmat(id, 0, 0);
    if(addr == (void*)-1) {
        shmctl(id, IPC_RMID, NULL);
        return G2D_ERR_FAILED;
    }
    if((((ewokos_addr_t)addr) & (G2D_SHM_ALIGN - 1u)) != 0) {
        shmdt(addr);
        shmctl(id, IPC_RMID, NULL);
        return G2D_ERR_FAILED;
    }
    *shm_id = id;
    *pixels = (uint32_t*)addr;
    if(phy != NULL) {
        *phy = shm_contig_phy_addr(id, (ewokos_addr_t)addr);
        if(*phy == 0 || ((*phy) & (G2D_SHM_ALIGN - 1u)) != 0) {
            shmdt(addr);
            shmctl(id, IPC_RMID, NULL);
            *pixels = NULL;
            *shm_id = -1;
            return G2D_ERR_FAILED;
        }
    }
    return 0;
}

void g2d_shm_free(uint32_t* pixels) {
    if(pixels != NULL)
        shmdt(pixels);
}

int g2d_fill_rect(const g2d_fill_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_FILL_RECT, req, sizeof(*req));
}

int g2d_fill_triangle(const g2d_triangle_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_FILL_TRIANGLE, req, sizeof(*req));
}

int g2d_blit(const g2d_blit_req_t* req) {
    return g2d_send_struct(G2D_DEV_CNTL_BLIT, req, sizeof(*req));
}

int g2d_blit_alpha(const g2d_blit_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    if(req->alpha == 0)
        return G2D_OK;
    return g2d_send_struct(G2D_DEV_CNTL_BLIT_ALPHA, req, sizeof(*req));
}

int g2d_rotate(const g2d_rotate_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    /* Identity rotation is a whole-canvas copy. Older daemons reject zero
       in ROTATE, while BLIT already provides the exact overwrite operation. */
    if(req->rotate % 360 == 0) {
        g2d_blit_req_t copy;
        if(req->src.w != req->dst.w || req->src.h != req->dst.h ||
                req->src.w == 0 || req->src.h == 0 ||
                req->src.w > 0x7fffffffu || req->src.h > 0x7fffffffu)
            return G2D_ERR_FAILED;
        g2d_blit_req_init(&copy, req->dst, req->src,
                g2d_rect(0, 0, req->src.w, req->src.h),
                g2d_rect(0, 0, req->dst.w, req->dst.h), 255);
        return g2d_blit(&copy);
    }
    return g2d_send_struct(G2D_DEV_CNTL_ROTATE, req, sizeof(*req));
}

int g2d_scale_to(const g2d_scale_to_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_SCALE_TO, req, sizeof(*req));
}

/* the driver answers with a status int (G2D_OK, or
   G2D_ERR_NOT_SUPPORTED when the platform has no engine clock)
   followed by the engine clock in Hz, so this reads the payload
   directly instead of going through g2d_send_struct() */
int g2d_get_clock(uint32_t* hz) {
    proto_t in;
    proto_t out;
    int32_t dummy = 0;
    int ret;

    if(hz == NULL)
        return G2D_ERR_FAILED;
    *hz = 0;

    int pid = g2d_dev_pid();
    if(pid <= 0)
        return G2D_ERR_FAILED;

    PF->init(&in)->add(&in, &dummy, sizeof(dummy));
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, G2D_DEV_CNTL_GET_CLOCK, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);
        if(ret == 0)
            *hz = (uint32_t)proto_read_int(&out);
    }
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

/* the driver answers with a status int (G2D_OK, or G2D_ERR_NOT_SUPPORTED when
   the platform has no V3D engine) followed by the architecture version x10,
   so this reads the payload directly instead of going through
   g2d_send_struct() - the exact two-int reply pattern g2d_get_clock() uses. */
int g2d_get_version(uint32_t* ver) {
    proto_t in;
    proto_t out;
    int32_t dummy = 0;
    int ret;

    if(ver == NULL)
        return G2D_ERR_FAILED;
    *ver = 0;

    int pid = g2d_dev_pid();
    if(pid <= 0)
        return G2D_ERR_FAILED;

    PF->init(&in)->add(&in, &dummy, sizeof(dummy));
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, G2D_DEV_CNTL_GET_VERSION, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);
        if(ret == 0)
            *ver = (uint32_t)proto_read_int(&out);
    }
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

int g2d_blit_to_phy(const g2d_blit_to_phy_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_BLIT_TO_PHY, req, sizeof(*req));
}

int g2d_rotate_to_phy(const g2d_rotate_to_phy_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_ROTATE_TO_PHY, req, sizeof(*req));
}

int g2d_gaussian_blur(const g2d_gaussian_blur_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_GAUSSIAN_BLUR, req, sizeof(*req));
}

int g2d_pgl_clear(const g2d_pgl_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_PGL_CLEAR, req, sizeof(*req));
}

int g2d_pgl_draw(const g2d_pgl_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_PGL_DRAW, req, sizeof(*req));
}

int g2d_pgl_program(const g2d_pgl_program_req_t* req) {
    proto_t in;
    proto_t out;
    int pid;
    int ret;

    if(req == NULL)
        return G2D_ERR_FAILED;

    pid = g2d_dev_pid();
    if(pid <= 0)
        return G2D_ERR_FAILED;

    PF->init(&in)->add(&in, req, sizeof(*req));
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, G2D_DEV_CNTL_PGL_PROGRAM, &in, &out);
    /* The handle cannot ride the status int: do_dev_cntl drops a positive
       handler return and dev_cntl_by_pid normalises a positive status to -1.
       So a successful upload answers status 0 with the non-negative handle
       appended as the reply's SECOND int (the two-int pattern g2d_get_clock
       uses); a failure answers a negative G2D_ERR_* status with no payload. */
    if(ret == 0) {
        ret = proto_read_int(&out);        /* handler status (0 == accepted) */
        if(ret == 0)
            ret = proto_read_int(&out);    /* the program handle (>= 0) */
    }
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

/* OpenCL compute offload.  g2d_cl_program() uses the exact two-int reply
   pattern g2d_pgl_program() does (status int, then the non-negative program
   handle) because a handle cannot ride the status int; the raspi5 back end
   parses the same g2d_pgl_program_blob_t wire format, compute stage only. */
int g2d_cl_program(const g2d_cl_program_req_t* req) {
    proto_t in;
    proto_t out;
    int pid;
    int ret;

    if(req == NULL)
        return G2D_ERR_FAILED;

    pid = g2d_dev_pid();
    if(pid <= 0)
        return G2D_ERR_FAILED;

    PF->init(&in)->add(&in, req, sizeof(*req));
    PF->init(&out);
    ret = dev_cntl_by_pid(pid, G2D_DEV_CNTL_CL_PROGRAM, &in, &out);
    if(ret == 0) {
        ret = proto_read_int(&out);        /* handler status (0 == accepted) */
        if(ret == 0)
            ret = proto_read_int(&out);    /* the program handle (>= 0) */
    }
    PF->clear(&out);
    PF->clear(&in);
    return ret;
}

/* One NDRange launch: a plain single-int status round trip (no handle comes
   back), so it rides g2d_send_struct() like every other fire-and-forget op. */
int g2d_cl_dispatch(const g2d_cl_dispatch_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_CL_DISPATCH, req, sizeof(*req));
}

int g2d_cl_program_free(const g2d_cl_program_free_req_t* req) {
    if(req == NULL)
        return G2D_ERR_FAILED;
    return g2d_send_struct(G2D_DEV_CNTL_CL_PROGRAM_FREE, req, sizeof(*req));
}
