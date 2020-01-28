#include <unistd.h>
#include <sys/vfs.h>
#include <string.h>
#include <stdlib.h>
#include <graph/tga.h>

#define UNMAPPED_RGB 2
#define RLE_RGB 10

typedef struct TGA_HEADER {
	uint8_t	id_size, colormap_type, image_type;
	short	origin_x, origin_y, width, height;
	uint8_t	bpp, img_descriptor;
	
} tga_header_t;

typedef struct COLOUR {
	unsigned char r,g,b,a;
} color_t;


static void tga_image_decode16(const short c, color_t* cl) {
	//TODO: Implement g
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

static unsigned char* tga_image_next_pixel(unsigned char* p, uint8_t bpp, color_t* cl) {
	if(bpp == 16) {
		short d;
		memcpy(&d, p, 2);
		p += 2;
		tga_image_decode16(d, cl);
	} else if(bpp == 24) {
		int d;
		memcpy(&d, p, 3);
		p += 3;
		tga_image_decode24(d, cl);
	} else if(bpp == 32) {
		int d;
		memcpy(&d, p, 4);
		p += 4;
		tga_image_decode32(d, cl);
	}
	return p;
}

static graph_t* tga_image_read_rgb(unsigned char* p, tga_header_t *header) {
	int i, j;
	graph_t* g = graph_new(NULL, header->width, header->height);
	unsigned char* data = (unsigned char*)g->buffer;
	for(j = header->width-1; j >=0; j--) {
		for(i = 0; i < header->width*4; i+=4) {
			color_t colour;
			p = tga_image_next_pixel(p, header->bpp, &colour);

			data[j*header->width*4 + i] = colour.r;
			data[j*header->width*4 + i + 1] = colour.g;
			data[j*header->width*4 + i + 2] = colour.b;
			data[j*header->width*4 + i + 3] = colour.a;
		}
	}
	return g;
}

static graph_t* tga_image_read_rle(unsigned char* p, tga_header_t *header) {
	int data_size = (header->width*4) * header->height;
	graph_t* g = graph_new(NULL, header->width, header->height);
	unsigned char* data = (unsigned char*)g->buffer;
	
	int i = 0;
	while( i < data_size ) {
		unsigned char h;
		h = *p;
		p++;
		
		if( (h>>7) == 1 ) { //RLE
			unsigned char rle_count = h - 127;
			color_t colour;
			p = tga_image_next_pixel(p, header->bpp, &colour);
			int j;
			for(j = 0; j < rle_count*4; j+= 4) {
				data[i+j] = colour.r;
				data[i+j+1] = colour.g;
				data[i+j+2] = colour.b;
				data[i+j+3] = colour.a;
			}
			i+= rle_count*4;
			
		} else { //RAW
			uint8_t raw_count = h+1;
			int j;
			for(j = 0; j < raw_count*4; j+= 4) {
				color_t colour;
				p = tga_image_next_pixel(p, header->bpp, &colour);
				data[i+j] = colour.r;
				data[i+j+1] = colour.g;
				data[i+j+2] = colour.b;
				data[i+j+3] = colour.a;
			}
			i += raw_count*4;
		}
	}
	return g;
}

graph_t* tga_image_new(const char* filename) {
	int sz;
	unsigned char* data = (unsigned char*)vfs_readfile(filename, &sz);
	unsigned char* p = data;
	if(p != NULL ) {
		tga_header_t header;
		memcpy(&header, p, 3);
		memcpy(&(header.width), p+12, 10);
		p = p + (18+header.id_size);
		
		graph_t* g = NULL;
		if( header.image_type == UNMAPPED_RGB ) {
			g = tga_image_read_rgb(p, &header);
		} else if( header.image_type == RLE_RGB ) {
			g = tga_image_read_rle(p, &header);
		}
		
		free(data);
		return g;
	}
	return NULL;
}
