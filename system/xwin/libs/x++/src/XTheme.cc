#include  <x++/XTheme.h>
#include  <x/x.h>
#include  <ewoksys/klog.h>
#include  <stdlib.h>

namespace Ewok {

void XTheme::setFont(const char* name, uint32_t size) {
	if(font != NULL)
		font_free(font);

	font = font_new(name, true);
	basic.fontSize = size;
	if(basic.fontName != name)  {
		memset(basic.fontName, 0, FONT_NAME_MAX);
		strncpy(basic.fontName, name, FONT_NAME_MAX-1);
	}
}

void XTheme::loadSystem() {
	x_get_theme(&basic);
	setFont(basic.fontName, basic.fontSize);
}

XTheme::XTheme(void) {
	memset(&basic, 0, sizeof(x_theme_t));
	font = NULL;
}

XTheme* XTheme::dup(XTheme* theme) {
	XTheme * ret = new XTheme();
	memcpy(&ret->basic, &theme->basic, sizeof(x_theme_t));
	ret->setFont(ret->basic.fontName, ret->basic.fontSize);
	return ret;
}

}