#include <Widget/Image.h>
#include <upng/upng.h>

namespace Ewok {

void Image::onRepaint(graph_t* g) {
	Widget::onRepaint(g);
	if(image == NULL)
		return;

	grect_t rect = getRootArea();
	graph_blt_alpha(image, 0, 0, image->w, image->h,
			g, (rect.w - image->w)/2 + rect.x, (rect.h - image->h)/2 + rect.y, image->w, image->h, 0xff);
}

Image::Image(const char* fname) {
	image = NULL;
	loadImage(fname);
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