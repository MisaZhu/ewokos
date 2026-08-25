#include <g2dclient/g2dclient.h>
#include <graph/graph.h>
#include <ewoksys/kernel_tic.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* stateless g2d test: the canvases are shm segments owned by this
   process (allocated via graph_new_shm), so every result can be
   verified pixel by pixel.
   rotation in g2d_rotate_req_t / g2d_blit_req_t is clockwise degrees,
   any angle is accepted and normalized to [0, 360) by the driver. */

/* fixed point cos(45) used by the bsp backend: round(0.70710678 * 16384),
   duplicated here to predict the rotated bounding box size. */
#define ROT45_FP   11585
#define ROT45_BITS 14

static uint32_t rotated45_size(uint32_t w, uint32_t h) {
    uint64_t sum = (uint64_t)(w + h) * ROT45_FP;
    return (uint32_t)((sum + (1u << ROT45_BITS) - 1) >> ROT45_BITS);
}

#define CANVAS_W 480u
#define CANVAS_H 272u

static graph_t* canvas_create(uint32_t width, uint32_t height) {
    return graph_new_shm((int32_t)width, (int32_t)height);
}

static void canvas_free(graph_t* g) {
    graph_free(g);
}

static g2d_canvas_t img_canvas(const graph_t* g) {
    return g2d_canvas(g->shm_id, (uint32_t)g->w * (uint32_t)g->h * 4u,
            (uint32_t)g->w, (uint32_t)g->h);
}

