#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/keydef.h>
#include <ewoksys/kernel_tic.h>
#include <font/font.h>
#include <x++/X.h>

using namespace Ewok;

/* iOS-style virtual keyboard for EwokOS
 *
 * Design principles:
 *  - Exact iOS keyboard layout (QWERTY / Numbers / Symbols)
 *  - Rounded white keys on light gray background
 *  - Shift: single tap = next char upper, double tap = caps lock
 *  - Responsive: adapts to phone (narrow) and pad (wide) screens
 *  - Key press visual feedback (darken + popup bubble)
 */

/* --- Color scheme (iOS light mode) --- */
#define IOS_BG_COLOR        0xFFD1D4DB
#define IOS_KEY_COLOR       0xFFFFFFFF
#define IOS_KEY_FUNC_COLOR  0xFFABAFB6
#define IOS_KEY_PRESSED     0xFF83878E
#define IOS_KEY_TEXT        0xFF000000
#define IOS_KEY_FUNC_TEXT   0xFF000000
#define IOS_SHADOW_COLOR    0x40000000
#define IOS_POPUP_BG        0xFFFFFFFF
#define IOS_POPUP_BORDER    0xFFCCCCCC
#define IOS_SPACE_TEXT      0xFF999999
#define IOS_KEY_BORDER      0xFFBFC2C7

/* --- Key types --- */
enum KeyType {
    KT_NORMAL = 0,   // white letter/number key
    KT_FUNC,         // gray function key (shift, backspace, 123, etc.)
    KT_SPACE,        // space bar
    KT_RETURN,       // return key
    KT_NONE          // empty slot
};

/* --- Keyboard layouts --- */
enum KBLayout {
    KBL_QWERTY = 0,
    KBL_NUMBERS,     // 123
    KBL_SYMBOLS,     // #+=
    KBL_COUNT
};

/* --- Key definition --- */
struct IOSKey {
    const char* label;      // display label
    const char* labelCaps;  // display label when shifted (NULL = same)
    char ch;                // character to emit (0 = special)
    char chShift;           // character to emit when shifted
    KeyType type;
    float widthRatio;       // relative width (1.0 = standard key)
    int action;             // special action ID
};

/* --- Special action IDs --- */
enum KeyAction {
    KA_NONE = 0,
    KA_SHIFT,
    KA_BACKSPACE,
    KA_TO_NUMBERS,
    KA_TO_SYMBOLS,
    KA_TO_ALPHA,
    KA_SPACE,
    KA_RETURN,
    KA_GLOBE,
    KA_DISMISS
};

#define MAX_KEYS_PER_ROW 12
#define MAX_ROWS 4
#define MAX_KEYS (MAX_ROWS * MAX_KEYS_PER_ROW)

/* --- Layout definitions --- */

