#include  <Widget/Theme.h>
#include  <string.h>

namespace Ewok {

Theme* Theme::loadDefault() {
	Theme* ret = new Theme();
	ret->font = font_new(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_SIZE, true);
	return ret;
}

Theme* Theme::clone(const Theme* theme) {
	Theme* ret = new Theme();
	memcpy(ret, theme, sizeof(Theme));
	ret->cloned = true;
	return ret;
}

}