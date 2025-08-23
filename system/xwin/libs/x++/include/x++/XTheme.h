#ifndef XTHEME_HH
#define XTHEME_HH

#include <x/xtheme.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <string.h>
#include <UniObject/UniObject.h>

namespace Ewok {

class XTheme : public UniObject {
	font_t*  font;

public:
	x_theme_t basic;

	XTheme();

	void setFont(const char* name, uint32_t size);
	void loadSystem(void);

	inline font_t* getFont(void) {
			return font;
	}

	static XTheme* dup(XTheme* theme);

	inline ~XTheme() {
		if(font != NULL)
			font_free(font);
	}
};

}

#endif