#include "svg.h"
#include "plutosvg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__)
#include <openlibm_fenv_amd64.h>
#endif

static void svg_begin_render_env(void* state) {
#if defined(__x86_64__)
    fenv_t* env = (fenv_t*)state;
    fegetenv(env);
    fesetround(FE_TONEAREST);
#else
    (void)state;
#endif
}

static void svg_end_render_env(void* state) {
#if defined(__x86_64__)
    fenv_t* env = (fenv_t*)state;
    fesetenv(env);
#else
    (void)state;
#endif
}

svg_image_t* svg_load_from_memory(const uint8_t* data, uint32_t size) {
#if defined(__x86_64__)
    fenv_t render_env;
#endif

    svg_begin_render_env(
#if defined(__x86_64__)
        &render_env
#else
        NULL
#endif
    );

    if (data == NULL || size == 0) {
        fprintf(stderr, "[libsvg] invalid input buffer data=%p size=%u\n", (const void*)data, size);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    // 1. 使用 plutosvg 加载 SVG 文档
    plutosvg_document_t* doc = plutosvg_document_load_from_data((const char*)data, size, -1.0f, -1.0f, NULL, NULL);
    if (doc == NULL) {
        fprintf(stderr, "[libsvg] plutosvg_document_load_from_data failed size=%u\n", size);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    // 2. 获取 SVG 原始尺寸
    int width = (int)plutosvg_document_get_width(doc);
    int height = (int)plutosvg_document_get_height(doc);

    if (width <= 0 || height <= 0) {
        fprintf(stderr, "[libsvg] invalid svg size width=%d height=%d\n", width, height);
        plutosvg_document_destroy(doc);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    // 3. 将 SVG 渲染到 plutovg surface
    plutovg_surface_t* surface = plutosvg_document_render_to_surface(doc, NULL, width, height, NULL, NULL, NULL);
    plutosvg_document_destroy(doc);

    if (surface == NULL) {
        fprintf(stderr, "[libsvg] render_to_surface failed width=%d height=%d\n", width, height);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    int surf_width = plutovg_surface_get_width(surface);
    int surf_height = plutovg_surface_get_height(surface);

    // 4. 分配输出图像结构体
    svg_image_t* img = (svg_image_t*)malloc(sizeof(svg_image_t));
    if (img == NULL) {
        fprintf(stderr, "[libsvg] alloc svg_image_t failed\n");
        plutovg_surface_destroy(surface);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    img->width = (uint32_t)surf_width;
    img->height = (uint32_t)surf_height;

    size_t pixel_bytes = (size_t)surf_width * surf_height * sizeof(uint32_t);
    img->pixels = (uint32_t*)malloc(pixel_bytes);
    if (img->pixels == NULL) {
        fprintf(stderr, "[libsvg] alloc pixel buffer failed bytes=%lu\n", (unsigned long)pixel_bytes);
        free(img);
        plutovg_surface_destroy(surface);
        svg_end_render_env(
#if defined(__x86_64__)
            &render_env
#else
            NULL
#endif
        );
        return NULL;
    }

    // 5. 从 surface 拷贝像素数据到输出缓冲区
    const unsigned char* src_pixels = plutovg_surface_get_data(surface);
    memcpy(img->pixels, src_pixels, pixel_bytes);

    plutovg_surface_destroy(surface);
    svg_end_render_env(
#if defined(__x86_64__)
        &render_env
#else
        NULL
#endif
    );
    return img;
}

svg_image_t* svg_load(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[libsvg] fopen failed file=%s\n", filename ? filename : "(null)");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fprintf(stderr, "[libsvg] invalid file size file=%s size=%ld\n",
                filename ? filename : "(null)", file_size);
        fclose(fp);
        return NULL;
    }

    uint8_t* data = (uint8_t*)malloc(file_size);
    if (data == NULL) {
        fprintf(stderr, "[libsvg] alloc file buffer failed file=%s size=%ld\n",
                filename ? filename : "(null)", file_size);
        fclose(fp);
        return NULL;
    }

    if (fread(data, 1, file_size, fp) != (size_t)file_size) {
        fprintf(stderr, "[libsvg] fread short read file=%s size=%ld\n",
                filename ? filename : "(null)", file_size);
        free(data);
        fclose(fp);
        return NULL;
    }

    fclose(fp);

    svg_image_t* img = svg_load_from_memory(data, file_size);
    free(data);
    return img;
}

void svg_free(svg_image_t* img) {
    if (img) {
        if (img->pixels) {
            free(img->pixels);
        }
        free(img);
    }
}
