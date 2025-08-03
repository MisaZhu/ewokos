#ifndef WIDGET_MENUBAR_HH
#define WIDGET_MENUBAR_HH

#include <Widget/List.h>
#include <WidgetEx/Menu.h>

#include <string>
#include <vector>
using namespace std;

namespace Ewok {

class Menubar: public List {
protected:
    vector<MenuItem*> items;
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r);
	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r);
	void onEnter(int index);
    MenuFuncT onMenuItemFunc;
public:
	Menubar();
	~Menubar();

	inline void setMenuItemFunc(MenuFuncT func) { onMenuItemFunc = func; }
    void add(uint32_t id, const string& title, graph_t* icon, Menu* menu, MenuFuncT func, void* funcArg);
};

}

#endif