// QWERTY layout (iOS style)
static IOSKey layout_qwerty[MAX_ROWS][MAX_KEYS_PER_ROW] = {
    // Row 0: q w e r t y u i o p
    {
        {"q","Q",'q','Q',KT_NORMAL,1.0f,KA_NONE},
        {"w","W",'w','W',KT_NORMAL,1.0f,KA_NONE},
        {"e","E",'e','E',KT_NORMAL,1.0f,KA_NONE},
        {"r","R",'r','R',KT_NORMAL,1.0f,KA_NONE},
        {"t","T",'t','T',KT_NORMAL,1.0f,KA_NONE},
        {"y","Y",'y','Y',KT_NORMAL,1.0f,KA_NONE},
        {"u","U",'u','U',KT_NORMAL,1.0f,KA_NONE},
        {"i","I",'i','I',KT_NORMAL,1.0f,KA_NONE},
        {"o","O",'o','O',KT_NORMAL,1.0f,KA_NONE},
        {"p","P",'p','P',KT_NORMAL,1.0f,KA_NONE},
    },
    // Row 1: a s d f g h j k l
    {
        {"a","A",'a','A',KT_NORMAL,1.0f,KA_NONE},
        {"s","S",'s','S',KT_NORMAL,1.0f,KA_NONE},
        {"d","D",'d','D',KT_NORMAL,1.0f,KA_NONE},
        {"f","F",'f','F',KT_NORMAL,1.0f,KA_NONE},
        {"g","G",'g','G',KT_NORMAL,1.0f,KA_NONE},
        {"h","H",'h','H',KT_NORMAL,1.0f,KA_NONE},
        {"j","J",'j','J',KT_NORMAL,1.0f,KA_NONE},
        {"k","K",'k','K',KT_NORMAL,1.0f,KA_NONE},
        {"l","L",'l','L',KT_NORMAL,1.0f,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
    // Row 2: shift z x c v b n m backspace
    {
        {"","",0,0,KT_FUNC,-1.0f,KA_SHIFT},
        {"z","Z",'z','Z',KT_NORMAL,1.0f,KA_NONE},
        {"x","X",'x','X',KT_NORMAL,1.0f,KA_NONE},
        {"c","C",'c','C',KT_NORMAL,1.0f,KA_NONE},
        {"v","V",'v','V',KT_NORMAL,1.0f,KA_NONE},
        {"b","B",'b','B',KT_NORMAL,1.0f,KA_NONE},
        {"n","N",'n','N',KT_NORMAL,1.0f,KA_NONE},
        {"m","M",'m','M',KT_NORMAL,1.0f,KA_NONE},
        {"","",0,0,KT_FUNC,-1.0f,KA_BACKSPACE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
    // Row 3: 123 | globe | space | return
    {
        {"123","",0,0,KT_FUNC,1.4f,KA_TO_NUMBERS},
        {"","",0,0,KT_FUNC,1.4f,KA_GLOBE},
        {"space","",' ',0,KT_SPACE,-1.0f,KA_SPACE},
        {"return","",'\r',0,KT_RETURN,2.0f,KA_RETURN},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
};


// Numbers layout (123)
static IOSKey layout_numbers[MAX_ROWS][MAX_KEYS_PER_ROW] = {
    // Row 0: 1 2 3 4 5 6 7 8 9 0
    {
        {"1","",'1',0,KT_NORMAL,1.0f,KA_NONE},
        {"2","",'2',0,KT_NORMAL,1.0f,KA_NONE},
        {"3","",'3',0,KT_NORMAL,1.0f,KA_NONE},
        {"4","",'4',0,KT_NORMAL,1.0f,KA_NONE},
        {"5","",'5',0,KT_NORMAL,1.0f,KA_NONE},
        {"6","",'6',0,KT_NORMAL,1.0f,KA_NONE},
        {"7","",'7',0,KT_NORMAL,1.0f,KA_NONE},
        {"8","",'8',0,KT_NORMAL,1.0f,KA_NONE},
        {"9","",'9',0,KT_NORMAL,1.0f,KA_NONE},
        {"0","",'0',0,KT_NORMAL,1.0f,KA_NONE},
    },
    // Row 1: - / : ; ( ) $ & @ "
    {
        {"-","",'-',0,KT_NORMAL,1.0f,KA_NONE},
        {"/","",  '/',0,KT_NORMAL,1.0f,KA_NONE},
        {":","",  ':',0,KT_NORMAL,1.0f,KA_NONE},
        {";","",  ';',0,KT_NORMAL,1.0f,KA_NONE},
        {"(","",  '(',0,KT_NORMAL,1.0f,KA_NONE},
        {")","",  ')',0,KT_NORMAL,1.0f,KA_NONE},
        {"$","",  '$',0,KT_NORMAL,1.0f,KA_NONE},
        {"&","",  '&',0,KT_NORMAL,1.0f,KA_NONE},
        {"@","",  '@',0,KT_NORMAL,1.0f,KA_NONE},
        {"\"","", '"',0,KT_NORMAL,1.0f,KA_NONE},
    },
    // Row 2: #+= . , ? ! ' backspace
    {
        {"#+=","",0,0,KT_FUNC,-1.0f,KA_TO_SYMBOLS},
        {".","", '.',0,KT_NORMAL,1.0f,KA_NONE},
        {",","", ',',0,KT_NORMAL,1.0f,KA_NONE},
        {"?","", '?',0,KT_NORMAL,1.0f,KA_NONE},
        {"!","", '!',0,KT_NORMAL,1.0f,KA_NONE},
        {"'","", '\'',0,KT_NORMAL,1.0f,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_FUNC,-1.0f,KA_BACKSPACE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
    // Row 3: ABC | globe | space | return
    {
        {"ABC","",0,0,KT_FUNC,1.4f,KA_TO_ALPHA},
        {"","",0,0,KT_FUNC,1.4f,KA_GLOBE},
        {"space","",' ',0,KT_SPACE,-1.0f,KA_SPACE},
        {"return","",'\r',0,KT_RETURN,2.0f,KA_RETURN},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
};


// Symbols layout (#+=)
static IOSKey layout_symbols[MAX_ROWS][MAX_KEYS_PER_ROW] = {
    // Row 0: [ ] { } # % ^ * + =
    {
        {"[","", '[',0,KT_NORMAL,1.0f,KA_NONE},
        {"]","", ']',0,KT_NORMAL,1.0f,KA_NONE},
        {"{","", '{',0,KT_NORMAL,1.0f,KA_NONE},
        {"}","", '}',0,KT_NORMAL,1.0f,KA_NONE},
        {"#","", '#',0,KT_NORMAL,1.0f,KA_NONE},
        {"%","", '%',0,KT_NORMAL,1.0f,KA_NONE},
        {"^","", '^',0,KT_NORMAL,1.0f,KA_NONE},
        {"*","", '*',0,KT_NORMAL,1.0f,KA_NONE},
        {"+","", '+',0,KT_NORMAL,1.0f,KA_NONE},
        {"=","", '=',0,KT_NORMAL,1.0f,KA_NONE},
    },
    // Row 1: _ \ | ~ < > EUR GBP YEN •
    {
        {"_","", '_',0,KT_NORMAL,1.0f,KA_NONE},
        {"\\","",'\\',0,KT_NORMAL,1.0f,KA_NONE},
        {"|","", '|',0,KT_NORMAL,1.0f,KA_NONE},
        {"~","", '~',0,KT_NORMAL,1.0f,KA_NONE},
        {"<","", '<',0,KT_NORMAL,1.0f,KA_NONE},
        {">","", '>',0,KT_NORMAL,1.0f,KA_NONE},
        {"$","", '$',0,KT_NORMAL,1.0f,KA_NONE},
        {"\xc2\xa3","", (char)0xA3,0,KT_NORMAL,1.0f,KA_NONE},
        {"\xc2\xa5","", (char)0xA5,0,KT_NORMAL,1.0f,KA_NONE},
        {"\xe2\x80\xa2","", (char)0x95,0,KT_NORMAL,1.0f,KA_NONE},
    },
    // Row 2: 123 . , ? ! ' backspace
    {
        {"123","",0,0,KT_FUNC,-1.0f,KA_TO_NUMBERS},
        {".","", '.',0,KT_NORMAL,1.0f,KA_NONE},
        {",","", ',',0,KT_NORMAL,1.0f,KA_NONE},
        {"?","", '?',0,KT_NORMAL,1.0f,KA_NONE},
        {"!","", '!',0,KT_NORMAL,1.0f,KA_NONE},
        {"'","", '\'',0,KT_NORMAL,1.0f,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_FUNC,-1.0f,KA_BACKSPACE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
    // Row 3: ABC | globe | space | return
    {
        {"ABC","",0,0,KT_FUNC,1.4f,KA_TO_ALPHA},
        {"","",0,0,KT_FUNC,1.4f,KA_GLOBE},
        {"space","",' ',0,KT_SPACE,-1.0f,KA_SPACE},
        {"return","",'\r',0,KT_RETURN,2.0f,KA_RETURN},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
        {"","",0,0,KT_NONE,0,KA_NONE},
    },
};


/* --- Computed key rect for hit-testing --- */
struct KeyRect {
    int x, y, w, h;
    int row, col;
    int action;
    char ch;
    char chShift;
    KeyType type;
    const char* label;
    const char* labelCaps;
};

class XIMIOS : public XWin {
    int xPid;
    font_t* font;

    gsize_t scrSize;
    gsize_t panelSize;

    /* Layout state */
    KBLayout currentLayout;
    bool shiftActive;      // shift pressed once (next char upper)
    bool capsLock;         // caps lock engaged
    int  pressedKey;       // index of currently pressed key (-1 = none)
    int  popupKey;         // key index showing popup bubble

    /* Responsive parameters */
    int fontSize;
    int keyGap;            // gap between keys in pixels
    int rowGap;            // gap between rows
    int sideMargin;        // side margin
    int bottomMargin;
    int topMargin;
    int keyRadius;         // rounded corner radius
    int keyHeight;         // computed key height
    int popupH;            // popup bubble height

    /* Computed key positions */
    KeyRect keyRects[MAX_KEYS];
    int keyCount;

    /* Hide mode */
    bool hideMode;

    /* Input buffer for display */
    static const int INPUT_MAX = 128;
    char inputS[INPUT_MAX];

    /* Double-tap shift detection */
    uint32_t lastShiftTap;

protected:
    /* --- Layout computation --- */
    void computeResponsiveParams(int w, int h) {
        fontSize = X::hidpiSize(14);
        keyGap = 4;
        rowGap = 6;
        sideMargin = 6;
        bottomMargin = 4;
        topMargin = 4;
        keyRadius = 5;
        popupH = 48;
        int fullW = w - sideMargin * 2;
        int unit = (fullW - keyGap * 9) / 10;
        float aspect = 0.85f;
        keyHeight = (int)(unit * aspect);

        /* Never grow beyond the panel we were given. */
        int availH = h - topMargin - bottomMargin - rowGap * (MAX_ROWS - 1);
        if(keyHeight * MAX_ROWS > availH)
            keyHeight = availH / MAX_ROWS;
        if(keyHeight < 20) keyHeight = 20;
    }

    /* Panel height that exactly fits four iOS-proportioned rows. */
    int idealPanelHeight(int w) {
        computeResponsiveParams(w, 0x7fffffff);
        return topMargin + bottomMargin +
            rowGap * (MAX_ROWS - 1) + keyHeight * MAX_ROWS;
    }

    IOSKey* getLayout(int row) {
        switch(currentLayout) {
        case KBL_QWERTY:  return layout_qwerty[row];
        case KBL_NUMBERS: return layout_numbers[row];
        case KBL_SYMBOLS: return layout_symbols[row];
        default:          return layout_qwerty[row];
        }
    }

    void computeKeyPositions(int totalW, int totalH) {
        computeResponsiveParams(totalW, totalH);
        keyCount = 0;

        int fullW = totalW - sideMargin * 2;

        /* iOS rule: all letter keys share one size on every row, so the
           unit width is derived once from the 10-column top row. */
        float unitW = 0;
        {
            IOSKey* keys0 = getLayout(0);
            int valid0 = 0;
            float ratio0 = 0;
            for(int i = 0; i < MAX_KEYS_PER_ROW; i++) {
                if(keys0[i].type == KT_NONE || keys0[i].label == NULL) continue;
                valid0++;
                ratio0 += (keys0[i].widthRatio > 0) ? keys0[i].widthRatio : 1.0f;
            }
            if(valid0 > 0)
                unitW = (float)(fullW - keyGap * (valid0 - 1)) / ratio0;
        }
        if(unitW <= 0)
            return;

        for(int row = 0; row < MAX_ROWS; row++) {
            IOSKey* keys = getLayout(row);

            /* Pass 1: measure. widthRatio < 0 marks a stretch key that
               absorbs leftover width (shift / backspace / space). */
            int validCount = 0, stretchCount = 0;
            float fixedW = 0;
            for(int i = 0; i < MAX_KEYS_PER_ROW; i++) {
                if(keys[i].type == KT_NONE || keys[i].label == NULL) continue;
                validCount++;
                if(keys[i].widthRatio < 0)
                    stretchCount++;
                else
                    fixedW += keys[i].widthRatio * unitW;
            }
            if(validCount == 0) continue;

            int gaps = keyGap * (validCount - 1);
            float stretchW = 0;
            if(stretchCount > 0)
                stretchW = (float)(fullW - (int)fixedW - gaps) / stretchCount;

            int y = topMargin + row * (keyHeight + rowGap);
            int x = sideMargin;

            /* A row narrower than the board (the 9-key asdf row) is
               centered, giving the iOS half-key stagger. */
            if(stretchCount == 0) {
                int rowW = (int)fixedW + gaps;
                if(rowW < fullW)
                    x += (fullW - rowW) / 2;
            }

            /* Pass 2: place */
            for(int i = 0; i < MAX_KEYS_PER_ROW; i++) {
                if(keys[i].type == KT_NONE || keys[i].label == NULL) continue;

                int kw = (keys[i].widthRatio < 0)
                    ? (int)stretchW
                    : (int)(keys[i].widthRatio * unitW);

                KeyRect* kr = &keyRects[keyCount];
                kr->x = x;
                kr->y = y;
                kr->w = kw;
                kr->h = keyHeight;
                kr->row = row;
                kr->col = i;
                kr->action = keys[i].action;
                kr->ch = keys[i].ch;
                kr->chShift = keys[i].chShift;
                kr->type = keys[i].type;
                kr->label = keys[i].label;
                kr->labelCaps = keys[i].labelCaps;
                keyCount++;

                x += kw + keyGap;
            }
        }
    }

    int hitTest(int px, int py) {
        for(int i = 0; i < keyCount; i++) {
            KeyRect* kr = &keyRects[i];
            if(px >= kr->x && px < kr->x + kr->w &&
               py >= kr->y && py < kr->y + kr->h) {
                return i;
            }
        }
        return -1;
    }

    bool isShifted() {
        return shiftActive || capsLock;
    }

    /* --- Drawing --- */
    void drawKeyShadow(graph_t* g, KeyRect* kr) {
        // Subtle shadow below key
        int sh = 1;
        graph_fill_round(g, kr->x, kr->y + sh, kr->w, kr->h, keyRadius, IOS_SHADOW_COLOR);
    }

    void drawKey(graph_t* g, int idx, bool pressed) {
        KeyRect* kr = &keyRects[idx];
        if(kr->type == KT_NONE) return;

        /* Draw shadow first */
        if(!pressed)
            drawKeyShadow(g, kr);

        /* Key background */
        uint32_t bgColor;
        if(pressed) {
            bgColor = IOS_KEY_PRESSED;
        } else {
            switch(kr->type) {
            case KT_NORMAL: bgColor = IOS_KEY_COLOR; break;
            case KT_FUNC:   bgColor = IOS_KEY_FUNC_COLOR; break;
            case KT_SPACE:  bgColor = IOS_KEY_COLOR; break;
            case KT_RETURN: bgColor = IOS_KEY_FUNC_COLOR; break;
            default:        bgColor = IOS_KEY_COLOR; break;
            }
        }
        graph_fill_round(g, kr->x, kr->y, kr->w, kr->h, keyRadius, bgColor);

        /* Key label */
        const char* label = kr->label;
        uint32_t textColor = (pressed) ? 0xFFFFFFFF : IOS_KEY_TEXT;

        /* Handle shift display for letter keys */
        if(isShifted() && kr->labelCaps != NULL && kr->labelCaps[0] != 0) {
            label = kr->labelCaps;
        }
        /* Shift icon: draw triangle arrow */
        if(kr->action == KA_SHIFT) {
            drawShiftIcon(g, kr, pressed);
            return;
        }
        /* Backspace icon */
        if(kr->action == KA_BACKSPACE) {
            drawBackspaceIcon(g, kr, pressed);
            return;
        }
        /* Globe icon */
        if(kr->action == KA_GLOBE) {
            drawGlobeIcon(g, kr, pressed);
            return;
        }
        /* Caps lock indicator */
        if(kr->action == KA_SHIFT && capsLock) {
            // handled in drawShiftIcon
        }

        if(label == NULL || label[0] == 0) return;

        /* Space label style */
        if(kr->type == KT_SPACE) {
            textColor = pressed ? 0xFFFFFFFF : IOS_SPACE_TEXT;
        }

        uint32_t tw, th;
        font_text_size(label, font, fontSize, &tw, &th);
        int tx = kr->x + (kr->w - tw) / 2;
        int ty = kr->y + (kr->h - fontSize) / 2;
        graph_draw_text_font(g, tx, ty, label, font, fontSize, textColor);
    }

    void drawShiftIcon(graph_t* g, KeyRect* kr, bool pressed) {
        int cx = kr->x + kr->w / 2;
        int cy = kr->y + kr->h / 2;
        int sz = X::hidpiSize(10);
        uint32_t color = pressed ? 0xFFFFFFFF : 0xFF000000;

        /* Draw up arrow triangle */
        graph_fill_triangle(g,
            cx, cy - sz,
            cx - sz, cy + sz/2,
            cx + sz, cy + sz/2,
            color);
        /* Arrow stem */
        graph_fill_rect(g, cx - sz/3, cy + sz/2, sz*2/3, sz/2, color);

        /* Caps lock: draw filled underline */
        if(capsLock) {
            graph_fill_rect(g, cx - sz, cy + sz + 2, sz*2, 2, color);
        }
        /* Shift active: fill the icon area with highlight */
        else if(shiftActive && !pressed) {
            graph_fill_round(g, kr->x+1, kr->y+1, kr->w-2, kr->h-2, keyRadius-1, 0xFFFFFFFF);
            /* Redraw arrow in black */
            graph_fill_triangle(g,
                cx, cy - sz,
                cx - sz, cy + sz/2,
                cx + sz, cy + sz/2,
                0xFF000000);
            graph_fill_rect(g, cx - sz/3, cy + sz/2, sz*2/3, sz/2, 0xFF000000);
        }
    }

    void drawBackspaceIcon(graph_t* g, KeyRect* kr, bool pressed) {
        int cx = kr->x + kr->w / 2;
        int cy = kr->y + kr->h / 2;
        int sz = X::hidpiSize(9);
        uint32_t color = pressed ? 0xFFFFFFFF : 0xFF000000;

        /* Draw a left-pointing arrow with x */
        // Arrow body (pentagon shape simplified as rect + triangle)
        graph_fill_rect(g, cx - sz/2, cy - sz/2, sz + sz/2, sz, color);
        graph_fill_triangle(g,
            cx - sz, cy,
            cx - sz/2, cy - sz/2,
            cx - sz/2, cy + sz/2,
            color);
        /* X mark inside */
        uint32_t xColor = pressed ? IOS_KEY_PRESSED : IOS_KEY_FUNC_COLOR;
        int xs = sz / 3;
        graph_wline(g, cx + 1, cy - xs, cx + 1 + xs*2, cy + xs, 1, xColor);
        graph_wline(g, cx + 1, cy + xs, cx + 1 + xs*2, cy - xs, 1, xColor);
    }

    void drawGlobeIcon(graph_t* g, KeyRect* kr, bool pressed) {
        int cx = kr->x + kr->w / 2;
        int cy = kr->y + kr->h / 2;
        int r = X::hidpiSize(9);
        uint32_t color = pressed ? 0xFFFFFFFF : 0xFF000000;

        /* Draw a simple globe: circle + horizontal/vertical lines */
        graph_circle(g, cx, cy, r, 1, color);
        graph_line(g, cx - r, cy, cx + r, cy, color);
        graph_line(g, cx, cy - r, cx, cy + r, color);
        /* Curved lines (simplified as shorter horizontal lines) */
        graph_line(g, cx - r/2, cy - r/2, cx + r/2, cy - r/2, color);
        graph_line(g, cx - r/2, cy + r/2, cx + r/2, cy + r/2, color);
    }

    void drawPopup(graph_t* g, int idx) {
        if(idx < 0 || idx >= keyCount) return;
        KeyRect* kr = &keyRects[idx];

        /* Only show popup for character keys in top rows */
        if(kr->type != KT_NORMAL && kr->type != KT_SPACE) return;
        if(kr->row >= 3) return;

        const char* label = kr->label;
        if(isShifted() && kr->labelCaps != NULL && kr->labelCaps[0] != 0)
            label = kr->labelCaps;

        int pw = kr->w + 8;
        int ph = popupH;
        int px = kr->x + (kr->w - pw) / 2;
        int py = kr->y - ph + 8;

        /* Clamp to window */
        if(px < 2) px = 2;
        if(px + pw > g->w - 2) px = g->w - pw - 2;
        if(py < 2) py = 2;

        /* Draw popup bubble */
        int pr = 10;
        graph_fill_round(g, px, py, pw, ph, pr, IOS_POPUP_BG);
        graph_round(g, px, py, pw, ph, pr, 1, IOS_POPUP_BORDER);

        /* Draw enlarged character */
        int bigFs = fontSize + 10;
        uint32_t tw, th;
        font_text_size(label, font, bigFs, &tw, &th);
        int tx = px + (pw - tw) / 2;
        int ty = py + (ph - bigFs) / 2;
        graph_draw_text_font(g, tx, ty, label, font, bigFs, 0xFF000000);
    }

    void onRepaint(graph_t* g) {
        /* Background */
        graph_fill_rect(g, 0, 0, g->w, g->h, IOS_BG_COLOR);

        if(hideMode) {
            /* Draw a small restore button */
            int bw = g->w;
            int bh = g->h;
            graph_fill_round(g, 2, 2, bw-4, bh-4, 4, IOS_KEY_FUNC_COLOR);
            const char* t = "...";
            uint32_t tw;
            font_text_size(t, font, fontSize, &tw, NULL);
            graph_draw_text_font(g, (bw-tw)/2, (bh-fontSize)/2, t, font, fontSize, 0xFF000000);
            return;
        }

        /* Recompute layout if needed */
        computeKeyPositions(g->w, g->h);

        /* Draw all keys */
        for(int i = 0; i < keyCount; i++) {
            bool pressed = (i == pressedKey);
            drawKey(g, i, pressed);
        }

        /* Draw popup bubble above pressed key */
        if(popupKey >= 0) {
            drawPopup(g, popupKey);
        }
    }

    /* --- Input handling --- */
    void inputX(char c) {
        xevent_t ev;
        ev.type = XEVT_IM_VKEYB;
        ev.value.im.value = c;
        ev.value.im.key_code = c;
        ev.value.im.shift = 0;
        ev.value.im.ctrl = 0;
        ev.state = XIM_STATE_PRESS;

        proto_t in;
        PF->init(&in)->add(&in, &ev, sizeof(xevent_t));
        dev_cntl_by_pid(xPid, X_DCNTL_INPUT, &in, NULL);
        PF->clear(&in);
    }

    void input(char c) {
        int len = strlen(inputS);
        if(c == KEY_BACKSPACE) {
            if(len > 0)
                inputS[len-1] = 0;
        }
        else if(c == '\r') {
            inputS[0] = 0;
        }
        else if((unsigned char)c < 0xF0 && len < INPUT_MAX-1) {
            inputS[len] = c;
            inputS[len+1] = 0;
        }
        inputX(c);
    }

    void doKeyAction(int idx) {
        if(idx < 0 || idx >= keyCount) return;
        KeyRect* kr = &keyRects[idx];

        switch(kr->action) {
        case KA_SHIFT: {
            uint32_t now = (uint32_t)kernel_tic_ms(0);
            if(now - lastShiftTap < 400 && lastShiftTap != 0) {
                /* Double tap: toggle caps lock */
                capsLock = !capsLock;
                shiftActive = false;
            } else {
                if(capsLock) {
                    capsLock = false;
                    shiftActive = false;
                } else {
                    shiftActive = !shiftActive;
                }
            }
            lastShiftTap = now;
            repaint();
            break;
        }
        case KA_BACKSPACE:
            input(KEY_BACKSPACE);
            break;
        case KA_TO_NUMBERS:
            currentLayout = KBL_NUMBERS;
            shiftActive = false;
            repaint();
            break;
        case KA_TO_SYMBOLS:
            currentLayout = KBL_SYMBOLS;
            shiftActive = false;
            repaint();
            break;
        case KA_TO_ALPHA:
            currentLayout = KBL_QWERTY;
            shiftActive = false;
            repaint();
            break;
        case KA_SPACE:
            input(' ');
            shiftActive = false;
            break;
        case KA_RETURN:
            input('\r');
            shiftActive = false;
            break;
        case KA_GLOBE:
            /* Cycle layouts: QWERTY -> Numbers -> Symbols -> QWERTY */
            currentLayout = (KBLayout)((currentLayout + 1) % KBL_COUNT);
            shiftActive = false;
            repaint();
            break;
        case KA_DISMISS:
            changeMode(true);
            break;
        default: {
            /* Normal character key */
            char c = kr->ch;
            if(isShifted() && kr->chShift != 0) {
                c = kr->chShift;
            } else if(isShifted() && c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
            if(c != 0)
                input(c);
            /* Auto-release shift after one character (unless caps lock) */
            if(shiftActive && !capsLock) {
                shiftActive = false;
                repaint();
            }
            break;
        }
        }
    }

    /* --- Mouse/Touch events --- */
    void doMouseEvent(xevent_t* ev) {
        if(hideMode) {
            if(ev->state == MOUSE_STATE_CLICK || ev->state == MOUSE_STATE_UP) {
                changeMode(false);
            }
            return;
        }

        gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
        int x = pos.x;
        int y = pos.y;

        if(ev->state == MOUSE_STATE_DOWN) {
            int idx = hitTest(x, y);
            if(idx >= 0) {
                pressedKey = idx;
                popupKey = idx;
                repaint();
            }
        }
        else if(ev->state == MOUSE_STATE_DRAG) {
            int idx = hitTest(x, y);
            if(idx != pressedKey) {
                pressedKey = idx;
                popupKey = idx;
                repaint();
            }
        }
        else if(ev->state == MOUSE_STATE_UP || ev->state == MOUSE_STATE_CLICK) {
            int idx = hitTest(x, y);
            if(idx >= 0 && idx == pressedKey) {
                doKeyAction(idx);
            }
            pressedKey = -1;
            popupKey = -1;
            repaint();
        }
    }

    /* --- Physical keyboard / IM events --- */
    void doIMEvent(xevent_t* ev) {
        uint8_t c = ev->value.im.value;

        if(ev->state == XIM_STATE_PRESS) {
            if(hideMode)
                return;
            /* Pass through arrow keys for navigation when in hide mode */
            return;
        }

        /* RELEASE events */
        if(hideMode) {
            if(c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT ||
               c == KEY_RIGHT || c == KEY_ENTER) {
                inputX(c);
                return;
            }
            changeMode(false);
            return;
        }

        /* Handle joystick shortcuts like vkey does */
        if(c == JOYSTICK_L1) {
            changeMode(true);
            return;
        }
        if(c == JOYSTICK_START) {
            input('\r');
            return;
        }
        if(c == JOYSTICK_B) {
            input(' ');
            return;
        }
        if(c == JOYSTICK_Y) {
            input(KEY_BACKSPACE);
            return;
        }
    }

    void onEvent(xevent_t* ev) {
        if(ev->type == XEVT_MOUSE) {
            doMouseEvent(ev);
        }
        else if(ev->type == XEVT_IM) {
            doIMEvent(ev);
        }
    }

    /* --- Mode management --- */
    void changeMode(bool hide) {
        hideMode = hide;
        if(hide) {
            int kw = 32;
            resizeTo(kw, kw);
            moveTo(scrSize.w - kw, scrSize.h - kw);
            return;
        }

        int w = scrSize.w;
        if(panelSize.w > 0)
            w = panelSize.w;

        /* Height follows the key geometry (iOS proportion) instead of
           blindly taking half the screen. */
        int h = idealPanelHeight(w);
        if(panelSize.h > 0)
            h = panelSize.h;
        else if(h > scrSize.h / 2)
            h = scrSize.h / 2;

        resizeTo(w, h);
        moveTo(scrSize.w - w, scrSize.h - h);
    }

    void onShow(void) {
        changeMode(false);
    }

public:
    inline XIMIOS(int fw, int fh, int pw, int ph) {
        scrSize.w = fw;
        scrSize.h = fh;
        panelSize.w = pw;
        panelSize.h = ph;
        font = font_new(DEFAULT_SYSTEM_FONT, true);
        xPid = dev_get_pid("/dev/x");
        currentLayout = KBL_QWERTY;
        shiftActive = false;
        capsLock = false;
        pressedKey = -1;
        popupKey = -1;
        hideMode = false;
        inputS[0] = 0;
        lastShiftTap = 0;
        keyCount = 0;
        fontSize = X::hidpiSize(14);
        keyGap = 4;
        rowGap = 6;
        sideMargin = 3;
        bottomMargin = 4;
        topMargin = 4;
        keyRadius = 5;
        keyHeight = 36;
        popupH = 48;
    }

    inline ~XIMIOS() {
        if(font != NULL) font_free(font);
    }
};

/* --- Main entry --- */
static void waitX() {
    while(true) {
        int pid = dev_get_pid("/dev/x");
        if(pid > 0)
            break;
        usleep(300000);
    }
}

static int32_t _panelW = 0;
static int32_t _panelH = 0;

static int doargs(int argc, char* argv[]) {
    int c = 0;
    while(c != -1) {
        c = getopt(argc, argv, "w:h:");
        if(c == -1)
            break;
        switch(c) {
        case 'w':
            _panelW = X::hidpiSize(atoi(optarg));
            break;
        case 'h':
            _panelH = X::hidpiSize(atoi(optarg));
            break;
        default:
            c = -1;
            break;
        }
    }
    return optind;
}

int main(int argc, char* argv[]) {
    waitX();

    X x;
    xscreen_info_t scr;
    x.getScreenInfo(scr, 0);

    doargs(argc, argv);

    if(_panelW == 0)
        _panelW = scr.size.w;
    if(_panelW > X::hidpiSize(640))
        _panelW = X::hidpiSize(640);

    /* Height 0 means auto: onShow() sizes the panel from the key
       geometry; open() only needs a placeholder. */
    int openH = (_panelH > 0) ? _panelH : scr.size.h / 2;

    XIMIOS xwin(scr.size.w, scr.size.h, _panelW, _panelH);
    xwin.open(&x, -1, scr.size.w - _panelW, scr.size.h - openH, _panelW, openH, "xim_ios",
            XWIN_STYLE_NO_FOCUS | XWIN_STYLE_SYSTOP | XWIN_STYLE_XIM | XWIN_STYLE_NO_BG_EFFECT | XWIN_STYLE_NO_TITLE, false);
    x.run(NULL, &xwin);
    return 0;
}
