#ifndef SVG_H
#define SVG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;
} svg_image_t;

svg_image_t* svg_load(const char* filename);

svg_image_t* svg_load_from_memory(const uint8_t* data, uint32_t size);

void svg_free(svg_image_t* img);

#ifdef __cplusplus
}
#endif

#endif