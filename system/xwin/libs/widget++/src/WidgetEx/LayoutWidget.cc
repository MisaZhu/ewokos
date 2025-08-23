#include <tinyjson/tinyjson.h>
#include <Widget/Blank.h>
#include <Widget/Image.h>
#include <Widget/Split.h>
#include <Widget/Splitter.h>
#include <Widget/Slider.h>
#include <Widget/Button.h>
#include <Widget/LabelButton.h>
#include <Widget/Label.h>
#include <Widget/EditLine.h>
#include <Widget/Text.h>
#include <WidgetEx/Menubar.h>
#include <WidgetEx/Menu.h>

#include <WidgetEx/LayoutWidget.h>

namespace Ewok {

LayoutWidget::LayoutWidget() {
    type = HORIZONTAL;
    createByTypeFunc = NULL;
    onMenuItemFunc = NULL;
}

Widget* LayoutWidget::createByBasicType(const string& type) {
    if(type == "Container") {
        return new Container();
    }
    else if(type == "Blank") {
        return new Blank();
    }
    else if(type == "Image") {
        return new Image(); 
    }
    else if(type == "Split") {
        return new Split();
    }
    else if(type == "Splitter") {
        return new Splitter();
    }
    else if(type == "Button") {
        return new Button();
    }
    else if(type == "EditLine") {
        return new EditLine();
    }
    else if(type == "Label") {
        return new Label();
    }
    else if(type == "LabelButton") {
        return new LabelButton();
    }
    else if(type == "Scoller") {
        return new Scroller();
    }
    else if(type == "Slider") {
        return new Slider();
    } 
    else if(type == "Text") {
        return new Text();
    } 
    else if(type == "Menubar") {
        return new Menubar();
    }
    return NULL;
}

Widget* LayoutWidget::createByType(const string& type) {
    string str = "type: ";
    str += type;
    Label* label = new Label(str);
    label->setAlpha(false);
    return label;
}

Widget* LayoutWidget::create(const string& type) {
    string tp = type;
    if(tp == "")
        tp = "Container";
    Widget* wd = createByBasicType(tp);
    if(wd != NULL)
        return wd;
    if(createByTypeFunc != NULL)
        return createByTypeFunc(tp); //use user define create func.
    return createByType(tp);
}

bool LayoutWidget::loadChildren(Widget* wd, json_node_t* node, const string& type) {
    json_var_t* v = node->var;
    uint32_t len = json_var_array_size(v);

    uint32_t j;
    for (j=0; j<len; j++) {
        json_node_t* n = json_var_array_get(v, j);
        if(n == NULL || n->var == NULL || n->var->type != JSON_V_OBJECT)
            continue;
        const char* tp = json_get_str(n->var, "type");
        Widget* chd = create(tp);
        if(chd == NULL)
            continue;
        chd->setEventFunc(onEventFunc, this);
        ((Container*)wd)->add(chd);
        load(chd, n->var, tp);
    }
    return true;
}


bool LayoutWidget::loadMenuItems(Menu* menu, json_node_t* node) {
    json_var_t* v = node->var;
    uint32_t len = json_var_array_size(v);

    uint32_t j;
    for (j=0; j<len; j++) {
        json_node_t* n = json_var_array_get(v, j);
        if(n == NULL || n->var == NULL || n->var->type != JSON_V_OBJECT)
            continue;
        Menu* submenu = NULL;
        json_var_t* menu_var = json_get_obj(n->var, "menu");
        if(menu_var != NULL) {
            submenu = new Menu();
            if(!loadMenu(submenu, menu_var))
                return false;
        }

        const char* label = json_get_str(n->var, "label");
        int32_t id = json_get_int(n->var, "id");
        menu->add(id, label, NULL, submenu, this->onMenuItemFunc, this);
    }
    return true;
}

bool LayoutWidget::loadMenu(Menu* menu, json_var_t* menu_var) {
    for(uint32_t i=0; i<menu_var->children.size; i++) {
		json_node_t* node = (json_node_t*)menu_var->children.items[i];
        if(node == NULL)
            break;

		if(node->name != NULL && node->var != NULL) {
            if(strcmp(node->name, "items") == 0 &&
                    node->var->json_is_array) { //load children
                if(!loadMenuItems(menu, node))
                    return false;
			}
		}
	}
    return true;
}

bool LayoutWidget::loadMenubarItems(Widget* wd, json_node_t* node, const string& type) {
    json_var_t* v = node->var;
    uint32_t len = json_var_array_size(v);

    uint32_t j;
    for (j=0; j<len; j++) {
        json_node_t* n = json_var_array_get(v, j);
        if(n == NULL || n->var == NULL || n->var->type != JSON_V_OBJECT)
            continue;
        Menu* menu = NULL;
        json_var_t* menu_var = json_get_obj(n->var, "menu");
        if(menu_var != NULL) {
            menu = new Menu();
            if(!loadMenu(menu, menu_var))
                return false;
        }

        const char* label = json_get_str(n->var, "label");
        int32_t id = json_get_int(n->var, "id");
        Menubar *mb = (Menubar*)wd;
        mb->add(id, label, NULL, menu, this->onMenuItemFunc, this);
    }
    return true;
}

bool LayoutWidget::load(Widget* wd, json_var_t* var, const string& type) {
	for(uint32_t i=0; i<var->children.size; i++) {
		json_node_t* node = (json_node_t*)var->children.items[i];
        if(node == NULL)
            break;

		if(node->name != NULL && node->var != NULL) {
			if(wd->isContainer() &&
                    strcmp(node->name, "children") == 0 &&
                    node->var->json_is_array) { //load children
                if(!loadChildren(wd, node, type))
                    return false;
			}
            else if(type == "Menubar" &&
                    strcmp(node->name, "items") == 0 &&
                    node->var->json_is_array) { //load children
                if(!loadMenubarItems(wd, node, type))
                    return false;
			}
            else { //set attr
                wd->set(node->name, node->var);
            }
		}
	}
    return true;
}

bool LayoutWidget::loadConfig(const string& fname) {
    json_var_t* conf_var = json_parse_file(fname.c_str());
    if(conf_var == NULL)
        return false;
    
    bool ret = load(this, conf_var, "");
    json_var_unref(conf_var);
    return ret;
}

}