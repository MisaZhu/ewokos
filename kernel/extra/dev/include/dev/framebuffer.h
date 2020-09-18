#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <fbinfo.h>
//#include <dev/kdevice.h>

extern fbinfo_t* fb_get_info(void);
extern int32_t fb_dev_init(void);
extern int32_t fb_dev_write(const void* buf, uint32_t size);

#endif
