#ifndef WIDGET_IMAGE_HH
#define WIDGET_IMAGE_HH

#include <Widget/Widget.h>
#include <upng/upng.h>

namespace Ewok {

class Image: public Widget {
	graph_t* image;
protected:
	void onRepaint(graph_t* g, grect_t* rect) {
		Widget::onRepaint(g, rect);
		if(image == NULL)
			return;
		graph_blt_alpha(image, 0, 0, image->w, image->h,
				g, (rect->w - image->w)/2 + rect->x, (rect->h - image->h)/2 + rect->y, image->w, image->h, 0xff);
	}

public:
	Image(const char* fname) {
		image = NULL;
		loadImage(fname);
	}

	~Image(void) {
		if(image != NULL)
			graph_free(image);
	}

	bool loadImage(const char* fname) {
		if(image != NULL)
			graph_free(image);
		image = png_image_new(fname);
		if(image == NULL)
			return false;
		return true;
	}

	gsize_t getMinSize(void) {
		gsize_t sz = { 0, 0 };
		if(image != NULL) {
			sz.w = image->w + marginH*2;
			sz.h = image->h + marginV*2;
		}
		return sz;
	}
};

}

#endif