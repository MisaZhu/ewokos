#include <g2dclient/g2dclient.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <ewoksys/kernel_tic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* rotation in g2d_rotate_req_t / g2d_blit_req_t is clockwise degrees,
   any angle is accepted and normalized to [0, 360) by the driver. */

/* fixed point cos(45) used by the bsp backend: round(0.70710678 * 16384),
   duplicated here to predict the rotated bounding box size. */
#define ROT45_FP   11585
#define ROT45_BITS 14

static uint32_t rotated45_size(uint32_t w, uint32_t h) {
    uint64_t sum = (uint64_t)(w + h) * ROT45_FP;
    return (uint32_t)((sum + (1u << ROT45_BITS) - 1) >> ROT45_BITS);
}

typedef struct {
    int shm_id;
    uint32_t* pixels;
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
} shm_image_t;

static int shm_image_create(shm_image_t* img, key_t key, uint32_t width, uint32_t height) {
    if(img == NULL || width == 0 || height == 0)
        return -1;

    memset(img, 0, sizeof(*img));
    img->stride = width * 4;
    img->size = img->stride * height;
    img->width = width;
    img->height = height;
    img->shm_id = shmget(key, img->size, 0666 | IPC_CREAT);
    if(img->shm_id < 0)
        return -1;
    img->pixels = (uint32_t*)shmat(img->shm_id, 0, 0);
    if(img->pixels == (void*)-1) {
        img->pixels = NULL;
        img->shm_id = -1;
        return -1;
    }
    return 0;
}

static void shm_image_destroy(shm_image_t* img) {
    if(img == NULL)
        return;
    if(img->pixels != NULL)
        shmdt(img->pixels);
    memset(img, 0, sizeof(*img));
    img->shm_id = -1;
}

static uint32_t make_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void fill_checker(shm_image_t* img) {
    uint32_t x;
    uint32_t y;

    if(img == NULL || img->pixels == NULL)
        return;

    for(y = 0; y < img->height; y++) {
        for(x = 0; x < img->width; x++) {
            uint8_t r = (uint8_t)((x * 255) / img->width);
            uint8_t g = (uint8_t)((y * 255) / img->height);
            uint8_t b = ((x / 16 + y / 16) & 1) ? 0xd0 : 0x30;
            img->pixels[y * img->width + x] = make_color(0xff, r, g, b);
        }
    }
}

static void fill_alpha_circle(shm_image_t* img) {
    int32_t x;
    int32_t y;
    int32_t cx;
    int32_t cy;
    int32_t radius;
    int32_t radius2;

    if(img == NULL || img->pixels == NULL)
        return;

    cx = (int32_t)img->width / 2;
    cy = (int32_t)img->height / 2;
    radius = (int32_t)((img->width < img->height ? img->width : img->height) / 2) - 2;
    radius2 = radius * radius;

    for(y = 0; y < (int32_t)img->height; y++) {
        for(x = 0; x < (int32_t)img->width; x++) {
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            int32_t d2 = dx * dx + dy * dy;
            uint8_t alpha = 0;
            if(d2 < radius2) {
                alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
            }
            img->pixels[y * img->width + x] = make_color(alpha, 0xff, 0xe0, 0x20);
        }
    }
}

static int check_ret(const char* label, int ret, int expect_ok, int* failures) {
    int ok = expect_ok ? (ret == 0) : (ret != 0);

    if(!ok) {
        printf("FAIL %-22s ret=%d (expect %s)\n", label, ret, expect_ok ? "ok" : "error");
        (*failures)++;
        return -1;
    }
    printf("PASS %-22s ret=%d\n", label, ret);
    return 0;
}

/* fps benchmark: each bench renders frames through /dev/g2d for about
   1 second and reports completed frames per second. a frame is one
   full render pass of the tested op mix, all ops are synchronous IPC,
   so fps measures the full client->driver round trip. */

#define BENCH_USEC       (1000000u) /* ~1 second per bench */
#define BENCH_MAX_FRAMES 5000u

