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

#define CANVAS_W 800u
#define CANVAS_H 600u

static graph_t* canvas_create(uint32_t width, uint32_t height) {
    return graph_new_shm((int32_t)width, (int32_t)height);
}

static void canvas_free(graph_t* g) {
    graph_free(g);
}

static g2d_canvas_t img_canvas(const graph_t* g) {
    return g2d_canvas(g->shm_id, (uint32_t)g->w * (uint32_t)g->h * 4u,
            (uint32_t)g->w, (uint32_t)g->h, g->shm_contig ? 1 : 0);
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

/* fps benchmark: the op set (fill / opaque blit / alpha blit / mixed
   frame / rotate 180,90,45 / scale 1:1) runs once per resolution group,
   so ops at the same resolution compare directly. every blit and the
   scale bench are 1:1 at the group's own resolution: the source canvas
   is group-sized and both rects are the full canvas, so each group
   exercises the identity copy path. each bench renders frames through
   /dev/g2d for about 1 second and reports completed frames per second;
   a frame is one full render pass of the op, all ops are synchronous
   IPC, so fps measures the full client->driver round trip. */

#define BENCH_USEC       (1000000u) /* ~1 second per bench */
#define BENCH_MAX_FRAMES 5000u

/* resolutions the full op set runs at */
#define BENCH_GROUPS 4u

static const uint32_t bench_group_w[BENCH_GROUPS] = { 640u, 800u, 1280u, 1920u };
static const uint32_t bench_group_h[BENCH_GROUPS] = { 480u, 600u, 960u, 1080u };

typedef struct {
    graph_t* canvas;
    graph_t* opaque;
    graph_t* alpha;
    graph_t* rot_dst;    /* rotate 90 ping-pong partner (h x w) */
    graph_t* rot180_dst; /* rotate 180 dst, same size as canvas */
    graph_t* rot45_dst;  /* rotate 45 dst, rotated bounding box of canvas */
    graph_t* scale_src;  /* scale_to source (group-sized) */
    graph_t* scale_same; /* scale_to 1:1 dst (same size as scale src) */
    uint32_t seq;    /* frame counter, varies positions/colors between frames */
    uint32_t rot_swap;   /* rotate 90 ping-pong phase selector */
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
    uint32_t seed = ctx->seq * 31u;

    /* one full-canvas fill per frame */
    g2d_fill_req_init(&fill, img_canvas(ctx->canvas),
            g2d_rect(0, 0, ctx->canvas->w, ctx->canvas->h),
            0xff000000u | (seed & 0xffffffu));
    if(g2d_fill_rect(&fill) != 0)
        return -1;
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

    dw = ctx->canvas->w;
    dh = ctx->canvas->h;
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
    ret = g2d_blit(&blit);
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

    dw = ctx->canvas->w;
    dh = ctx->canvas->h;
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
    ret = g2d_blit_alpha(&blit);
    ctx->seq++;
    return ret;
}

/* one mixed frame = 1 full-canvas fill + opaque blit + alpha blit */
static int bench_frame_mixed(void* p) {
    if(bench_frame_fill(p) != 0)
        return -1;
    if(bench_frame_blit(p) != 0)
        return -1;
    return bench_frame_blit_alpha(p);
}

/* rotate 90 ping-pong: each frame rotates the previous result by 90, so
   the dimensions alternate w x h -> h x w -> ... and every frame is one
   full-canvas rotate through a size-swapping dst. */
static int bench_frame_rotate90(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_rotate_req_t req;
    graph_t* src;
    graph_t* dst;

    if((ctx->rot_swap & 1u) != 0) {
        src = ctx->rot_dst;
        dst = ctx->canvas;
    }
    else {
        src = ctx->canvas;
        dst = ctx->rot_dst;
    }
    g2d_rotate_req_init(&req, img_canvas(src), img_canvas(dst), G2D_ROTATE_90);
    ctx->rot_swap++;
    ctx->seq++;
    return g2d_rotate(&req);
}

/* rotate 180 into a dedicated same-size dst: the source never changes,
   so this measures raw 180 throughput without dimension swaps */
static int bench_frame_rotate180(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_rotate_req_t req;

    g2d_rotate_req_init(&req, img_canvas(ctx->canvas), img_canvas(ctx->rot180_dst), G2D_ROTATE_180);
    ctx->seq++;
    return g2d_rotate(&req);
}

/* arbitrary angle (45) into the rotated bounding box dst: exercises the
   inverse-mapped nearest-neighbor path, the most expensive rotate */
static int bench_frame_rotate45(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_rotate_req_t req;

    g2d_rotate_req_init(&req, img_canvas(ctx->canvas), img_canvas(ctx->rot45_dst), 45);
    ctx->seq++;
    return g2d_rotate(&req);
}

/* same-size scale_to is a plain 1:1 copy (row-copy fast path) */
static int bench_frame_scale_1to1(void* p) {
    bench_ctx_t* ctx = (bench_ctx_t*)p;
    g2d_scale_to_req_t req;

    g2d_scale_to_req_init(&req, img_canvas(ctx->scale_src), img_canvas(ctx->scale_same));
    ctx->seq++;
    return g2d_scale_to(&req);
}

static void bench_run(const char* label, bench_frame_fn fn,
        bench_ctx_t* ctx, int* failures) {
    uint32_t frames = 0;
    uint32_t elapsed = 0;
    uint32_t t0;

    t0 = bench_now_usec();
    while(elapsed < BENCH_USEC && frames < BENCH_MAX_FRAMES) {
        if(fn(ctx) != 0) {
            printf("FAIL %-30s bench frame %u failed\n", label, frames);
            (*failures)++;
            return;
        }
        frames++;
        elapsed = bench_now_usec() - t0;
    }
    if(elapsed == 0)
        elapsed = 1;
    /* fixed-width numeric columns so the rows align vertically and the
       fps of ops in the same resolution group compare directly */
    printf("PERF %-30s %8u %8u %8u %8u\n",
            label, frames, elapsed,
            (uint32_t)(((uint64_t)frames * 1000000u) / elapsed),
            elapsed / frames);
}

/* run one op of the full set with a self-contained "<op>@<w>x<h>" label
   so every PERF row is readable without its group header */
static void bench_run_group(const char* op, bench_frame_fn fn,
        uint32_t gw, uint32_t gh, bench_ctx_t* ctx, int* failures) {
    char label[40];

    snprintf(label, sizeof(label), "%s@%ux%u", op, gw, gh);
    bench_run(label, fn, ctx, failures);
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
    graph_t* wide;
    bench_ctx_t bench_ctx;
    uint32_t bg_color = 0xff101820;
    uint32_t fill_color = 0xff204060;
    uint32_t w0 = CANVAS_W;
    uint32_t h0 = CANVAS_H;
    uint32_t rot45;
    uint32_t g;
    int failures = 0;
    int ret;

    (void)argc;
    (void)argv;

    if(has_g2d() != 0) {
        printf("g2d device not found\n");
        return -1;
    }
    /* confirm the g2d engine clock every run: hardware backends pin the
       gpu to its max rate at driver startup, so fps numbers are only
       comparable across runs at a known clock; software backends have
       no engine clock and report n/a */
    {
        uint32_t gpu_hz = 0;
        if(g2d_get_clock(&gpu_hz) == 0 && gpu_hz > 0)
            printf("gpu clock: %u Hz (%u MHz)\n", gpu_hz, gpu_hz / 1000000);
        else
            printf("gpu clock: n/a (no engine clock reported)\n");
    }
    printf("g2d: stateless shm canvas test %ux%u\n", w0, h0);

    canvas = canvas_create(w0, h0);
    if(canvas == NULL) {
        printf("create canvas shm failed\n");
        return -1;
    }
    opaque_img = canvas_create(640, 480);
    alpha_img = canvas_create(640, 480);
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

    /* 16-pixel-aligned fills.  Aligned and full-width rects used to take a
       different kernel from every other fill: one that walked a whole row
       block inside a single QPU thread, so its iteration count grew with
       the surface width.  640 px and 800 px retired cleanly but 1280 px
       wedged VideoCore for good.  These cases keep the aligned geometries
       pixel-verified now that they share the general span dispatch. */
    g2d_fill_req_init(&fill, img_canvas(canvas), g2d_rect(64, 32, 512, 256),
            0xff305070);
    ret = g2d_fill_rect(&fill);
    check_ret("fill_aligned", ret, 1, &failures);
    check_pixel("fill_align_inside", canvas, 320, 160, 0xff305070, &failures);
    check_pixel("fill_align_first", canvas, 64, 32, 0xff305070, &failures);
    check_pixel("fill_align_last", canvas, 575, 287, 0xff305070, &failures);
    check_pixel("fill_align_left", canvas, 63, 300, bg_color, &failures);
    check_pixel("fill_align_right", canvas, 576, 32, bg_color, &failures);

    /* A 1280-wide full-canvas fill is the exact shape that used to ask one
       thread for 1280 serial VDWs, so it is the regression guard for the
       wedge; it also covers a full-width rect whose rows are physically
       contiguous across the whole surface. */
    wide = canvas_create(1280, 32);
    if(wide == NULL) {
        printf("FAIL %-22s create 1280x32 failed\n", "fill_fullwidth");
        failures++;
    } else {
        img_clear(wide, bg_color);
        g2d_fill_req_init(&fill, img_canvas(wide),
                g2d_rect(0, 0, wide->w, wide->h), 0xff406080);
        ret = g2d_fill_rect(&fill);
        check_ret("fill_fullwidth", ret, 1, &failures);
        check_pixel("fill_full_TL", wide, 0, 0, 0xff406080, &failures);
        check_pixel("fill_full_BR", wide, 1279, 31, 0xff406080, &failures);
        check_pixel("fill_full_mid", wide, 640, 16, 0xff406080, &failures);
        check_pixel("fill_full_group", wide, 16, 0, 0xff406080, &failures);
        canvas_free(wide);
        wide = NULL;
    }

    /* opaque 1:1 blit: canvas pixels become the pattern */
    g2d_blit_req_init(&blit,
            img_canvas(canvas),
            img_canvas(opaque_img),
            g2d_rect(0, 0, opaque_img->w, opaque_img->h),
            g2d_rect(48, 72, opaque_img->w, opaque_img->h),
            0xff);
    ret = g2d_blit(&blit);
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
    ret = g2d_blit(&blit);
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
    ret = g2d_blit_alpha(&blit);
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

    /* big-surface rotate: past the driver's large-surface threshold the
       destination is dispatched in 2D tiles, which the small tests above
       never exercise (the fps groups only time those paths).  90 degrees
       with a source width % 16 != 0 falls back from the rot90 kernel to
       the tiled affine blit; 45 degrees uses the tiled rotate kernel,
       whose out-of-content samples must read back as transparent 0. */
    {
        graph_t* big = canvas_create(1080, 1920);
        graph_t* big_rot = canvas_create(1920, 1080);
        if(big == NULL || big_rot == NULL) {
            printf("create big rotate shm failed\n");
            failures++;
        }
        else {
            img_clear(big, 0xff304050u);
            big->buffer[0] = 0xff000001u;                            /* TL */
            big->buffer[1080 - 1] = 0xff000002u;                     /* TR */
            big->buffer[(1920 - 1) * 1080 + 1080 - 1] = 0xff000003u; /* BR */
            big->buffer[(1920 - 1) * 1080] = 0xff000004u;            /* BL */
            g2d_rotate_req_init(&rotate_req, img_canvas(big), img_canvas(big_rot), 90);
            ret = g2d_rotate(&rotate_req);
            check_ret("big_rotate_90", ret, 1, &failures);
            check_pixel("big_rot90_TL_from_BL", big_rot, 0, 0, 0xff000004u, &failures);
            check_pixel("big_rot90_TR_from_TL", big_rot, 1919, 0, 0xff000001u, &failures);
            check_pixel("big_rot90_BR_from_TR", big_rot, 1919, 1079, 0xff000002u, &failures);
            check_pixel("big_rot90_BL_from_BR", big_rot, 0, 1079, 0xff000003u, &failures);
            check_pixel("big_rot90_body", big_rot, 960, 540, 0xff304050u, &failures);
        }
        canvas_free(big);
        canvas_free(big_rot);
    }
    {
        uint32_t r45big = rotated45_size(1280, 720);
        graph_t* big = canvas_create(1280, 720);
        graph_t* big45 = canvas_create(r45big, r45big);
        if(big == NULL || big45 == NULL) {
            printf("create big rot45 shm failed\n");
            failures++;
        }
        else {
            img_clear(big, 0xff664422u);
            g2d_rotate_req_init(&rotate_req, img_canvas(big), img_canvas(big45), 45);
            ret = g2d_rotate(&rotate_req);
            check_ret("big_rotate_45", ret, 1, &failures);
            /* the bbox corners sit outside the rotated content box, so
               argb_rotate writes transparent 0 there - in every tile */
            check_pixel("big_rot45_TL_clear", big45, 0, 0, 0x00000000u, &failures);
            check_pixel("big_rot45_BR_clear", big45, r45big - 1, r45big - 1,
                    0x00000000u, &failures);
            check_pixel("big_rot45_center", big45, r45big / 2, r45big / 2,
                    0xff664422u, &failures);
        }
        canvas_free(big);
        canvas_free(big45);
    }

    /* fps benchmark: the full op set runs once per resolution group */
    printf("--- fps benchmark ---\n");
    printf("PERF %-30s %8s %8s %8s %8s\n", "label", "frames", "us", "fps", "us/frame");
    /* each group allocates its canvases in three stages so the peak
       contiguous shm stays small: the fill/blit stage (canvas + pattern
       src + alpha src), the rotate stage (reuses the canvas, frees the
       alpha src, adds the three rotate dsts) and the scale stage (frees
       the rotate dsts, adds the 1:1 scale dst). at 1920x1080 the rotate
       stage peaks around 55 MB, inside the 128 MB contiguous slab the
       raspi5 kernel reserves. */
    for(g = 0; g < BENCH_GROUPS; g++) {
        uint32_t gw = bench_group_w[g];
        uint32_t gh = bench_group_h[g];
        uint32_t r45 = rotated45_size(gw, gh);
        graph_t* g_canvas = NULL;
        graph_t* g_src = NULL;
        graph_t* g_alpha = NULL;
        graph_t* g_rot90 = NULL;
        graph_t* g_rot180 = NULL;
        graph_t* g_rot45 = NULL;
        graph_t* g_scale_same = NULL;

        printf("group %ux%u: fill, blit, alpha, mixed, rotate 180/90/45, scale 1:1\n",
                gw, gh);

        /* stage a: fill / blit / alpha / mixed share the work canvas,
           the pattern src and the alpha src */
        g_canvas = canvas_create(gw, gh);
        g_src = canvas_create(gw, gh);
        g_alpha = canvas_create(gw, gh);
        if(g_canvas == NULL || g_src == NULL || g_alpha == NULL) {
            printf("create group %ux%u stage a shm failed, group skipped\n", gw, gh);
            failures++;
            canvas_free(g_canvas);
            canvas_free(g_src);
            canvas_free(g_alpha);
            continue;
        }
        fill_pattern(g_src);
        fill_alpha_circle(g_alpha);
        bench_ctx.canvas = g_canvas;
        bench_ctx.opaque = g_src;
        bench_ctx.alpha = g_alpha;
        bench_ctx.seq = 0;
        bench_ctx.rot_swap = 0;
        bench_run_group("fill_rect", bench_frame_fill, gw, gh, &bench_ctx, &failures);
        bench_run_group("blit_opaque", bench_frame_blit, gw, gh, &bench_ctx, &failures);
        bench_run_group("blit_alpha", bench_frame_blit_alpha, gw, gh, &bench_ctx, &failures);
        bench_run_group("mixed_frame", bench_frame_mixed, gw, gh, &bench_ctx, &failures);

        /* stage b: rotate reuses the work canvas and the pattern src,
           frees the alpha src and adds the three rotate dsts */
        canvas_free(g_alpha);
        g_alpha = NULL;
        g_rot90 = canvas_create(gh, gw);
        g_rot180 = canvas_create(gw, gh);
        g_rot45 = canvas_create(r45, r45);
        if(g_rot90 == NULL || g_rot180 == NULL || g_rot45 == NULL) {
            printf("create group %ux%u stage b shm failed, rotate/scale benches skipped\n", gw, gh);
            failures++;
            canvas_free(g_rot90);
            canvas_free(g_rot180);
            canvas_free(g_rot45);
            canvas_free(g_src);
            canvas_free(g_canvas);
            continue;
        }
        bench_ctx.rot_dst = g_rot90;
        bench_ctx.rot180_dst = g_rot180;
        bench_ctx.rot45_dst = g_rot45;
        bench_ctx.rot_swap = 0;
        bench_run_group("rotate_180", bench_frame_rotate180, gw, gh, &bench_ctx, &failures);
        bench_run_group("rotate_90_pingpong", bench_frame_rotate90, gw, gh, &bench_ctx, &failures);
        bench_run_group("rotate_45_bbox", bench_frame_rotate45, gw, gh, &bench_ctx, &failures);

        /* stage c: scale reuses the pattern src, frees the rotate dsts
           and adds the 1:1 scale dst */
        canvas_free(g_rot45);
        canvas_free(g_rot180);
        canvas_free(g_rot90);
        g_scale_same = canvas_create(gw, gh);
        if(g_scale_same == NULL) {
            printf("create group %ux%u stage c shm failed, scale bench skipped\n", gw, gh);
            failures++;
            canvas_free(g_src);
            canvas_free(g_canvas);
            continue;
        }
        bench_ctx.scale_src = g_src;
        bench_ctx.scale_same = g_scale_same;
        bench_run_group("scale_to_1to1", bench_frame_scale_1to1, gw, gh, &bench_ctx, &failures);

        canvas_free(g_scale_same);
        canvas_free(g_src);
        canvas_free(g_canvas);
    }

    printf("g2dtest summary: %s (%d failure)\n", failures == 0 ? "PASS" : "FAIL", failures);
    usleep(50000);

    canvas_free(alpha_img);
    canvas_free(opaque_img);
    canvas_free(canvas);
    return failures == 0 ? 0 : -1;
}
