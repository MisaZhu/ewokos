#ifndef VIRTUALSCREEN_H
#define VIRTUALSCREEN_H

#include<stdint.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>

namespace sn
{
    class VirtualScreen 
    {
    public:
        void update(void* buf, unsigned int width, unsigned int height);
        void setPixel (int  x, int y, uint32_t color);
        void draw();

    private:
		int width;
		int height;
		uint32_t *frameBuffer;
    int scale;
    int offset_x;
    int offset_y;
    };
}
#endif // VIRTUALSCREEN_H
