#ifndef WIDGET_LAYOUT_WIDGET_HH
#define WIDGET_LAYOUT_WIDGET_HH

#include <Widget/Container.h>
#include <string>
#include <tinyjson/tinyjson.h>
using namespace std;
namespace Ewok {

class LayoutWidget: public Container {
	bool load(Widget* wd, json_var_t* var);
protected:
	virtual Widget* createByType(const string& type);
	
public:
	LayoutWidget();
	bool loadConfig(const string& fname);
};

}

#endif
