#ifndef WIDGET_MENUBAR_HH
#define WIDGET_MENUBAR_HH

#include <Widget/List.h>

#include <string>
#include <vector>
using namespace EwokSTL;

namespace Ewok {

class Menu;

struct MenuItem {
    string title;
    graph_t* icon;
    Menu* menu;

    inline MenuItem() {
        icon = NULL;
        menu = NULL;
    }

    inline ~MenuItem() {
        if(icon != NULL)
            graph_free(icon);

        if(menu != NULL)
            delete menu;
    }
};

class Menubar: public List {
protected:
    vector<MenuItem*> items;
	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r);
	void onEnter(int index);
public:
	Menubar();
	~Menubar();

    void add(const string& title, graph_t* icon, Menu* menu);
};

}

#endif