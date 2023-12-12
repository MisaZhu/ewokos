#include  <Widget/Theme.h>

namespace Ewok {

Theme* Theme::load(const char* theme) {
	Theme* ret = new Theme();
	ret->theme = theme;
	return ret;
}

}