#include <graph/graph_image.h>
#include <graph/graph_png.h>
#include <graph/graph_jpeg.h>
#include <graph/graph_gif.h>
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
		else {
			// Try all formats
			graph_t* img = png_image_new(filename);
			if(img != NULL)
				return img;
			img = jpeg_image_new(filename);
			if(img != NULL)
				return img;
			return gif_image_new(filename);
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
		return gif_image_new(filename);
	}
}
