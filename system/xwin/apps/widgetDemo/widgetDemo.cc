#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <stdlib.h>
#include <font/font.h>
#include <graph/graph_png.h>
#include <ewoksys/ewokdef.h>
#include <ewoksys/sys.h>
#include <ewoksys/session.h>
#include <ewoksys/syscall.h>
#include <procinfo.h>
#include <openlibm.h>

using namespace Ewok;

class WidgetDemo : public Widget {
    uint32_t i;
    float angle; // 新增角度变量
public:
    inline WidgetDemo() {
        i = 0;
        angle = 0.0f; // 初始化角度
    }
    
    inline ~WidgetDemo() {
    }

    void updateWidgetDemo() {
        i++;
        angle += 4.0f; // 每次刷新角度增加 4 度
        if (angle >= 360.0f) {
            angle -= 360.0f; // 确保角度在 0-360 度之间
        }
        update();
    }

    protected:

    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // 移除原有的背景填充代码
        // graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.widgetBGColor);
        
        float rad = angle * 3.1415926f / 180.0f; // 转换为弧度
        float cosAngle = cos(rad);
        float sinAngle = sin(rad);
        for (int y = r.y; y < r.y + r.h; y++) {
            for (int x = r.x; x < r.x + r.w; x++) {
                float dx = x - r.x - r.w / 2.0f;
                float dy = y - r.y - r.h / 2.0f;
                float projected = dx * cosAngle + dy * sinAngle;
                float maxProjected = sqrt(r.w * r.w + r.h * r.h) / 2.0f;
                float ratio = (projected + maxProjected) / (2.0f * maxProjected);
                if (ratio > 1.0f) ratio = 1.0f;
                if (ratio < 0.0f) ratio = 0.0f;
                
                // 定义绿色、蓝色和红色
                uint32_t green = 0xFF00FF00;
                uint32_t blue = 0xFF0000FF;
                uint32_t red = 0xFFFF0000;
                
                uint32_t startColor, endColor;
                if (ratio < 0.5f) {
                    startColor = green;
                    endColor = blue;
                    ratio = ratio * 2;
                } else {
                    startColor = blue;
                    endColor = red;
                    ratio = (ratio - 0.5f) * 2;
                }
                
                uint8_t rValue = (uint8_t)((((startColor >> 16) & 0xFF) * (1 - ratio)) + (((endColor >> 16) & 0xFF) * ratio));
                uint8_t gValue = (uint8_t)((((startColor >> 8) & 0xFF) * (1 - ratio)) + (((endColor >> 8) & 0xFF) * ratio));
                uint8_t bValue = (uint8_t)(((startColor & 0xFF) * (1 - ratio)) + ((endColor & 0xFF) * ratio));
                uint32_t color = (rValue << 16) | (gValue << 8) | bValue;
                graph_pixel(g, x, y, color);
            }
        }
        
        char txt[32] = { 0 };
        snprintf(txt, 31, "%d", i);
        font_t* font = theme->getFont();
        uint32_t w;
        font_text_size(txt, font, 10, &w, NULL);
        graph_draw_text_font(g, r.x + r.w - w, r.y, txt, theme->getFont(), 10, theme->basic.widgetFGColor);
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        updateWidgetDemo();
    }
};


int main(int argc, char** argv) {
    X x;
    WidgetWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);
    root->setAlpha(false);

    WidgetDemo* widgetDemo = new WidgetDemo();
    root->add(widgetDemo);

    win.open(&x, 0, -1, -1, 360, 140, "widgetDemo", XWIN_STYLE_NORMAL);
    win.setTimer(60);

    widgetXRun(&x, &win);    
    return 0;
}