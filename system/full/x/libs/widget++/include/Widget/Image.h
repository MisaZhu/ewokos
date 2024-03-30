#ifndef WIDGET_IMAGE_HH
#define WIDGET_IMAGE_HH

#include <Widget/Widget.h>

namespace Ewok {

class Image: public Widget {
	graph_t* image;

protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r);

public:
	Image(const char* fname);
	~Image(void);

	bool loadImage(const char* fname);
	gsize_t getMinSize(void);
};

}

#endif