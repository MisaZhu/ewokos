#ifndef WIDGET_FONT_UNIT_HH
#define WIDGET_FONT_UNIT_HH

#include <font/font.h>
#include <Widget/Theme.h>
#include <string>

using namespace EwokSTL;
namespace Ewok {

class FontUnit {
protected:
	font_t* font;
	uint32_t fontSize;
	font_t* getFont(const Theme* theme, uint32_t* size = NULL);
	virtual void onFont() { }
public:
	FontUnit();
	~FontUnit();

	void setFont(const string& fontName, uint32_t fontSize);
};

}

#endif
