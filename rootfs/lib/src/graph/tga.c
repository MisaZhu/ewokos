/*
 * Copyright 2011 Ben Van Houtven. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 * 
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 * 
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY BEN VAN HOUTVEN ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BEN VAN HOUTVEN OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Ben Van Houtven.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <graph/tga.h>

#define UNMAPPED_RGB 2
#define RLE_RGB 10

typedef struct TGA_HEADER {
	byte	id_size, colormap_type, image_type;
	short	origin_x, origin_y, width, height;
	byte	bpp, img_descriptor;
	
} tga_header_t;

struct TGA_IMAGE {
	short			width, height;
	byte			bpp;
	unsigned char*	data;
};

typedef struct COLOUR {
	unsigned char r,g,b,a;
} color_t;

tga_image* tga_image_read_rgb(int fd, tga_header_t *header);
tga_image* tga_image_read_rle(int fd, tga_header_t *header);

static void tga_image_decode16(const short c, color_t* cl) {
	//TODO: Implement this
	(void)c;
	(void)cl;
}

static void tga_image_decode24(const int c, color_t* cl) {
	cl->a = 0xFF;
	cl->r = (c>>16)&0xFF;
	cl->g = (c>>8)&0xFF;
	cl->b = c&0xFF;
}

static void tga_image_decode32(const int c, color_t* cl) {
	cl->a = c>>24;
	cl->r = (c>>16)&0xFF;
	cl->g = (c>>8)&0xFF;
	cl->b = c&0xFF;
}

static unsigned char* tga_image_next_pixel(unsigned char* p, tga_header_t *header, color_t* cl) {
	if(header->bpp == 16) {
		short d;
		memcpy(&d, p, 2);
		p += 2;
		tga_image_decode16(d, cl);
	} else if(header->bpp == 24) {
		int d;
		memcpy(&d, p, 3);
		p += 3;
		tga_image_decode24(d, cl);
	} else if(header->bpp == 32) {
		int d;
		memcpy(&d, p, 4);
		p += 4;
		tga_image_decode32(d, cl);
	}
	return p;
}

tga_image* tga_image_new(const char* filename) {
	int fd = open(filename, O_RDONLY);
	if( fd >= 0 ) {
		tga_image *this = 0;
		tga_header_t *header = (tga_header_t*)malloc(sizeof(tga_header_t));
		
		read(fd, header, 3);
		
		lseek(fd, 12, SEEK_SET);
		read(fd, &(header->width), 10);
		
		lseek(fd, 18+header->id_size, SEEK_SET);
		
		if( header->image_type == UNMAPPED_RGB ) {
			this = tga_image_read_rgb(fd, header);
		} else if( header->image_type == RLE_RGB ) {
			this = tga_image_read_rle(fd, header);
		}
		
		free(header);
		close(fd);
		return this;
	}
	return 0;
	
}

void tga_image_destroy(tga_image *this) {
	free(this->data);
	free(this);
}

int	tga_image_get_width(tga_image *this) {
	return this->width;
}
int	tga_image_get_height(tga_image *this) {
	return this->height;
}

byte tga_image_get_bpp(tga_image *this) {
	return this->bpp;
}

unsigned char* tga_image_get_data(tga_image *this) {
	return this->data;
}

tga_image* tga_image_read_rgb(int fd, tga_header_t *header) {
	tga_image *this = (tga_image*)malloc(sizeof(tga_image));
	
	int i;
	int data_size = (header->width*4) * header->height;
	unsigned char* fdata = (unsigned char*)malloc( data_size*sizeof(unsigned char) );
	read(fd, fdata, data_size);
	unsigned char* p = fdata;

	this->data = (unsigned char*)malloc( data_size*sizeof(unsigned char) );
	this->bpp = 32;
	this->width = header->width;
	this->height = header->height;
	
	for(i = 0; i < data_size;i+=4) {
		color_t colour;
		p = tga_image_next_pixel(p, header, &colour);
		
		this->data[i] = colour.r;
		this->data[i+1] = colour.g;
		this->data[i+2] = colour.b;
		this->data[i+3] = colour.a;
	}
	free(fdata);
	return this;
}

tga_image* tga_image_read_rle(int fd, tga_header_t *header) {
	tga_image *this = (tga_image*)malloc(sizeof(tga_image));
	
	int i = 0;
	int data_size = (header->width*4) * header->height;
	unsigned char* fdata = (unsigned char*)malloc( data_size*sizeof(unsigned char) );
	read(fd, fdata, data_size);
	unsigned char* p = fdata;

	this->data = (unsigned char*)malloc( data_size*sizeof(unsigned char) );
	this->bpp = 32;
	this->width = header->width;
	this->height = header->height;
	
	while( i < data_size ) {
		unsigned char h;
		h = *p;
		p++;
		
		if( (h>>7) == 1 ) { //RLE
			unsigned char rle_count = h - 127;
			color_t colour;
			p = tga_image_next_pixel(p, header, &colour);
			int j;
			for(j = 0; j < rle_count*4; j+= 4) {
				this->data[i+j] = colour.r;
				this->data[i+j+1] = colour.g;
				this->data[i+j+2] = colour.b;
				this->data[i+j+3] = colour.a;
			}
			i+= rle_count*4;
			
		} else { //RAW
			byte raw_count = h+1;
			int j;
			for(j = 0; j < raw_count*4; j+= 4) {
				color_t colour;
				p = tga_image_next_pixel(p, header, &colour);
				this->data[i+j] = colour.r;
				this->data[i+j+1] = colour.g;
				this->data[i+j+2] = colour.b;
				this->data[i+j+3] = colour.a;
			}
			i += raw_count*4;
		}
	}
	free(fdata);
	return this;
}

