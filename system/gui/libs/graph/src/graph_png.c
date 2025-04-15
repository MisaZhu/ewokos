#include "graph/graph_png.h"
#include <png.h>

#ifdef __cplusplus
extern "C" {
#endif


static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	FILE* src = png_get_io_ptr(ctx);
    fread(area, size, 1, src);
}

graph_t* png_image_new(const char* fname) {
    const char *error;
    png_structp png_ptr;
    png_infop info_ptr;
    png_uint_32 width, height;
    int bit_depth, color_type, interlace_type, num_channels;
    uint32_t Rmask;
    uint32_t Gmask;
    uint32_t Bmask;
    uint32_t Amask;
    png_bytep *volatile row_pointers;
    int row, i;
    int ckey = -1;
    png_color_16 *transv;

	FILE* src = fopen(fname, "rb");
    if ( !src ) {
        /* The error message has been set in SDL_RWFromFile */
        return NULL;
    }

    /* Initialize the data we will clean up when we're done */
    png_ptr = NULL; info_ptr = NULL; row_pointers = NULL;

    /* Create the PNG loading context structure */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                      NULL,NULL,NULL);
    if (png_ptr == NULL){
        goto done;
    }

     /* Allocate/initialize the memory for image information.  REQUIRED. */
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        goto done;
    }

    /* Set error handling if you are using setjmp/longjmp method (this is
     * the normal method of doing things with libpng).  REQUIRED unless you
     * set up your own error handlers in png_create_read_struct() earlier.
     */
#ifndef LIBPNG_VERSION_12
    if ( setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof (jmp_buf))) )
#else
    if ( setjmp(png_ptr->jmpbuf) )
#endif
    {
        goto done;
    }

    /* Set up the input control */
    png_set_read_fn(png_ptr, src, png_read_data);

    /* Read PNG header info */
    png_read_info(png_ptr, info_ptr);
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
            &color_type, &interlace_type, NULL, NULL);

    /* tell libpng to strip 16 bit/color files down to 8 bits/color */
    png_set_strip_16(png_ptr) ;

    /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
     * byte into separate bytes (useful for paletted and grayscale images).
     */
    png_set_packing(png_ptr);

    /* scale greyscale values to the range 0..255 */
    if (color_type == PNG_COLOR_TYPE_GRAY)
        png_set_expand(png_ptr);

    /* For images with a single "transparent colour", set colour key;
       if more than one index has transparency, or if partially transparent
       entries exist, use full alpha channel */
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        int num_trans;
        uint8_t *trans;
        png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &transv);
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            /* Check if all tRNS entries are opaque except one */
            int j, t = -1;
            for (j = 0; j < num_trans; j++) {
                if (trans[j] == 0) {
                    if (t >= 0) {
                        break;
                    }
                    t = j;
                } else if (trans[j] != 255) {
                    break;
                }
            }
            if (j == num_trans) {
                /* exactly one transparent index */
                ckey = t;
            } else {
                /* more than one transparent index, or translucency */
                png_set_expand(png_ptr);
            }
        } else {
            ckey = 0; /* actual value will be set later */
        }
    }

    if ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
        png_set_gray_to_rgb(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
            &color_type, &interlace_type, NULL, NULL);

    /* Allocate the SDL surface to hold the image */
    Rmask = Gmask = Bmask = Amask = 0 ;
    num_channels = png_get_channels(png_ptr, info_ptr);
    if ( num_channels < 3 ) {
		goto done;
	}

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
	Rmask = 0x000000FF;
	Gmask = 0x0000FF00;
	Bmask = 0x00FF0000;
	Amask = (num_channels == 4) ? 0xFF000000 : 0;
#else
	int s = (num_channels == 4) ? 0 : 8;
	Rmask = 0xFF000000 >> s;
	Gmask = 0x00FF0000 >> s;
	Bmask = 0x0000FF00 >> s;
	Amask = 0x000000FF >> s;
#endif
	graph_t* g = graph_new(NULL, width, height);
	if(g == NULL) {
		goto done;
	}

    /* Create the array of pointers to image data */
    row_pointers = (png_bytep*) malloc(sizeof(png_bytep)*height);
    if ( (row_pointers == NULL) ) {
        goto done;
    }

    for (row = 0; row < (int)height; row++) {
        row_pointers[row] = (png_bytep)
                (uint8_t *)g->buffer + row*(g->w*4);
    }

    /* Read the entire image in one go */
    png_read_image(png_ptr, row_pointers);

    /* and we're done!  (png_read_end() can be omitted if no processing of
     * post-IDAT text/time/etc. is desired)
     * In some cases it can't read PNG's created by some popular programs (ACDSEE),
     * we do not want to process comments, so we omit png_read_end

    lib.png_read_end(png_ptr, info_ptr);
    */
	graph_reverse_rgb(g);

done:   /* Clean up and return */
	if(src != NULL)
		fclose(src);

    if ( png_ptr ) {
        png_destroy_read_struct(&png_ptr,
                                info_ptr ? &info_ptr : (png_infopp)0,
                                (png_infopp)0);
    }
    if ( row_pointers ) {
        free(row_pointers);
    }

    return g;
}

graph_t* png_image_new_bg(const char* filename, uint32_t bg_color) {
	graph_t* png = png_image_new(filename);
	if(png == NULL)
		return NULL;
	graph_t* ret = graph_new(NULL, png->w, png->h);
	graph_clear(ret, bg_color);
	graph_blt_alpha(png, 0, 0, png->w, png->h, ret, 0, 0, png->w, png->h, 0xff);
	graph_free(png);
	return ret;

}

#ifdef __cplusplus
}
#endif

