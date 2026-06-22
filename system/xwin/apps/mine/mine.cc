#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Container.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <ewoksys/kernel_tic.h>
#include <mouse/mouse.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace Ewok;

static const uint32_t COLOR_BG = 0xFFC0C0C0;
static const uint32_t COLOR_PANEL = 0xFFC0C0C0;
static const uint32_t COLOR_COUNTER_BG = 0xFF000000;
static const uint32_t COLOR_COUNTER_FG = 0xFFFF0000;
static const uint32_t COLOR_CELL_OPEN = 0xFFBDBDBD;
static const uint32_t COLOR_MINE = 0xFF111111;
static const uint32_t COLOR_FLAG = 0xFFCC0000;
static const uint32_t COLOR_EXPLODED = 0xFFFF4444;
static const uint32_t COLOR_FACE = 0xFFFFFF33;
static const uint32_t COLOR_TEXT = 0xFF000000;
static const uint32_t COLOR_HINT = 0xFF404040;

static const int TIMER_FPS = 10;
static const uint64_t DOUBLE_CLICK_MS = 300;
static const int MAX_ROWS = 20;
static const int MAX_COLS = 30;
static const int MAX_CELLS = MAX_ROWS * MAX_COLS;
static const int DEFAULT_WIN_W = 520;
static const int DEFAULT_WIN_H = 620;

enum DifficultyLevel {
    LEVEL_BEGINNER = 0,
    LEVEL_INTERMEDIATE = 1,
    LEVEL_EXPERT = 2,
    LEVEL_COUNT = 3
};

typedef struct {
    const char* label;
    int rows;
    int cols;
    int mines;
} DifficultyConfig;

static const DifficultyConfig DIFFICULTIES[LEVEL_COUNT] = {
    {"B", 12, 12, 18},
    {"I", 16, 20, 50},
    {"E", 20, 30, 120}
};

typedef struct {
    bool mine;
    bool open;
    bool flag;
    bool exploded;
    bool wrongFlag;
    uint8_t adjacent;
} Cell;

class MineWidget : public Widget {
private:
    struct LayoutMetrics {
        int outerPad;
        int sectionGap;
        int contentX;
        int contentW;
        int topPanelY;
        int topPanelH;
        int boardPanelY;
        int boardPanelH;
        int boardX;
        int boardY;
        int boardW;
        int boardH;
        int cell;
        int digitW;
        int digitH;
        int faceSize;
        int buttonW;
        int buttonH;
        int fontSize;
        int hintFontSize;
        grect_t minesRect;
        grect_t faceRect;
        grect_t timeRect;
        grect_t difficultyRect[LEVEL_COUNT];
    };

    Cell board[MAX_ROWS][MAX_COLS];
    int rows;
    int cols;
    int minesTotal;
    int flagsUsed;
    int openedCount;
    int elapsedSec;
    bool started;
    bool lost;
    bool won;
    bool facePressed;
    bool leftDown;
    int pressedRow;
    int pressedCol;
    int pressedDifficulty;
    bool pendingClick;
    int pendingClickRow;
    int pendingClickCol;
    uint64_t pendingClickMs;
    uint64_t startMs;
    DifficultyLevel difficulty;

    int minInt(int a, int b) const {
        return a < b ? a : b;
    }

    int maxInt(int a, int b) const {
        return a > b ? a : b;
    }

    int clampInt(int value, int minValue, int maxValue) const {
        if(value < minValue) {
            return minValue;
        }
        if(value > maxValue) {
            return maxValue;
        }
        return value;
    }

    bool inRect(int x, int y, const grect_t& r) const {
        return x >= r.x && y >= r.y && x < (r.x + r.w) && y < (r.y + r.h);
    }

    bool inBoard(int row, int col) const {
        return row >= 0 && row < rows && col >= 0 && col < cols;
    }

    grect_t makeRect(int x, int y, int w, int h) const {
        grect_t r;
        r.x = x;
        r.y = y;
        r.w = w;
        r.h = h;
        return r;
    }

