#ifndef THEME_HH
#define THEME_HH

#include <font/font.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Theme {
	bool cloned;
public:
	uint32_t fgColor;
	uint32_t bgColor;
	font_t*  font;


	inline Theme(font_t* font = NULL) {
		cloned = false;
		fgColor = 0xff000000;
		bgColor = 0xffffffff;
		this->font = font;
	}

	inline ~Theme() {
		if(font != NULL && !cloned)
			font_free(font);
	}

	static Theme* loadDefault();
	static Theme* clone(const Theme* theme);
};

}

#endif