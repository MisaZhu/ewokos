#include "tga.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TGA header structure */
typedef struct {
    uint8_t  id_length;         /* ID length */
    uint8_t  color_map_type;    /* Color map type (0=no colormap) */
    uint8_t  image_type;        /* Image type */
    uint8_t  color_map_spec[5]; /* Color map specification */
    uint16_t x_origin;          /* X origin */
    uint16_t y_origin;          /* Y origin */
    uint16_t width;             /* Width */
    uint16_t height;            /* Height */
    uint8_t  pixel_depth;       /* Bits per pixel */
    uint8_t  image_descriptor;  /* Image descriptor */
} __attribute__((packed)) tga_header_t;

/* Helper to read little-endian uint16_t */
static uint16_t read_u16_le(const uint8_t* data) {
    return data[0] | (data[1] << 8);
}

/* Load TGA image from file */
tga_image_t* tga_load(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    /* Read header */
    tga_header_t header;
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fclose(fp);
        return NULL;
    }

    /* Check image type: 2=uncompressed true-color, 10=RLE true-color */
    if (header.image_type != 2 && header.image_type != 10) {
        fclose(fp);
        return NULL;
    }

    /* Skip ID field */
    if (header.id_length > 0) {
        fseek(fp, header.id_length, SEEK_CUR);
    }

    /* Skip color map if present */
    if (header.color_map_type != 0) {
        uint16_t color_map_length = read_u16_le(&header.color_map_spec[2]);
        uint8_t color_map_entry_size = header.color_map_spec[4];
        fseek(fp, (color_map_length * color_map_entry_size + 7) / 8, SEEK_CUR);
    }

    /* Allocate image structure */
    tga_image_t* img = (tga_image_t*)malloc(sizeof(tga_image_t));
    if (!img) {
        fclose(fp);
        return NULL;
    }

    img->width = header.width;
    img->height = header.height;
    img->bpp = header.pixel_depth;
    img->pixels = (uint32_t*)malloc(img->width * img->height * sizeof(uint32_t));
    if (!img->pixels) {
        free(img);
        fclose(fp);
        return NULL;
    }

    /* Determine pixel format */
    bool rle = (header.image_type == 10);
    uint32_t pixel_bytes = (header.pixel_depth + 7) / 8;
    
    /* Read pixel data */
    uint32_t num_pixels = img->width * img->height;
    uint32_t pixels_read = 0;
    
    while (pixels_read < num_pixels) {
        uint32_t count;
        uint8_t pixel_data[4];
        
        if (rle) {
            /* RLE packet */
            uint8_t packet_header;
            if (fread(&packet_header, 1, 1, fp) != 1) {
                tga_free(img);
                fclose(fp);
                return NULL;
            }
            
            count = (packet_header & 0x7F) + 1;
            bool rle_packet = (packet_header & 0x80) != 0;
            
            if (rle_packet) {
                /* RLE packet: read one pixel, repeat count times */
                if (fread(pixel_data, 1, pixel_bytes, fp) != pixel_bytes) {
                    tga_free(img);
                    fclose(fp);
                    return NULL;
                }
                
                for (uint32_t i = 0; i < count && pixels_read < num_pixels; i++) {
                    uint8_t b = pixel_data[0];
                    uint8_t g = pixel_data[1];
                    uint8_t r = pixel_data[2];
                    uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
                    img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            } else {
                /* Raw packet: read count pixels */
                for (uint32_t i = 0; i < count && pixels_read < num_pixels; i++) {
                    if (fread(pixel_data, 1, pixel_bytes, fp) != pixel_bytes) {
                        tga_free(img);
                        fclose(fp);
                        return NULL;
                    }
                    uint8_t b = pixel_data[0];
                    uint8_t g = pixel_data[1];
                    uint8_t r = pixel_data[2];
                    uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
                    img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
        } else {
            /* Uncompressed: read one pixel at a time */
            if (fread(pixel_data, 1, pixel_bytes, fp) != pixel_bytes) {
                tga_free(img);
                fclose(fp);
                return NULL;
            }
            uint8_t b = pixel_data[0];
            uint8_t g = pixel_data[1];
            uint8_t r = pixel_data[2];
            uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
            img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
            count = 1;
        }
    }

    fclose(fp);

    /* Handle origin: TGA can have origin at top-left or bottom-left */
    bool origin_top = (header.image_descriptor & 0x20) != 0;
    if (!origin_top) {
        /* Flip vertically */
        for (uint32_t y = 0; y < img->height / 2; y++) {
            for (uint32_t x = 0; x < img->width; x++) {
                uint32_t idx1 = y * img->width + x;
                uint32_t idx2 = (img->height - 1 - y) * img->width + x;
                uint32_t tmp = img->pixels[idx1];
                img->pixels[idx1] = img->pixels[idx2];
                img->pixels[idx2] = tmp;
            }
        }
    }

    return img;
}

/* Load TGA image from memory */
tga_image_t* tga_load_from_memory(const uint8_t* data, uint32_t size) {
    if (data == NULL || size < sizeof(tga_header_t)) {
        return NULL;
    }

    /* Read header */
    tga_header_t header;
    memcpy(&header, data, sizeof(header));

    /* Check image type: 2=uncompressed true-color, 10=RLE true-color */
    if (header.image_type != 2 && header.image_type != 10) {
        return NULL;
    }

    /* Calculate data offset */
    uint32_t offset = sizeof(header);

    /* Skip ID field */
    if (header.id_length > 0) {
        offset += header.id_length;
    }

    /* Skip color map if present */
    if (header.color_map_type != 0) {
        uint16_t color_map_length = read_u16_le(&header.color_map_spec[2]);
        uint8_t color_map_entry_size = header.color_map_spec[4];
        offset += (color_map_length * color_map_entry_size + 7) / 8;
    }

    if (offset >= size) {
        return NULL;
    }

    /* Allocate image structure */
    tga_image_t* img = (tga_image_t*)malloc(sizeof(tga_image_t));
    if (!img) {
        return NULL;
    }

    img->width = header.width;
    img->height = header.height;
    img->bpp = header.pixel_depth;
    img->pixels = (uint32_t*)malloc(img->width * img->height * sizeof(uint32_t));
    if (!img->pixels) {
        free(img);
        return NULL;
    }

    /* Determine pixel format */
    bool rle = (header.image_type == 10);
    uint32_t pixel_bytes = (header.pixel_depth + 7) / 8;

    /* Read pixel data */
    uint32_t num_pixels = img->width * img->height;
    uint32_t pixels_read = 0;
    const uint8_t* ptr = data + offset;
    const uint8_t* end = data + size;

    while (pixels_read < num_pixels && ptr < end) {
        uint32_t count;
        uint8_t pixel_data[4];

        if (rle) {
            /* RLE packet */
            if (ptr >= end) {
                tga_free(img);
                return NULL;
            }
            uint8_t packet_header = *ptr++;

            count = (packet_header & 0x7F) + 1;
            bool rle_packet = (packet_header & 0x80) != 0;

            if (rle_packet) {
                /* RLE packet: read one pixel, repeat count times */
                if (ptr + pixel_bytes > end) {
                    tga_free(img);
                    return NULL;
                }
                memcpy(pixel_data, ptr, pixel_bytes);
                ptr += pixel_bytes;

                for (uint32_t i = 0; i < count && pixels_read < num_pixels; i++) {
                    uint8_t b = pixel_data[0];
                    uint8_t g = pixel_data[1];
                    uint8_t r = pixel_data[2];
                    uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
                    img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            } else {
                /* Raw packet: read count pixels */
                for (uint32_t i = 0; i < count && pixels_read < num_pixels; i++) {
                    if (ptr + pixel_bytes > end) {
                        tga_free(img);
                        return NULL;
                    }
                    memcpy(pixel_data, ptr, pixel_bytes);
                    ptr += pixel_bytes;
                    uint8_t b = pixel_data[0];
                    uint8_t g = pixel_data[1];
                    uint8_t r = pixel_data[2];
                    uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
                    img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
        } else {
            /* Uncompressed: read one pixel at a time */
            if (ptr + pixel_bytes > end) {
                tga_free(img);
                return NULL;
            }
            memcpy(pixel_data, ptr, pixel_bytes);
            ptr += pixel_bytes;
            uint8_t b = pixel_data[0];
            uint8_t g = pixel_data[1];
            uint8_t r = pixel_data[2];
            uint8_t a = (pixel_bytes >= 4) ? pixel_data[3] : 0xFF;
            img->pixels[pixels_read++] = (a << 24) | (r << 16) | (g << 8) | b;
            count = 1;
        }
    }

    /* Handle origin: TGA can have origin at top-left or bottom-left */
    bool origin_top = (header.image_descriptor & 0x20) != 0;
    if (!origin_top) {
        /* Flip vertically */
        for (uint32_t y = 0; y < img->height / 2; y++) {
            for (uint32_t x = 0; x < img->width; x++) {
                uint32_t idx1 = y * img->width + x;
                uint32_t idx2 = (img->height - 1 - y) * img->width + x;
                uint32_t tmp = img->pixels[idx1];
                img->pixels[idx1] = img->pixels[idx2];
                img->pixels[idx2] = tmp;
            }
        }
    }

    return img;
}

/* Free TGA image */
void tga_free(tga_image_t* img) {
    if (img) {
        if (img->pixels) {
            free(img->pixels);
        }
        free(img);
    }
}
