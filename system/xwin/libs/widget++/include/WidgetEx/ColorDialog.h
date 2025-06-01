#ifndef COLOR_DIALOG_H
#define COLOR_DIALOG_H

#include <WidgetEx/Dialog.h>
#include <Widget/Slider.h>
#include <string>

using namespace Ewok;

class ColorDialog : public Dialog {
private:
    class ColorWidget;
    ColorWidget* colorWidget;
    Slider* sliderWidget;

    void onBuild() override;

public:
    ColorDialog();
    uint32_t getColor();
    uint8_t getTransparent();
};

#endif