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

bool LayoutWidget::load(const string& name) {
    json_var_t* conf_var = json_parse_file(name.c_str());
    if(conf_var == NULL)
        return false;

    json_var_unref(conf_var);
    return true;
}

}