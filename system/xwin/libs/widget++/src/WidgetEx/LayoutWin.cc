#include <WidgetEx/LayoutWin.h>

using namespace Ewok;

LayoutWin::LayoutWin() {
    layoutWidget = new LayoutWidget();
	root->add(layoutWidget);
}

LayoutWin::~LayoutWin() {
}

bool LayoutWin::loadConfig(const string& fname) {
    return layoutWidget->loadConfig(fname);
}
