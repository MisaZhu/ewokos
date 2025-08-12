#include <WidgetEx/ColorDialog.h>
#include <Widget/LabelButton.h>
#include <Widget/EditLine.h>
#include <Widget/Slider.h>
#include <x++/X.h>
#include <vector>

using namespace Ewok;

// 生成 64 种颜色
std::vector<uint32_t> generate64Colors() {
    std::vector<uint32_t> colors;
    for (int i = 3; i >= 0; --i) {
        uint8_t red = i * 85;
        uint8_t green = i * 85;
        uint8_t blue = i * 85;
        colors.push_back(0xFF000000 | (red << 16) | (green << 8) | blue);
    }

    for (int r = 0; r < 4; ++r) {
        for (int g = 0; g < 4; ++g) {
            for (int b = 0; b < 4; ++b) {
                uint8_t red = r * 85;
                uint8_t green = g * 85;
                uint8_t blue = b * 85;
                if(red == green && green == blue)
                    continue;
                colors.push_back(0xFF000000 | (red << 16) | (green << 8) | blue);
            }
        }
    }
    return colors;
}

class ColorPanel: public Widget {
    uint32_t color;
protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        graph_set(g, r.x, r.y, r.w, r.h, color | 0xFF000000);
        graph_set(g, r.x, r.y+r.h/2, r.w, r.h/2, color);
		graph_frame(g, r.x, r.y, r.w, r.h, 2, theme->basic.bgColor, true);
    }
public:
    ColorPanel() {
        color = 0xFF000000;
    }

    void setColor(uint32_t c) {
        ColorDialog* dialog = (ColorDialog*)getWin();
        color = (c & 0x00FFFFFF) | (dialog->getTransparent() << 24);
        dialog->setAlpha(true);
        update();
    }

    uint32_t getColor() const {
        return color;
    }

    void setTransparent(uint8_t transparent) {
        ColorDialog* dialog = (ColorDialog*)getWin();
        color = (color & 0x00FFFFFF) | (transparent << 24);
        dialog->setAlpha(true);
        update();
    }
};

class ColorDialog::ColorWidget : public Widget {
    ColorDialog* dialog;
    std::vector<uint32_t> colors;
    int selectedIndex = -1;
    EditLine* hexEditLine;
    ColorPanel* colorPanel;
    Slider* sliderWidget;

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);

        // 固定每行 16 个颜色
        const int cols = 16;
        int rows = (colors.size() + cols - 1) / cols;

        // 确保色彩块填满 widget
        int cellWidth = r.w / cols;
        int cellHeight = r.h / rows;

        for (size_t i = 0; i < colors.size(); ++i) {
            int x = r.x + (i % cols) * cellWidth;
            int y = r.y + (i / cols) * cellHeight;
            graph_fill(g, x, y, cellWidth, cellHeight, colors[i]);

            if (static_cast<int>(i) == selectedIndex) {
                // 绘制选中边框
                graph_box_3d(g, x, y, cellWidth, cellHeight, 0xFF000000, 0xFFffffff);
            }
        }
    }

    bool onMouse(xevent_t* ev) {
        if (ev->state == MOUSE_STATE_CLICK) {
            gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);

            // 固定每行 16 个颜色
            const int cols = 16;
            int rows = (colors.size() + cols - 1) / cols;

            // 确保色彩块填满 widget
            int cellWidth = area.w / cols;
            int cellHeight = area.h / rows;

            int colIndex = pos.x / cellWidth;
            int rowIndex = pos.y / cellHeight;
            int index = rowIndex * cols + colIndex;

            if (index >= 0 && index < static_cast<int>(colors.size())) {
                selectedIndex = index;
                uint32_t color = getSelectedColor() ;
                uint8_t transparent = getTransparent();
                color = (color & 0x00FFFFFF) | (transparent << 24);
                setColor(color);
                update();
            }
            return true;
        }
        return false;
    }

    uint32_t getSelectedColor() const {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(colors.size())) {
            return colors[selectedIndex];
        }
        return 0xFF000000;
    }