    LayoutMetrics getLayout() const {
        LayoutMetrics layout;
        int widgetW = area.w > 0 ? area.w : DEFAULT_WIN_W;
        int widgetH = area.h > 0 ? area.h : DEFAULT_WIN_H;

        layout.outerPad = 12;
        layout.sectionGap = 10;
        layout.faceSize = 30;
        layout.digitW = 14;
        layout.digitH = 26;
        layout.buttonH = 24;
        layout.buttonW = 48;
        layout.fontSize = 12;
        layout.hintFontSize = 10;

        int buttonsRowW = LEVEL_COUNT * layout.buttonW + (LEVEL_COUNT - 1) * 8;
        int framePad = 6;
        int availableBoardH = widgetH - layout.outerPad * 3 - 70 - layout.buttonH - 24;
        int availableBoardW = widgetW - layout.outerPad * 2 - framePad * 2;
        if(availableBoardH < 80) {
            availableBoardH = 80;
        }
        if(availableBoardW < 80) {
            availableBoardW = 80;
        }

        layout.cell = minInt(availableBoardW / cols, availableBoardH / rows);
        layout.cell = clampInt(layout.cell, 12, 36);
        layout.boardW = layout.cell * cols;
        layout.boardH = layout.cell * rows;
        layout.contentW = maxInt(layout.boardW + framePad * 2, buttonsRowW + 12);
        if(layout.contentW > widgetW - layout.outerPad * 2) {
            layout.contentW = widgetW - layout.outerPad * 2;
        }
        layout.contentX = (widgetW - layout.contentW) / 2;
        if(layout.contentX < layout.outerPad) {
            layout.contentX = layout.outerPad;
        }

        layout.topPanelY = layout.outerPad;
        layout.topPanelH = 78;
        layout.boardPanelY = layout.topPanelY + layout.topPanelH + layout.sectionGap + layout.buttonH + 10;
        layout.boardPanelH = layout.boardH + framePad * 2;
        layout.boardX = layout.contentX + (layout.contentW - layout.boardW) / 2;
        layout.boardY = layout.boardPanelY + framePad;

        int counterW = layout.digitW * 3 + 10;
        int counterH = layout.digitH + 6;
        int statusY = layout.topPanelY + 10;
        layout.minesRect = makeRect(layout.contentX + 8, statusY, counterW, counterH);
        layout.timeRect = makeRect(layout.contentX + layout.contentW - counterW - 8, statusY, counterW, counterH);
        layout.faceRect = makeRect(layout.contentX + (layout.contentW - layout.faceSize) / 2,
                statusY + (counterH - layout.faceSize) / 2, layout.faceSize, layout.faceSize);

        int buttonsStartX = layout.contentX + (layout.contentW - buttonsRowW) / 2;
        int buttonsY = layout.topPanelY + layout.topPanelH + layout.sectionGap;
        for(int i = 0; i < LEVEL_COUNT; i++) {
            layout.difficultyRect[i] = makeRect(buttonsStartX + i * (layout.buttonW + 8),
                    buttonsY, layout.buttonW, layout.buttonH);
        }
        return layout;
    }

    void resetBoardStorage() {
        memset(board, 0, sizeof(board));
    }

    void applyDifficulty(DifficultyLevel level) {
        difficulty = level;
        rows = DIFFICULTIES[level].rows;
        cols = DIFFICULTIES[level].cols;
        minesTotal = DIFFICULTIES[level].mines;
    }

    void newGame() {
        resetBoardStorage();
        flagsUsed = 0;
        openedCount = 0;
        elapsedSec = 0;
        started = false;
        lost = false;
        won = false;
        facePressed = false;
        leftDown = false;
        pressedRow = -1;
        pressedCol = -1;
        pressedDifficulty = -1;
        pendingClick = false;
        pendingClickRow = -1;
        pendingClickCol = -1;
        pendingClickMs = 0;
        startMs = 0;
        update();
    }

    void clearPendingClick() {
        pendingClick = false;
        pendingClickRow = -1;
        pendingClickCol = -1;
        pendingClickMs = 0;
    }

