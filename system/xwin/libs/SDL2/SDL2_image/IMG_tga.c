/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2013 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* TGA image loading for SDL_image */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_image.h"

/* TGA header structure */
typedef struct {
    Uint8  id_length;         /* ID length */
    Uint8  color_map_type;    /* Color map type (0=no colormap) */
    Uint8  image_type;        /* Image type */
    Uint8  color_map_spec[5]; /* Color map specification */
    Uint16 x_origin;          /* X origin */
    Uint16 y_origin;          /* Y origin */
    Uint16 width;             /* Width */
    Uint16 height;            /* Height */
    Uint8  pixel_depth;       /* Bits per pixel */
    Uint8  image_descriptor;  /* Image descriptor */
} __attribute__((packed)) TGAHeader;

/* Helper to read little-endian Uint16 */
static Uint16 ReadLE16(const Uint8* data) {
    return data[0] | (data[1] << 8);
}

SDL_Surface *IMG_LoadTGA_RW(SDL_RWops *src)
{
    Sint64 start;
    TGAHeader header;
    SDL_Surface *surface = NULL;
    Uint8 *pixels = NULL;
    int rle = 0;
    int bytes_per_pixel;
    int num_pixels;
    int pixels_read = 0;
    Uint8 pixel_data[4];
    Uint8 *row;
    int y;
    
    if (!src) {
        return NULL;
    }
    
    start = SDL_RWtell(src);
    
    /* Read header */
    if (SDL_RWread(src, &header, sizeof(header), 1) != 1) {
        return NULL;
    }
    
    /* Check image type: 2=uncompressed true-color, 10=RLE true-color */
    if (header.image_type != 2 && header.image_type != 10) {
        return NULL;
    }
    
    rle = (header.image_type == 10);
    
    /* Skip ID field */
    if (header.id_length > 0) {
        SDL_RWseek(src, header.id_length, RW_SEEK_CUR);
    }
    
    /* Skip color map if present */
    if (header.color_map_type != 0) {
        Uint16 color_map_length = ReadLE16(&header.color_map_spec[2]);
        Uint8 color_map_entry_size = header.color_map_spec[4];
        SDL_RWseek(src, (color_map_length * color_map_entry_size + 7) / 8, RW_SEEK_CUR);
    }
    
    bytes_per_pixel = (header.pixel_depth + 7) / 8;
    num_pixels = header.width * header.height;
    
    /* Create SDL surface */
    surface = SDL_CreateRGBSurface(0, header.width, header.height, 
                                   32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
    if (!surface) {
        return NULL;
    }
    
    pixels = (Uint8*)surface->pixels;
    
    /* Read pixel data */
    while (pixels_read < num_pixels) {
        Uint32 count;
        
        if (rle) {
            /* RLE packet */
            Uint8 packet_header;
            if (SDL_RWread(src, &packet_header, 1, 1) != 1) {
                SDL_FreeSurface(surface);
                return NULL;
            }
            
            count = (packet_header & 0x7F) + 1;
            int rle_packet = (packet_header & 0x80) != 0;
            
            if (rle_packet) {
                /* RLE packet: read one pixel, repeat count times */
                if (SDL_RWread(src, pixel_data, bytes_per_pixel, 1) != 1) {
                    SDL_FreeSurface(surface);
                    return NULL;
                }
                
                for (Uint32 i = 0; i < count && pixels_read < num_pixels; i++) {
                    Uint8 b = pixel_data[0];
                    Uint8 g = pixel_data[1];
                    Uint8 r = pixel_data[2];
                    Uint8 a = (bytes_per_pixel >= 4) ? pixel_data[3] : 0xFF;
                    Uint32 *p = (Uint32*)(pixels + pixels_read * 4);
                    *p = (a << 24) | (r << 16) | (g << 8) | b;
                    pixels_read++;
                }
            } else {
                /* Raw packet: read count pixels */
                for (Uint32 i = 0; i < count && pixels_read < num_pixels; i++) {
                    if (SDL_RWread(src, pixel_data, bytes_per_pixel, 1) != 1) {
                        SDL_FreeSurface(surface);
                        return NULL;
                    }
                    Uint8 b = pixel_data[0];
                    Uint8 g = pixel_data[1];
                    Uint8 r = pixel_data[2];
                    Uint8 a = (bytes_per_pixel >= 4) ? pixel_data[3] : 0xFF;
                    Uint32 *p = (Uint32*)(pixels + pixels_read * 4);
                    *p = (a << 24) | (r << 16) | (g << 8) | b;
                    pixels_read++;
                }
            }
        } else {
            /* Uncompressed: read one pixel at a time */
            if (SDL_RWread(src, pixel_data, bytes_per_pixel, 1) != 1) {
                SDL_FreeSurface(surface);
                return NULL;
            }
            Uint8 b = pixel_data[0];
            Uint8 g = pixel_data[1];
            Uint8 r = pixel_data[2];
            Uint8 a = (bytes_per_pixel >= 4) ? pixel_data[3] : 0xFF;
            Uint32 *p = (Uint32*)(pixels + pixels_read * 4);
            *p = (a << 24) | (r << 16) | (g << 8) | b;
            pixels_read++;
        }
    }
    
    /* Handle origin: TGA can have origin at top-left or bottom-left */
    int origin_top = (header.image_descriptor & 0x20) != 0;
    if (!origin_top) {
        /* Flip vertically */
        Uint8 *temp_row = (Uint8*)malloc(header.width * 4);
        if (temp_row) {
            for (y = 0; y < header.height / 2; y++) {
                Uint8 *row1 = pixels + y * surface->pitch;
                Uint8 *row2 = pixels + (header.height - 1 - y) * surface->pitch;
                memcpy(temp_row, row1, header.width * 4);
                memcpy(row1, row2, header.width * 4);
                memcpy(row2, temp_row, header.width * 4);
            }
            free(temp_row);
        }
    }
    
    return surface;
}
