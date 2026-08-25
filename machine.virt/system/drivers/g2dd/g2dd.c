#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <ewoksys/proto.h>
#include <ewoksys/vdevice.h>
#include <bsp/bsp_g2d.h>
#include <tinyjson/tinyjson.h>
#include <g2dclient/g2dclient.h>

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* buffer;
} g2d_surface_t;

typedef struct {
    uint32_t* buffer;
    uint32_t width;
    uint32_t height;
    uint32_t* owned_buffer;
    void* shm_ptr;
} g2d_import_t;

typedef struct {
    g2d_surface_t dst;
    uint32_t clear_color;
} g2d_state_t;

static int32_t g2d_surface_init(g2d_surface_t* surface, uint32_t width, uint32_t height) {
    if (surface == NULL || width == 0 || height == 0)
        return -1;

    memset(surface, 0, sizeof(*surface));
    surface->buffer = (uint32_t*)malloc((size_t)width * height * sizeof(uint32_t));
    if (surface->buffer == NULL)
        return -1;

    surface->width = width;
    surface->height = height;
    return 0;
}

static void g2d_surface_release(g2d_surface_t* surface) {
    if (surface == NULL)
        return;
    if (surface->buffer != NULL) {
        free(surface->buffer);
        surface->buffer = NULL;
    }
    surface->width = 0;
    surface->height = 0;
}

static int32_t g2d_surface_clear(g2d_surface_t* surface, uint32_t color) {
    if (surface == NULL || surface->buffer == NULL)
        return -1;
    bsp_g2d_fill(surface->buffer, (int32_t)surface->width, (int32_t)surface->height,
            0, 0, (int32_t)surface->width, (int32_t)surface->height, color);
    return 0;
}

static int32_t g2d_surface_fill_rect(g2d_surface_t* surface, const g2d_fill_req_t* req) {
    if (surface == NULL || surface->buffer == NULL || req == NULL)
        return -1;
    bsp_g2d_fill(surface->buffer, (int32_t)surface->width, (int32_t)surface->height,
            req->rect.x, req->rect.y, req->rect.w, req->rect.h, req->color);
    return 0;
}

static int32_t g2d_norm_degree(int32_t degree) {
    return ((degree % 360) + 360) % 360;
}

static int32_t g2d_surface_rotate(g2d_surface_t* surface, int32_t degree) {
    uint32_t* rotated;
    int32_t new_w;
    int32_t new_h;

    if (surface == NULL || surface->buffer == NULL)
        return -1;

    degree = g2d_norm_degree(degree);
    if (degree == 0)
        return 0;

    if (degree == 180) {
        /* bsp_g2d_rotate supports 180 in place */
        bsp_g2d_rotate(surface->buffer, (int32_t)surface->width, (int32_t)surface->height,
                surface->buffer, (int32_t)surface->width, (int32_t)surface->height, 180);
        return 0;
    }

    /* any other angle changes the surface dimensions (90/270 swap them,
       other angles grow to the rotated bounding box), need a new buffer */
    bsp_g2d_rotated_size((int32_t)surface->width, (int32_t)surface->height,
            degree, &new_w, &new_h);
    if (new_w <= 0 || new_h <= 0)
        return -1;
    rotated = (uint32_t*)malloc((size_t)new_w * new_h * sizeof(uint32_t));
    if (rotated == NULL)
        return -1;
    bsp_g2d_rotate(surface->buffer, (int32_t)surface->width, (int32_t)surface->height,
            rotated, new_w, new_h, degree);
    free(surface->buffer);
    surface->buffer = rotated;
    surface->width = (uint32_t)new_w;
    surface->height = (uint32_t)new_h;
    return 0;
}

static int32_t g2d_surface_scale_to(g2d_surface_t* surface, uint32_t width, uint32_t height) {
    uint32_t* scaled;

    if (surface == NULL || surface->buffer == NULL || width == 0 || height == 0)
        return -1;
    if (width == surface->width && height == surface->height)
        return 0;

    scaled = (uint32_t*)malloc((size_t)width * height * sizeof(uint32_t));
    if (scaled == NULL)
        return -1;
    bsp_g2d_scale_to(surface->buffer, (int32_t)surface->width, (int32_t)surface->height,
            scaled, (int32_t)width, (int32_t)height);
    free(surface->buffer);
    surface->buffer = scaled;
    surface->width = width;
    surface->height = height;
    return 0;
}

