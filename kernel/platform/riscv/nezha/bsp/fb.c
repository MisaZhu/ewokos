#include <dev/fb.h>
#include <kstring.h>
#include <graph/graph.h>
#include <stddef.h>

static uint16_t* _g16 = NULL;
int32_t fb_init_bsp(uint32_t w, uint32_t h, uint8_t dep, fbinfo_t* fbinfo) {
	return 0;
}

void fb_flush32_bsp(uint32_t* g32, uint32_t w, uint32_t h) {

}