typedef struct {
    shm_image_t* opaque;
    shm_image_t* alpha;
    uint32_t w;
    uint32_t h;
    uint32_t seq; /* frame counter, varies positions/colors between frames */
} bench_ctx_t;

typedef int (*bench_frame_fn)(g2d_t* g2d, void* ctx);

static uint32_t bench_now_usec(void) {
    uint32_t low = 0;
    kernel_tic32(NULL, NULL, &low);
    return low;
}

static int bench_frame_clear(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    uint32_t color = 0xff000000u | ((ctx->seq * 0x010101u) & 0xffffffu);
    ctx->seq++;
    return g2d_clear(g2d, color);
}

static int bench_frame_fill(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_fill_req_t fill;
    int32_t maxx;
    int32_t maxy;
    uint32_t i;

    maxx = (int32_t)ctx->w - 64;
    maxy = (int32_t)ctx->h - 64;
    if(maxx < 0)
        maxx = 0;
    if(maxy < 0)
        maxy = 0;
    for(i = 0; i < 16; i++) {
        uint32_t seed = ctx->seq * 17u + i * 977u;
        int32_t x = maxx > 0 ? (int32_t)(seed % (uint32_t)maxx) : 0;
        int32_t y = maxy > 0 ? (int32_t)((seed >> 8) % (uint32_t)maxy) : 0;
        g2d_fill_req_init(&fill, g2d_rect(x, y, 64, 64),
                0xff000000u | ((seed * 31u) & 0xffffffu));
        if(g2d_fill_rect(g2d, &fill) != 0)
            return -1;
    }
    ctx->seq++;
    return 0;
}

static int bench_frame_blit(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_blit_req_t blit;
    int32_t dw;
    int32_t dh;
    int32_t maxx;
    int32_t maxy;
    int32_t x;
    int32_t y;
    int ret;

    dw = (int32_t)(ctx->w / 2);
    dh = (int32_t)(ctx->h / 2);
    maxx = (int32_t)ctx->w - dw;
    maxy = (int32_t)ctx->h - dh;
    x = maxx > 0 ? (int32_t)((ctx->seq * 37u) % (uint32_t)maxx) : 0;
    y = maxy > 0 ? (int32_t)((ctx->seq * 53u) % (uint32_t)maxy) : 0;
    g2d_blit_req_init(&blit,
            ctx->opaque->shm_id,
            ctx->opaque->size,
            ctx->opaque->width,
            ctx->opaque->height,
            ctx->opaque->stride,
            g2d_rect(0, 0, (int32_t)ctx->opaque->width, (int32_t)ctx->opaque->height),
            g2d_rect(x, y, dw, dh),
            0xff);
    ret = g2d_blit_shm(g2d, &blit);
    ctx->seq++;
    return ret;
}

static int bench_frame_blit_alpha(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_blit_req_t blit;
    int32_t dw;
    int32_t dh;
    int32_t maxx;
    int32_t maxy;
    int32_t x;
    int32_t y;
    int ret;

    dw = (int32_t)(ctx->w / 3);
    dh = (int32_t)(ctx->h / 3);
    maxx = (int32_t)ctx->w - dw;
    maxy = (int32_t)ctx->h - dh;
    x = maxx > 0 ? (int32_t)((ctx->seq * 41u) % (uint32_t)maxx) : 0;
    y = maxy > 0 ? (int32_t)((ctx->seq * 59u) % (uint32_t)maxy) : 0;
    g2d_blit_req_init(&blit,
            ctx->alpha->shm_id,
            ctx->alpha->size,
            ctx->alpha->width,
            ctx->alpha->height,
            ctx->alpha->stride,
            g2d_rect(0, 0, (int32_t)ctx->alpha->width, (int32_t)ctx->alpha->height),
            g2d_rect(x, y, dw, dh),
            0xff);
    ret = g2d_blit_alpha_shm(g2d, &blit);
    ctx->seq++;
    return ret;
}

/* rotate the destination surface back and forth: even frames rotate 90,
   odd frames rotate 270, so dimensions only oscillate between
   wxh and hxw instead of growing with every rotation. */