static int32_t g2d_surface_render(g2d_surface_t* dst,
        uint32_t* src_buf, int32_t src_w, int32_t src_h,
        int32_t sx, int32_t sy, int32_t sw, int32_t sh,
        const g2d_blit_req_t* req, uint8_t use_alpha) {
    if (dst == NULL || dst->buffer == NULL || src_buf == NULL || req == NULL)
        return -1;
    if (req->dw <= 0 || req->dh <= 0)
        return -1;

    if (use_alpha != 0) {
        bsp_g2d_blt_alpha(src_buf, src_w, src_h, sx, sy, sw, sh,
                dst->buffer, (int32_t)dst->width, (int32_t)dst->height,
                req->dx, req->dy, req->dw, req->dh, req->alpha);
    }
    else {
        bsp_g2d_blt(src_buf, src_w, src_h, sx, sy, sw, sh,
                dst->buffer, (int32_t)dst->width, (int32_t)dst->height,
                req->dx, req->dy, req->dw, req->dh);
    }
    return 0;
}

static int32_t g2d_surface_blit(g2d_surface_t* dst, const g2d_import_t* src,
        const g2d_blit_req_t* req, uint8_t use_alpha) {
    uint32_t* cropped;
    uint32_t* rotated;
    int32_t degree;
    int32_t rw;
    int32_t rh;
    int32_t ret;

    if (dst == NULL || dst->buffer == NULL || src == NULL ||
            src->buffer == NULL || req == NULL)
        return -1;
    if (req->sx < 0 || req->sy < 0 || req->sw <= 0 || req->sh <= 0)
        return -1;
    if (req->sx + req->sw > (int32_t)src->width ||
            req->sy + req->sh > (int32_t)src->height)
        return -1;

    /* rotate is clockwise degrees, normalized to [0, 360) */
    degree = g2d_norm_degree(req->rotate);

    /* no rotation: blt scales and clips the crop rect directly */
    if (degree == 0) {
        return g2d_surface_render(dst, src->buffer,
                (int32_t)src->width, (int32_t)src->height,
                req->sx, req->sy, req->sw, req->sh, req, use_alpha);
    }

    /* rotated path: crop into a temp surface, then rotate into another */
    cropped = (uint32_t*)malloc((size_t)req->sw * req->sh * sizeof(uint32_t));
    if (cropped == NULL)
        return -1;
    bsp_g2d_blt(src->buffer, (int32_t)src->width, (int32_t)src->height,
            req->sx, req->sy, req->sw, req->sh,
            cropped, req->sw, req->sh, 0, 0, req->sw, req->sh);

    bsp_g2d_rotated_size(req->sw, req->sh, degree, &rw, &rh);
    if (rw <= 0 || rh <= 0) {
        free(cropped);
        return -1;
    }
    rotated = (uint32_t*)malloc((size_t)rw * rh * sizeof(uint32_t));
    if (rotated == NULL) {
        free(cropped);
        return -1;
    }
    bsp_g2d_rotate(cropped, req->sw, req->sh, rotated, rw, rh, degree);
    free(cropped);

    ret = g2d_surface_render(dst, rotated, rw, rh,
            0, 0, rw, rh, req, use_alpha);
    free(rotated);
    return ret;
}

static void g2d_import_init(g2d_import_t* import) {
    if (import == NULL)
        return;
    memset(import, 0, sizeof(*import));
}

static void g2d_import_release(g2d_import_t* import) {
    if (import == NULL)
        return;
    if (import->owned_buffer != NULL)
        free(import->owned_buffer);
    if (import->shm_ptr != NULL)
        shmdt(import->shm_ptr);
    memset(import, 0, sizeof(*import));
}

