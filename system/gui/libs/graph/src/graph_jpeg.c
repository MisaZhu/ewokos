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

// Structure to hold memory buffer for JPEG reading from memory
struct jpeg_memory_buffer {
    struct jpeg_source_mgr pub;
    const uint8_t* data;
    size_t size;
    size_t pos;
};

static void jpeg_memory_init_source(j_decompress_ptr cinfo) {
    (void)cinfo;
}

static boolean jpeg_memory_fill_input_buffer(j_decompress_ptr cinfo) {
    struct jpeg_memory_buffer* buffer = (struct jpeg_memory_buffer*)cinfo->src;
    // Return FALSE to indicate end of data
    buffer->pub.next_input_byte = (JOCTET*)buffer->data;
    buffer->pub.bytes_in_buffer = 0;
    return FALSE;
}

static void jpeg_memory_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    struct jpeg_memory_buffer* buffer = (struct jpeg_memory_buffer*)cinfo->src;
    if (num_bytes > 0) {
        if ((size_t)num_bytes > buffer->pub.bytes_in_buffer) {
            buffer->pub.bytes_in_buffer = 0;
        } else {
            buffer->pub.next_input_byte += num_bytes;
            buffer->pub.bytes_in_buffer -= num_bytes;
        }
    }
}

static void jpeg_memory_term_source(j_decompress_ptr cinfo) {
    (void)cinfo;
}

static void jpeg_memory_set_source(j_decompress_ptr cinfo, const uint8_t* data, size_t size) {
    struct jpeg_memory_buffer* buffer;
    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr*)(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(struct jpeg_memory_buffer));
    }
    buffer = (struct jpeg_memory_buffer*)cinfo->src;
    buffer->pub.init_source = jpeg_memory_init_source;
    buffer->pub.fill_input_buffer = jpeg_memory_fill_input_buffer;
    buffer->pub.skip_input_data = jpeg_memory_skip_input_data;
    buffer->pub.resync_to_restart = jpeg_resync_to_restart;
    buffer->pub.term_source = jpeg_memory_term_source;
    buffer->pub.bytes_in_buffer = size;
    buffer->pub.next_input_byte = (JOCTET*)data;
    buffer->data = data;
    buffer->size = size;
    buffer->pos = 0;
}

graph_t* jpeg_image_new_from_data(const uint8_t* data, uint32_t size) {
    graph_t* g = NULL;
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;
    int width, height;
    int i, j;

    if (data == NULL || size == 0) {
        return NULL;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        if (g != NULL)
            graph_free(g);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_memory_set_source(&cinfo, data, size);

    (void)jpeg_read_header(&cinfo, TRUE);

    cinfo.out_color_space = JCS_RGB;
    (void)jpeg_start_decompress(&cinfo);

    width = cinfo.output_width;
    height = cinfo.output_height;
    row_stride = cinfo.output_width * cinfo.output_components;

    g = graph_new(NULL, width, height);
    if (g == NULL) {
        jpeg_destroy_decompress(&cinfo);
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

    graph_reverse_rgb(g);

    return g;
}

#ifdef __cplusplus
}
#endif
