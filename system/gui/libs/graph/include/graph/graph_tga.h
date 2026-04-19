#if !defined(GRAPH_TGA_H)
#define GRAPH_TGA_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

graph_t* tga_image_new(const char* filename);
graph_t* tga_image_new_from_data(const uint8_t* data, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