static int32_t g2d_import_from_shm(g2d_import_t* import, const g2d_blit_req_t* req) {
    uint8_t* shm;
    uint32_t stride;
    uint32_t min_size;
    uint32_t y;
    uint32_t* packed;

    if (import == NULL || req == NULL)
        return -1;
    if (req->src_shm_id < 0 || req->src_w == 0 || req->src_h == 0 || req->src_format != G2D_FMT_ARGB8888)
        return -1;

    stride = req->src_stride;
    if (stride == 0)
        stride = req->src_w * 4;
    if (stride < req->src_w * 4)
        return -1;

    min_size = stride * req->src_h;
    if (req->src_size < min_size)
        return -1;

    shm = (uint8_t*)shmat(req->src_shm_id, 0, 0);
    if (shm == (void*)-1)
        return -1;

    g2d_import_init(import);
    import->shm_ptr = shm;
    if (stride == req->src_w * 4) {
        import->buffer = (uint32_t*)shm;
        import->width = req->src_w;
        import->height = req->src_h;
        return 0;
    }

    packed = (uint32_t*)malloc((size_t)req->src_w * req->src_h * sizeof(uint32_t));
    if (packed == NULL) {
        g2d_import_release(import);
        return -1;
    }

    for (y = 0; y < req->src_h; ++y) {
        memcpy(((uint8_t*)packed) + (size_t)y * req->src_w * 4,
                shm + (size_t)y * stride,
                (size_t)req->src_w * 4);
    }
    import->owned_buffer = packed;
    import->buffer = packed;
    import->width = req->src_w;
    import->height = req->src_h;
    return 0;
}

static int32_t g2d_state_init(g2d_state_t* state, uint32_t width, uint32_t height, uint32_t dep) {
    (void)dep;
    if (state == NULL)
        return -1;

    if (bsp_g2d_init() != 0)
        return -1;

    memset(state, 0, sizeof(*state));
    if (g2d_surface_init(&state->dst, width, height) != 0)
        return -1;

    state->clear_color = 0xff000000u;
    return 0;
}

static void g2d_state_release(g2d_state_t* state) {
    if (state == NULL)
        return;
    g2d_surface_release(&state->dst);
}

static int32_t g2d_state_clear(g2d_state_t* state, uint32_t color) {
    if (state == NULL)
        return -1;
    if (g2d_surface_clear(&state->dst, color) != 0)
        return -1;

    state->clear_color = color;
    return 0;
}

static int32_t g2d_state_fill_rect(g2d_state_t* state, const g2d_fill_req_t* req) {
    if (state == NULL)
        return -1;
    return g2d_surface_fill_rect(&state->dst, req);
}

static int32_t g2d_state_blit(g2d_state_t* state, const g2d_blit_req_t* req, const g2d_import_t* src, uint8_t use_alpha) {
    if (state == NULL)
        return -1;
    return g2d_surface_blit(&state->dst, src, req, use_alpha);
}

static int32_t g2d_state_rotate(g2d_state_t* state, int32_t degree) {
    if (state == NULL)
        return -1;
    return g2d_surface_rotate(&state->dst, degree);
}

static int32_t g2d_state_scale_to(g2d_state_t* state, uint32_t width, uint32_t height) {
    if (state == NULL)
        return -1;
    return g2d_surface_scale_to(&state->dst, width, height);
}

static int32_t g2d_reply_info(proto_t* ret, const g2d_state_t* state) {
    g2d_info_t info;

    if (ret == NULL || state == NULL || state->dst.buffer == NULL)
        return -1;

    memset(&info, 0, sizeof(info));
    info.width = state->dst.width;
    info.height = state->dst.height;
    info.depth = 32;
    info.format = G2D_FMT_ARGB8888;
    info.backend = 0; /* software backend */
    PF->init(ret)->add(ret, &info, sizeof(info));
    return 0;
}

static int32_t g2dd_handle_get_info(proto_t* ret, g2d_state_t* state) {
    return g2d_reply_info(ret, state);
}

static int32_t g2dd_handle_clear(proto_t* in, g2d_state_t* state) {
    uint32_t color;

    if (in == NULL || state == NULL)
        return -1;
    if (proto_read_to(in, &color, sizeof(color)) != sizeof(color))
        return -1;
    return g2d_state_clear(state, color);
}

static int32_t g2dd_handle_fill_rect(proto_t* in, g2d_state_t* state) {
    g2d_fill_req_t req;

    if (in == NULL || state == NULL)
        return -1;
    if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
        return -1;
    return g2d_state_fill_rect(state, &req);
}

static int32_t g2dd_handle_blit(proto_t* in, g2d_state_t* state, uint8_t use_alpha) {
    g2d_blit_req_t req;
    g2d_import_t import;
    int32_t ret;

    if (in == NULL || state == NULL)
        return -1;
    if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
        return -1;

    g2d_import_init(&import);
    if (g2d_import_from_shm(&import, &req) != 0)
        return -1;

    ret = g2d_state_blit(state, &req, &import, use_alpha);
    g2d_import_release(&import);
    return ret;
}

static int32_t g2dd_handle_rotate(proto_t* in, g2d_state_t* state) {
    g2d_rotate_req_t req;

    if (in == NULL || state == NULL)
        return -1;
    if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
        return -1;
    return g2d_state_rotate(state, req.rotate);
}