static uint32_t make_color(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

static void img_clear(graph_t* g, uint32_t color) {
    uint32_t i;
    uint32_t n;

    if(g == NULL || g->buffer == NULL)
        return;
    n = (uint32_t)g->w * (uint32_t)g->h;
    for(i = 0; i < n; i++)
        g->buffer[i] = color;
}

/* deterministic pattern: color encodes the pixel position, so blits
   and rotations can be verified against the expected mapping */
static void fill_pattern(graph_t* g) {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;

    if(g == NULL || g->buffer == NULL)
        return;

    w = (uint32_t)g->w;
    h = (uint32_t)g->h;
    for(y = 0; y < h; y++) {
        for(x = 0; x < w; x++) {
            uint8_t r = (uint8_t)((x * 255) / w);
            uint8_t gc = (uint8_t)((y * 255) / h);
            uint8_t b = (uint8_t)((x + y) & 0xff);
            g->buffer[y * w + x] = make_color(0xff, r, gc, b);
        }
    }
}

static void fill_alpha_circle(graph_t* g) {
    int32_t x;
    int32_t y;
    int32_t cx;
    int32_t cy;
    int32_t radius;
    int32_t radius2;

    if(g == NULL || g->buffer == NULL)
        return;

    cx = g->w / 2;
    cy = g->h / 2;
    radius = (g->w < g->h ? g->w : g->h) / 2 - 2;
    radius2 = radius * radius;

    for(y = 0; y < g->h; y++) {
        for(x = 0; x < g->w; x++) {
            int32_t dx = x - cx;
            int32_t dy = y - cy;
            int32_t d2 = dx * dx + dy * dy;
            uint8_t alpha = 0;
            if(d2 < radius2) {
                alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
            }
            g->buffer[y * g->w + x] = make_color(alpha, 0xff, 0xe0, 0x20);
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

static int check_pixel(const char* label, const graph_t* g,
        uint32_t x, uint32_t y, uint32_t expect, int* failures) {
    uint32_t got;

    if(g == NULL || g->buffer == NULL ||
            x >= (uint32_t)g->w || y >= (uint32_t)g->h) {
        printf("FAIL %-22s bad coords\n", label);
        (*failures)++;
        return -1;
    }
    got = g->buffer[y * g->w + x];
    if(got != expect) {
        printf("FAIL %-22s (%u,%u) %08x (expect %08x)\n", label, x, y, got, expect);
        (*failures)++;
        return -1;
    }
    printf("PASS %-22s (%u,%u) %08x\n", label, x, y, got);
    return 0;
}

/* fps benchmark: each bench renders frames through /dev/g2d for about
   1 second and reports completed frames per second. a frame is one
   full render pass of the tested op mix, all ops are synchronous IPC,
   so fps measures the full client->driver round trip. */

#define BENCH_USEC       (1000000u) /* ~1 second per bench */
#define BENCH_MAX_FRAMES 5000u

typedef struct {
    graph_t* canvas;
    graph_t* opaque;
    graph_t* alpha;
    uint32_t seq; /* frame counter, varies positions/colors between frames */
} bench_ctx_t;

typedef int (*bench_frame_fn)(void* ctx);

static uint32_t bench_now_usec(void) {
    uint32_t low = 0;
    kernel_tic32(NULL, NULL, &low);
    return low;
}

static int bench_frame_fill(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_fill_req_t fill;
    int32_t maxx;
    int32_t maxy;
    uint32_t i;

    maxx = ctx->canvas->w - 64;
    maxy = ctx->canvas->h - 64;
    if(maxx < 0)
        maxx = 0;
    if(maxy < 0)
        maxy = 0;
    for(i = 0; i < 16; i++) {
        uint32_t seed = ctx->seq * 17u + i * 977u;
        int32_t x = maxx > 0 ? (int32_t)(seed % (uint32_t)maxx) : 0;
        int32_t y = maxy > 0 ? (int32_t)((seed >> 8) % (uint32_t)maxy) : 0;
        g2d_fill_req_init(&fill, img_canvas(ctx->canvas), g2d_rect(x, y, 64, 64),
                0xff000000u | ((seed * 31u) & 0xffffffu));
        if(g2d_fill_rect(&fill) != 0)
            return -1;
    }
    ctx->seq++;
    return 0;
}

static int bench_frame_blit(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_blit_req_t blit;
    int32_t dw;
    int32_t dh;
    int32_t maxx;
    int32_t maxy;
    int32_t x;
    int32_t y;
    int ret;

    dw = ctx->canvas->w / 2;
    dh = ctx->canvas->h / 2;
    maxx = ctx->canvas->w - dw;
    maxy = ctx->canvas->h - dh;
    x = maxx > 0 ? (int32_t)((ctx->seq * 37u) % (uint32_t)maxx) : 0;
    y = maxy > 0 ? (int32_t)((ctx->seq * 53u) % (uint32_t)maxy) : 0;
    g2d_blit_req_init(&blit,
            img_canvas(ctx->canvas),
            img_canvas(ctx->opaque),
            g2d_rect(0, 0, ctx->opaque->w, ctx->opaque->h),
            g2d_rect(x, y, dw, dh),
            0xff);
    ret = g2d_blit_shm(&blit);
    ctx->seq++;
    return ret;
}

static int bench_frame_blit_alpha(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_blit_req_t blit;
    int32_t dw;
    int32_t dh;
    int32_t maxx;
    int32_t maxy;
    int32_t x;
    int32_t y;
    int ret;

    dw = ctx->canvas->w / 3;
    dh = ctx->canvas->h / 3;
    maxx = ctx->canvas->w - dw;
    maxy = ctx->canvas->h - dh;
    x = maxx > 0 ? (int32_t)((ctx->seq * 41u) % (uint32_t)maxx) : 0;
    y = maxy > 0 ? (int32_t)((ctx->seq * 59u) % (uint32_t)maxy) : 0;
    g2d_blit_req_init(&blit,
            img_canvas(ctx->canvas),
            img_canvas(ctx->alpha),
            g2d_rect(0, 0, ctx->alpha->w, ctx->alpha->h),
            g2d_rect(x, y, dw, dh),
            0xff);
    ret = g2d_blit_alpha_shm(&blit);
    ctx->seq++;
    return ret;
}

/* one mixed frame = 16 fills + opaque blit + alpha blit */
static int bench_frame_mixed(void* p) {
    if(bench_frame_fill(p) != 0)
        return -1;
    if(bench_frame_blit(p) != 0)
        return -1;
    return bench_frame_blit_alpha(p);
}

static void bench_run(const char* label, bench_frame_fn fn,
        bench_ctx_t* ctx, int* failures) {
    uint32_t frames = 0;
    uint32_t elapsed = 0;
    uint32_t t0;

    t0 = bench_now_usec();
    while(elapsed < BENCH_USEC && frames < BENCH_MAX_FRAMES) {
        if(fn(ctx) != 0) {
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

int main(int argc, char** argv) {
    g2d_fill_req_t fill;
    g2d_blit_req_t blit;
    g2d_rotate_req_t rotate_req;
    g2d_scale_to_req_t scale_req;
    graph_t* canvas;
    graph_t* rotated;
    graph_t* scaled;
    graph_t* opaque_img;
    graph_t* alpha_img;
    bench_ctx_t bench_ctx;
    uint32_t bg_color = 0xff101820;
    uint32_t fill_color = 0xff204060;
    uint32_t w0 = CANVAS_W;
    uint32_t h0 = CANVAS_H;
    uint32_t rot45;
    int failures = 0;
    int ret;

    (void)argc;
    (void)argv;

    if(has_g2d() != 0) {
        printf("g2d device not found\n");
        return -1;
    }
    printf("g2d: stateless shm canvas test %ux%u\n", w0, h0);

    canvas = canvas_create(w0, h0);
    if(canvas == NULL) {
        printf("create canvas shm failed\n");
        return -1;
    }
    opaque_img = canvas_create(160, 120);
    alpha_img = canvas_create(128, 128);
    if(opaque_img == NULL || alpha_img == NULL) {
        printf("create source shm failed\n");
        canvas_free(opaque_img);
        canvas_free(alpha_img);
        canvas_free(canvas);
        return -1;
    }

    fill_pattern(opaque_img);
    fill_alpha_circle(alpha_img);
    img_clear(canvas, bg_color);

    /* fill: inside the rect becomes the color, outside stays */
    g2d_fill_req_init(&fill, img_canvas(canvas), g2d_rect(24, 24, 220, 120), fill_color);
    ret = g2d_fill_rect(&fill);
    check_ret("fill_rect", ret, 1, &failures);
    check_pixel("fill_inside", canvas, 100, 60, fill_color, &failures);
    check_pixel("fill_edge", canvas, 24, 24, fill_color, &failures);
    check_pixel("fill_outside", canvas, 10, 10, bg_color, &failures);

    /* out-of-bounds rect is clipped, not rejected */
    g2d_fill_req_init(&fill, img_canvas(canvas),
            g2d_rect((int32_t)w0 - 20, (int32_t)h0 - 20, 80, 80), 0xff503040);
    ret = g2d_fill_rect(&fill);
    check_ret("fill_rect_clip", ret, 1, &failures);
    check_pixel("fill_clip_inside", canvas, w0 - 5, h0 - 5, 0xff503040, &failures);

    /* opaque 1:1 blit: canvas pixels become the pattern */
    g2d_blit_req_init(&blit,
            img_canvas(canvas),
            img_canvas(opaque_img),
            g2d_rect(0, 0, opaque_img->w, opaque_img->h),
            g2d_rect(48, 72, opaque_img->w, opaque_img->h),
            0xff);
    ret = g2d_blit_shm(&blit);
    check_ret("blit_opaque", ret, 1, &failures);
    check_pixel("blit_pixel", canvas, 48, 72, opaque_img->buffer[0], &failures);
    check_pixel("blit_pixel2", canvas, 48 + 100, 72 + 60,
            opaque_img->buffer[60 * opaque_img->w + 100], &failures);

    /* scaled blit keeps the corners (nearest neighbor) */
    g2d_blit_req_init(&blit,
            img_canvas(canvas),
            img_canvas(opaque_img),
            g2d_rect(0, 0, opaque_img->w, opaque_img->h),
            g2d_rect(260, 40, 160, 120),
            0xff);
    ret = g2d_blit_shm(&blit);
    check_ret("blit_scale", ret, 1, &failures);
    check_pixel("blit_scale_tl", canvas, 260, 40, opaque_img->buffer[0], &failures);

    /* alpha blit: fully transparent source corners leave dst untouched */
    img_clear(canvas, bg_color);
    g2d_blit_req_init(&blit,
            img_canvas(canvas),
            img_canvas(alpha_img),
            g2d_rect(0, 0, alpha_img->w, alpha_img->h),
            g2d_rect(32, 32, alpha_img->w, alpha_img->h),
            0xff);
    ret = g2d_blit_alpha_shm(&blit);
    check_ret("blit_alpha", ret, 1, &failures);
    check_pixel("blit_alpha_corner", canvas, 32, 32, bg_color, &failures);
    check_pixel("blit_alpha_center", canvas,
            32 + alpha_img->w / 2, 32 + alpha_img->h / 2,
            alpha_img->buffer[(alpha_img->h / 2) * alpha_img->w + alpha_img->w / 2],
            &failures);

    /* rotate: corners travel clockwise. 90/270 swap the dimensions,
       the dst canvas must be created at the rotated size. */
    img_clear(canvas, bg_color);
    canvas->buffer[0] = 0xff000001u;                                  /* TL */
    canvas->buffer[w0 - 1] = 0xff000002u;                             /* TR */
    canvas->buffer[(h0 - 1) * w0 + w0 - 1] = 0xff000003u;             /* BR */
    canvas->buffer[(h0 - 1) * w0] = 0xff000004u;                      /* BL */

    rotated = canvas_create(h0, w0);
    if(rotated == NULL) {
        printf("create rotated shm failed\n");
        failures++;
    }
    else {
        g2d_rotate_req_init(&rotate_req, img_canvas(canvas), img_canvas(rotated), 90);
        ret = g2d_rotate(&rotate_req);
        check_ret("rotate_90", ret, 1, &failures);
        check_pixel("rot90_TL_from_BL", rotated, 0, 0, 0xff000004u, &failures);
        check_pixel("rot90_TR_from_TL", rotated, rotated->w - 1, 0, 0xff000001u, &failures);
        check_pixel("rot90_BR_from_TR", rotated, rotated->w - 1, rotated->h - 1, 0xff000002u, &failures);
        check_pixel("rot90_BL_from_BR", rotated, 0, rotated->h - 1, 0xff000003u, &failures);

        /* wrong dst size is rejected */
        g2d_rotate_req_init(&rotate_req, img_canvas(canvas), img_canvas(canvas), 90);
        ret = g2d_rotate(&rotate_req);
        check_ret("rotate_90_badsize", ret, 0, &failures);
    }

    /* 180 keeps the dimensions: rotate in a same-size canvas */
    scaled = canvas_create(w0, h0);
    if(scaled == NULL) {
        printf("create scaled shm failed\n");
        failures++;
    }
    else {
        g2d_rotate_req_init(&rotate_req, img_canvas(canvas), img_canvas(scaled), 180);
        ret = g2d_rotate(&rotate_req);
        check_ret("rotate_180", ret, 1, &failures);
        check_pixel("rot180_TL_from_BR", scaled, 0, 0, 0xff000003u, &failures);
        check_pixel("rot180_BR_from_TL", scaled, w0 - 1, h0 - 1, 0xff000001u, &failures);

        /* arbitrary angle: dst canvas must match the rotated bounding box */
        canvas_free(scaled);
        rot45 = rotated45_size(w0, h0);
        scaled = canvas_create(rot45, rot45);
        if(scaled != NULL) {
            g2d_rotate_req_init(&rotate_req, img_canvas(canvas), img_canvas(scaled), 45);
            ret = g2d_rotate(&rotate_req);
            check_ret("rotate_45", ret, 1, &failures);

            g2d_rotate_req_init(&rotate_req, img_canvas(canvas), img_canvas(scaled), -270);
            ret = g2d_rotate(&rotate_req);
            check_ret("rotate_-270_badsize", ret, 0, &failures);
        }
        canvas_free(scaled);

        /* scale_to: nearest neighbor keeps the corners */
        fill_pattern(canvas);
        scaled = canvas_create(320, 240);
        if(scaled != NULL) {
            g2d_scale_to_req_init(&scale_req, img_canvas(canvas), img_canvas(scaled));
            ret = g2d_scale_to(&scale_req);
            check_ret("scale_to_320x240", ret, 1, &failures);
            check_pixel("scale_tl", scaled, 0, 0, canvas->buffer[0], &failures);
            check_pixel("scale_br", scaled, 319, 239,
                    canvas->buffer[(h0 - 1) * w0 + w0 - 1], &failures);
        }
        canvas_free(scaled);
    }
    canvas_free(rotated);

    /* fps benchmark on the canvas */
    bench_ctx.canvas = canvas;
    bench_ctx.opaque = opaque_img;
    bench_ctx.alpha = alpha_img;
    bench_ctx.seq = 0;
    printf("--- fps benchmark ---\n");
    bench_run("fill_rect x16", bench_frame_fill, &bench_ctx, &failures);
    bench_run("blit_opaque", bench_frame_blit, &bench_ctx, &failures);
    bench_run("blit_alpha", bench_frame_blit_alpha, &bench_ctx, &failures);
    bench_run("mixed_frame", bench_frame_mixed, &bench_ctx, &failures);

    printf("g2dtest summary: %s (%d failure)\n", failures == 0 ? "PASS" : "FAIL", failures);
    usleep(50000);

    canvas_free(alpha_img);
    canvas_free(opaque_img);
    canvas_free(canvas);
    return failures == 0 ? 0 : -1;
}
