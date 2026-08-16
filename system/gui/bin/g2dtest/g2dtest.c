#include <g2d/g2d.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <graph/graph.h>

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

static uint32_t blend_over(uint32_t dst, uint32_t src) {
    uint8_t sa = (src >> 24) & 0xff;
    uint8_t sr = (src >> 16) & 0xff;
    uint8_t sg = (src >> 8) & 0xff;
    uint8_t sb = src & 0xff;
    uint8_t da = (dst >> 24) & 0xff;
    uint8_t dr = (dst >> 16) & 0xff;
    uint8_t dg = (dst >> 8) & 0xff;
    uint8_t db = dst & 0xff;
    uint32_t inv_a = 255 - sa;

    if(sa == 0)
        return dst;
    if(sa == 0xff)
        return src;

    da = da + (uint8_t)((255 - da) * sa / 255);
    dr = (uint8_t)((sr * sa + dr * inv_a) / 255);
    dg = (uint8_t)((sg * sa + dg * inv_a) / 255);
    db = (uint8_t)((sb * sa + db * inv_a) / 255);
    return make_color(da, dr, dg, db);
}

static uint32_t checker_color(uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
    uint8_t r = (uint8_t)((x * 255) / width);
    uint8_t g = (uint8_t)((y * 255) / height);
    uint8_t b = ((x / 16 + y / 16) & 1) ? 0xd0 : 0x30;
    return make_color(0xff, r, g, b);
}

static uint32_t alpha_circle_color(uint32_t width, uint32_t height, int32_t x, int32_t y) {
    int32_t cx = (int32_t)width / 2;
    int32_t cy = (int32_t)height / 2;
    int32_t radius = (int32_t)((width < height ? width : height) / 2) - 2;
    int32_t radius2 = radius * radius;
    int32_t dx = x - cx;
    int32_t dy = y - cy;
    int32_t d2 = dx * dx + dy * dy;
    uint8_t alpha = 0;

    if(d2 < radius2)
        alpha = (uint8_t)(255 - ((d2 * 255) / radius2));
    return make_color(alpha, 0xff, 0xe0, 0x20);
}

static int verify_pixel(g2d_t* g2d, const char* label, int32_t x, int32_t y, uint32_t expected, int* failures) {
    uint32_t actual = 0;
    int ret = g2d_get_pixel(g2d, x, y, &actual);

    if(ret != 0) {
        printf("FAIL %-22s readback (%d,%d) ret=%d\n", label, x, y, ret);
        (*failures)++;
        return -1;
    }
    if(actual != expected) {
        printf("FAIL %-22s (%d,%d) actual=0x%08x expected=0x%08x\n",
                label, x, y, actual, expected);
        (*failures)++;
        return -1;
    }

    printf("PASS %-22s (%d,%d) actual=0x%08x\n", label, x, y, actual);
    return 0;
}

static void print_stats_delta(const g2d_stats_t* before, const g2d_stats_t* after) {
    printf("stats delta: vc_clear=%u vc_fill=%u vc_blit=%u vc_alpha=%u vc_present=%u "
            "soft_clear=%u soft_fill=%u soft_blit=%u soft_alpha=%u soft_present=%u soft_fallback=%u\n",
            after->vc_clear_ops - before->vc_clear_ops,
            after->vc_fill_ops - before->vc_fill_ops,
            after->vc_blit_ops - before->vc_blit_ops,
            after->vc_alpha_blit_ops - before->vc_alpha_blit_ops,
            after->vc_present_ops - before->vc_present_ops,
            after->soft_clear_ops - before->soft_clear_ops,
            after->soft_fill_ops - before->soft_fill_ops,
            after->soft_blit_ops - before->soft_blit_ops,
            after->soft_alpha_blit_ops - before->soft_alpha_blit_ops,
            after->soft_present_ops - before->soft_present_ops,
            after->soft_fallback_ops - before->soft_fallback_ops);
}

