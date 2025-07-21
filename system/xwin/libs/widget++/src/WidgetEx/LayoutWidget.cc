#include <tinyjson/tinyjson.h>
#include <WidgetEx/LayoutWidget.h>
#include <Widget/Blank.h>
#include <Widget/Image.h>
#include <Widget/Split.h>
#include <Widget/Slider.h>
#include <Widget/Button.h>
#include <Widget/LabelButton.h>
#include <Widget/Label.h>
#include <Widget/EditLine.h>
#include <Widget/Text.h>

namespace Ewok {

LayoutWidget::LayoutWidget() {
    type = HORIZONTAL;
}

Widget* LayoutWidget::createByType(const string& type) {
    if(type == "Blank") {
        return new Blank();
    }
    else if(type == "Container") {
        return new Container();
    }
    else if(type == "Image") {
        return new Image(); 
    }
    else if(type == "Split") {
        return new Split();
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
    return NULL;
}

bool LayoutWidget::load(Widget* wd, json_var_t* var) {
	for(uint32_t i=0; i<var->children.size; i++) {
		json_node_t* node = (json_node_t*)var->children.items[i];
        if(node == NULL)
            break;

		if(node->name != NULL && node->var != NULL) {
			if(wd->isContainer() &&
                    strcmp(node->name, "children") == 0 &&
                    node->var->json_is_array) { //load children
                json_var_t* v = node->var;
                uint32_t len = json_var_array_size(v);
        
                uint32_t j;
                for (j=0; j<len; j++) {
                    json_node_t* n = json_var_array_get(v, j);
                    if(n == NULL || n->var == NULL || n->var->type != JSON_V_OBJECT)
                        continue;
                    const char* type = json_get_str(n->var, "type");
                    Widget* chd = createByType(type);
                    if(chd == NULL)
                        continue;
                    load(chd, n->var);
                    ((Container*)wd)->add(chd);
                }
			}
            else { //set attr
                str_t* str = str_new("");
                json_var_to_str(node->var, str);
                wd->set(node->name, str->cstr);
                str_free(str);
            }
		}
	}
    return true;
}

bool LayoutWidget::loadConfig(const string& fname) {
    json_var_t* conf_var = json_parse_file(fname.c_str());
    if(conf_var == NULL)
        return false;
    
    bool ret = load(this, conf_var);
    json_var_unref(conf_var);
    return ret;
}

}