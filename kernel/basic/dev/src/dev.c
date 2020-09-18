#include <stddef.h>
#include <kprintf.h>
#include <dev/uart.h>

#ifdef FRAMEBUFFER
#include <dev/framebuffer.h>
#include <kconsole.h>
#endif

#ifdef FRAMEBUFFER
static int32_t fb_init(void) {
	fbinfo_t* fbinfo = NULL;
	printf("kernel: framebuffer initing");
	if(fb_dev_init() != 0) {
		printf("  [Failed!]\n");
		return -1;
	}

	fbinfo = fb_get_info();
	if(fbinfo == NULL || (fbinfo->width * fbinfo->height * fbinfo->depth/8) != fbinfo->size) {
		fbinfo->pointer = 0;
		printf("  [Failed!]\n");
		return -1;
	}

	printf(" %dx%d %dbits, addr: 0x%X, size:%d [OK]\n", 
			fbinfo->width, fbinfo->height, fbinfo->depth,
			fbinfo->pointer, fbinfo->size);
	return 0;
}
#endif

void dev_init(void) {
	uart_dev_init();

#ifdef FRAMEBUFFER
	fb_init();
	kconsole_setup();
#endif
}