    int countProtectedCells(int safeRow, int safeCol) const {
        int protectedCells = 0;
        for(int row = safeRow - 1; row <= safeRow + 1; row++) {
            for(int col = safeCol - 1; col <= safeCol + 1; col++) {
                if(inBoard(row, col)) {
                    protectedCells++;
                }
            }
        }
        return protectedCells;
    }

    void placeMines(int safeRow, int safeCol) {
        int placed = 0;
        int totalCells = rows * cols;
        bool protectNeighbors = (totalCells - countProtectedCells(safeRow, safeCol)) >= minesTotal;

        while(placed < minesTotal) {
            int pick = rand() % totalCells;
            int row = pick / cols;
            int col = pick % cols;
            if(board[row][col].mine) {
                continue;
            }
            if(row == safeRow && col == safeCol) {
                continue;
            }
            if(protectNeighbors &&
                    abs(row - safeRow) <= 1 &&
                    abs(col - safeCol) <= 1) {
                continue;
            }
            board[row][col].mine = true;
            placed++;
        }

        for(int row = 0; row < rows; row++) {
            for(int col = 0; col < cols; col++) {
                if(board[row][col].mine) {
                    board[row][col].adjacent = 0;
                    continue;
                }
                int count = 0;
                for(int nr = row - 1; nr <= row + 1; nr++) {
                    for(int nc = col - 1; nc <= col + 1; nc++) {
                        if((nr != row || nc != col) && inBoard(nr, nc) && board[nr][nc].mine) {
                            count++;
                        }
                    }
                }
                board[row][col].adjacent = (uint8_t)count;
            }
        }
    }

