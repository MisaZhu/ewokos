#ifndef WIDGET_MENU_HH
#define WIDGET_MENU_HH

#include <WidgetEx/Popup.h>

#include <string>
#include <vector>
using namespace std;

namespace Ewok {

class Menubar;
class MenuItem;
typedef void (*MenuFuncT)(MenuItem*, void*);

class Menu: public Popup {
protected:
    Menubar* menubar;
    Menu* menu;
    void onUnfocus();
    void onBuild();
    bool subMenued;
    uint32_t itemSize;
    MenuFuncT onMenuItemFunc;
public:
    inline void setMenuItemFunc(MenuFuncT func) { onMenuItemFunc = func; }
    inline void setItemSize(uint32_t size) { itemSize = size; }
    inline uint32_t getItemSize() { return itemSize; }

    inline void attachMenubar(Menubar* bar) { menubar = bar; }
    inline void attachMenu(Menu* menu) { this->menu = menu; }

    Menu();
    uint32_t getItemNum();
    void add(uint32_t id, const string& title, graph_t* icon, Menu* menu, MenuFuncT func, void* funcArg);
    void subMenu(bool s);
    void hide();
};

class MenuItem : public UniObject {
public:
    string title;
    uint32_t id;
    graph_t* icon;
    Menu* menu;
    MenuFuncT func;
    void* funcArg;

    inline MenuItem() {
        id = 0;
        icon = NULL;
        menu = NULL;
        func = NULL;
        funcArg = NULL;
    }

    inline ~MenuItem() {
        if(icon != NULL)
            graph_free(icon);

        if(menu != NULL)
            delete menu;
    }
};



}

#endif