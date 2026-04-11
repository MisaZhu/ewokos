#include <graph/graph_image.h>
#include <graph/graph_png.h>
#include <graph/graph_jpeg.h>
#include <graph/graph_gif.h>
#include <graph/graph_tga.h>
#include <string.h>
#include <strings.h>

graph_t* graph_image_new(const char* filename) {
	const char* ext = strrchr(filename, '.');
	if(ext != NULL) {
		if(strcasecmp(ext, ".png") == 0) {
			return png_image_new(filename);
		}
		else if(strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) {
			return jpeg_image_new(filename);
		}
		else if(strcasecmp(ext, ".gif") == 0) {
			return gif_image_new(filename);
		}
		else if(strcasecmp(ext, ".tga") == 0) {
			return tga_image_new(filename);
		}
		else {
			// Try all formats
			graph_t* img = png_image_new(filename);
			if(img != NULL)
				return img;
			img = jpeg_image_new(filename);
			if(img != NULL)
				return img;
			img = gif_image_new(filename);
			if(img != NULL)
				return img;
			return tga_image_new(filename);
		}
	}
	else {
		// No extension, try all formats
		graph_t* img = png_image_new(filename);
		if(img != NULL)
			return img;
		img = jpeg_image_new(filename);
		if(img != NULL)
			return img;
		img = gif_image_new(filename);
		if(img != NULL)
			return img;
		return tga_image_new(filename);
	}
}

graph_t* graph_image_new_bg(const char* filename, uint32_t bg_color) {
	graph_t* img = graph_image_new(filename);
	if(img == NULL)
		return NULL;
	graph_t* ret = graph_new(NULL, img->w, img->h);
	graph_clear(ret, bg_color);
	graph_blt_alpha(img, 0, 0, img->w, img->h, ret, 0, 0, img->w, img->h, 0xff);
	graph_free(img);
	return ret;
}