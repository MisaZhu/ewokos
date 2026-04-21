#if !defined(GRAPH_SVG_H)
#define GRAPH_SVG_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

graph_t* svg_image_new(const char* filename);
graph_t* svg_image_new_from_data(const uint8_t* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif