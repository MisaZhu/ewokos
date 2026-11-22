#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/SpriteWin.h>
#include <Widget/Container.h>
#include <Widget/Label.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace Ewok;

// Color definitions matching the reference style.
static const uint32_t COLOR_HEADER_RED = 0xFFD32F2F;      // Red header bar
static const uint32_t COLOR_HEADER_TEXT = 0xFFFFFFFF;      // White header text
static const uint32_t COLOR_CELL_BG = 0xFFFFFFFF;          // White day cell background
static const uint32_t COLOR_CELL_TEXT = 0xFF000000;        // Black day number text
static const uint32_t COLOR_TODAY_BG = 0xFFFFCDD2;         // Highlight background for today
static const uint32_t COLOR_TODAY_TEXT = 0xFFD32F2F;       // Highlight text color for today
static const uint32_t COLOR_BORDER = 0xFF888888;           // Border color
static const uint32_t COLOR_WEEKDAY_BG = 0xFFEEEEEE;       // Weekday row background
static const uint32_t COLOR_WEEKDAY_TEXT = 0xFF333333;     // Weekday text color
static const uint32_t COLOR_OTHER_MONTH = 0xFFAAAAAA;      // Day color for adjacent months
static const uint32_t COLOR_SHADOW = 0xFF666666;           // Shadow color

// Month names
static const char* MONTH_NAMES[] = {
    "January", "February", "March", "April",
    "May", "June", "July", "August",
    "September", "October", "November", "December"
};

