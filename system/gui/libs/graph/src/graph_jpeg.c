#include "graph/graph_jpeg.h"
#include <stdio.h>
#include <setjmp.h>
#include <jpeglib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr* my_error_ptr;

static void my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

graph_t* jpeg_image_new(const char* filename)
{
    graph_t* g = NULL;
    FILE* infile;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;
    int width, height;
    int i, j;

    if ((infile = fopen(filename, "rb")) == NULL) {
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        if (infile != NULL)
            fclose(infile);
        if (g != NULL)
            graph_free(g);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);

    (void)jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    (void)jpeg_start_decompress(&cinfo);

    width = cinfo.output_width;
    height = cinfo.output_height;
    row_stride = cinfo.output_width * cinfo.output_components;

    g = graph_new(NULL, width, height);
    if (g == NULL) {
        jpeg_destroy_decompress(&cinfo);
        fclose(infile);
        return NULL;
    }

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    int row = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        (void)jpeg_read_scanlines(&cinfo, buffer, 1);

        uint8_t* dst = (uint8_t*)g->buffer + row * (width * 4);
        uint8_t* src = buffer[0];

        for (i = 0, j = 0; i < width; i++, j += 3) {
            dst[i * 4 + 0] = src[j + 0];  // R
            dst[i * 4 + 1] = src[j + 1];  // G
            dst[i * 4 + 2] = src[j + 2];  // B
            dst[i * 4 + 3] = 255;         // A
        }
        row++;
    }

    (void)jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    graph_reverse_rgb(g);

    return g;
}

#ifdef __cplusplus
}
#endif
