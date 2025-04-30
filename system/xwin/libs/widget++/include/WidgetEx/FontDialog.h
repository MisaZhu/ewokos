#ifndef WIDGET_FONT_DIALOG_HH
#define WIDGET_FONT_DIALOG_HH

#include <WidgetEx/Dialog.h>

namespace Ewok {

class FontDialog: public Dialog {
protected:
	string fontName;
	void onBuild();

public:
	inline void setFontName(const string& name) { fontName = name; }
	FontDialog();
	string getResult();
};

}

#endif
