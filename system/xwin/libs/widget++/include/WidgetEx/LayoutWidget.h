#ifndef WIDGET_LAYOUT_WIDGET_HH
#define WIDGET_LAYOUT_WIDGET_HH

#include <Widget/Container.h>
#include <string>
using namespace std;
namespace Ewok {

class LayoutWidget: public Container {
protected:
	virtual Widget* createByType(const string& type);
	
public:
	LayoutWidget();

	bool load(const string& dirName);
};

}

#endif
