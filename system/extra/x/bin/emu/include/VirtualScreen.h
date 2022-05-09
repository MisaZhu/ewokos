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
        void setPixel (size_t x, size_t y, uint32_t color);
        void draw();

    private:
		size_t width;
		size_t height;
		uint32_t *frameBuffer;
    };
}
#endif // VIRTUALSCREEN_H
