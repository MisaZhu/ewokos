#include  <x++/XTheme.h>
#include  <x/x.h>
#include  <ewoksys/klog.h>
#include  <stdlib.h>

namespace Ewok {

void XTheme::setFont(const char* name, uint32_t size) {
	if(font != NULL)
		font_free(font);

	font = font_new(name, size, true);
	basic.fontSize = size;
	if(basic.fontName != name)  {
		memset(basic.fontName, 0, FONT_NAME_MAX);
		strncpy(basic.fontName, name, FONT_NAME_MAX-1);
	}
}

void XTheme::loadConfig(sconf_t* sconf) {
	if(sconf == NULL)
		return;

	int font_size = basic.fontSize;
	const char* v = sconf_get(sconf, "font_size");
	if(v[0] != 0) 
		font_size = atoi(v);

	v = sconf_get(sconf, "font");
	if(v[0] != 0) 
		setFont(v, font_size);

	v = sconf_get(sconf, "fg_color");
	if(v[0] != 0) 
		basic.fgColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_color");
	if(v[0] != 0) 
		basic.bgColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "fg_unfocus_color");
	if(v[0] != 0) 
		basic.fgUnfocusColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_unfocus_color");
	if(v[0] != 0) 
		basic.bgUnfocusColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "fg_disable_color");
	if(v[0] != 0) 
		basic.fgDisableColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "bg_disable_color");
	if(v[0] != 0) 
		basic.bgDisableColor = strtoul(v, NULL, 16);

	v = sconf_get(sconf, "font_fixed");
	if(v[0] != 0) 
		basic.fontFixedSize = atoi(v);
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