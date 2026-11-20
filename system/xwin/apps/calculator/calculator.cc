#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/LabelButton.h>
#include <Widget/Container.h>
#include <Widget/Label.h>
#include <Widget/Blank.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

using namespace Ewok;

// Color definitions
static const uint32_t COLOR_BG_DARK = 0xFF3A3A3A;    // dark gray (top)
static const uint32_t COLOR_BG_ORANGE = 0xFFE8A838;  // orange (bottom)
static const uint32_t COLOR_BTN_BG = 0xFFF5E6D3;     // beige button
static const uint32_t COLOR_BTN_TEXT = 0xFF5C4033;   // dark brown text
static const uint32_t COLOR_EQUAL = 0xFFFF6B9D;      // pink equals sign
static const uint32_t COLOR_DISPLAY_BG = 0xFFF5E6D3; // display background

// Custom display component
class DisplayWidget : public Widget {
private:
    char displayText[32];
    
protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        // draw the dark gray background (rounded)
        graph_fill_round(g, r.x, r.y, r.w, r.h, X::hidpiSize(20), COLOR_BG_DARK);
        
        // draw the display (beige rounded rect with padding)
        int padding = X::hidpiSize(15);
        int displayX = r.x + padding;
        int displayY = r.y + padding;
        int displayW = r.w - padding * 2;
        int displayH = r.h - padding * 2;
        
        // display background
        graph_fill_round(g, displayX, displayY, displayW, displayH, X::hidpiSize(15), COLOR_DISPLAY_BG);
        
        // draw text (right-aligned)
        uint32_t tw, th;
        font_text_size(displayText, theme->getFont(), theme->basic.fontSize + X::hidpiSize(8), &tw, &th);
        int textX = displayX + displayW - (int32_t)tw - X::hidpiSize(15);
        int textY = displayY + (displayH - (int32_t)th) / 2;
        graph_draw_text_font(g, textX, textY, displayText, theme->getFont(), 
                            theme->basic.fontSize + X::hidpiSize(8), 0xFF333333);
    }
    
public:
    DisplayWidget() : Widget() {
        strcpy(displayText, "0");
    }
    
    void setText(const char* text) {
        strncpy(displayText, text, sizeof(displayText) - 1);
        displayText[sizeof(displayText) - 1] = '\0';
        update();
    }
};

// Custom 3D rounded oval button
class CalcButton : public LabelButton {
private:
    uint32_t bgColor;
    uint32_t textColor;
    bool pressed;

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        uint32_t color = bgColor;
        
        // compute the corner radius (oval)
        int radius = r.h / 2;
        int roundWidth = X::hidpiSize(2); // 3D effect width
        
        if(pressed) {
            // pressed state: use the inverted 3D effect (depressed)
            graph_fill_round_3d(g, r.x, r.y, r.w, r.h, radius, roundWidth, color, true);
        } else {
            // normal state: use the raised 3D effect
            graph_fill_round_3d(g, r.x, r.y, r.w, r.h, radius, roundWidth, color, false);
        }
        
        // draw the button text
        const string& text = getLabel();
        uint32_t tw, th;
        font_text_size(text.c_str(), theme->getFont(), theme->basic.fontSize + X::hidpiSize(6), &tw, &th);
        int textX = r.x + (r.w - (int32_t)tw) / 2;
        int textY = r.y + (r.h - (int32_t)th) / 2;
        graph_draw_text_font(g, textX, textY, text.c_str(), theme->getFont(), 
                            theme->basic.fontSize + X::hidpiSize(6), textColor);
    }

    bool onMouse(xevent_t* ev) {
        if(ev->state == MOUSE_STATE_DOWN) {
            pressed = true;
            update();
        }
        else if(ev->state == MOUSE_STATE_UP) {
            pressed = false;
            update();
        }
        return LabelButton::onMouse(ev);
    }

public:
    CalcButton(const char* label, uint32_t bg = 0xFFF5E6D3, uint32_t text = 0xFF5C4033) 
        : LabelButton(label), bgColor(bg), textColor(text), pressed(false) {
        setMarginH(X::hidpiSize(8));
        setMarginV(X::hidpiSize(8));
    }
};

class Calculator : public WidgetWin {
private:
    DisplayWidget* display;
    char currentInput[32];
    char previousInput[32];
    char currentOperator;
    bool shouldResetDisplay;
    bool hasDecimal;

    void updateDisplay() {
        if(currentInput[0] == '\0') {
            display->setText("0");
        } else {
            display->setText(currentInput);
        }
    }

    void appendNumber(const char* num) {
        if(shouldResetDisplay) {
            currentInput[0] = '\0';
            shouldResetDisplay = false;
            hasDecimal = false;
        }
        if(strlen(currentInput) < 12) {
            strcat(currentInput, num);
            updateDisplay();
        }
    }

    void appendDecimal() {
        if(shouldResetDisplay) {
            strcpy(currentInput, "0");
            shouldResetDisplay = false;
        }
        if(!hasDecimal && strlen(currentInput) < 11) {
            if(currentInput[0] == '\0') {
                strcpy(currentInput, "0");
            }
            strcat(currentInput, ".");
            hasDecimal = true;
            updateDisplay();
        }
    }

    void setOperator(char op) {
        if(currentInput[0] != '\0') {
            if(previousInput[0] != '\0' && !shouldResetDisplay) {
                calculate();
            }
            strcpy(previousInput, currentInput);
            currentOperator = op;
            shouldResetDisplay = true;
        }
    }

