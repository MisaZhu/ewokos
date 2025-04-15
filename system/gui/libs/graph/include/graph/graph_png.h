#if !defined(GRAPH_PNG_H)
#define GRAPH_PNG_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

graph_t* png_image_new(const char* filename);
graph_t* png_image_new_bg(const char* filename, uint32_t bg_color);

#ifdef __cplusplus
}
#endif

#endif /*defined(UPNG_H)*/
