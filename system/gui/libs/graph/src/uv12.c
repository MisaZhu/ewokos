#include "graph/uv12.h"
#include "graph/bsp_graph.h"
/*
*	Following are the conversion formulas from RGB to YUV 
*	and from YUV to RGB. See YUV and color space conversion.
*	From RGB to YUV Y = 0.299R + 0.587G + 0.114B 
*	U = 0.492 (B-Y) V = 0.877 (R-Y) 
*	It can also be represented as: 
*	Y = 0.299R + 0.587G + 0.114B 
*	U = -0.147R - 0.289G + 0.436B 
*	V = 0.615R - 0.515G - 0.100B
*/
static inline uint8_t rgb2y(uint8_t *rgb){
	//return(((257 * rgb[0])/1000) + ((504 * rgb[1])/1000) + ((98 * rgb[2])/1000) + 16);
	register uint32_t b = rgb[0];
	register uint32_t g = rgb[1];
	register uint32_t r = rgb[2];
	return ((306*r + 601*g + 117*b) >> 10) ;
}

static inline void rgb2uv(uint8_t *rgb, uint8_t *uv){
	register int32_t b = rgb[0];
	register int32_t g = rgb[1];
	register int32_t r = rgb[2];
	uv[0] = (uint8_t)(((-38*r - 74*g + 112*b + 128) >> 8) + 128);
	uv[1] = (uint8_t)(((112*r - 94*g - 18*b + 128) >> 8) + 128);
}

void rgb2nv12_cpu(uint8_t  *out,  uint32_t *in , int w, int h)
{
	uint8_t *y = out;
	uint8_t *uv = y + w * h;
	uint32_t *rgb = (in + w * h);

	for(int i = 0; i < h; i+=2){
		for( int j = 0; j < w; j+=2){
			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;

			rgb2uv(rgb, uv);
			uv+=2;		

			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;
		}
		for( int j = 0; j < w; j+=2){
			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;

			rgb--;
			*y = rgb2y((uint8_t*)rgb); 
			y++;
		}
	}	

	return 0;
}

inline void rgb2nv12(uint8_t  *out,  uint32_t *in , int w, int h) {
#ifdef BSP_BOOST
    rgb2nv12_bsp(out,  in , w, h);
#else
    rgb2nv12_cpu(out,  in , w, h);
#endif
}