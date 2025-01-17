#ifndef __ENGINE_H__
#define __ENGINE_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C"{
#endif
void setup(int width, int height);
void update(void);
void render(void);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
#ifdef __cplusplus
}
#endif
#endif
