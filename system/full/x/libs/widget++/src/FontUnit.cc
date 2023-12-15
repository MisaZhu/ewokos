#include <Widget/FontUnit.h>
#include <sys/klog.h>

using namespace EwokSTL;
namespace Ewok {

FontUnit::FontUnit() {
	font = NULL;
	fontSize = 0;
}

FontUnit::~FontUnit(void) {
	if(font != NULL)
		font_free(font);
}

void FontUnit::setFont(const string& fontName, uint32_t fontSize) {
	if(font != NULL)
		font_free(font);
	
	font = font_new(fontName.c_str(), fontSize, true);
	this->fontSize = fontSize;
	onFont();
}

font_t* FontUnit::getFont(const Theme* theme, uint32_t* size) {
	if(size != NULL)
		*size = fontSize == 0 ? theme->fontSize:fontSize;

	if(font != NULL)
		return font;
	return theme->font;
}

}