static int verify_stats_total(const g2d_stats_t* before, const g2d_stats_t* after) {
    uint32_t clear_ops;
    uint32_t fill_ops;
    uint32_t blit_ops;
    uint32_t alpha_ops;

    clear_ops = (after->vc_clear_ops - before->vc_clear_ops) +
            (after->soft_clear_ops - before->soft_clear_ops);
    fill_ops = (after->vc_fill_ops - before->vc_fill_ops) +
            (after->soft_fill_ops - before->soft_fill_ops);
    blit_ops = (after->vc_blit_ops - before->vc_blit_ops) +
            (after->soft_blit_ops - before->soft_blit_ops);
    alpha_ops = (after->vc_alpha_blit_ops - before->vc_alpha_blit_ops) +
            (after->soft_alpha_blit_ops - before->soft_alpha_blit_ops);

    if(clear_ops != 1 || fill_ops != 3 || blit_ops != 2 || alpha_ops != 2) {
        printf("FAIL unexpected stats totals clear=%u fill=%u blit=%u alpha=%u\n",
                clear_ops, fill_ops, blit_ops, alpha_ops);
        return -1;
    }
    if((after->vc_present_ops - before->vc_present_ops) != 0 ||
            (after->soft_present_ops - before->soft_present_ops) != 0) {
        printf("FAIL present counters changed in offscreen-only mode vc_present=%u soft_present=%u\n",
                after->vc_present_ops - before->vc_present_ops,
                after->soft_present_ops - before->soft_present_ops);
        return -1;
    }

    printf("PASS backend stats confirm offscreen g2d call totals\n");
    return 0;
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

static int blit_rotate_valid(uint8_t rotate) {
    switch (rotate) {
    case G2D_ROTATE_0:
    case G2D_ROTATE_90:
    case G2D_ROTATE_180:
    case G2D_ROTATE_270:
        return 1;
    default:
        return 0;
    }
}

static graph_t* reference_crop_source(graph_t* src, const g2d_blit_req_t* req) {
    graph_t* cropped;

    if(src == NULL || req == NULL)
        return NULL;
    if(req->sx < 0 || req->sy < 0 || req->sw <= 0 || req->sh <= 0)
        return NULL;
    if(req->sx + req->sw > src->w || req->sy + req->sh > src->h)
        return NULL;

    cropped = graph_new(NULL, req->sw, req->sh);
    if(cropped == NULL || cropped->buffer == NULL) {
        if(cropped != NULL)
            graph_free(cropped);
        return NULL;
    }

    graph_blt(src,
            req->sx, req->sy, req->sw, req->sh,
            cropped,
            0, 0, req->sw, req->sh);
    return cropped;
}

static graph_t* reference_prepare_source(graph_t* src, const g2d_blit_req_t* req) {
    graph_t* cropped;
    graph_t* rotated;

    if(src == NULL || req == NULL || !blit_rotate_valid(req->rotate))
        return NULL;

    cropped = reference_crop_source(src, req);
    if(cropped == NULL)
        return NULL;
    if(req->rotate == G2D_ROTATE_0)
        return cropped;

    rotated = graph_rotate(cropped, req->rotate);
    graph_free(cropped);
    return rotated;
}

static int reference_blit(graph_t* dst, graph_t* src, const g2d_blit_req_t* req, uint8_t use_alpha) {
    graph_t* prepared;

    if(dst == NULL || src == NULL || req == NULL || req->dw <= 0 || req->dh <= 0)
        return -1;
    prepared = reference_prepare_source(src, req);
    if(prepared == NULL)
        return -1;

    if(use_alpha != 0) {
        if(prepared->w == req->dw && prepared->h == req->dh) {
            graph_blt_alpha(prepared,
                    0, 0, prepared->w, prepared->h,
                    dst,
                    req->dx, req->dy, req->dw, req->dh,
                    req->alpha);
        }
        else {
            graph_blt_fit_alpha(prepared,
                    0, 0, prepared->w, prepared->h,
                    dst,
                    req->dx, req->dy, req->dw, req->dh,
                    req->alpha);
        }
    }
    else {
        if(prepared->w == req->dw && prepared->h == req->dh) {
            graph_blt(prepared,
                    0, 0, prepared->w, prepared->h,
                    dst,
                    req->dx, req->dy, req->dw, req->dh);
        }
        else {
            graph_blt_fit(prepared,
                    0, 0, prepared->w, prepared->h,
                    dst,
                    req->dx, req->dy, req->dw, req->dh);
        }
    }

    graph_free(prepared);
    return 0;
}

static int verify_ref_pixel(g2d_t* g2d, graph_t* ref, const char* label, int32_t x, int32_t y, int* failures) {
    if(ref == NULL || x < 0 || y < 0 || x >= ref->w || y >= ref->h) {
        printf("FAIL %-22s invalid ref point (%d,%d)\n", label, x, y);
        (*failures)++;
        return -1;
    }
    return verify_pixel(g2d, label, x, y, graph_get_pixel(ref, x, y), failures);
}

int main(int argc, char** argv) {
    g2d_t g2d;
    g2d_info_t info;
    g2d_stats_t stats_before;
    g2d_stats_t stats_after;
    g2d_fill_req_t fill;
    g2d_blit_req_t blit;
    shm_image_t opaque_img;
    shm_image_t alpha_img;
    g2d_rect_t src_rect;
    graph_t opaque_graph;
    graph_t alpha_graph;
    graph_t* ref;
    uint32_t bg_color = 0xff101820;
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
        return -1;
    }

    printf("g2d: %ux%u depth=%u backend=%u(%s)\n",
            info.width, info.height, info.depth,
            info.backend, g2d_backend_name(info.backend));
    if(g2d_get_stats(&g2d, &stats_before) != 0) {
        printf("g2d_get_stats(before) failed\n");
        g2d_close(&g2d);
        return -1;
    }

    if(shm_image_create(&opaque_img, 0x47324410, 160, 120) != 0) {
        printf("create opaque shm failed\n");
        return -1;
    }
    if(shm_image_create(&alpha_img, 0x47324411, 128, 128) != 0) {
        printf("create alpha shm failed\n");
        shm_image_destroy(&opaque_img);
        return -1;
    }

    fill_checker(&opaque_img);
    fill_alpha_circle(&alpha_img);
    graph_init(&opaque_graph, opaque_img.pixels, opaque_img.width, opaque_img.height);
    graph_init(&alpha_graph, alpha_img.pixels, alpha_img.width, alpha_img.height);
    ref = graph_new(NULL, info.width, info.height);
    if(ref == NULL || ref->buffer == NULL) {
        printf("create reference graph failed\n");
        if(ref != NULL)
            graph_free(ref);
        shm_image_destroy(&alpha_img);
        shm_image_destroy(&opaque_img);
        g2d_close(&g2d);
        return -1;
    }

    ret = g2d_clear(&g2d, bg_color);
    printf("g2d_clear: %d\n", ret);
    if(ret != 0)
        failures++;
    graph_clear(ref, bg_color);

    g2d_fill_req_init(&fill, g2d_rect(24, 24, 220, 120), 0xff204060);
    ret = g2d_fill_rect(&g2d, &fill);
    printf("fill_rect #1: %d\n", ret);
    if(ret != 0)
        failures++;
    graph_fill_rect(ref, fill.rect.x, fill.rect.y, fill.rect.w, fill.rect.h, fill.color);

    g2d_fill_req_init(&fill, g2d_rect((int32_t)info.width - 180, 40, 140, 96), 0xff503040);
    ret = g2d_fill_rect(&g2d, &fill);
    printf("fill_rect #2: %d\n", ret);
    if(ret != 0)
        failures++;
    graph_fill_rect(ref, fill.rect.x, fill.rect.y, fill.rect.w, fill.rect.h, fill.color);

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
    printf("blit_opaque: %d\n", ret);
    if(ret != 0)
        failures++;
    reference_blit(ref, &opaque_graph, &blit, 0);

    blit2_x = (int32_t)info.width - 280;
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
            G2D_ROTATE_90);
    ret = g2d_blit_shm(&g2d, &blit);
    printf("blit_scale_rotate: %d\n", ret);
    if(ret != 0)
        failures++;
    reference_blit(ref, &opaque_graph, &blit, 0);

    src_rect = g2d_rect(0, 0, (int32_t)alpha_img.width, (int32_t)alpha_img.height);
    g2d_blit_req_init(&blit,
            alpha_img.shm_id,
            alpha_img.size,
            alpha_img.width,
            alpha_img.height,
            alpha_img.stride,
            src_rect,
            g2d_rect((int32_t)info.width / 2, (int32_t)info.height / 2 - 32,
                    (int32_t)alpha_img.width, (int32_t)alpha_img.height),
            0xff);
    ret = g2d_blit_alpha_shm(&g2d, &blit);
    printf("blit_alpha: %d\n", ret);
    if(ret != 0)
        failures++;
    reference_blit(ref, &alpha_graph, &blit, 1);

    alpha2_x = (int32_t)info.width / 2 - 220;
    alpha2_y = (int32_t)info.height / 2 + 40;
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
            G2D_ROTATE_270);
    ret = g2d_blit_alpha_shm(&g2d, &blit);
    printf("blit_alpha_scale_rotate: %d\n", ret);
    if(ret != 0)
        failures++;
    reference_blit(ref, &alpha_graph, &blit, 1);

    g2d_fill_req_init(&fill, g2d_rect(0, (int32_t)info.height - 36, (int32_t)info.width, 36), 0xff000000);
    ret = g2d_fill_rect(&g2d, &fill);
    printf("fill_rect footer: %d\n", ret);
    if(ret != 0)
        failures++;
    graph_fill_rect(ref, fill.rect.x, fill.rect.y, fill.rect.w, fill.rect.h, fill.color);

    verify_ref_pixel(&g2d, ref, "clear_bg", 0, 0, &failures);
    verify_ref_pixel(&g2d, ref, "fill1_inside", 24, 24, &failures);
    verify_ref_pixel(&g2d, ref, "fill1_outside", 23, 24, &failures);
    verify_ref_pixel(&g2d, ref, "fill2_inside", (int32_t)info.width - 180, 40, &failures);
    verify_ref_pixel(&g2d, ref, "blit_opaque_tl", 48, 72, &failures);
    verify_ref_pixel(&g2d, ref, "blit_opaque_mid", 58, 92, &failures);
    verify_ref_pixel(&g2d, ref, "blit_opaque_br",
            48 + (int32_t)opaque_img.width - 1, 72 + (int32_t)opaque_img.height - 1, &failures);
    verify_ref_pixel(&g2d, ref, "blit_scale_rot_tl", blit2_x, blit2_y, &failures);
    verify_ref_pixel(&g2d, ref, "blit_scale_rot_mid", blit2_x + 90, blit2_y + 70, &failures);
    verify_ref_pixel(&g2d, ref, "blit_scale_rot_br", blit2_x + 179, blit2_y + 139, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_base_tl",
            (int32_t)info.width / 2, (int32_t)info.height / 2 - 32, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_base_mid",
            (int32_t)info.width / 2 + 64, (int32_t)info.height / 2 + 32, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_base_partial",
            (int32_t)info.width / 2 + 96, (int32_t)info.height / 2 + 32, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_rot_tl", alpha2_x, alpha2_y, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_rot_mid", alpha2_x + 100, alpha2_y + 60, &failures);
    verify_ref_pixel(&g2d, ref, "alpha_rot_br", alpha2_x + 199, alpha2_y + 119, &failures);
    verify_ref_pixel(&g2d, ref, "footer_fill", 0, (int32_t)info.height - 1, &failures);

    if(g2d_get_stats(&g2d, &stats_after) != 0) {
        printf("g2d_get_stats(after) failed\n");
        failures++;
    }
    else {
        print_stats_delta(&stats_before, &stats_after);
        if(verify_stats_total(&stats_before, &stats_after) != 0)
            failures++;
    }
    printf("g2dtest summary: %s (%d failure)\n", failures == 0 ? "PASS" : "FAIL", failures);
    usleep(50000);

    shm_image_destroy(&alpha_img);
    shm_image_destroy(&opaque_img);
    graph_free(ref);
    g2d_close(&g2d);
    return failures == 0 ? ret : -1;
}
