#ifndef __ST_7586_H__
#define __ST_7586_H__

void st7586_init(void);
int st7586_update(uint8_t* buf, int width, int height);

#endif