static int bench_frame_rotate(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_rotate_req_t rq;
    int ret;

    g2d_rotate_req_init(&rq, (ctx->seq % 2) == 0 ? 90 : 270);
    ret = g2d_rotate(g2d, &rq);
    ctx->seq++;
    return ret;
}

/* scale_to ping-pong between the original size and half size,
   keeping the surface dimensions bounded. */
static int bench_frame_scale(g2d_t* g2d, void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_scale_to_req_t sq;
    uint32_t tw;
    uint32_t th;
    int ret;

    if((ctx->seq % 2) == 0) {
        tw = ctx->w / 2;
        th = ctx->h / 2;
        if(tw == 0)
            tw = 1;
        if(th == 0)
            th = 1;
    }
    else {
        tw = ctx->w;
        th = ctx->h;
    }
    g2d_scale_to_req_init(&sq, tw, th);
    ret = g2d_scale_to(g2d, &sq);
    ctx->seq++;
    return ret;
}

/* one mixed frame = clear + 16 fills + opaque blit + alpha blit */
static int bench_frame_mixed(g2d_t* g2d, void* p) {
    if(bench_frame_clear(g2d, p) != 0)
        return -1;
    if(bench_frame_fill(g2d, p) != 0)
        return -1;
    if(bench_frame_blit(g2d, p) != 0)
        return -1;
    return bench_frame_blit_alpha(g2d, p);
}

static void bench_run(g2d_t* g2d, const char* label, bench_frame_fn fn,
        bench_ctx_t* ctx, int* failures) {
    uint32_t frames = 0;
    uint32_t elapsed = 0;
    uint32_t t0;

    t0 = bench_now_usec();
    while(elapsed < BENCH_USEC && frames < BENCH_MAX_FRAMES) {
        if(fn(g2d, ctx) != 0) {
            printf("FAIL %-22s bench frame %u failed\n", label, frames);
            (*failures)++;
            return;
        }
        frames++;
        elapsed = bench_now_usec() - t0;
    }
    if(elapsed == 0)
        elapsed = 1;
    printf("PERF %-22s %u frames %u us => %u fps (%u us/frame)\n",
            label, frames, elapsed,
            (uint32_t)(((uint64_t)frames * 1000000u) / elapsed),
            elapsed / frames);
}

/* the destination surface size is only observable through g2d_info,
   used to verify rotate/scale_to actually took effect. */
static int check_info(g2d_t* g2d, const char* label,
        uint32_t exp_w, uint32_t exp_h, int* failures) {
    g2d_info_t info;

    if(g2d_info(g2d, &info) != 0) {
        printf("FAIL %-22s g2d_info failed\n", label);
        (*failures)++;
        return -1;
    }
    if(info.width != exp_w || info.height != exp_h) {
        printf("FAIL %-22s %ux%u (expect %ux%u)\n",
                label, info.width, info.height, exp_w, exp_h);
        (*failures)++;
        return -1;
    }
    printf("PASS %-22s %ux%u\n", label, info.width, info.height);
    return 0;
}

