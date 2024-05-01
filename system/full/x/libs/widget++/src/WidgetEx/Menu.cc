#include <WidgetEx/Menu.h>
#include <WidgetEx/Menubar.h>
#include <Widget/RootWidget.h>

using namespace Ewok;

class MenuList: public List {
    vector<MenuItem*> items;
    Menu* menu;
protected:
    void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
        graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
    }

    void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
        if(index < 0 || index >= items.size())
            return;
        MenuItem *item = items.at(index);

        if(index == itemSelected)
            graph_fill_round(g, r.x+1, r.y+1, r.w-2, r.h-2, 3, theme->basic.selectBGColor);

        int dx = 0;

        if(item->icon != NULL) {
            graph_blt_alpha(item->icon, 0, 0, item->icon->w, item->icon->h, 
                    g, r.x+dx, r.y, item->icon->w, item->icon->h, 0xff);
            dx += item->icon->w;
        }

        if(item->title.length() > 0) {
            graph_draw_text_font_align(g, r.x+dx, r.y, r.w-dx, r.h, item->title.c_str(),
                    theme->getFont(), theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
        }

        if(item->menu != NULL) {
            graph_fill(g, r.x+r.w-9, r.y+(r.h/2)-2, 4, 4, theme->basic.fgColor);
        }
    }

    void onEnter(int index) {
        MenuItem *item = items.at(index);
        if(item->func != NULL) {
            item->func(item, item->funcArg);
        }

        if(item->menu != NULL) {
            menu->subMenu(true);
            gpos_t pos = getScreenPos(area.x, area.y);
            if(item->menu->getCWin() == NULL) {
                item->menu->open(getWin()->getX(), 0, pos.x + area.w, pos.y+ index*itemSize,
                        100, item->menu->getItemNum()*item->menu->getItemSize(), "menu", XWIN_STYLE_NO_FRAME);
            }
            else  {
        		item->menu->moveTo(pos.x + area.w, pos.y+ index*itemSize);
            }
            item->menu->pop();
            return;
        }

        menu->hide();
    }
public:
    friend Menu;
    MenuList(Menu* menu) {
        this->menu = menu;
    }
};

void Menu::subMenu(bool s) {
    subMenued = s;
}

void Menu::onUnfocus() {
    if(subMenued)
        return;
    hide(); 
}

void Menu::hide() {
    Menu* m = this;
    while(m != NULL) {
        m->subMenu(false);
        m->setVisible(false);
        if(m->menubar != NULL) {
            m->menubar->select(-1);
            m->menubar->getWin()->repaint();
        }
        m = m->menu;
    }
}

Menu::Menu() {
    subMenued = false;
    menubar = NULL;
    menu = NULL;
    itemSize = 24;
    build();
}

void Menu::onBuild() {
    Popup::onBuild();
    MenuList* list = new MenuList(this);
    list->setID(1);
    list->setDefaultScrollType(Scrollable::SCROLL_TYPE_V);
    root->add(list);
}

uint32_t Menu::getItemNum() {
    MenuList* list = (MenuList*)root->get(1);
    return list->getItemNum();
}

void Menu::add(const string& title, graph_t* icon, Menu* menu, menufunc_t func, void* funcArg) {
    MenuList* list = (MenuList*)root->get(1);
    if(list == NULL)
        return;
    
    if(menu != NULL)
        menu->attachMenu(this);

    MenuItem *item = new MenuItem();
    item->title = title;
    item->icon = icon;
    item->menu = menu;
    item->func = func;
    item->funcArg = funcArg;

    list->items.push_back(item);
    list->setItemNum(list->items.size());
    list->setItemNumInView(list->items.size());
}