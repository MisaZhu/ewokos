#include "VirtualScreen.h"
#include <stdio.h>
#if 0
#pragma pack(1)
	typedef struct                       /**** BMP file header structure ****/
	{
	    unsigned short bfType;           /* Magic number for file */
	    unsigned int   bfSize;           /* Size of file */
	    unsigned short bfReserved1;      /* Reserved */
	    unsigned short bfReserved2;      /* ... */
	    unsigned int   bfOffBits;        /* Offset to bitmap data */
	} BITMAPFILEHEADER;
	
	typedef struct                       /**** BMP file info structure ****/
	{
	    unsigned int   biSize;           /* Size of info header */
	    int            biWidth;          /* Width of image */
	    int            biHeight;         /* Height of image */
	    unsigned short biPlanes;         /* Number of color planes */
	    unsigned short biBitCount;       /* Number of bits per pixel */
	    unsigned int   biCompression;    /* Type of compression to use */
	    unsigned int   biSizeImage;      /* Size of image data */
	    int            biXPelsPerMeter;  /* X pixels per meter */
	    int            biYPelsPerMeter;  /* Y pixels per meter */
	    unsigned int   biClrUsed;        /* Number of colors used */
	    unsigned int   biClrImportant;   /* Number of important colors */
	} BITMAPINFOHEADER;
	
	BITMAPFILEHEADER bfh;
	BITMAPINFOHEADER bih;
	
#pragma pack()


void saveAsBitmap(int w, int h, uint32_t* buf) {
	static int cnt = 0;
	bfh.bfType = 0x4d42;
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)+w*h*3;
	bfh.bfOffBits = 0x36;
	
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = w;
	bih.biHeight = h;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = 0;
	bih.biSizeImage = 0;
	bih.biXPelsPerMeter = 5000;
	bih.biYPelsPerMeter = 5000;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	char path[16];
	sprintf(path, "img/%d.bmp", cnt++);
	FILE *file = fopen(path, "wb");
	if (!file) {
	    printf("File not found");
	    return;
	}

	fwrite(&bfh, 1, sizeof(bfh), file);
	fwrite(&bih, 1, sizeof(bih), file);
	
	for (int y = h - 1; y >= 0; y--) {
	    for (int x = 0; x < w; x++) {
			uint32_t pix = buf[y*w +x];
	        uint8_t r = (pix>>8)&0xFF;
	        uint8_t g = (pix>>16)&0xFF;
	        uint8_t b = (pix>>24&0xFF);
	        fwrite(&r, 1, 1, file);
	        fwrite(&g, 1, 1, file);
	        fwrite(&b, 1, 1, file);
	    }
	}
	fclose(file);
}
#endif

namespace sn
{
    void VirtualScreen::update(void* buf, unsigned int w, unsigned int h)
    {
		//only support rgb888
		width = w;
		height = h;
		if(w >= 512 && h >= 480){
			scale = 2;
			offset_x = (width - 512)/2;
			offset_y = (height - 480)/2;
		}else{
			scale = 1;
			offset_x = (width - 256)/2;
			offset_y = (height - 240)/2;
		}
		frameBuffer = (uint32_t*)buf;
    }


    void VirtualScreen::draw(uint32_t picData[256][261])
    {
		register uint32_t color;

		if(scale == 2) {
			int w = width / 2;
			if(w > 256)
				w = 256;
			int h = height / 2;
			if(h > 261)
				h = 261;

			for (int x = 0; x < w; ++x) {
				for (int y = 0; y < h; ++y) {
					color = picData[x][y];
					register int fbx = x*scale + offset_x;
					register int fby = y*scale + offset_y;
					register int tmp = fby*width;
					frameBuffer[tmp + fbx] = color;
					frameBuffer[tmp + fbx + 1] = color;

					tmp = (fby+1)*width;
					frameBuffer[tmp + fbx + 1] = color;
					frameBuffer[tmp + fbx] = color;
				}
			}
		}
		else if(scale == 1) {
			int w = width;
			if(w > 256)
				w = 256;
			int h = height;
			if(h > 261)
				h = 261;

			for (int x = 0; x < w; ++x) {
				for (int y = 0; y < h; ++y) {
					color = picData[x][y];
					frameBuffer[(y + offset_y)*width + x + offset_x] = color;
				}
			}
		}
	}
}