static int32_t g2dd_handle_scale_to(proto_t* in, g2d_state_t* state) {
    g2d_scale_to_req_t req;

    if (in == NULL || state == NULL)
        return -1;
    if (proto_read_to(in, &req, sizeof(req)) != sizeof(req))
        return -1;
    return g2d_state_scale_to(state, req.width, req.height);
}

static char* g2d_strdup(const char* s) {
    size_t len;
    char* ret;

    if (s == NULL)
        return NULL;
    len = strlen(s);
    ret = (char*)malloc(len + 1);
    if (ret == NULL)
        return NULL;
    memcpy(ret, s, len + 1);
    return ret;
}

static char* g2d_cmd(vdevice_t* dev, int from_pid, int argc, char** argv, void* p) {
    (void)dev;
    (void)from_pid;
    g2d_state_t* state = (g2d_state_t*)p;

    if (argc <= 0 || argv == NULL || argv[0] == NULL || state == NULL || state->dst.buffer == NULL)
        return NULL;

    if (strcmp(argv[0], "info") == 0) {
        static char info[96];
        snprintf(info, sizeof(info), "%ux%u argb8888 via soft",
                state->dst.width, state->dst.height);
        return g2d_strdup(info);
    }
    if (strcmp(argv[0], "clear") == 0 && argc > 1) {
        uint32_t color = (uint32_t)strtoul(argv[1], NULL, 0);
        return g2d_state_clear(state, color) == 0 ? g2d_strdup("ok") : NULL;
    }
    return NULL;
}

static int g2d_dev_cntl(vdevice_t* dev, int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
    (void)dev;
    (void)from_pid;
    g2d_state_t* state = (g2d_state_t*)p;

    if (state == NULL)
        return -1;

    switch (cmd) {
    case G2D_DEV_CNTL_GET_INFO:
        return g2dd_handle_get_info(ret, state);
    case G2D_DEV_CNTL_CLEAR:
        return g2dd_handle_clear(in, state);
    case G2D_DEV_CNTL_FILL_RECT:
        return g2dd_handle_fill_rect(in, state);
    case G2D_DEV_CNTL_BLIT:
        return g2dd_handle_blit(in, state, 0);
    case G2D_DEV_CNTL_BLIT_ALPHA:
        return g2dd_handle_blit(in, state, 1);
    case G2D_DEV_CNTL_ROTATE:
        return g2dd_handle_rotate(in, state);
    case G2D_DEV_CNTL_SCALE_TO:
        return g2dd_handle_scale_to(in, state);
    default:
        return -1;
    }
}

static void read_config(const char* conf_file, uint32_t* w, uint32_t* h, uint32_t* dep) {
    json_var_t* conf_var;

    if (conf_file == NULL || conf_file[0] == 0)
        conf_file = "/etc/display.json";

    conf_var = json_parse_file(conf_file);
    *w = (uint32_t)json_get_int_def(conf_var, "width", 1024);
    *h = (uint32_t)json_get_int_def(conf_var, "height", 768);
    *dep = (uint32_t)json_get_int_def(conf_var, "depth", 32);
    if (conf_var != NULL)
        json_var_unref(conf_var);
}

static int doargs(int argc, char* argv[], const char** conf_file) {
    int c = 0;

    while (c != -1) {
        c = getopt(argc, argv, "c:");
        if (c == -1)
            break;
        switch (c) {
        case 'c':
            *conf_file = optarg;
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char** argv) {
    const char* conf_file = "/etc/display.json";
    const char* mnt_point;
    g2d_state_t state;
    vdevice_t dev;
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t dep = 32;
    int opti;

    opti = doargs(argc, argv, &conf_file);
    mnt_point = (opti < argc && opti >= 0) ? argv[opti] : "/dev/g2d";

    read_config(conf_file, &w, &h, &dep);
    if (g2d_state_init(&state, w, h, dep) != 0)
        return -1;

    memset(&dev, 0, sizeof(dev));
    strcpy(dev.desc, "g2d");
    dev.dev_cntl = g2d_dev_cntl;
    dev.cmd = g2d_cmd;
    dev.extra_data = &state;

    if (device_run(&dev, mnt_point, FS_TYPE_CHAR, 0666) != 0) {
        g2d_state_release(&state);
        return -1;
    }

    g2d_state_release(&state);
    return 0;
}
