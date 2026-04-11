#include "graph/graph_gif.h"
#include <stdio.h>
#include <gif_lib.h>

#ifdef __cplusplus
extern "C" {
#endif

static int next_row[4096];

static int load_gif_init(GifFileType* gf, GifByteType** p)
{
    (void)gf;
    *p = (GifByteType*)next_row;
    return 1;
}

graph_t* gif_image_new(const char* filename)
{
    graph_t* g = NULL;
    GifFileType* gf = NULL;
    GifRecordType rec_type;
    GifByteType* p;
    int transp_index = -1;
    int delay_time = 0;
    int frame_count = 0;
    int width, height;
    ColorMapObject* cmap = NULL;
    GifImageDesc* img_desc = NULL;

    int error;
    gf = DGifOpenFileName(filename, &error);
    if (gf == NULL) {
        return NULL;
    }

    do {
        if (DGifGetRecordType(gf, &rec_type) == GIF_ERROR) {
            break;
        }

        if (rec_type == IMAGE_DESC_RECORD_TYPE) {
            if (DGifGetImageDesc(gf) == GIF_ERROR) {
                break;
            }

            img_desc = &gf->Image;
            width = img_desc->Width;
            height = img_desc->Height;

            if (g == NULL) {
                g = graph_new(NULL, width, height);
                if (g == NULL) {
                    break;
                }
                graph_clear(g, 0);
            }

            cmap = img_desc->ColorMap ? img_desc->ColorMap : gf->SColorMap;

            int interlaced = img_desc->Interlace;
            int pass = 0, i;
            for (i = 0; i < img_desc->Height; i++) {
                int row;
                if (interlaced) {
                    if (pass == 0) row = img_desc->Top + (i % img_desc->Height);
                    else if (pass == 1) row = img_desc->Top + (4 + (i - 4) % 8);
                    else if (pass == 2) row = img_desc->Top + (2 + (i - 2) % 4);
                    else row = img_desc->Top + (1 + (i - 1) % 2);
                    if (i == 0) pass++;
                } else {
                    row = img_desc->Top + i;
                }

                if (DGifGetLine(gf, next_row, img_desc->Width) == GIF_ERROR) {
                    break;
                }

                uint32_t* dst = (uint32_t*)g->buffer + row * width;
                for (int col = 0; col < img_desc->Width; col++) {
                    int index = next_row[col];
                    if (index != transp_index && cmap && index < cmap->ColorCount) {
                        GifColorType* c = &cmap->Colors[index];
                        dst[img_desc->Left + col] = (255 << 24) | (c->Red << 16) | (c->Green << 8) | c->Blue;
                    }
                }
            }
            frame_count++;
        }
        else if (rec_type == EXTENSION_RECORD_TYPE) {
            int ext_code;
            GifByteType* ext = NULL;
            if (DGifGetExtension(gf, &ext_code, &ext) == GIF_ERROR) {
                break;
            }

            if (ext_code == GRAPHICS_EXT_FUNC_CODE && ext) {
                unsigned char* p = ext + 1;
                int disposal_method = (p[0] >> 2) & 0x07;
                int delay = (p[2] | (p[3] << 8)) * 10;
                int transparent = (p[0] & 0x01) ? -1 : -1;

                if (p[0] & 0x01) {
                    transp_index = p[3];
                } else {
                    transp_index = -1;
                }
                (void)disposal_method;
                (void)delay;
            }

            while (ext != NULL) {
                if (DGifGetExtensionNext(gf, &ext) == GIF_ERROR) {
                    break;
                }
            }
        }
    } while (rec_type != TERMINATE_RECORD_TYPE);

    DGifCloseFile(gf, &error);

    if (g != NULL) {
        graph_reverse_rgb(g);
    }

    return g;
}

int gif_get_frame_count(const char* filename)
{
    GifFileType* gf = NULL;
    GifRecordType rec_type;
    int frame_count = 0;
    int error;

    gf = DGifOpenFileName(filename, &error);
    if (gf == NULL) {
        return 0;
    }

    do {
        if (DGifGetRecordType(gf, &rec_type) == GIF_ERROR) {
            break;
        }
        if (rec_type == IMAGE_DESC_RECORD_TYPE) {
            frame_count++;
            if (DGifGetImageDesc(gf) == GIF_ERROR) {
                break;
            }
            int row, total_rows = gf->Image.Height;
            if (gf->Image.Interlace) {
                total_rows = 0;
            }
            for (row = 0; row < total_rows; row++) {
                if (DGifGetLine(gf, next_row, gf->Image.Width) == GIF_ERROR) {
                    break;
                }
            }
        }
        else if (rec_type == EXTENSION_RECORD_TYPE) {
            int ext_code;
            GifByteType* ext = NULL;
            if (DGifGetExtension(gf, &ext_code, &ext) == GIF_ERROR) {
                break;
            }
            while (ext != NULL) {
                if (DGifGetExtensionNext(gf, &ext) == GIF_ERROR) {
                    break;
                }
            }
        }
    } while (rec_type != TERMINATE_RECORD_TYPE);

    DGifCloseFile(gf, &error);
    return frame_count;
}

#ifdef __cplusplus
}
#endif
