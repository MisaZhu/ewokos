#ifndef TGA_H
#define TGA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TGA image structure */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t bpp;        /* bits per pixel: 24 or 32 */
    uint32_t* pixels;    /* RGBA pixel data */
} tga_image_t;

/* Load TGA image from file */
tga_image_t* tga_load(const char* filename);

/* Free TGA image */
void tga_free(tga_image_t* img);

#ifdef __cplusplus
}
#endif

#endif /* TGA_H */
