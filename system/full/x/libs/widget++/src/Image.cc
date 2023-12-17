#include <Widget/Image.h>
#include <upng/upng.h>

namespace Ewok {

void Image::onRepaint(graph_t* g, const Theme* theme, const grect_t& r) {
	graph_blt_alpha(image, 0, 0, image->w, image->h,
			g, (r.w - image->w)/2 + r.x, (r.h - image->h)/2 + r.y, image->w, image->h, 0xff);
}

Image::Image(const char* fname) {
	image = NULL;
	loadImage(fname);
	alpha = true;
}

Image::~Image(void) {
	if(image != NULL)
		graph_free(image);
}

bool  Image::loadImage(const char* fname) {
	if(image != NULL)
		graph_free(image);
	image = png_image_new(fname);
	if(image == NULL)
		return false;
	return true;
}

gsize_t  Image::getMinSize(void) {
	gsize_t sz = { 0, 0 };
	if(image != NULL) {
		sz.w = image->w + marginH*2;
		sz.h = image->h + marginV*2;
	}
	return sz;
}

}