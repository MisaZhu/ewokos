#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

void bitmap_free(bitmap_t* b) {
	if(b == NULL)
		return;
	if(b->buffer != NULL)
		free(b->buffer);
	free(b);
}

bitmap_t* graph_to_bitmap(graph_t* g) {
	int32_t bw = g->w / 8;
	uint8_t* p = (uint8_t*)malloc(bw * g->h);
	memset(p, 0, bw * g->h);

	for(int32_t j=0; j<g->h; j++) {
		for(int32_t i=0; i<bw; i++) {
			uint32_t at = j*bw + i;
			for(uint8_t b=0; b<8; b++) {
				uint32_t c = g->buffer[j*g->w + i*8 + b];
				if((c & 0x00ffffff) != 0)
					p[at] |= 1<<(7 - b); 
			}	
		}
	}

	bitmap_t* ret = (bitmap_t*)malloc(sizeof(bitmap_t));
	ret->buffer = p;
	ret->w = g->w;
	ret->h = g->h;
	return  ret;
}

graph_t*  graph_from_bitmap(bitmap_t* bmp, uint32_t color) {
	int32_t bw = bmp->w / 8;
	uint32_t* p = (uint32_t*)malloc(bmp->w * bmp->h * 4);
	memset(p, 0, bmp->w * bmp->h * 4);

	for(int32_t j=0; j<bmp->h; j++) {
		for(int32_t i=0; i<bw; i++) {
			uint8_t bt = bmp->buffer[j*bw + i];
			for(uint8_t b=0; b<8; b++) {
				uint32_t at = j*bmp->w + i*8 + b;
				if((bt & (1 << (7-b))) != 0)
					p[at] = color;
			}
		}
	}

	graph_t* ret = (graph_t*)malloc(sizeof(graph_t));
	ret->buffer = p;
	ret->w = bmp->w;
	ret->h = bmp->h;
	return  ret;
}

#ifdef __cplusplus
}
#endif
