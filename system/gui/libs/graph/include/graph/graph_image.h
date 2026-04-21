#if !defined(GRAPH_IMAGE_H)
#define GRAPH_IMAGE_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

enum graph_image_type {
    GRAPH_IMAGE_TYPE_AUTO = 0,
    GRAPH_IMAGE_TYPE_PNG,
    GRAPH_IMAGE_TYPE_JPEG,
    GRAPH_IMAGE_TYPE_TGA,
    GRAPH_IMAGE_TYPE_GIF,
    GRAPH_IMAGE_TYPE_SVG
};

typedef struct graph_image graph_image_t;

graph_t* graph_image_new(const char* filename);
graph_t* graph_image_new_from_data(enum graph_image_type type, const uint8_t* data, uint32_t size);

graph_t* graph_image_new_bg(const char* filename, uint32_t bg_color);

#ifdef __cplusplus
}
#endif

#endif