    void calculate() {
        if(previousInput[0] == '\0' || currentInput[0] == '\0') return;
        
        double prev = atof(previousInput);
        double curr = atof(currentInput);
        double result = 0;
        
        switch(currentOperator) {
            case '+': result = prev + curr; break;
            case '-': result = prev - curr; break;
            case '*': result = prev * curr; break;
            case '/': 
                if(curr != 0) result = prev / curr;
                else {
                    strcpy(currentInput, "Error");
                    updateDisplay();
                    shouldResetDisplay = true;
                    return;
                }
                break;
        }
        
        if(result == (int)result) {
            snprintf(currentInput, sizeof(currentInput), "%d", (int)result);
        } else {
            snprintf(currentInput, sizeof(currentInput), "%.6g", result);
        }
        
        previousInput[0] = '\0';
        currentOperator = 0;
        shouldResetDisplay = true;
        hasDecimal = (strchr(currentInput, '.') != NULL);
        updateDisplay();
    }

    void clear() {
        currentInput[0] = '\0';
        previousInput[0] = '\0';
        currentOperator = 0;
        shouldResetDisplay = false;
        hasDecimal = false;
        updateDisplay();
    }

    void clearEntry() {
        currentInput[0] = '\0';
        hasDecimal = false;
        updateDisplay();
    }

    void backspace() {
        size_t len = strlen(currentInput);
        if(len > 0 && !shouldResetDisplay) {
            if(currentInput[len-1] == '.') {
                hasDecimal = false;
            }
            currentInput[len-1] = '\0';
            updateDisplay();
        }
    }

public:
    Calculator() : WidgetWin() {
        currentInput[0] = '\0';
        previousInput[0] = '\0';
        currentOperator = 0;
        shouldResetDisplay = false;
        hasDecimal = false;

        RootWidget* root = new RootWidget();
        setRoot(root);
        root->setType(Container::VERTICAL);

        // create the display area (dark gray rounded background)
        display = new DisplayWidget();
        display->fix(0, X::hidpiSize(80));
        root->add(display);

        // create the button area (orange background)
        Container* buttonArea = new Container();
        buttonArea->setType(Container::VERTICAL);
        root->add(buttonArea);

        // button layout: 4 rows x 4 columns
        const char* buttons[4][4] = {
            {"C", "CE", "<-", "/"},
            {"7", "8", "9", "*"},
            {"4", "5", "6", "-"},
            {"1", "2", "3", "+"}
        };

        for(int row = 0; row < 4; row++) {
            Container* rowContainer = new Container();
            rowContainer->setType(Container::HORIZONTAL);
            buttonArea->add(rowContainer);

            for(int col = 0; col < 4; col++) {
                const char* label = buttons[row][col];
                CalcButton* btn = NULL;
                
                if(strcmp(label, "C") == 0 || strcmp(label, "CE") == 0 || strcmp(label, "<-") == 0) {
                    btn = new CalcButton(label, 0xFFD4A574, COLOR_BTN_TEXT);
                } else if(strcmp(label, "/") == 0 || strcmp(label, "*") == 0 || 
                          strcmp(label, "-") == 0 || strcmp(label, "+") == 0) {
                    btn = new CalcButton(label, 0xFFD4A574, COLOR_BTN_TEXT);
                } else {
                    btn = new CalcButton(label, COLOR_BTN_BG, COLOR_BTN_TEXT);
                }
                
                btn->setEventFunc(onButtonClick, this);
                rowContainer->add(btn);
            }
        }

        // last row: 0, ., =
        Container* lastRow = new Container();
        lastRow->setType(Container::HORIZONTAL);
        buttonArea->add(lastRow);

        CalcButton* btn0 = new CalcButton("0", COLOR_BTN_BG, COLOR_BTN_TEXT);
        btn0->setEventFunc(onButtonClick, this);
        lastRow->add(btn0);

        CalcButton* btnDot = new CalcButton(".", COLOR_BTN_BG, COLOR_BTN_TEXT);
        btnDot->setEventFunc(onButtonClick, this);
        lastRow->add(btnDot);

        CalcButton* btnEqual = new CalcButton("=", COLOR_EQUAL, 0xFFFFFFFF);
        btnEqual->setEventFunc(onButtonClick, this);
        lastRow->add(btnEqual);
    }

    static void onButtonClick(Widget* wd, xevent_t* evt, void* data) {
        if(evt->state != MOUSE_STATE_UP) return;
        
        Calculator* calc = (Calculator*)data;
        CalcButton* btn = (CalcButton*)wd;
        const char* label = btn->getLabel().c_str();

        if(strcmp(label, "C") == 0) {
            calc->clear();
        } else if(strcmp(label, "CE") == 0) {
            calc->clearEntry();
        } else if(strcmp(label, "<-") == 0) {
            calc->backspace();
        } else if(strcmp(label, "/") == 0) {
            calc->setOperator('/');
        } else if(strcmp(label, "*") == 0) {
            calc->setOperator('*');
        } else if(strcmp(label, "-") == 0) {
            calc->setOperator('-');
        } else if(strcmp(label, "+") == 0) {
            calc->setOperator('+');
        } else if(strcmp(label, "=") == 0) {
            calc->calculate();
        } else if(strcmp(label, ".") == 0) {
            calc->appendDecimal();
        } else {
            calc->appendNumber(label);
        }
    }
};

int main(int argc, char** argv) {
    X x;
    Calculator win;

    grect_t wr;
    wr.x = 200;
    wr.y = 100;
    wr.w = X::hidpiSize(260);
    wr.h = X::hidpiSize(320);

    win.open(&x, -1, wr, "Calculator", XWIN_STYLE_NO_RESIZE);
    widgetXRun(&x, &win);
    return 0;
}
