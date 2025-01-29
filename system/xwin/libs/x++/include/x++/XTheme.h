#ifndef XTHEME_HH
#define XTHEME_HH

#include <x/xtheme.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <string.h>

namespace Ewok {

class XTheme {
	font_t*  font;

public:
	x_theme_t basic;

	XTheme();

	void setFont(const char* name, uint32_t size);

	void loadSystem(void);
	void loadConfig(json_var_t* conf_var);

	inline font_t* getFont(void) {
			return font;
	}

	inline ~XTheme() {
		if(font != NULL)
			font_free(font);
	}
};

}

#endif