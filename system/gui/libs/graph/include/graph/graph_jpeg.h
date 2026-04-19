#if !defined(GRAPH_JPEG_H)
#define GRAPH_JPEG_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

graph_t* jpeg_image_new(const char* filename);
graph_t* jpeg_image_new_from_data(const uint8_t* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