public:
    ColorWidget(ColorDialog* d) : dialog(d) {
        colors = generate64Colors();
        colorPanel = NULL;
        hexEditLine = NULL;
    }

    uint32_t getColor() const {
        if(colorPanel == NULL)
            return 0xFF000000;
        return colorPanel->getColor();
    }

    uint8_t getTransparent() const {
        if(sliderWidget == NULL)
            return 0;
        return  0xff - (0x0ff & sliderWidget->getValue());
    }

    void setColor(uint32_t color) {
        char hex[10];
        uint8_t transparent = (color >> 24) & 0xff;
        snprintf(hex, sizeof(hex), "0x%06x", color & 0x00FFFFFF);
        if (hexEditLine)
            hexEditLine->setContent(hex);
        if (colorPanel) {
            colorPanel->setColor(color);
            colorPanel->setTransparent(transparent);
        }
        if(sliderWidget) {
            sliderWidget->setValue(0xff - transparent);
        }
    }

    void setHexEditLine(EditLine* editLine) {
        hexEditLine = editLine;
    }

    void setColorPanel(ColorPanel* panel) {
        colorPanel = panel;
    }

    void setTransparentSlider(Slider* slider) {
        sliderWidget = slider;
    }

    void setTransparent(uint8_t transparent) {
        if(colorPanel != NULL)
            colorPanel->setTransparent(transparent);
    }

    static void onEditLineChange(Widget* wd) {
        EditLine* editLine = (EditLine*)wd;
        ColorDialog* dialog = (ColorDialog*)editLine->getWin();
        ColorWidget* colorWidget = dialog->getColorWidget();
        uint32_t color = strtoul(editLine->getContent().c_str(), 0, 16);
        color = color & 0x00FFFFFF;
        colorWidget->colorPanel->setColor(color);
    }
};

class TransparentSlider : public Slider {
protected:
    void onPosChange() {
        ColorDialog* dialog = (ColorDialog*)getWin();
        uint8_t transp =  0xff - (0x0ff & getValue());
        dialog->getColorWidget()->setTransparent(transp);
        dialog->setAlpha(true);
    }
public:
    TransparentSlider() {
        setRange(255);
        horizontal = true;
    }
};

static void okFunc(Widget* wd, xevent_t* evt, void* arg) {
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
    ColorDialog* dialog = (ColorDialog*)wd->getWin();
    dialog->submit(Dialog::RES_OK);
}

static void cancelFunc(Widget* wd, xevent_t* evt, void* arg) {
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
		return;
    ColorDialog* dialog = (ColorDialog*)wd->getWin();
    dialog->submit(Dialog::RES_CANCEL);
}

void ColorDialog::onBuild() {
    RootWidget* root = new RootWidget();
    setRoot(root);
    root->setType(Container::VERTICLE);

    colorWidget = new ColorWidget(this);
    root->add(colorWidget);

    Container* c = new Container();
    c->setType(Container::HORIZONTAL);
	root->add(c);
	c->fix(0, 60);

    ColorPanel* color = new ColorPanel();
	color->fix(60, 0);
    c->add(color);
    colorWidget->setColorPanel(color);

    Container* v = new Container();
    v->setType(Container::VERTICLE);
    c->add(v);

    EditLine* editLine = new EditLine();
	v->add(editLine);
    colorWidget->setHexEditLine(editLine);
    editLine->onInputFunc = ColorWidget::onEditLineChange;
	root->focus(editLine);

    TransparentSlider* slider = new TransparentSlider();
	v->add(slider);
    colorWidget->setTransparentSlider(slider);

    c = new Container();
    c->setType(Container::HORIZONTAL);
    c->fix(0, 20);
    root->add(c);

    LabelButton* okButton = new LabelButton("OK");
    okButton->setEventFunc(okFunc);
    c->add(okButton);

    LabelButton* cancelButton = new LabelButton("Cancel");
    cancelButton->setEventFunc(cancelFunc);
    c->add(cancelButton);
    setAlpha(true);
}

ColorDialog::ColorDialog() {
}

void ColorDialog::setColor(uint32_t color) {
    colorWidget->setColor(color);
}

uint32_t ColorDialog::getColor() {
    return colorWidget->getColor();
}

uint8_t ColorDialog::getTransparent() {
    return  colorWidget->getTransparent();
}