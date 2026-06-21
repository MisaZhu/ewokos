#include <Widget/SpriteWin.h>
#include <Widget/WidgetX.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/kernel_tic.h>
#include <openlibm.h>
#include <ewoksys/vdevice.h>
#include <time.h>

using namespace Ewok;

class CircularClock : public Widget {
    uint32_t sec;
    uint32_t min;
    uint32_t hour;

    // Draw the clock ticks.
    void drawClockTicks(graph_t* g, XTheme* theme, const grect_t& r) {
        int centerX = r.x + r.w / 2;
        int centerY = r.y + r.h / 2;
        int radius = (r.w < r.h ? r.w : r.h) / 2 - 10;

        graph_set(g, r.x, r.y, r.w, r.h, 0x0);
        // Draw the white clock face.
        graph_fill_circle(g, centerX, centerY, radius, 0xffffffff);
        graph_circle(g, centerX, centerY, radius, 3, 0xff000000);
        radius -= 6;

        // Draw the hour numbers.
        font_t* font = theme->getFont();
        int fontSize = 10;
        for (int hour = 1; hour <= 12; ++hour) {
            double angle = (hour * 30 - 90) * M_PI / 180; // Offset by 90 degrees so 12 o'clock is at the top
            int textX = centerX + (int)((radius - 12) * cos(angle));
            int textY = centerY + (int)((radius - 12) * sin(angle))- fontSize + 2;
            char text[3];
            snprintf(text, sizeof(text), "%d", hour);
            uint32_t textWidth, textHeight;
            font_text_size(text, font, fontSize, &textWidth, &textHeight);
            graph_draw_text_font(g, textX - textWidth / 2, textY + textHeight / 4, text, font, fontSize, 0xff000000);
        }

        for (int i = 0; i < 60; ++i) {
            double angle = i * M_PI / 30;
            int dotX = centerX + (int)(radius * cos(angle));
            int dotY = centerY + (int)(radius * sin(angle));
            int dotRadius;

            if (i % 5 == 0) {
                // Use a slightly larger dot for hour marks.
                dotRadius = 1;
                graph_fill_circle(g, dotX, dotY, dotRadius, 0xff000000);
            } else {
                // Minor ticks use a smaller dot radius.
                //dotRadius = 1;
                //graph_fill_circle(g, dotX, dotY, dotRadius, 0xff000000);
            }

        }
    }

    // Draw the hands.
    void drawHands(graph_t* g, XTheme* theme, const grect_t& r) {
        int centerX = r.x + r.w / 2;
        int centerY = r.y + r.h / 2;
        int radius = (r.w < r.h ? r.w : r.h) / 2 - 10;
    
        // Define the widths of the hour, minute, and second hands.
        int hourHandWidth = 5; // The hour hand is the thickest
        int minHandWidth = 3;  // The minute hand is thicker than the second hand
        int secHandWidth = 2;  // The second hand is the thinnest
    
        // Hour hand
        double hourAngle = (hour % 12 + min / 60.0) * (M_PI / 6);
        int hourX = centerX + (int)(radius * 0.5 * cos(hourAngle - M_PI / 2));
        int hourY = centerY + (int)(radius * 0.5 * sin(hourAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, hourX, hourY, hourHandWidth, 0xff000000);
    
        // Minute hand
        double minAngle = (min + sec / 60.0) * (M_PI / 30);
        int minX = centerX + (int)(radius * 0.7 * cos(minAngle - M_PI / 2));
        int minY = centerY + (int)(radius * 0.7 * sin(minAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, minX, minY, minHandWidth, 0xff000000);
    
        // Second hand
        double secAngle = sec * (M_PI / 30);
        int secX = centerX + (int)(radius * 0.8 * cos(secAngle - M_PI / 2));
        int secY = centerY + (int)(radius * 0.8 * sin(secAngle - M_PI / 2));
        graph_wline(g, centerX, centerY, secX, secY, secHandWidth, 0xffff0000);
    
        // Draw the circle at the center pivot.
        int rootRadius = 5; // Radius of the center pivot circle
        graph_fill_circle(g, centerX, centerY, rootRadius, 0xffff0000);
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        drawClockTicks(g, theme, r);
        drawHands(g, theme, r);
    }


    uint32_t updateTime() {
        time_t now = time(NULL);
        struct tm time_info;
        localtime_r(&now, &time_info);
        return time_info.tm_hour * 3600 + time_info.tm_min * 60 + time_info.tm_sec;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        uint32_t ksec = updateTime();
        if(ksec == 0)
            return;
        min = (ksec / 60);
        hour = (min / 60);
        min = min % 60;
        sec = ksec % 60;
        update();
    }

public:
    CircularClock() {
        sec = 0;
        min = 0;
        hour = 0;
    }
};

int main(int argc, char** argv) {
    X x;
    SpriteWin win;

    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);

    CircularClock* clock = new CircularClock();
    root->add(clock);

    win.open(&x, -1, -1, -1, 160, 160, "Circular Clock", XWIN_STYLE_SPRITE);
    win.setTimer(1);
    win.setAlpha(true);
    widgetXRun(&x, &win);
    return 0;
}
