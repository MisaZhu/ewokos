#include <Widget/Image.h>
#include <graph/graph_png.h>

namespace Ewok {

void Image::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	grect_t ir = {(r.w - image->w)/2 + r.x, (r.h - image->h)/2 + r.y, image->w, image->h};
	int dw = r.x + r.w - ir.x - ir.w;
	ir.w = ir.w + dw;
	int dh = r.y + r.h - ir.y - ir.h;
	ir.h = ir.h + dh;
	graph_blt_alpha(image, 0, 0, image->w, image->h,
			g, ir.x, ir.y, ir.w, ir.h, 0xff);
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