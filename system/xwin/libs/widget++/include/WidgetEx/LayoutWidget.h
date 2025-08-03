#ifndef WIDGET_LAYOUT_WIDGET_HH
#define WIDGET_LAYOUT_WIDGET_HH

#include <Widget/Container.h>
#include <WidgetEx/Menu.h>
#include <string>
#include <tinyjson/tinyjson.h>
using namespace std;
namespace Ewok {

class LayoutWidget: public Container {
	bool load(Widget* wd, json_var_t* var, const string& type);
	bool loadChildren(Widget* wd, json_node_t* node, const string& type);
	bool loadMenubarItems(Widget* wd, json_node_t* node, const string& type);
	bool loadMenu(Menu* menu, json_var_t* node);
	bool loadMenuItems(Menu* menu, json_node_t* node);
	Widget* createByBasicType(const string& type);
	Widget* create(const string& type);
protected:
	virtual Widget* createByType(const string& type);
	
public:
	LayoutWidget();
	bool loadConfig(const string& fname);

	MenuFuncT onMenuItemFunc;
	Widget* (*createByTypeFunc)(const string& type);
};

}

#endif
