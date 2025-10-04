#include <Widget/Image.h>
#include <x++/X.h>
#include <graph/graph_png.h>

namespace Ewok {

void Image::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
	if(!alpha)
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

	int ix = 0, iy = 0;
	int iw = image->w, ih = image->h;
	if(image->w > r.w) {
		ix = (image->w - r.w)/2;
		iw = r.w;
	}
	if(image->h > r.h) {
		iy = (image->h - r.h)/2;
		ih = r.h;
	}

	grect_t ir = {(r.w - iw)/2 + r.x, (r.h - ih)/2 + r.y, iw, ih};
	int dw = r.x + r.w - ir.x - ir.w;
	ir.w = ir.w + dw;
	int dh = r.y + r.h - ir.y - ir.h;
	ir.h = ir.h + dh;

	graph_blt_alpha(image, ix, iy, iw, ih,
			g, ir.x, ir.y, ir.w, ir.h, 0xff);
}

Image::Image(const char* fname) {
	image = NULL;

	if(fname == NULL || fname[0] == 0)
		return;
	loadImage(fname);
}

Image::~Image(void) {
	if(image != NULL)
		graph_free(image);
}

bool  Image::loadImage(const char* fname) {
	if(image != NULL)
		graph_free(image);
	image = png_image_new(X::getResName(fname).c_str());
	if(image == NULL)
		return false;
	update();
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

void Image::setAttr(const string& attr, json_var_t*value) {
	Widget::setAttr(attr, value);
	if(attr == "file") {
		loadImage(json_var_get_str(value));
	}	
}

}