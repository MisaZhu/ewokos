#include <graph/graph_svg.h>
#include <svg.h>
#include <stdlib.h>
#include <string.h>

graph_t* svg_image_new(const char* filename) {
    svg_image_t* svg = svg_load(filename);
    if (svg == NULL)
        return NULL;

    graph_t* g = graph_new(NULL, svg->width, svg->height);
    if (g == NULL) {
        svg_free(svg);
        return NULL;
    }

    memcpy(g->buffer, svg->pixels, svg->width * svg->height * sizeof(uint32_t));

    svg_free(svg);

    return g;
}

graph_t* svg_image_new_from_data(const uint8_t* data, uint32_t size) {
    if (data == NULL || size == 0) {
        return NULL;
    }

    svg_image_t* svg = svg_load_from_memory(data, size);
    if (svg == NULL)
        return NULL;

    graph_t* g = graph_new(NULL, svg->width, svg->height);
    if (g == NULL) {
        svg_free(svg);
        return NULL;
    }

    memcpy(g->buffer, svg->pixels, svg->width * svg->height * sizeof(uint32_t));

    svg_free(svg);

    return g;
}
