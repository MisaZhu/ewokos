#include "svg.h"
#include "plutosvg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

svg_image_t* svg_load_from_memory(const uint8_t* data, uint32_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }

    // 1. 使用 plutosvg 加载 SVG 文档
    plutosvg_document_t* doc = plutosvg_document_load_from_data((const char*)data, size, -1.0f, -1.0f, NULL, NULL);
    if (doc == NULL) {
        return NULL;
    }

    // 2. 获取 SVG 原始尺寸
    int width = (int)plutosvg_document_get_width(doc);
    int height = (int)plutosvg_document_get_height(doc);

    if (width <= 0 || height <= 0) {
        plutosvg_document_destroy(doc);
        return NULL;
    }

    // 3. 将 SVG 渲染到 plutovg surface
    plutovg_surface_t* surface = plutosvg_document_render_to_surface(doc, NULL, width, height, NULL, NULL, NULL);
    plutosvg_document_destroy(doc);

    if (surface == NULL) {
        return NULL;
    }

    int surf_width = plutovg_surface_get_width(surface);
    int surf_height = plutovg_surface_get_height(surface);

    // 4. 分配输出图像结构体
    svg_image_t* img = (svg_image_t*)malloc(sizeof(svg_image_t));
    if (img == NULL) {
        plutovg_surface_destroy(surface);
        return NULL;
    }

    img->width = (uint32_t)surf_width;
    img->height = (uint32_t)surf_height;

    size_t pixel_bytes = (size_t)surf_width * surf_height * sizeof(uint32_t);
    img->pixels = (uint32_t*)malloc(pixel_bytes);
    if (img->pixels == NULL) {
        free(img);
        plutovg_surface_destroy(surface);
        return NULL;
    }

    // 5. 从 surface 拷贝像素数据到输出缓冲区
    const uint32_t* src_pixels = plutovg_surface_get_data(surface);
    memcpy(img->pixels, src_pixels, pixel_bytes);

    plutovg_surface_destroy(surface);
    return img;
}

svg_image_t* svg_load(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0) {
        fclose(fp);
        return NULL;
    }

    uint8_t* data = (uint8_t*)malloc(file_size);
    if (data == NULL) {
        fclose(fp);
        return NULL;
    }

    if (fread(data, 1, file_size, fp) != (size_t)file_size) {
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
