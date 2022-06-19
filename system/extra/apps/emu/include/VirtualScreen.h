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

        void draw(uint32_t picData[256][261]);

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