// Weekday names
static const char* WEEKDAY_NAMES[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// Calendar widget
class CalendarWidget : public Widget {
private:
    int currentYear;
    int currentMonth;
    int currentDay;
    int todayYear;
    int todayMonth;
    int todayDay;
    
    // Return the number of days in the given month.
    int getDaysInMonth(int year, int month) {
        static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (month == 2 && isLeapYear(year)) return 29;
        return days[month - 1];
    }
    
    // Check whether the year is a leap year.
    bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    }
    
    // Compute the weekday of the first day of the month. (0 = Sunday, 1 = Monday, ...)
    int getFirstDayOfMonth(int year, int month) {
        // Simplified Zeller formula.
        int m = month;
        int y = year;
        if (m < 3) {
            m += 12;
            y--;
        }
        int c = y / 100;
        int d = y % 100;
        int f = 1; // First day of the month
        
        int w = (d + d/4 + c/4 - 2*c + 26*(m+1)/10 + f - 1) % 7;
        if (w < 0) w += 7;
        return w;
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // Draw the shadow border for a simple 3D effect.
        int shadowOffset = X::hidpiSize(4);
        graph_fill_rect(g, r.x + shadowOffset, r.y + shadowOffset, r.w, r.h, COLOR_SHADOW);
        
        // Draw the main background.
        graph_fill_rect(g, r.x, r.y, r.w, r.h, 0xFFFFFFFF);
        
        // Draw the outer border.
        graph_rect(g, r.x, r.y, r.w, r.h, COLOR_BORDER);
        
        // Compute the layout.
        int padding = X::hidpiSize(13);
        int headerHeight = X::hidpiSize(50);
        int weekdayHeight = X::hidpiSize(30);
        int cellHeight = (r.h - headerHeight - weekdayHeight - padding * 2) / 6;
        // Width of the content area, excluding the outer border.
        int contentW = r.w - padding * 2;
        // Each cell width equals the content width divided by 7 columns.
        int cellWidth = contentW / 7;

        int contentX = r.x + padding;
        int contentY = r.y + padding;
        
        // Draw the red title bar.
        graph_fill_rect(g, contentX, contentY, contentW, headerHeight, COLOR_HEADER_RED);
        
        // Draw the previous/next month button areas.
        int buttonWidth = X::hidpiSize(40);
        int buttonHeight = headerHeight - X::hidpiSize(10);
        int buttonY = contentY + X::hidpiSize(5);
        
        // Left button for the previous month.
        int leftBtnX = contentX + X::hidpiSize(5);
        graph_fill_round(g, leftBtnX, buttonY, buttonWidth, buttonHeight, X::hidpiSize(8), 0x99FFFFFF);
        // Draw the left arrow.
        int arrowY = buttonY + buttonHeight / 2;
        int arrowLeftX = leftBtnX + X::hidpiSize(12);
        int arrowRightX = leftBtnX + buttonWidth - X::hidpiSize(12);
        int arrowTopY = arrowY - X::hidpiSize(8);
        int arrowBottomY = arrowY + X::hidpiSize(8);
        graph_line(g, arrowRightX, arrowTopY, arrowLeftX, arrowY, COLOR_HEADER_TEXT);
        graph_line(g, arrowRightX, arrowBottomY, arrowLeftX, arrowY, COLOR_HEADER_TEXT);
        
        // Right button for the next month.
        int rightBtnX = contentX + contentW - buttonWidth - X::hidpiSize(5);
        graph_fill_round(g, rightBtnX, buttonY, buttonWidth, buttonHeight, X::hidpiSize(8), 0x99FFFFFF);
        // Draw the right arrow.
        arrowLeftX = rightBtnX + 12;
        arrowRightX = rightBtnX + buttonWidth - 12;
        graph_line(g, arrowLeftX, arrowTopY, arrowRightX, arrowY, COLOR_HEADER_TEXT);
        graph_line(g, arrowLeftX, arrowBottomY, arrowRightX, arrowY, COLOR_HEADER_TEXT);
        
        // Draw the month title text.
        char title[64];
        snprintf(title, sizeof(title), "%s %d", MONTH_NAMES[currentMonth - 1], currentYear);
        
        uint32_t tw, th;
        font_text_size(title, theme->getFont(), theme->basic.fontSize + 12, &tw, &th);
        int titleX = contentX + (contentW - (int32_t)tw) / 2;
        int titleY = contentY + (headerHeight - (int32_t)th) / 2;
        graph_draw_text_font(g, titleX, titleY, title, theme->getFont(),
                            theme->basic.fontSize + 12, COLOR_HEADER_TEXT);
        
        // Draw the weekday row background.
        int weekdayY = contentY + headerHeight;
        graph_fill_rect(g, contentX, weekdayY, contentW, weekdayHeight, COLOR_WEEKDAY_BG);
        
        // Draw the weekday labels.
        for (int i = 0; i < 7; i++) {
            int cellX = contentX + i * cellWidth;
            font_text_size(WEEKDAY_NAMES[i], theme->getFont(), theme->basic.fontSize, &tw, &th);
            int textX = cellX + (cellWidth - (int32_t)tw) / 2;
            int textY = weekdayY + (weekdayHeight - (int32_t)th) / 2;
            graph_draw_text_font(g, textX, textY, WEEKDAY_NAMES[i], theme->getFont(),
                                theme->basic.fontSize, COLOR_WEEKDAY_TEXT);
        }
        
        // Draw the day cells.
        int calendarStartY = weekdayY + weekdayHeight;
        int firstDay = getFirstDayOfMonth(currentYear, currentMonth);
        int daysInMonth = getDaysInMonth(currentYear, currentMonth);
        
        // Days carried over from the previous month.
        int prevMonthDays = getDaysInMonth(currentMonth == 1 ? currentYear - 1 : currentYear, 
                                           currentMonth == 1 ? 12 : currentMonth - 1);
        int prevMonthStartDay = prevMonthDays - firstDay + 1;
        
        // Draw the 6-by-7 calendar grid.
        for (int row = 0; row < 6; row++) {
            for (int col = 0; col < 7; col++) {
                int cellX = contentX + col * cellWidth;
                int cellY = calendarStartY + row * cellHeight;

                // Draw the cell border.
                graph_rect(g, cellX, cellY, cellWidth, cellHeight, COLOR_BORDER);

                // Resolve which day number belongs to this cell.
                int dayNum = 0;
                bool isCurrentMonth = false;
                bool isToday = false;

                int cellIndex = row * 7 + col;
                if (cellIndex < firstDay) {
                    // Day from the previous month.
                    dayNum = prevMonthStartDay + cellIndex;
                } else if (cellIndex < firstDay + daysInMonth) {
                    // Day from the current month.
                    dayNum = cellIndex - firstDay + 1;
                    isCurrentMonth = true;
                    // Check whether this cell represents today.
                    if (dayNum == todayDay && currentMonth == todayMonth && currentYear == todayYear) {
                        isToday = true;
                    }
                } else {
                    // Day from the next month.
                    dayNum = cellIndex - firstDay - daysInMonth + 1;
                }

                // Draw the cell background.
                if (isToday) {
                    graph_fill_rect(g, cellX + 1, cellY + 1, cellWidth - 2, cellHeight - 2, COLOR_TODAY_BG);
                } else {
                    graph_fill_rect(g, cellX + 1, cellY + 1, cellWidth - 2, cellHeight - 2, COLOR_CELL_BG);
                }

                // Draw the day number.
                char dayStr[16];
                snprintf(dayStr, sizeof(dayStr), "%d", dayNum);

                uint32_t textColor = isCurrentMonth ?
                    (isToday ? COLOR_TODAY_TEXT : COLOR_CELL_TEXT) : COLOR_OTHER_MONTH;

                font_text_size(dayStr, theme->getFont(), theme->basic.fontSize + 4, &tw, &th);
                int textX = cellX + (cellWidth - (int32_t)tw) / 2;
                int textY = cellY + (cellHeight - (int32_t)th) / 2;
                graph_draw_text_font(g, textX, textY, dayStr, theme->getFont(),
                                    theme->basic.fontSize + 4, textColor);
            }
        }
    }
    
    bool onMouse(xevent_t* ev) {
        if (ev->state == MOUSE_STATE_UP) {
            // Get the click position.
            gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
            int x = pos.x;
            int y = pos.y;

            // Compute the button bounds.
            int padding = 10;
            int headerHeight = X::hidpiSize(50);
            int buttonWidth = X::hidpiSize(40);
            int buttonHeight = headerHeight - X::hidpiSize(10);
            int buttonY = padding + 5;
            int leftBtnX = padding + 5;
            int rightBtnX = area.w - padding - buttonWidth - 5;

            // Check whether the click falls inside the header area.
            if (y >= buttonY && y <= buttonY + buttonHeight) {
                if (x >= leftBtnX && x <= leftBtnX + buttonWidth) {
                    // Left button: switch to the previous month.
                    currentMonth--;
                    if (currentMonth < 1) {
                        currentMonth = 12;
                        currentYear--;
                    }
                    update();
                } else if (x >= rightBtnX && x <= rightBtnX + buttonWidth) {
                    // Right button: switch to the next month.
                    currentMonth++;
                    if (currentMonth > 12) {
                        currentMonth = 1;
                        currentYear++;
                    }
                    update();
                }
                return true;
            }
        }
        return false;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        // If the current year is still 1970, retry fetching the date every 200 ms.
        if (currentYear == 1970) {
            time_t now = time(NULL);
            struct tm time_info;
            localtime_r(&now, &time_info);

            int newYear = time_info.tm_year + 1900;
            int newMonth = time_info.tm_mon + 1;
            int newDay = time_info.tm_mday;

            // Refresh the widget when the date changes.
            if (newYear != currentYear || newMonth != currentMonth || newDay != currentDay) {
                currentYear = newYear;
                currentMonth = newMonth;
                currentDay = newDay;
                todayYear = newYear;
                todayMonth = newMonth;
                todayDay = newDay;
                update();
            }
        }
    }

public:
    CalendarWidget() : Widget() {
        // Read the current local date.
        time_t now = time(NULL);
        struct tm time_info;
        localtime_r(&now, &time_info);

        todayYear = currentYear = time_info.tm_year + 1900;
        todayMonth = currentMonth = time_info.tm_mon + 1;
        todayDay = currentDay = time_info.tm_mday;
    }
};

int main(int argc, char** argv) {
    X x;
    SpriteWin win;
    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::HORIZONTAL);
    
    CalendarWidget* calendar = new CalendarWidget();
    root->add(calendar);
    
    grect_t wr;
    wr.x = 200;
    wr.y = 100;
    wr.w = X::hidpiSize(320);
    wr.h = X::hidpiSize(300);
    
    win.open(&x, -1, wr, "Calendar", XWIN_STYLE_SPRITE);
    win.setTimer(2);
    widgetXRun(&x, &win);
    return 0;
}
