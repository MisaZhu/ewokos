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

void XTheme::loadConfig(var_t* conf_var) {
	int font_size = get_int(conf_var, "font_size");
	if(font_size == 0) 
		font_size = basic.fontSize;

	const char* v = get_str(conf_var, "font");
	if(v[0] != 0) 
		setFont(v, font_size);

	basic.fgColor = get_int(conf_var, "fg_color");
	basic.bgColor = get_int(conf_var, "bg_color");

	basic.fgUnfocusColor = get_int(conf_var, "fg_unfocus_color");
	basic.bgUnfocusColor = get_int(conf_var, "bg_unfocus_color");
	basic.fgDisableColor = get_int(conf_var, "fg_disable_color");
	basic.bgDisableColor = get_int(conf_var, "bg_disable_color");

	basic.fontFixedSize = get_int(conf_var, "font_fixed");
	if(basic.fontFixedSize == 0)
		basic.fontFixedSize = basic.fontSize;
}

void XTheme::loadSystem() {
	x_get_theme(&basic);
	setFont(basic.fontName, basic.fontSize);
}

XTheme::XTheme(void) {
	memset(&basic, 0, sizeof(x_theme_t));
	font = NULL;
}

}