int main(int argc, char** argv) {
    g2d_t g2d;
    g2d_info_t info;
    g2d_fill_req_t fill;
    g2d_blit_req_t blit;
    g2d_rotate_req_t rotate_req;
    g2d_scale_to_req_t scale_req;
    shm_image_t opaque_img;
    shm_image_t alpha_img;
    bench_ctx_t bench_ctx;
    g2d_rect_t src_rect;
    uint32_t bg_color = 0xff101820;
    uint32_t w0;
    uint32_t h0;
    uint32_t rot45;
    int32_t blit2_x;
    int32_t blit2_y;
    int32_t alpha2_x;
    int32_t alpha2_y;
    int failures = 0;
    int ret;

    (void)argc;
    (void)argv;

    if(g2d_open("/dev/g2d", &g2d) != 0) {
        printf("open /dev/g2d failed\n");
        return -1;
    }

    ret = g2d_info(&g2d, &info);
    if(ret != 0) {
        printf("g2d_info failed\n");
        g2d_close(&g2d);
        return -1;
    }
    w0 = info.width;
    h0 = info.height;
    printf("g2d: %ux%u depth=%u backend=%u\n",
            info.width, info.height, info.depth, info.backend);

    if(shm_image_create(&opaque_img, 0x47324410, 160, 120) != 0) {
        printf("create opaque shm failed\n");
        g2d_close(&g2d);
        return -1;
    }
    if(shm_image_create(&alpha_img, 0x47324411, 128, 128) != 0) {
        printf("create alpha shm failed\n");
        shm_image_destroy(&opaque_img);
        g2d_close(&g2d);
        return -1;
    }

    fill_checker(&opaque_img);
    fill_alpha_circle(&alpha_img);

    ret = g2d_clear(&g2d, bg_color);
    check_ret("clear", ret, 1, &failures);

    g2d_fill_req_init(&fill, g2d_rect(24, 24, 220, 120), 0xff204060);
    ret = g2d_fill_rect(&g2d, &fill);
    check_ret("fill_rect #1", ret, 1, &failures);

    g2d_fill_req_init(&fill, g2d_rect((int32_t)w0 - 180, 40, 140, 96), 0xff503040);
    ret = g2d_fill_rect(&g2d, &fill);
    check_ret("fill_rect #2", ret, 1, &failures);

    src_rect = g2d_rect(0, 0, (int32_t)opaque_img.width, (int32_t)opaque_img.height);
    g2d_blit_req_init(&blit,
            opaque_img.shm_id,
            opaque_img.size,
            opaque_img.width,
            opaque_img.height,
            opaque_img.stride,
            src_rect,
            g2d_rect(48, 72, (int32_t)opaque_img.width, (int32_t)opaque_img.height),
            0xff);
    ret = g2d_blit_shm(&g2d, &blit);
    check_ret("blit_opaque", ret, 1, &failures);

    blit2_x = (int32_t)w0 - 280;
    blit2_y = 96;
    src_rect = g2d_rect(20, 16, 80, 60);
    g2d_blit_req_init_ex(&blit,
            opaque_img.shm_id,
            opaque_img.size,
            opaque_img.width,
            opaque_img.height,
            opaque_img.stride,
            src_rect,
            g2d_rect(blit2_x, blit2_y, 180, 140),
            0xff,
            90);
    ret = g2d_blit_shm(&g2d, &blit);
    check_ret("blit_scale_rotate", ret, 1, &failures);

    src_rect = g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height);
    g2d_blit_req_init(&blit,
            alpha_img.shm_id,
            alpha_img.size,
            alpha_img.width,
            alpha_img.height,
            alpha_img.stride,
            src_rect,
            g2d_rect((int32_t)w0 / 2, (int32_t)h0 / 2 - 32,
                    (int32_t)alpha_img.width, (int32_t)alpha_img.height),
            0xff);
    ret = g2d_blit_alpha_shm(&g2d, &blit);
    check_ret("blit_alpha", ret, 1, &failures);

    alpha2_x = (int32_t)w0 / 2 - 220;
    alpha2_y = (int32_t)h0 / 2 + 40;
    src_rect = g2d_rect(16, 16, 96, 80);
    g2d_blit_req_init_ex(&blit,
            alpha_img.shm_id,
            alpha_img.size,
            alpha_img.width,
            alpha_img.height,
            alpha_img.stride,
            src_rect,
            g2d_rect(alpha2_x, alpha2_y, 200, 120),
            0xff,
            45);
    ret = g2d_blit_alpha_shm(&g2d, &blit);
    check_ret("blit_alpha_scale_rot", ret, 1, &failures);

    /* rotate the destination surface by any clockwise degree.
       90/270 swap dimensions, other angles grow to the bounding box. */
    g2d_rotate_req_init(&rotate_req, 90);
    ret = g2d_rotate(&g2d, &rotate_req);
    check_ret("rotate_90", ret, 1, &failures);
    check_info(&g2d, "rotate_90_size", h0, w0, &failures);

    g2d_rotate_req_init(&rotate_req, 180);
    ret = g2d_rotate(&g2d, &rotate_req);
    check_ret("rotate_180", ret, 1, &failures);
    check_info(&g2d, "rotate_180_size", h0, w0, &failures);

    /* negative degrees are normalized: -270 == 90, back to w0 x h0 */
    g2d_rotate_req_init(&rotate_req, -270);
    ret = g2d_rotate(&g2d, &rotate_req);
    check_ret("rotate_-270", ret, 1, &failures);
    check_info(&g2d, "rotate_-270_size", w0, h0, &failures);

    /* arbitrary angle: surface becomes the rotated bounding box */
    rot45 = rotated45_size(w0, h0);
    g2d_rotate_req_init(&rotate_req, 45);
    ret = g2d_rotate(&g2d, &rotate_req);
    check_ret("rotate_45", ret, 1, &failures);
    check_info(&g2d, "rotate_45_size", rot45, rot45, &failures);

    /* scale the destination surface back to the original size */
    g2d_scale_to_req_init(&scale_req, w0, h0);
    ret = g2d_scale_to(&g2d, &scale_req);
    check_ret("scale_to_restore", ret, 1, &failures);
    check_info(&g2d, "scale_restore_size", w0, h0, &failures);

    g2d_scale_to_req_init(&scale_req, 320, 240);
    ret = g2d_scale_to(&g2d, &scale_req);
    check_ret("scale_to_320x240", ret, 1, &failures);
    check_info(&g2d, "scale_to_size", 320, 240, &failures);

    g2d_scale_to_req_init(&scale_req, w0, h0);
    ret = g2d_scale_to(&g2d, &scale_req);
    check_ret("scale_to_restore2", ret, 1, &failures);
    check_info(&g2d, "scale_restore2_size", w0, h0, &failures);

    g2d_scale_to_req_init(&scale_req, 0, 0);
    ret = g2d_scale_to(&g2d, &scale_req);
    check_ret("scale_to_invalid", ret, 0, &failures);
    check_info(&g2d, "scale_invalid_size", w0, h0, &failures);

    /* fps benchmark on the restored w0 x h0 surface */
    bench_ctx.opaque = &opaque_img;
    bench_ctx.alpha = &alpha_img;
    bench_ctx.w = w0;
    bench_ctx.h = h0;
    bench_ctx.seq = 0;
    printf("--- fps benchmark ---\n");
    bench_run(&g2d, "clear", bench_frame_clear, &bench_ctx, &failures);
    bench_run(&g2d, "fill_rect x16", bench_frame_fill, &bench_ctx, &failures);
    bench_run(&g2d, "blit_opaque", bench_frame_blit, &bench_ctx, &failures);
    bench_run(&g2d, "blit_alpha", bench_frame_blit_alpha, &bench_ctx, &failures);
    bench_run(&g2d, "mixed_frame", bench_frame_mixed, &bench_ctx, &failures);

    bench_ctx.seq = 0;
    bench_run(&g2d, "rotate_90/270", bench_frame_rotate, &bench_ctx, &failures);
    /* odd frame count leaves the surface rotated 90: rotate back */
    if((bench_ctx.seq % 2) != 0) {
        g2d_rotate_req_init(&rotate_req, 270);
        ret = g2d_rotate(&g2d, &rotate_req);
        check_ret("rotate_bench_restore", ret, 1, &failures);
    }
    check_info(&g2d, "rotate_bench_size", w0, h0, &failures);

    bench_ctx.seq = 0;
    bench_run(&g2d, "scale_to ping-pong", bench_frame_scale, &bench_ctx, &failures);
    /* odd frame count leaves the surface scaled down: restore */
    if((bench_ctx.seq % 2) != 0) {
        g2d_scale_to_req_init(&scale_req, w0, h0);
        ret = g2d_scale_to(&g2d, &scale_req);
        check_ret("scale_bench_restore", ret, 1, &failures);
    }
    check_info(&g2d, "scale_bench_size", w0, h0, &failures);

    printf("g2dtest summary: %s (%d failure)\n", failures == 0 ? "PASS" : "FAIL", failures);
    usleep(50000);

    shm_image_destroy(&alpha_img);
    shm_image_destroy(&opaque_img);
    g2d_close(&g2d);
    return failures == 0 ? 0 : -1;
}
