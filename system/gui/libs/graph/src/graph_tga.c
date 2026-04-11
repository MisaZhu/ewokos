#include <graph/graph_tga.h>
#include <tga.h>
#include <stdlib.h>
#include <string.h>

graph_t* tga_image_new(const char* filename) {
    tga_image_t* tga = tga_load(filename);
    if (tga == NULL)
        return NULL;

    // Use graph_new to create a properly allocated graph_t
    graph_t* g = graph_new(NULL, tga->width, tga->height);
    if (g == NULL) {
        tga_free(tga);
        return NULL;
    }

    // Copy pixel data from TGA to graph
    memcpy(g->buffer, tga->pixels, tga->width * tga->height * sizeof(uint32_t));

    // Free TGA (this frees tga->pixels with standard free)
    tga_free(tga);
    
    return g;
}
