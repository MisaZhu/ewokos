#include <graph/graph_image.h>
#include <graph/graph_png.h>
#include <graph/graph_jpeg.h>
#include <graph/graph_gif.h>
#include <graph/graph_tga.h>
#include <graph/graph_svg.h>
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
		else if(strcasecmp(ext, ".svg") == 0) {
			return svg_image_new(filename);
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
			img = tga_image_new(filename);
			if(img != NULL)
				return img;
			return svg_image_new(filename);
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
		img = tga_image_new(filename);
		if(img != NULL)
			return img;
		return svg_image_new(filename);
	}
}

static enum graph_image_type detect_image_type(const uint8_t* data, uint32_t size) {
	if (size < 4) {
		return GRAPH_IMAGE_TYPE_AUTO;
	}

	// PNG signature: 89 50 4E 47 0D 0A 1A 0A
	if (size >= 8 && data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
		return GRAPH_IMAGE_TYPE_PNG;
	}

	// JPEG signature: FF D8 FF
	if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) {
		return GRAPH_IMAGE_TYPE_JPEG;
	}

	// GIF signature: GIF87a or GIF89a
	if (size >= 6 && data[0] == 'G' && data[1] == 'I' && data[2] == 'F' && data[3] == '8') {
		if (data[4] == '7' || data[4] == '9') {
			if (data[5] == 'a') {
				return GRAPH_IMAGE_TYPE_GIF;
			}
		}
	}

	// TGA: no reliable signature, but we can check for common TGA patterns
	// TGA type 2 (uncompressed true-color) or type 10 (RLE true-color)
	// Check if the file is at least as large as the TGA header (18 bytes)
	if (size >= 18) {
		uint8_t image_type = data[2];
		if (image_type == 2 || image_type == 10) {
			// This could be a TGA file
			return GRAPH_IMAGE_TYPE_TGA;
		}
	}

	// SVG signature: starts with <?xml or <svg
	if (size >= 4 && data[0] == '<' && data[1] == 's' && data[2] == 'v' && data[3] == 'g') {
		return GRAPH_IMAGE_TYPE_SVG;
	}
	if (size >= 5 && data[0] == '<' && data[1] == '?' && data[2] == 'x' && data[3] == 'm' && data[4] == 'l') {
		const char* svg_ptr = strstr((const char*)data, "<svg");
		if (svg_ptr != NULL) {
			return GRAPH_IMAGE_TYPE_SVG;
		}
	}

	return GRAPH_IMAGE_TYPE_AUTO;
}

graph_t* graph_image_new_from_data(enum graph_image_type type, const uint8_t* data, uint32_t size) {
	if (data == NULL || size == 0) {
		return NULL;
	}

	// Auto-detect image type if requested
	if (type == GRAPH_IMAGE_TYPE_AUTO) {
		type = detect_image_type(data, size);
		if (type == GRAPH_IMAGE_TYPE_AUTO) {
			return NULL;
		}
	}

	switch (type) {
	case GRAPH_IMAGE_TYPE_PNG:
		return png_image_new_from_data(data, size);
	case GRAPH_IMAGE_TYPE_JPEG:
		return jpeg_image_new_from_data(data, size);
	case GRAPH_IMAGE_TYPE_TGA:
		return tga_image_new_from_data(data, size);
	case GRAPH_IMAGE_TYPE_GIF:
		return gif_image_new_from_data(data, size);
	case GRAPH_IMAGE_TYPE_SVG:
		return svg_image_new_from_data(data, size);
	default:
		return NULL;
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