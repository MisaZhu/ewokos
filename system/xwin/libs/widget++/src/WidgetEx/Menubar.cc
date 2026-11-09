#include <WidgetEx/Menu.h>
#include <WidgetEx/Menubar.h>
#include <x++/X.h>

using namespace Ewok;

void Menubar::drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
    graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
}

void Menubar::drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
    if(index < 0 || index >= items.size())
        return;
    MenuItem *item = items.at(index);

	if(index == itemSelected)
		graph_fill_round(g, r.x+1, r.y+1, r.w-2, r.h-2, X::hidpiSize(3), theme->basic.selectBGColor);

    int dx = 0;
    if(item->icon != NULL) {
        /*hidpi: icons are 1x assets, blit-fit them into a hidpiSize-scaled
          cell so they match the 2x text and item height*/
        int32_t iw = X::hidpiSize(item->icon->w);
        int32_t ih = X::hidpiSize(item->icon->h);
        graph_blt_fit_alpha(item->icon, 0, 0, item->icon->w, item->icon->h,
                g, r.x+dx, r.y+(r.h-ih)/2, iw, ih, 0xff);
        dx += iw;
    }

    font_t* font = theme->getFont();
    if(item->title.length() > 0) {
        graph_draw_text_font_align(g, r.x+dx, r.y, r.w-dx, r.h, item->title.c_str(),
                font, theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
    }
}

void Menubar::onEnter(int index) {
    MenuItem *item = items.at(index);
    if(item->func != NULL)
        item->func(item, item->funcArg);

    if(item->menu != NULL) {
        gpos_t pos = getScreenPos(area.x, area.y);
        if(item->menu->getCWin() == NULL) {
    		item->menu->popup(getWin(), pos.x + index*itemSize, pos.y+area.h,
                    X::hidpiSize(100), item->menu->getItemNum()*item->menu->getItemSize(), "menu", XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_TITLE);
        }
        else {
    		item->menu->moveTo(pos.x + index*itemSize, pos.y+area.h);
        }
        item->menu->pop();
    }
    else 
        itemSelected = -1;
}

Menubar::Menubar() {
    onMenuItemFunc = NULL;
    itemSelected = -1;
	setDefaultScrollType(Scrollable::SCROLL_TYPE_H);
    setItemSize(X::hidpiSize(60));
}

Menubar::~Menubar() {
    for(int i=0; i<items.size(); i++) {
        MenuItem *item = items.at(i);
        if(item != NULL)
            delete item;
    }
}

void Menubar::add(uint32_t id, const string& title, graph_t* icon, Menu* menu, MenuFuncT func, void* funcArg) {
    MenuItem *item = new MenuItem();
    item->id = id; 
    item->title = title;
    item->icon = icon;
    item->menu = menu;
    if(func!= NULL)
        item->func = func;
    else
        item->func = onMenuItemFunc;
    item->funcArg = funcArg;

    if(menu != NULL)
    	menu->attachMenubar(this);

    items.push_back(item);
    setItemNum(items.size());
}