    void loseAt(int row, int col) {
        lost = true;
        board[row][col].exploded = true;
        for(int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                if(board[r][c].mine) {
                    board[r][c].open = true;
                }
                else if(board[r][c].flag) {
                    board[r][c].wrongFlag = true;
                }
            }
        }
    }

    void finishWin() {
        won = true;
        if(started) {
            uint64_t now = kernel_tic_ms(0);
            elapsedSec = (int)((now - startMs) / 1000);
            if(elapsedSec > 999) {
                elapsedSec = 999;
            }
        }
        flagsUsed = 0;
        for(int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                if(board[r][c].mine) {
                    if(!board[r][c].flag) {
                        board[r][c].flag = true;
                    }
                    flagsUsed++;
                }
            }
        }
    }

    void checkWin() {
        if(lost) {
            return;
        }
        if(openedCount == rows * cols - minesTotal) {
            finishWin();
        }
    }

    void floodOpen(int startRow, int startCol) {
        int queueRow[MAX_CELLS];
        int queueCol[MAX_CELLS];
        bool queued[MAX_ROWS][MAX_COLS];
        memset(queued, 0, sizeof(queued));

        int head = 0;
        int tail = 0;
        queueRow[tail] = startRow;
        queueCol[tail] = startCol;
        queued[startRow][startCol] = true;
        tail++;

        while(head < tail) {
            int row = queueRow[head];
            int col = queueCol[head];
            head++;

            Cell& cell = board[row][col];
            if(cell.open || cell.flag) {
                continue;
            }
            cell.open = true;
            openedCount++;

            if(cell.adjacent != 0) {
                continue;
            }

            for(int nr = row - 1; nr <= row + 1; nr++) {
                for(int nc = col - 1; nc <= col + 1; nc++) {
                    if(!inBoard(nr, nc) || queued[nr][nc]) {
                        continue;
                    }
                    if(board[nr][nc].mine || board[nr][nc].flag) {
                        continue;
                    }
                    queueRow[tail] = nr;
                    queueCol[tail] = nc;
                    queued[nr][nc] = true;
                    tail++;
                }
            }
        }
    }

    bool revealCell(int row, int col) {
        if(!inBoard(row, col)) {
            return false;
        }

        Cell& cell = board[row][col];
        if(cell.open || cell.flag || lost || won) {
            return false;
        }

        if(!started) {
            started = true;
            startMs = kernel_tic_ms(0);
            placeMines(row, col);
        }

        if(cell.mine) {
            loseAt(row, col);
            return true;
        }

        floodOpen(row, col);
        checkWin();
        return true;
    }

    bool toggleFlag(int row, int col) {
        if(!inBoard(row, col) || lost || won) {
            return false;
        }

        Cell& cell = board[row][col];
        if(cell.open) {
            return false;
        }

        cell.flag = !cell.flag;
        if(cell.flag) {
            flagsUsed++;
        }
        else {
            flagsUsed--;
        }
        return true;
    }

    int countNeighborFlags(int row, int col) const {
        int count = 0;
        for(int nr = row - 1; nr <= row + 1; nr++) {
            for(int nc = col - 1; nc <= col + 1; nc++) {
                if((nr != row || nc != col) && inBoard(nr, nc) && board[nr][nc].flag) {
                    count++;
                }
            }
        }
        return count;
    }

    bool chordCell(int row, int col) {
        if(!inBoard(row, col) || lost || won) {
            return false;
        }

        Cell& cell = board[row][col];
        if(!cell.open || cell.adjacent == 0) {
            return false;
        }
        if(countNeighborFlags(row, col) != cell.adjacent) {
            return false;
        }

        bool changed = false;
        for(int nr = row - 1; nr <= row + 1; nr++) {
            for(int nc = col - 1; nc <= col + 1; nc++) {
                if(!inBoard(nr, nc) || (nr == row && nc == col)) {
                    continue;
                }
                if(board[nr][nc].flag) {
                    continue;
                }
                if(revealCell(nr, nc)) {
                    changed = true;
                }
            }
        }
        return changed;
    }

    bool hitCell(const LayoutMetrics& layout, int x, int y, int& row, int& col) const {
        if(x < layout.boardX || y < layout.boardY) {
            return false;
        }
        if(x >= layout.boardX + layout.boardW || y >= layout.boardY + layout.boardH) {
            return false;
        }
        col = (x - layout.boardX) / layout.cell;
        row = (y - layout.boardY) / layout.cell;
        return inBoard(row, col);
    }

    int hitDifficulty(const LayoutMetrics& layout, int x, int y) const {
        for(int i = 0; i < LEVEL_COUNT; i++) {
            if(inRect(x, y, layout.difficultyRect[i])) {
                return i;
            }
        }
        return -1;
    }

    uint32_t getNumberColor(int number) const {
        static const uint32_t colors[9] = {
            0x00000000,
            0xFF0000FF,
            0xFF007B00,
            0xFFFF0000,
            0xFF00007B,
            0xFF7B0000,
            0xFF008080,
            0xFF000000,
            0xFF7B7B7B
        };
        if(number < 0 || number > 8) {
            return COLOR_TEXT;
        }
        return colors[number];
    }

    void drawCenteredText(graph_t* g, XTheme* theme, const grect_t& r,
            const char* text, int fontSize, uint32_t color) const {
        uint32_t tw, th;
        font_text_size(text, theme->getFont(), fontSize, &tw, &th);
        int tx = r.x + (r.w - (int)tw) / 2;
        int ty = r.y + (r.h - (int)th) / 2;
        graph_draw_text_font(g, tx, ty, text, theme->getFont(), fontSize, color);
    }

    void drawPanel(graph_t* g, const grect_t& r, bool sunken) const {
        graph_fill_3d(g, r.x, r.y, r.w, r.h, COLOR_PANEL, sunken);
    }

    void drawSevenSegment(graph_t* g, const grect_t& r, char ch) const {
        static const uint8_t digits[10] = {
            0x3F, 0x06, 0x5B, 0x4F, 0x66,
            0x6D, 0x7D, 0x07, 0x7F, 0x6F
        };
        uint8_t mask = 0;
        if(ch >= '0' && ch <= '9') {
            mask = digits[ch - '0'];
        }
        else if(ch == '-') {
            mask = 0x40;
        }

        int thick = maxInt(2, r.w / 5);
        int midY = r.y + r.h / 2 - thick / 2;
        int rightX = r.x + r.w - thick;
        int bottomY = r.y + r.h - thick;

        if(mask & 0x01) graph_fill_rect(g, r.x + thick, r.y, r.w - thick * 2, thick, COLOR_COUNTER_FG);
        if(mask & 0x02) graph_fill_rect(g, rightX, r.y + thick, thick, r.h / 2 - thick, COLOR_COUNTER_FG);
        if(mask & 0x04) graph_fill_rect(g, rightX, midY + thick, thick, r.h / 2 - thick * 2, COLOR_COUNTER_FG);
        if(mask & 0x08) graph_fill_rect(g, r.x + thick, bottomY, r.w - thick * 2, thick, COLOR_COUNTER_FG);
        if(mask & 0x10) graph_fill_rect(g, r.x, midY + thick, thick, r.h / 2 - thick * 2, COLOR_COUNTER_FG);
        if(mask & 0x20) graph_fill_rect(g, r.x, r.y + thick, thick, r.h / 2 - thick, COLOR_COUNTER_FG);
        if(mask & 0x40) graph_fill_rect(g, r.x + thick, midY, r.w - thick * 2, thick, COLOR_COUNTER_FG);
    }

    void drawCounter(graph_t* g, const grect_t& outer, int value, int digitW, int digitH) const {
        drawPanel(g, outer, true);

        grect_t inner = makeRect(outer.x + 2, outer.y + 2, outer.w - 4, outer.h - 4);
        graph_fill_rect(g, inner.x, inner.y, inner.w, inner.h, COLOR_COUNTER_BG);

        char chars[3];
        if(value < 0) {
            int absValue = -value;
            if(absValue > 99) {
                absValue = 99;
            }
            chars[0] = '-';
            chars[1] = (char)('0' + (absValue / 10) % 10);
            chars[2] = (char)('0' + absValue % 10);
        }
        else {
            if(value > 999) {
                value = 999;
            }
            chars[0] = (char)('0' + (value / 100) % 10);
            chars[1] = (char)('0' + (value / 10) % 10);
            chars[2] = (char)('0' + value % 10);
        }

        int gap = 1;
        int totalW = digitW * 3 + gap * 2;
        int startX = inner.x + (inner.w - totalW) / 2;
        int startY = inner.y + (inner.h - digitH) / 2;
        for(int i = 0; i < 3; i++) {
            grect_t dr = makeRect(startX + i * (digitW + gap), startY, digitW, digitH);
            drawSevenSegment(g, dr, chars[i]);
        }
    }

    void drawMine(graph_t* g, const grect_t& r) const {
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        int radius = maxInt(3, r.w / 5);
        graph_fill_circle(g, cx, cy, radius, COLOR_MINE);
        graph_line(g, cx - radius - 3, cy, cx + radius + 3, cy, COLOR_MINE);
        graph_line(g, cx, cy - radius - 3, cx, cy + radius + 3, COLOR_MINE);
        graph_line(g, cx - radius - 2, cy - radius - 2, cx + radius + 2, cy + radius + 2, COLOR_MINE);
        graph_line(g, cx - radius - 2, cy + radius + 2, cx + radius + 2, cy - radius - 2, COLOR_MINE);
        graph_fill_circle(g, cx - radius / 2, cy - radius / 2, maxInt(1, radius / 3), 0xFFFFFFFF);
    }

    void drawFlag(graph_t* g, const grect_t& r, bool wrong) const {
        int poleX = r.x + r.w / 2 - 1;
        int poleTop = r.y + maxInt(2, r.h / 5);
        int poleBottom = r.y + r.h - maxInt(3, r.h / 5);
        graph_fill_rect(g, poleX, poleTop, 2, poleBottom - poleTop, 0xFF202020);
        graph_line(g, poleX + 1, poleTop, poleX + r.w / 4, poleTop + r.h / 4, COLOR_FLAG);
        graph_line(g, poleX + r.w / 4, poleTop + r.h / 4, poleX + 1, poleTop + r.h / 2 - 1, COLOR_FLAG);
        graph_fill_rect(g, poleX + 1, poleTop + 1, maxInt(3, r.w / 4 - 1), maxInt(3, r.h / 4), COLOR_FLAG);
        graph_fill_rect(g, poleX - r.w / 4, poleBottom, r.w / 2, 2, 0xFF202020);
        if(wrong) {
            graph_line(g, r.x + 2, r.y + 2, r.x + r.w - 3, r.y + r.h - 3, 0xFF000000);
            graph_line(g, r.x + 2, r.y + r.h - 3, r.x + r.w - 3, r.y + 2, 0xFF000000);
        }
    }

    void drawFace(graph_t* g, const grect_t& r) const {
        graph_fill_3d(g, r.x, r.y, r.w, r.h, COLOR_PANEL, facePressed);
        int cx = r.x + r.w / 2;
        int cy = r.y + r.h / 2;
        int radius = r.w / 2 - 5;
        graph_fill_circle(g, cx, cy, radius, COLOR_FACE);
        graph_circle(g, cx, cy, radius, 1, 0xFF000000);

        if(lost) {
            graph_line(g, cx - radius / 2 - 2, cy - 4, cx - radius / 2 + 2, cy, 0xFF000000);
            graph_line(g, cx - radius / 2 - 2, cy, cx - radius / 2 + 2, cy - 4, 0xFF000000);
            graph_line(g, cx + radius / 2 - 2, cy - 4, cx + radius / 2 + 2, cy, 0xFF000000);
            graph_line(g, cx + radius / 2 - 2, cy, cx + radius / 2 + 2, cy - 4, 0xFF000000);
            graph_line(g, cx - radius / 2, cy + radius / 2, cx + radius / 2, cy + radius / 2, 0xFF000000);
        }
        else if(won) {
            graph_fill_rect(g, cx - radius / 2 - 2, cy - 5, 6, 3, 0xFF000000);
            graph_fill_rect(g, cx + radius / 2 - 4, cy - 5, 6, 3, 0xFF000000);
            graph_line(g, cx - radius / 2 + 4, cy - 4, cx + radius / 2 - 4, cy - 4, 0xFF000000);
            graph_line(g, cx - radius / 2, cy + radius / 3, cx - 1, cy + radius / 2, 0xFF000000);
            graph_line(g, cx - 1, cy + radius / 2, cx + radius / 2, cy + radius / 3, 0xFF000000);
        }
        else if(leftDown) {
            graph_fill_circle(g, cx - radius / 2, cy - 4, 2, 0xFF000000);
            graph_fill_circle(g, cx + radius / 2, cy - 4, 2, 0xFF000000);
            graph_circle(g, cx, cy + radius / 3, maxInt(2, radius / 3), 1, 0xFF000000);
        }
        else {
            graph_fill_circle(g, cx - radius / 2, cy - 4, 2, 0xFF000000);
            graph_fill_circle(g, cx + radius / 2, cy - 4, 2, 0xFF000000);
            graph_line(g, cx - radius / 2, cy + radius / 4, cx - 1, cy + radius / 2, 0xFF000000);
            graph_line(g, cx - 1, cy + radius / 2, cx + radius / 2, cy + radius / 4, 0xFF000000);
        }
    }

    void drawDifficultyButtons(graph_t* g, XTheme* theme, const LayoutMetrics& layout) const {
        for(int i = 0; i < LEVEL_COUNT; i++) {
            bool down = (difficulty == i) || (pressedDifficulty == i);
            drawPanel(g, layout.difficultyRect[i], down);
            drawCenteredText(g, theme, layout.difficultyRect[i], DIFFICULTIES[i].label,
                    layout.fontSize, COLOR_TEXT);
        }
    }

    void drawTopPanel(graph_t* g, XTheme* theme, const LayoutMetrics& layout) const {
        grect_t top = makeRect(layout.contentX, layout.topPanelY, layout.contentW, layout.topPanelH);
        drawPanel(g, top, true);

        drawCounter(g, layout.minesRect, minesTotal - flagsUsed, layout.digitW, layout.digitH);
        drawCounter(g, layout.timeRect, elapsedSec, layout.digitW, layout.digitH);
        drawFace(g, layout.faceRect);

        grect_t labelRect = makeRect(layout.contentX + 6, layout.topPanelY + layout.topPanelH - 22,
                layout.contentW - 12, 12);
        drawCenteredText(g, theme, labelRect, "R: restart   1/2/3: level", layout.hintFontSize, COLOR_HINT);
    }

    void drawNumberCell(graph_t* g, XTheme* theme, const grect_t& r, int number, int fontSize) const {
        char text[2];
        text[0] = (char)('0' + number);
        text[1] = '\0';
        uint32_t tw, th;
        font_text_size(text, theme->getFont(), fontSize, &tw, &th);
        int tx = r.x + (r.w - (int)tw) / 2;
        int ty = r.y + (r.h - (int)th) / 2 - 1;
        graph_draw_text_font(g, tx, ty, text, theme->getFont(), fontSize, getNumberColor(number));
    }

    void drawCell(graph_t* g, XTheme* theme, const LayoutMetrics& layout, int row, int col) const {
        const Cell& cell = board[row][col];
        grect_t r = makeRect(layout.boardX + col * layout.cell,
                layout.boardY + row * layout.cell, layout.cell, layout.cell);

        bool pressed = leftDown && row == pressedRow && col == pressedCol && !cell.open && !cell.flag;
        if(cell.open) {
            graph_fill_rect(g, r.x, r.y, r.w, r.h, cell.exploded ? COLOR_EXPLODED : COLOR_CELL_OPEN);
            graph_rect(g, r.x, r.y, r.w, r.h, 0xFF808080);
            if(cell.mine) {
                drawMine(g, r);
            }
            else if(cell.wrongFlag) {
                drawFlag(g, r, true);
            }
            else if(cell.adjacent > 0) {
                drawNumberCell(g, theme, r, cell.adjacent, maxInt(10, layout.cell * 2 / 3));
            }
        }
        else {
            drawPanel(g, r, pressed);
            if(cell.flag) {
                drawFlag(g, r, cell.wrongFlag);
            }
        }
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        (void)r;
        LayoutMetrics layout = getLayout();

        graph_fill_rect(g, 0, 0, area.w, area.h, COLOR_BG);
        drawTopPanel(g, theme, layout);
        drawDifficultyButtons(g, theme, layout);

        grect_t boardPanel = makeRect(layout.contentX, layout.boardPanelY, layout.contentW, layout.boardPanelH);
        drawPanel(g, boardPanel, true);

        graph_fill_rect(g, layout.boardX, layout.boardY, layout.boardW, layout.boardH, COLOR_CELL_OPEN);
        for(int row = 0; row < rows; row++) {
            for(int col = 0; col < cols; col++) {
                drawCell(g, theme, layout, row, col);
            }
        }

        if(area.h > layout.boardPanelY + layout.boardPanelH + 18) {
            grect_t hintRect = makeRect(layout.contentX, layout.boardPanelY + layout.boardPanelH + 6,
                    layout.contentW, 12);
            drawCenteredText(g, theme, hintRect,
                    "Click: flag   Double click: open",
                    layout.hintFontSize, COLOR_HINT);
        }
    }

    bool onMouse(xevent_t* ev) {
        LayoutMetrics layout = getLayout();
        gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
        int x = pos.x;
        int y = pos.y;
        int button = ev->value.mouse.button;
        int row = -1;
        int col = -1;
        bool overCell = hitCell(layout, x, y, row, col);

        if(ev->state == MOUSE_STATE_DOWN) {
            if(button == MOUSE_BUTTON_LEFT) {
                if(inRect(x, y, layout.faceRect)) {
                    facePressed = true;
                    update();
                    return true;
                }

                int diff = hitDifficulty(layout, x, y);
                if(diff >= 0) {
                    pressedDifficulty = diff;
                    update();
                    return true;
                }

                if(overCell && !lost && !won) {
                    leftDown = true;
                    pressedRow = row;
                    pressedCol = col;
                    update();
                    return true;
                }
            }
        }
        else if(ev->state == MOUSE_STATE_UP) {
            bool handled = false;

            if(button == MOUSE_BUTTON_LEFT) {
                if(facePressed) {
                    if(inRect(x, y, layout.faceRect)) {
                        newGame();
                    }
                    facePressed = false;
                    handled = true;
                }

                if(pressedDifficulty >= 0) {
                    int releaseDiff = hitDifficulty(layout, x, y);
                    if(releaseDiff == pressedDifficulty) {
                        applyDifficulty((DifficultyLevel)releaseDiff);
                        newGame();
                    }
                    pressedDifficulty = -1;
                    handled = true;
                }

                if(leftDown) {
                    leftDown = false;
                    pressedRow = -1;
                    pressedCol = -1;
                    handled = true;
                }
            }

            if(handled) {
                update();
            }
            return handled;
        }
        else if(ev->state == MOUSE_STATE_CLICK) {
            if(button == MOUSE_BUTTON_LEFT && overCell && !lost && !won) {
                uint64_t now = kernel_tic_ms(0);
                if(pendingClick &&
                        pendingClickRow == row &&
                        pendingClickCol == col &&
                        (now - pendingClickMs) <= DOUBLE_CLICK_MS) {
                    clearPendingClick();
                    if(board[row][col].open) {
                        chordCell(row, col);
                    }
                    else {
                        revealCell(row, col);
                    }
                }
                else {
                    pendingClick = true;
                    pendingClickRow = row;
                    pendingClickCol = col;
                    pendingClickMs = now;
                }
                update();
                return true;
            }
            else if(button == MOUSE_BUTTON_RIGHT && overCell && toggleFlag(row, col)) {
                clearPendingClick();
                update();
                return true;
            }
        }
        else if(ev->state == MOUSE_STATE_DRAG) {
            if(facePressed || leftDown || pressedDifficulty >= 0) {
                update();
                return true;
            }
        }

        return false;
    }

    bool onIM(xevent_t* ev) {
        if(ev->state != XIM_STATE_PRESS) {
            return false;
        }

        int key = ev->value.im.key_code;
        if(key == 'r' || key == 'R' || key == 'n' || key == 'N') {
            newGame();
            return true;
        }
        if(key == '1') {
            applyDifficulty(LEVEL_BEGINNER);
            newGame();
            return true;
        }
        if(key == '2') {
            applyDifficulty(LEVEL_INTERMEDIATE);
            newGame();
            return true;
        }
        if(key == '3') {
            applyDifficulty(LEVEL_EXPERT);
            newGame();
            return true;
        }
        return false;
    }

    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        (void)timerFPS;
        (void)timerSteps;
        if(pendingClick) {
            uint64_t now = kernel_tic_ms(0);
            if((now - pendingClickMs) > DOUBLE_CLICK_MS) {
                int row = pendingClickRow;
                int col = pendingClickCol;
                clearPendingClick();
                if(inBoard(row, col) && !lost && !won) {
                    if(toggleFlag(row, col)) {
                        update();
                        return;
                    }
                }
            }
        }
        if(started && !lost && !won) {
            int nextElapsed = (int)((kernel_tic_ms(0) - startMs) / 1000);
            if(nextElapsed > 999) {
                nextElapsed = 999;
            }
            if(nextElapsed != elapsedSec) {
                elapsedSec = nextElapsed;
                update();
            }
        }
    }

public:
    MineWidget() : Widget() {
        srand((unsigned int)time(NULL));
        difficulty = LEVEL_BEGINNER;
        applyDifficulty(difficulty);
        pendingClick = false;
        pendingClickRow = -1;
        pendingClickCol = -1;
        pendingClickMs = 0;
        newGame();
    }
};

class MineWin : public WidgetWin {
public:
    MineWin() : WidgetWin() {
        RootWidget* root = new RootWidget();
        setRoot(root);
        root->setType(Container::VERTICAL);

        MineWidget* mine = new MineWidget();
        root->add(mine);
        setTimer(TIMER_FPS);
    }
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    X x;
    MineWin win;

    grect_t wr;
    wr.x = 120;
    wr.y = 60;
    wr.w = DEFAULT_WIN_W;
    wr.h = DEFAULT_WIN_H;

    win.open(&x, -1, wr, "Mine", XWIN_STYLE_NORMAL, true);
    win.max();
    widgetXRun(&x, &win);
    return 0;
}
