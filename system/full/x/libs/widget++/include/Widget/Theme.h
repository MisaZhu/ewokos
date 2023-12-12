#ifndef THEME_HH
#define THEME_HH

#include <font/font.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class Theme {
	Theme() {
		fgColor = 0xff000000;
		bgColor = 0xffffffff;
		fontName = DEFAULT_SYSTEM_FONT;
		fontSize = DEFAULT_SYSTEM_FONT_SIZE;
		font = font_new(fontName.c_str(), fontSize, true);
	}
public:
	string   theme;
	uint32_t fgColor;
	uint32_t bgColor;
	uint32_t fontSize;
	string   fontName;
	font_t*  font;

	~Theme() {
		if(font != NULL)
			font_free(font);
	}

	static Theme* load(const char* theme);
};

}

#endif