#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale) {
    if(scale <= 0.0 ||
            dst->w < (int)(g->w*scale) ||
            dst->h < (int)(g->h*scale))
        return;
    
    // Use bilinear interpolation algorithm for anti-aliasing smoothing
	int hmin = g->h-2;
	int wmin = g->w-2;
    for(int i=0; i<dst->h; i++) {
        float gi = (float)i / scale; // Floating point coordinate
        int gi0 = (int)gi;           // Integer part
        float gi_frac = gi - gi0;    // Fractional part
        
        // Ensure no out of bounds
		gi0 = CLAMP(gi0, 0, hmin);
        
		int gi0w = gi0 * g->w;
		int gi1w = (gi0+1) * g->w;
        for(int j=0; j<dst->w; j++) {
            float gj = (float)j / scale; // Floating point coordinate
            int gj0 = (int)gj;           // Integer part
            float gj_frac = gj - gj0;    // Fractional part
            
            // Ensure no out of bounds
			gj0 = CLAMP(gj0, 0, wmin);
            
            // Get colors of four adjacent pixels
            uint32_t p00 = g->buffer[gi0w + gj0];
            uint32_t p01 = g->buffer[gi0w + (gj0+1)];
            uint32_t p10 = g->buffer[gi1w + gj0];
            uint32_t p11 = g->buffer[gi1w + (gj0+1)];
			if(p00 == p01 && p00 == p10 && p00 == p11) {
            	dst->buffer[i*dst->w + j] = p00;
				continue;
			}
            
            // Decompose color channels
            uint8_t r00 = (p00 >> 16) & 0xFF;
            uint8_t g00 = (p00 >> 8) & 0xFF;
            uint8_t b00 = p00 & 0xFF;
            uint8_t a00 = (p00 >> 24) & 0xFF;
            
            uint8_t r01 = (p01 >> 16) & 0xFF;
            uint8_t g01 = (p01 >> 8) & 0xFF;
            uint8_t b01 = p01 & 0xFF;
            uint8_t a01 = (p01 >> 24) & 0xFF;
            
            uint8_t r10 = (p10 >> 16) & 0xFF;
            uint8_t g10 = (p10 >> 8) & 0xFF;
            uint8_t b10 = p10 & 0xFF;
            uint8_t a10 = (p10 >> 24) & 0xFF;
            
            uint8_t r11 = (p11 >> 16) & 0xFF;
            uint8_t g11 = (p11 >> 8) & 0xFF;
            uint8_t b11 = p11 & 0xFF;
            uint8_t a11 = (p11 >> 24) & 0xFF;
            
            // Bilinear interpolation to calculate target pixel color
            uint8_t r = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * r00 + gj_frac * r01) + 
                                 gi_frac * ((1 - gj_frac) * r10 + gj_frac * r11));
            uint8_t g_val = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * g00 + gj_frac * g01) + 
                                     gi_frac * ((1 - gj_frac) * g10 + gj_frac * g11));
            uint8_t b = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * b00 + gj_frac * b01) + 
                                 gi_frac * ((1 - gj_frac) * b10 + gj_frac * b11));
            uint8_t a = (uint8_t)((1 - gi_frac) * ((1 - gj_frac) * a00 + gj_frac * a01) + 
                                 gi_frac * ((1 - gj_frac) * a10 + gj_frac * a11));
            
            // Recombine color channels
            dst->buffer[i*dst->w + j] = (a << 24) | (r << 16) | (g_val << 8) | b;
        }
    }
}

inline void graph_scale_tof(graph_t* g, graph_t* dst, float scale) {
#if BSP_BOOST
    graph_scale_tof_bsp(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

inline void graph_scale_tof_fast(graph_t* g, graph_t* dst, float scale) {
#if BSP_BOOST
    graph_scale_tof_fast_bsp(g, dst, scale);
#else
    graph_scale_tof_cpu(g, dst, scale);
#endif
}

graph_t* graph_scalef_fast(graph_t* g, float scale) {
	graph_t* ret = NULL;
	if(scale <= 0.0)
		return NULL;
	ret = graph_new(NULL, g->w*scale, g->h*scale);
	if(ret == NULL)
		return NULL;
	graph_scale_tof_fast(g, ret, scale);
	return ret;
}

graph_t* graph_scalef(graph_t* g, float scale) {
	graph_t* ret = NULL;
	if(scale <= 0.0)
		return NULL;
	
	ret = graph_new(NULL, g->w*scale, g->h*scale);
	if(ret == NULL)
		return NULL;
	graph_scale_tof(g, ret, scale);
	return ret;
}

#ifdef __cplusplus
}
#endif
