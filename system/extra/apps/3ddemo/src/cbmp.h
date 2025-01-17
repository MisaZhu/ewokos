#ifndef CBMP_CBMP_H
#define CBMP_CBMP_H

#ifdef __cplusplus
extern "C" {
#endif

// Pixel structure
// Not meant to be edited directly
// Please use the API

typedef struct pixel_data
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
} pixel;

// BMP structure
// Not meant to be edited directly
// Please use the API

typedef struct BMP_data
{
    unsigned int file_byte_number;
    unsigned char* file_byte_contents;

    unsigned int pixel_array_start;

    unsigned int width;
    unsigned int height;
    unsigned int depth;

    pixel* pixels;
} BMP;

// Public function declarations

BMP* bopen(char* file_path);
BMP* b_deep_copy(BMP* to_copy);
int get_width(BMP* bmp);
int get_height(BMP* bmp);
unsigned int get_depth(BMP* bmp);
void get_pixel_rgb(BMP* bmp, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b);
void set_pixel_rgb(BMP* bmp, int x, int y, unsigned char r, unsigned char g, unsigned char b);
void bwrite(BMP* bmp, char* file_name);
void bclose(BMP* bmp);

#ifdef __cplusplus
}
#endif

#endif // CBMP_CBMP_H
