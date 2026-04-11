#if !defined(GRAPH_GIF_H)
#define GRAPH_GIF_H

#include <graph/graph.h>
#ifdef __cplusplus
extern "C" {
#endif

graph_t* gif_image_new(const char* filename);
int gif_get_frame_count(const char* filename);

#ifdef __cplusplus
}
#endif

#endif
