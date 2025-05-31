#ifndef COLOR_DIALOG_H
#define COLOR_DIALOG_H

#include <WidgetEx/Dialog.h>
#include <string>

using namespace Ewok;

class ColorDialog : public Dialog {
private:
    class ColorWidget;
    ColorWidget* colorWidget;

    void onBuild() override;

public:
    ColorDialog();
    uint32_t getResult();
};

#endif