#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

#define FPS 60
#define FRAME_TARGET_TIME (1000 / FPS)

// Extern because they are external variables defined in implementation (display.c)
extern uint32_t *color_buffer; // u_int32_t to make sure that each element is exactly 32 bit (4 bytes) long
extern float* z_buffer; 
extern int window_width;
extern int window_height;

// Function prototypes/signatures/declarations. (Definition/implementation is in .c file)
bool initialize_window(void);
void draw_grid(uint32_t color, int gap_size);
void draw_rect(int startX, int startY, int width, int height, uint32_t color);
void draw_pixel(int x, int y, uint32_t color);
void render_color_buffer();
void clear_color_buffer(uint32_t color);
void clear_z_buffer();
void destroy_window(void);
void draw_line(int x0, int y0, int x1, int y1, uint32_t color);

#endif
