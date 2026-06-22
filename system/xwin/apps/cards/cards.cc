#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Container.h>
#include <Widget/LabelButton.h>
#include <x++/X.h>
#include <graph/graph.h>
#include <graph/graph_ex.h>
#include <ewoksys/kernel_tic.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

using namespace Ewok;

// Color definitions inspired by classic Windows Solitaire
static const uint32_t COLOR_BG          = 0xFF008080; // Classic solitaire table green
static const uint32_t COLOR_CARD_BG     = 0xFFFFFFFF; // White card face
static const uint32_t COLOR_CARD_BACK   = 0xFF0000CC; // Blue card back
static const uint32_t COLOR_CARD_BACK_P = 0xFFFF0000; // Red pattern on card back
static const uint32_t COLOR_CARD_BORDER = 0xFF000000; // Black card border
static const uint32_t COLOR_RED         = 0xFFCC0000; // Red suit color
static const uint32_t COLOR_BLACK       = 0xFF000000; // Black suit color
static const uint32_t COLOR_SLOT        = 0xFF006666; // Empty slot color
static const uint32_t COLOR_SLOT_BORDER = 0xFF004D4D; // Empty slot border
static const uint32_t COLOR_HIGHLIGHT    = 0xFFFFFF00; // Yellow highlight
static const uint32_t COLOR_BTN_BG      = 0xFFD4D0C8; // Button gray
static const uint32_t COLOR_BTN_TEXT    = 0xFF000000; // Button text
static const uint32_t COLOR_TITLE_BAR  = 0xFF000080; // Dark blue title bar
static const uint32_t COLOR_TITLE_TEXT  = 0xFFFFFFFF; // White title text

// Base layout metrics scaled against the current window size
static const int BASE_CARD_W = 56;
static const int BASE_CARD_H = 80;
static const int BASE_CARD_OFFSET_OPEN  = 22; // Offset for face-up stacked cards
static const int BASE_CARD_OFFSET_CLOSE = 8;  // Offset for face-down stacked cards
static const int BASE_PILE_SPACING_X    = 12; // Column spacing
static const int BASE_PILE_SPACING_Y    = 16; // Row spacing
static const int BASE_HEADER_HEIGHT     = 28; // Fixed top status bar height
static const int BASE_TOP_PADDING       = BASE_HEADER_HEIGHT + 8; // Top row start Y
static const int BASE_LEFT_PADDING      = 12;
static const int BASE_TOP_ROW_HEIGHT    = BASE_CARD_H + BASE_PILE_SPACING_Y;
static const int BASE_BOTTOM_PADDING    = 20;
static const int MAX_TABLEAU_VISIBLE    = 19;
static const int AUTO_FINISH_TIMER_FPS  = 60;
static const int AUTO_FINISH_STEP_FPS   = 30;
static const int AUTO_FINISH_MOVE_MS    = 200;
static const int AUTO_FINISH_MIN_STEPS  = 6;
static const int AUTO_FINISH_MAX_STEPS  = 8;
static const int BASE_BOARD_W =
    BASE_LEFT_PADDING * 2 + 7 * BASE_CARD_W + 6 * BASE_PILE_SPACING_X;
static const int BASE_BOARD_H =
    BASE_TOP_PADDING + BASE_TOP_ROW_HEIGHT +
    MAX_TABLEAU_VISIBLE * BASE_CARD_OFFSET_OPEN + BASE_CARD_H + BASE_BOTTOM_PADDING;

// Card suits
enum Suit {
    SUIT_HEARTS   = 0,
    SUIT_DIAMONDS = 1,
    SUIT_CLUBS    = 2,
    SUIT_SPADES   = 3
};

static const int FOUNDATION_SLOT_SUITS[4] = {
    SUIT_HEARTS,
    SUIT_DIAMONDS,
    SUIT_CLUBS,
    SUIT_SPADES
};

// Card values: 1=A, 11=J, 12=Q, 13=K
typedef struct {
    int value;     // 1-13
    int suit;      // Suit
    bool faceUp;   // Whether the card is face up
} Card;

// Pile types
enum PileType {
    PILE_STOCK     = 0, // Stock pile
    PILE_WASTE     = 1, // Waste pile
    PILE_FOUNDATION = 2, // Foundation piles (4)
    PILE_TABLEAU    = 3  // Tableau piles (7)
};

// Card pile
typedef struct {
    PileType type;
    int x, y;       // Pile position
    Card cards[24]; // Up to 24 cards
    int count;
} Pile;

// Cards currently being dragged
typedef struct {
    Pile* srcPile;
    int srcIndex;          // Starting index in the source pile
    Card cards[13];        // Cards in the drag stack
    int count;
    int offsetX, offsetY; // Mouse offset from card origin
    int curX, curY;        // Current drag position
} DragInfo;

// Main card game widget
class CardsWidget : public Widget {
private:
    struct LayoutMetrics {
        int cardW;
        int cardH;
        int cardOffsetOpen;
        int cardOffsetClose;
        int pileSpacingX;
        int pileSpacingY;
        int headerHeight;
        int topPadding;
        int leftPadding;
        int topRowHeight;
        int boardLeft;
        int boardW;
        int boardH;
        int tableauBaseY;
        int infoFontSize;
        int cardFontSize;
        int foundationHintSize;
        int centerSuitSize;
        int smallSuitSize;
    };

    Pile stock;       // Stock pile
    Pile waste;       // Waste pile
    Pile foundation[4]; // Foundation piles
    Pile tableau[7];    // Tableau piles

    DragInfo drag;
    bool dragging;
    Pile* pressedPile;
    int pressedIndex;
    int pressedMouseX;
    int pressedMouseY;
    Pile* lastClickPile;
    int lastClickIndex;
    uint64_t lastClickMs;

    int score;
    int moves;
    bool won;
    uint64_t winStartTime;
    uint32_t winFrame;
    bool autoFinishing;

    struct AutoMoveAnimation {
        bool active;
        Card card;
        Pile* srcPile;
        Pile* dstPile;
        bool revealSrcTop;
        bool countStats;
        bool triggerAutoFinishAfterMove;
        int startX;
        int startY;
        int endX;
        int endY;
        int step;
        int steps;
    } autoMoveAnim;

    int clampInt(int value, int minValue, int maxValue) const {
        if(value < minValue) return minValue;
        if(value > maxValue) return maxValue;
        return value;
    }

    int scaleMetric(int baseValue, float scale, int minValue) const {
        int scaled = (int)(baseValue * scale + 0.5f);
        if(scaled < minValue) return minValue;
        return scaled;
    }

    float clampFloat(float value, float minValue, float maxValue) const {
        if(value < minValue) return minValue;
        if(value > maxValue) return maxValue;
        return value;
    }

    int getTableauPileHeight(const Pile& pile, int cardH, int cardOffsetOpen,
            int cardOffsetClose) const {
        int height = cardH;
        if(pile.count <= 0) {
            return height;
        }

        for(int i = 0; i < pile.count - 1; i++) {
            if(pile.cards[i].faceUp) {
                height += cardOffsetOpen;
            }
            else {
                height += cardOffsetClose;
            }
        }
        return height;
    }

    LayoutMetrics buildLayoutMetrics(float scale, int widgetW, int widgetH) const {
        (void)widgetH;

        LayoutMetrics layout;
        if(scale < 0.0f) {
            scale = 0.0f;
        }

        layout.cardW = scaleMetric(BASE_CARD_W, scale, 18);
        layout.cardH = scaleMetric(BASE_CARD_H, scale, 26);
        layout.cardOffsetOpen = scaleMetric(BASE_CARD_OFFSET_OPEN, scale, 6);
        layout.cardOffsetClose = scaleMetric(BASE_CARD_OFFSET_CLOSE, scale, 3);
        layout.pileSpacingX = scaleMetric(BASE_PILE_SPACING_X, scale, 4);
        layout.pileSpacingY = scaleMetric(BASE_PILE_SPACING_Y, scale, 6);
        layout.headerHeight = BASE_HEADER_HEIGHT;
        layout.leftPadding = scaleMetric(BASE_LEFT_PADDING, scale, 4);
        layout.topPadding = layout.headerHeight + scaleMetric(8, scale, 4);
        layout.topRowHeight = layout.cardH + layout.pileSpacingY;
        layout.boardW = layout.leftPadding * 2 +
            7 * layout.cardW + 6 * layout.pileSpacingX;
        layout.boardLeft = 0;
        if(widgetW > layout.boardW) {
            layout.boardLeft = (widgetW - layout.boardW) / 2;
        }

        layout.tableauBaseY = layout.topPadding + layout.topRowHeight;

        int maxTableauHeight = layout.cardH;
        for(int i = 0; i < 7; i++) {
            int pileHeight = getTableauPileHeight(tableau[i], layout.cardH,
                layout.cardOffsetOpen, layout.cardOffsetClose);
            if(pileHeight > maxTableauHeight) {
                maxTableauHeight = pileHeight;
            }
        }

        int bottomPadding = scaleMetric(BASE_BOTTOM_PADDING, scale, 8);
        layout.boardH = layout.tableauBaseY + maxTableauHeight + bottomPadding;
        layout.infoFontSize = 12;
        layout.cardFontSize = clampInt(scaleMetric(12, scale, 8), 8, 24);
        layout.foundationHintSize = scaleMetric(24, scale, 10);
        layout.centerSuitSize = scaleMetric(32, scale, 12);
        layout.smallSuitSize = scaleMetric(10, scale, 4);
        return layout;
    }

    bool layoutFits(const LayoutMetrics& layout, int widgetW, int widgetH) const {
        return layout.boardW <= widgetW && layout.boardH <= widgetH;
    }

    LayoutMetrics getLayoutMetrics() const {
        int widgetW = area.w > 0 ? area.w : BASE_BOARD_W;
        int widgetH = area.h > 0 ? area.h : BASE_BOARD_H;

        float low = 0.0f;
        float high = 1.0f;
        LayoutMetrics bestLayout = buildLayoutMetrics(low, widgetW, widgetH);
        if(!layoutFits(bestLayout, widgetW, widgetH)) {
            return bestLayout;
        }

        LayoutMetrics trialLayout = buildLayoutMetrics(high, widgetW, widgetH);
        while(layoutFits(trialLayout, widgetW, widgetH) && high < 8.0f) {
            low = high;
            bestLayout = trialLayout;
            high *= 2.0f;
            trialLayout = buildLayoutMetrics(high, widgetW, widgetH);
        }

        for(int i = 0; i < 24; i++) {
            float mid = (low + high) * 0.5f;
            LayoutMetrics midLayout = buildLayoutMetrics(mid, widgetW, widgetH);
            if(layoutFits(midLayout, widgetW, widgetH)) {
                low = mid;
                bestLayout = midLayout;
            }
            else {
                high = mid;
            }
        }

        return bestLayout;
    }

    void applyLayout() {
        LayoutMetrics layout = getLayoutMetrics();

        stock.type = PILE_STOCK;
        stock.x = layout.boardLeft + layout.leftPadding;
        stock.y = layout.topPadding;

        waste.type = PILE_WASTE;
        waste.x = layout.boardLeft + layout.leftPadding +
            (layout.cardW + layout.pileSpacingX);
        waste.y = layout.topPadding;

        for(int i = 0; i < 4; i++) {
            foundation[i].type = PILE_FOUNDATION;
            foundation[i].x = layout.boardLeft + layout.leftPadding +
                (3 + i) * (layout.cardW + layout.pileSpacingX);
            foundation[i].y = layout.topPadding;
        }

        for(int i = 0; i < 7; i++) {
            tableau[i].type = PILE_TABLEAU;
            tableau[i].x = layout.boardLeft + layout.leftPadding +
                i * (layout.cardW + layout.pileSpacingX);
            tableau[i].y = layout.tableauBaseY;
        }
    }

    grect_t getNewGameButtonRect(const LayoutMetrics& layout) const {
        grect_t rect;
        rect.w = scaleMetric(84, (float)layout.cardW / (float)BASE_CARD_W, 56);
        rect.h = clampInt(layout.headerHeight - 4, 12, layout.headerHeight - 2);
        rect.x = area.w - rect.w - scaleMetric(8, (float)layout.cardW / (float)BASE_CARD_W, 6);
        rect.y = (layout.headerHeight - rect.h) / 2;
        return rect;
    }

    bool inRect(int x, int y, const grect_t& rect) const {
        return x >= rect.x && x < (rect.x + rect.w) &&
               y >= rect.y && y < (rect.y + rect.h);
    }

    // Return the display character for a suit
    const char* getSuitChar(int suit) {
        switch(suit) {
            case SUIT_HEARTS:   return "H";
            case SUIT_DIAMONDS: return "D";
            case SUIT_CLUBS:    return "C";
            case SUIT_SPADES:   return "S";
            default: return "?";
        }
    }

    // Return the display string for a card value
    const char* getValueChar(int value) {
        switch(value) {
            case 1:  return "A";
            case 2:  return "2";
            case 3:  return "3";
            case 4:  return "4";
            case 5:  return "5";
            case 6:  return "6";
            case 7:  return "7";
            case 8:  return "8";
            case 9:  return "9";
            case 10: return "10";
            case 11: return "J";
            case 12: return "Q";
            case 13: return "K";
            default: return "?";
        }
    }

    // Check whether a suit is red
    bool isRed(int suit) {
        return suit == SUIT_HEARTS || suit == SUIT_DIAMONDS;
    }

    int getFoundationSuit(const Pile* pile) const {
        for(int i = 0; i < 4; i++) {
            if(&foundation[i] == pile) {
                return FOUNDATION_SLOT_SUITS[i];
            }
        }
        return -1;
    }

    // Initialize pile positions
    void initPiles() {
        stock.count = 0;
        waste.count = 0;
        for(int i = 0; i < 4; i++) {
            foundation[i].count = 0;
        }
        for(int i = 0; i < 7; i++) {
            tableau[i].count = 0;
        }
        applyLayout();
    }

    // Create and shuffle a deck
    void createDeck() {
        Card deck[52];
        for(int s = 0; s < 4; s++) {
            for(int v = 1; v <= 13; v++) {
                int idx = s * 13 + (v - 1);
                deck[idx].value = v;
                deck[idx].suit = s;
                deck[idx].faceUp = false;
            }
        }

        // Fisher-Yates shuffle
        for(int i = 51; i > 0; i--) {
            int j = rand() % (i + 1);
            Card tmp = deck[i];
            deck[i] = deck[j];
            deck[j] = tmp;
        }

        // Deal to the tableau: pile i gets i+1 cards, last card face up
        int idx = 0;
        for(int col = 0; col < 7; col++) {
            for(int row = 0; row <= col; row++) {
                Card c = deck[idx++];
                if(row == col) c.faceUp = true;
                tableau[col].cards[tableau[col].count++] = c;
            }
        }

        // Put the remaining cards into the stock pile
        while(idx < 52) {
            Card c = deck[idx++];
            c.faceUp = false;
            stock.cards[stock.count++] = c;
        }
    }

    // Start a new game
    void newGame() {
        initPiles();
        createDeck();
        score = 0;
        moves = 0;
        won = false;
        winStartTime = 0;
        winFrame = 0;
        autoFinishing = false;
        autoMoveAnim.active = false;
        autoMoveAnim.srcPile = NULL;
        autoMoveAnim.dstPile = NULL;
        autoMoveAnim.revealSrcTop = false;
        autoMoveAnim.countStats = false;
        autoMoveAnim.triggerAutoFinishAfterMove = false;
        autoMoveAnim.step = 0;
        autoMoveAnim.steps = 0;
        dragging = false;
        pressedPile = NULL;
        pressedIndex = -1;
        pressedMouseX = 0;
        pressedMouseY = 0;
        lastClickPile = NULL;
        lastClickIndex = -1;
        lastClickMs = 0;
        memset(&drag, 0, sizeof(drag));
        update();
    }

    // Draw one card from stock to waste
    void drawFromStock() {
        if(stock.count > 0) {
            Card c = stock.cards[--stock.count];
            c.faceUp = true;
            waste.cards[waste.count++] = c;
            moves++;
            requestAutoFinishGame();
            update();
        }
        else if(waste.count > 0) {
            // Move all waste cards back to the stock pile
            while(waste.count > 0) {
                Card c = waste.cards[--waste.count];
                c.faceUp = false;
                stock.cards[stock.count++] = c;
            }
            moves++;
            requestAutoFinishGame();
            update();
        }
    }

    // Get the actual position of the card at idx in the pile
    void getCardPos(Pile* p, int idx, int& cx, int& cy, const LayoutMetrics& layout) {
        cx = p->x;
        cy = p->y;
        if(p->type == PILE_TABLEAU) {
            // Tableau cards stack with offsets based on face-up state
            for(int i = 0; i < idx; i++) {
                if(p->cards[i].faceUp)
                    cy += layout.cardOffsetOpen;
                else
                    cy += layout.cardOffsetClose;
            }
        }
        else if(p->type == PILE_WASTE) {
            // Waste shows up to three spread cards
            int visible = p->count - idx;
            if(visible > 3) visible = 3;
            cx += (3 - visible) * scaleMetric(14, (float)layout.cardW / (float)BASE_CARD_W, 4);
        }
    }

    // Check whether a card can be placed onto the target pile
    bool canPlaceOn(Card c, Pile* dst) {
        if(dst->type == PILE_FOUNDATION) {
            // Foundation piles match the slot suit and build upward from Ace.
            int suit = getFoundationSuit(dst);
            if(dst->count == 0) {
                return c.value == 1 && c.suit == suit;
            }
            Card top = dst->cards[dst->count - 1];
            return top.suit == c.suit && top.value + 1 == c.value;
        }
        else if(dst->type == PILE_TABLEAU) {
            // Tableau piles alternate color and descend in value
            if(dst->count == 0)
                return c.value == 13; // Only a King can be placed on an empty pile
            Card top = dst->cards[dst->count - 1];
            if(!top.faceUp) return false;
            return isRed(top.suit) != isRed(c.suit) && top.value - 1 == c.value;
        }
        return false;
    }

    bool canPlaceOnFoundationSet(Card c, Pile* foundationSet, int foundationIndex) const {
        Pile* dst = &foundationSet[foundationIndex];
        int suit = FOUNDATION_SLOT_SUITS[foundationIndex];
        if(dst->count == 0) {
            return c.value == 1 && c.suit == suit;
        }
        Card top = dst->cards[dst->count - 1];
        return top.suit == c.suit && top.value + 1 == c.value;
    }

    // Check whether a run of cards can be dragged from srcIndex
    bool canDragFrom(Pile* srcPile, int srcIndex) {
        if(srcPile->type == PILE_STOCK) return false;
        if(srcPile->type == PILE_WASTE) {
            // Only the top waste card can be dragged
            return srcIndex == srcPile->count - 1;
        }
        if(srcPile->type == PILE_FOUNDATION) {
            // Only the top foundation card can be dragged
            return srcIndex == srcPile->count - 1;
        }
        // Tableau drag requires a face-up, valid descending sequence
        if(srcPile->type == PILE_TABLEAU) {
            if(!srcPile->cards[srcIndex].faceUp) return false;
            for(int i = srcIndex; i < srcPile->count - 1; i++) {
                Card a = srcPile->cards[i];
                Card b = srcPile->cards[i + 1];
                if(!b.faceUp) return false;
                if(isRed(a.suit) == isRed(b.suit)) return false;
                if(a.value - 1 != b.value) return false;
            }
            return true;
        }
        return false;
    }

    // Find the card under the mouse position
    bool findCardAt(int x, int y, Pile*& outPile, int& outIndex, const LayoutMetrics& layout) {
        // Search back to front because later draws appear on top
        // Check tableau piles first
        for(int i = 6; i >= 0; i--) {
            Pile* p = &tableau[i];
            for(int j = p->count - 1; j >= 0; j--) {
                int cx, cy;
                getCardPos(p, j, cx, cy, layout);
                if(x >= cx && x < cx + layout.cardW && y >= cy && y < cy + layout.cardH) {
                    outPile = p;
                    outIndex = j;
                    return true;
                }
            }
            // Check empty tableau slot hit area
            if(p->count == 0) {
                if(x >= p->x && x < p->x + layout.cardW &&
                   y >= p->y && y < p->y + layout.cardH) {
                    outPile = p;
                    outIndex = -1;
                    return true;
                }
            }
        }
        // Waste pile
        if(waste.count > 0) {
            int cx, cy;
            getCardPos(&waste, waste.count - 1, cx, cy, layout);
            if(x >= cx && x < cx + layout.cardW && y >= cy && y < cy + layout.cardH) {
                outPile = &waste;
                outIndex = waste.count - 1;
                return true;
            }
        }
        // Foundation piles
        for(int i = 3; i >= 0; i--) {
            Pile* p = &foundation[i];
            if(p->count > 0) {
                int cx, cy;
                getCardPos(p, p->count - 1, cx, cy, layout);
                if(x >= cx && x < cx + layout.cardW && y >= cy && y < cy + layout.cardH) {
                    outPile = p;
                    outIndex = p->count - 1;
                    return true;
                }
            }
            else {
                if(x >= p->x && x < p->x + layout.cardW &&
                   y >= p->y && y < p->y + layout.cardH) {
                    outPile = p;
                    outIndex = -1;
                    return true;
                }
            }
        }
        // Stock pile
        if(x >= stock.x && x < stock.x + layout.cardW &&
           y >= stock.y && y < stock.y + layout.cardH) {
            outPile = &stock;
            outIndex = stock.count - 1;
            return true;
        }
        return false;
    }

    // Check whether the game has been won
    void checkWin() {
        int total = 0;
        for(int i = 0; i < 4; i++) total += foundation[i].count;
        if(total == 52 && !won) {
            won = true;
            winFrame = 0;
        }
    }

    bool moveTopCardToFoundationSet(Pile* srcPile, int srcIndex, Pile* foundationSet,
            bool countStats) {
        if(srcPile == NULL || srcIndex != srcPile->count - 1) return false;
        Card c = srcPile->cards[srcIndex];
        for(int i = 0; i < 4; i++) {
            if(canPlaceOnFoundationSet(c, foundationSet, i)) {
                srcPile->count--;
                foundationSet[i].cards[foundationSet[i].count++] = c;
                if(srcPile->type == PILE_TABLEAU && srcPile->count > 0) {
                    srcPile->cards[srcPile->count - 1].faceUp = true;
                }
                if(countStats) {
                    score += 10;
                    moves++;
                }
                return true;
            }
        }
        return false;
    }

    bool allTableauFaceUp() {
        for(int i = 0; i < 7; i++) {
            for(int j = 0; j < tableau[i].count; j++) {
                if(!tableau[i].cards[j].faceUp) {
                    return false;
                }
            }
        }
        return true;
    }

    bool canAutoFinishGame() {
        if(won || stock.count > 0 || !allTableauFaceUp()) {
            return false;
        }

        Pile testWaste = waste;
        Pile testFoundation[4];
        Pile testTableau[7];
        memcpy(testFoundation, foundation, sizeof(testFoundation));
        memcpy(testTableau, tableau, sizeof(testTableau));

        bool moved = true;
        while(moved) {
            moved = false;
            if(testWaste.count > 0 &&
               moveTopCardToFoundationSet(&testWaste, testWaste.count - 1, testFoundation, false)) {
                moved = true;
            }

            for(int i = 0; i < 7; i++) {
                if(testTableau[i].count <= 0) {
                    continue;
                }
                if(moveTopCardToFoundationSet(&testTableau[i], testTableau[i].count - 1,
                        testFoundation, false)) {
                    moved = true;
                }
            }
        }

        int total = 0;
        for(int i = 0; i < 4; i++) {
            total += testFoundation[i].count;
        }
        return total == 52;
    }

    bool requestAutoFinishGame() {
        if(autoFinishing || autoMoveAnim.active || !canAutoFinishGame()) {
            return false;
        }
        autoFinishing = true;
        return true;
    }

    bool findFoundationDestination(Card c, Pile*& dstPile) {
        dstPile = NULL;
        for(int i = 0; i < 4; i++) {
            if(canPlaceOn(c, &foundation[i])) {
                dstPile = &foundation[i];
                return true;
            }
        }
        return false;
    }

    bool beginAnimatedFoundationMove(Pile* srcPile, int srcIndex, Pile* dstPile,
            bool countStats, bool triggerAutoFinishAfterMove, const LayoutMetrics& layout) {
        if(srcPile == NULL || dstPile == NULL || srcIndex < 0 ||
           srcIndex != srcPile->count - 1 || autoMoveAnim.active) {
            return false;
        }

        Card c = srcPile->cards[srcIndex];
        if(!canPlaceOn(c, dstPile)) {
            return false;
        }

        int startX, startY;
        getCardPos(srcPile, srcIndex, startX, startY, layout);
        autoMoveAnim.active = true;
        autoMoveAnim.card = c;
        autoMoveAnim.srcPile = srcPile;
        autoMoveAnim.dstPile = dstPile;
        autoMoveAnim.revealSrcTop = (srcPile->type == PILE_TABLEAU && srcIndex > 0);
        autoMoveAnim.countStats = countStats;
        autoMoveAnim.triggerAutoFinishAfterMove = triggerAutoFinishAfterMove;
        autoMoveAnim.startX = startX;
        autoMoveAnim.startY = startY;
        autoMoveAnim.endX = dstPile->x;
        autoMoveAnim.endY = dstPile->y;
        autoMoveAnim.step = 0;
        int dx = autoMoveAnim.endX - autoMoveAnim.startX;
        int dy = autoMoveAnim.endY - autoMoveAnim.startY;
        int distance = abs(dx) + abs(dy);
        int distanceSteps = distance / scaleMetric(36, (float)layout.cardW / (float)BASE_CARD_W, 12);
        // Keep the same step count as before, but drive the timer faster so
        // the animation completes in roughly half the time.
        int timeSteps = (AUTO_FINISH_MOVE_MS * AUTO_FINISH_STEP_FPS + 999) / 1000;
        autoMoveAnim.steps = clampInt(timeSteps + distanceSteps / 3,
            AUTO_FINISH_MIN_STEPS, AUTO_FINISH_MAX_STEPS);

        srcPile->count--;
        update();
        return true;
    }

    bool findNextAutoFinishMove(Pile*& srcPile, int& srcIndex, Pile*& dstPile) {
        srcPile = NULL;
        dstPile = NULL;
        srcIndex = -1;

        if(waste.count > 0) {
            Card c = waste.cards[waste.count - 1];
            if(findFoundationDestination(c, dstPile)) {
                srcPile = &waste;
                srcIndex = waste.count - 1;
                return true;
            }
        }

        for(int i = 0; i < 7; i++) {
            if(tableau[i].count <= 0) {
                continue;
            }
            int idx = tableau[i].count - 1;
            Card c = tableau[i].cards[idx];
            if(findFoundationDestination(c, dstPile)) {
                srcPile = &tableau[i];
                srcIndex = idx;
                return true;
            }
        }
        return false;
    }

    bool beginAutoFinishMove(const LayoutMetrics& layout) {
        if(!autoFinishing || autoMoveAnim.active) {
            return false;
        }

        Pile* srcPile = NULL;
        Pile* dstPile = NULL;
        int srcIndex = -1;
        if(!findNextAutoFinishMove(srcPile, srcIndex, dstPile)) {
            return false;
        }

        return beginAnimatedFoundationMove(srcPile, srcIndex, dstPile, false, false, layout);
    }

    void finishAutoFinishMove() {
        if(!autoMoveAnim.active) {
            return;
        }

        if(autoMoveAnim.revealSrcTop && autoMoveAnim.srcPile != NULL &&
           autoMoveAnim.srcPile->count > 0) {
            autoMoveAnim.srcPile->cards[autoMoveAnim.srcPile->count - 1].faceUp = true;
        }
        if(autoMoveAnim.dstPile != NULL) {
            autoMoveAnim.dstPile->cards[autoMoveAnim.dstPile->count++] = autoMoveAnim.card;
        }
        if(autoMoveAnim.countStats) {
            score += 10;
            moves++;
        }

        autoMoveAnim.active = false;
        autoMoveAnim.srcPile = NULL;
        autoMoveAnim.dstPile = NULL;
        autoMoveAnim.revealSrcTop = false;
        autoMoveAnim.countStats = false;
        bool triggerAutoFinishAfterMove = autoMoveAnim.triggerAutoFinishAfterMove;
        autoMoveAnim.triggerAutoFinishAfterMove = false;
        autoMoveAnim.step = 0;
        autoMoveAnim.steps = 0;

        checkWin();
        if(won) {
            autoFinishing = false;
        }
        else if(triggerAutoFinishAfterMove) {
            requestAutoFinishGame();
        }
        update();
    }

    // Try to auto-move a card to a foundation pile
    bool tryAutoMoveToFoundation(Pile* srcPile, int srcIndex) {
        if(srcPile == NULL || srcIndex < 0 || srcIndex != srcPile->count - 1) {
            return false;
        }
        Pile* dstPile = NULL;
        if(!findFoundationDestination(srcPile->cards[srcIndex], dstPile)) {
            return false;
        }
        LayoutMetrics layout = getLayoutMetrics();
        return beginAnimatedFoundationMove(srcPile, srcIndex, dstPile, true, true, layout);
    }

    bool beginDrag(Pile* p, int idx, int mouseDownX, int mouseDownY,
            int currentMouseX, int currentMouseY, const LayoutMetrics& layout) {
        if(p == NULL || idx < 0 || !canDragFrom(p, idx)) {
            return false;
        }

        dragging = true;
        drag.srcPile = p;
        drag.srcIndex = idx;
        drag.count = 0;
        for(int i = idx; i < p->count; i++) {
            drag.cards[drag.count++] = p->cards[i];
        }

        int cardX, cardY;
        getCardPos(p, idx, cardX, cardY, layout);
        drag.offsetX = mouseDownX - cardX;
        drag.offsetY = mouseDownY - cardY;
        drag.curX = currentMouseX - drag.offsetX;
        drag.curY = currentMouseY - drag.offsetY;

        // Only detach cards once dragging really starts, so double-click is not stolen by drag start.
        p->count = idx;
        update();
        return true;
    }

    void clearPendingPress() {
        pressedPile = NULL;
        pressedIndex = -1;
        pressedMouseX = 0;
        pressedMouseY = 0;
    }

    void clearPendingClick() {
        lastClickPile = NULL;
        lastClickIndex = -1;
        lastClickMs = 0;
    }

    // Double-click handler: try moving the card to foundation
    void onDoubleClick(Pile* p, int idx) {
        if(p == NULL || idx < 0 || p->type == PILE_STOCK || p->count <= 0) {
            return;
        }

        int topIndex = p->count - 1;
        if(p->type == PILE_TABLEAU && !p->cards[topIndex].faceUp) {
            return;
        }

        if(tryAutoMoveToFoundation(p, topIndex)) {
            update();
        }
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
        applyLayout();
        LayoutMetrics layout = getLayoutMetrics();
        grect_t newGameBtn = getNewGameButtonRect(layout);

        // Background
        graph_fill_rect(g, r.x, r.y, r.w, r.h, COLOR_BG);

        // Draw the top status bar so piles never overlap it
        graph_fill_rect(g, r.x, r.y, r.w, layout.headerHeight, COLOR_TITLE_BAR);
        // Bottom border of the status bar
        graph_line(g, r.x, r.y + layout.headerHeight - 1,
            r.x + r.w, r.y + layout.headerHeight - 1, 0xFF000000);

        // Draw the score and move count
        char info[64];
        snprintf(info, sizeof(info), "Score: %d   Moves: %d", score, moves);
        uint32_t tw, th;
        font_text_size(info, theme->getFont(), layout.infoFontSize, &tw, &th);
        graph_draw_text_font(g, r.x + 8, r.y + (layout.headerHeight - th) / 2, info,
            theme->getFont(), layout.infoFontSize, COLOR_TITLE_TEXT);

        // Draw the top restart button for mouse-friendly reset
        int btnRadius = scaleMetric(5, (float)layout.cardH / (float)BASE_CARD_H, 3);
        graph_fill_round(g, newGameBtn.x, newGameBtn.y, newGameBtn.w, newGameBtn.h,
            btnRadius, COLOR_BTN_BG);
        graph_round(g, newGameBtn.x, newGameBtn.y, newGameBtn.w, newGameBtn.h,
            btnRadius, 1, COLOR_CARD_BORDER);
        const char* restartText = "Restart";
        uint32_t rw, rh;
        font_text_size(restartText, theme->getFont(), layout.infoFontSize, &rw, &rh);
        graph_draw_text_font(g,
            newGameBtn.x + (newGameBtn.w - (int)rw) / 2,
            newGameBtn.y + (newGameBtn.h - (int)rh) / 2,
            restartText, theme->getFont(), layout.infoFontSize, COLOR_BTN_TEXT);

        // Draw shortcut hints beside the button
        const char* hint = "S:Stock  Dbl:Auto";
        uint32_t hw, hh;
        font_text_size(hint, theme->getFont(), layout.infoFontSize, &hw, &hh);
        int hintRight = newGameBtn.x - scaleMetric(10, (float)layout.cardW / (float)BASE_CARD_W, 6);
        graph_draw_text_font(g, hintRight - hw, r.y + (layout.headerHeight - hh) / 2, hint,
            theme->getFont(), layout.infoFontSize, COLOR_TITLE_TEXT);

        // Draw stock pile
        int pileLabelFont = clampInt(layout.cardFontSize - 1, 8, 14);
        drawPileLabel(g, stock.x, stock.y, layout.cardW, "Stock",
            theme, pileLabelFont, COLOR_TITLE_TEXT);
        drawPileSlot(g, stock.x, stock.y, stock.count == 0, layout);
        drawPile(g, &stock, theme, layout);

        // Draw waste pile
        drawPileLabel(g, waste.x, waste.y, layout.cardW, "Waste",
            theme, pileLabelFont, COLOR_TITLE_TEXT);
        drawPileSlot(g, waste.x, waste.y, waste.count == 0, layout);
        drawPile(g, &waste, theme, layout);

        // Draw foundation piles
        for(int i = 0; i < 4; i++) {
            int suit = FOUNDATION_SLOT_SUITS[i];
            uint32_t labelColor = isRed(suit) ? COLOR_RED : COLOR_TITLE_TEXT;
            drawPileLabel(g, foundation[i].x, foundation[i].y, layout.cardW,
                getSuitChar(suit), theme, pileLabelFont, labelColor);
            drawPileSlot(g, foundation[i].x, foundation[i].y, foundation[i].count == 0, layout);
            // Show suit hint in empty foundation slots.
            if(foundation[i].count == 0) {
                uint32_t col = isRed(suit) ? 0xFF884444 : 0xFF444444;
                drawSuit(g, foundation[i].x + layout.cardW / 2,
                    foundation[i].y + layout.cardH / 2,
                    layout.foundationHintSize, suit, col);
            }
            drawPile(g, &foundation[i], theme, layout);
        }

        // Draw tableau piles
        for(int i = 0; i < 7; i++) {
            drawPileSlot(g, tableau[i].x, tableau[i].y, tableau[i].count == 0, layout);
            drawPile(g, &tableau[i], theme, layout);
        }

        // Draw dragged cards on top of everything else
        if(dragging && drag.count > 0) {
            for(int i = 0; i < drag.count; i++) {
                int cx = drag.curX;
                int cy = drag.curY + i * layout.cardOffsetOpen;
                drawCard(g, cx, cy, drag.cards[i], theme, true, layout);
            }
        }
        else if(autoMoveAnim.active && autoMoveAnim.steps > 0) {
            float t = (float)autoMoveAnim.step / (float)autoMoveAnim.steps;
            t = clampFloat(t, 0.0f, 1.0f);
            // Smoothstep keeps the same total duration while making motion
            // accelerate and decelerate more naturally.
            float eased = t * t * (3.0f - 2.0f * t);
            int cx = autoMoveAnim.startX +
                (int)((autoMoveAnim.endX - autoMoveAnim.startX) * eased);
            int cy = autoMoveAnim.startY +
                (int)((autoMoveAnim.endY - autoMoveAnim.startY) * eased);
            drawCard(g, cx, cy, autoMoveAnim.card, theme, true, layout);
        }

        // Victory animation
        if(won) {
            int phase = (int)winFrame;

            // Make the top foundation cards float slightly
            for(int i = 0; i < 4; i++) {
                if(foundation[i].count > 0) {
                    int baseX, baseY;
                    getCardPos(&foundation[i], foundation[i].count - 1, baseX, baseY, layout);
                    // Give each card a different time-based float offset
                    int offsetY = ((phase + i * 3) % 7 - 3) * 2;
                    int offsetX = ((phase + i * 2) % 5 - 2) * 2;
                    // Redraw the top card with the floating offset
                    Card c = foundation[i].cards[foundation[i].count - 1];
                    drawCard(g, baseX + offsetX, baseY + offsetY, c, theme, true, layout);
                }
            }

            // Fireworks effect with colorful suit shapes at random positions
            for(int i = 0; i < 25; i++) {
                // Pseudo-random placement based on phase and index
                int seed = (int)(phase * 137 + i * 53) % 1000;
                int x = r.x + (seed % r.w);
                seed = (seed * 7 + i * 31) % 1000;
                int y = r.y + (seed % r.h);
                int sz = 8 + ((phase + i) % 14);
                int suit = (phase + i) % 4;
                uint32_t colors[4] = {0xFFCC0000, 0xFF00AA00, 0xFF0066FF, 0xFFFFAA00};
                drawSuit(g, x, y, sz, suit, colors[i % 4]);
            }

            // Blinking stars
            uint32_t starColor = (phase % 2 == 0) ? 0xFFFFFF00 : 0xFFFFFFFF;
            for(int i = 0; i < 15; i++) {
                int seed = (int)(phase * 31 + i * 17) % 1000;
                int x = r.x + (seed % r.w);
                seed = (seed * 11 + i * 23) % 1000;
                int y = r.y + (seed % r.h);
                graph_fill_circle(g, x, y, 2, starColor);
            }

            // Victory message box
            int winBoxW = scaleMetric(220, (float)layout.cardW / (float)BASE_CARD_W, 120);
            int winBoxH = scaleMetric(70, (float)layout.cardH / (float)BASE_CARD_H, 42);
            int winBoxR = scaleMetric(14, (float)layout.cardH / (float)BASE_CARD_H, 8);
            char win[64];
            snprintf(win, sizeof(win), "You Win!  Score: %d", score);
            uint32_t ww, wh;
            int winFontSize = clampInt(layout.infoFontSize + 2, 12, 28);
            font_text_size(win, theme->getFont(), winFontSize, &ww, &wh);
            const char* hint = "Press N for new game";
            font_text_size(hint, theme->getFont(), layout.infoFontSize, &hw, &hh);

            int winTextW = (int)ww;
            if((int)hw > winTextW) {
                winTextW = (int)hw;
            }
            int winBoxPadX = scaleMetric(20, (float)layout.cardW / (float)BASE_CARD_W, 12);
            int winBoxMarginX = scaleMetric(20, (float)layout.cardW / (float)BASE_CARD_W, 8);
            int desiredWinBoxW = winTextW + winBoxPadX * 2;
            if(desiredWinBoxW > winBoxW) {
                winBoxW = desiredWinBoxW;
            }
            int maxWinBoxW = r.w - winBoxMarginX * 2;
            if(maxWinBoxW > 0 && winBoxW > maxWinBoxW) {
                winBoxW = maxWinBoxW;
            }

            graph_fill_round(g, r.x + r.w/2 - winBoxW/2, r.y + r.h/2 - winBoxH/2,
                winBoxW, winBoxH, winBoxR, 0xFF000080);
            graph_round(g, r.x + r.w/2 - winBoxW/2, r.y + r.h/2 - winBoxH/2,
                winBoxW, winBoxH, winBoxR, 3, 0xFFFFFFFF);
            graph_draw_text_font(g, r.x + r.w/2 - ww/2, r.y + r.h/2 - wh,
                win, theme->getFont(), winFontSize, 0xFFFFFFFF);
            graph_draw_text_font(g, r.x + r.w/2 - hw/2, r.y + r.h/2 + 4,
                hint, theme->getFont(), layout.infoFontSize, 0xFFFFFF00);
        }
    }

    // Draw an empty pile slot
    void drawPileSlot(graph_t* g, int x, int y, bool empty, const LayoutMetrics& layout) {
        if(empty) {
            int radius = scaleMetric(6, (float)layout.cardH / (float)BASE_CARD_H, 2);
            graph_fill_round(g, x, y, layout.cardW, layout.cardH, radius, COLOR_SLOT);
            graph_round(g, x, y, layout.cardW, layout.cardH, radius, 2, COLOR_SLOT_BORDER);
        }
    }

    void drawPileLabel(graph_t* g, int x, int y, int width, const char* label,
            XTheme* theme, int fontSize, uint32_t color) {
        uint32_t tw, th;
        font_text_size(label, theme->getFont(), fontSize, &tw, &th);
        int labelX = x + (width - (int)tw) / 2;
        int labelY = y - (int)th - 1;
        if(labelY < 0) {
            labelY = 0;
        }
        graph_draw_text_font(g, labelX, labelY, label,
            theme->getFont(), fontSize, color);
    }

    // Draw all cards in a pile
    void drawPile(graph_t* g, Pile* p, XTheme* theme, const LayoutMetrics& layout) {
        for(int i = 0; i < p->count; i++) {
            int cx, cy;
            getCardPos(p, i, cx, cy, layout);
            // In tableau piles only the last card needs to be fully visible,
            // but drawing the full pile keeps the logic simple and is fast enough.
            drawCard(g, cx, cy, p->cards[i], theme, p->cards[i].faceUp, layout);
        }
    }

    // Draw a suit icon centered at (cx, cy) with the given total size
    void drawSuit(graph_t* g, int cx, int cy, int size, int suit, uint32_t color) {
        int r = size / 4;       // Circle radius
        int halfH = size / 2;   // Half height
        int halfW = size / 2;   // Half width
        
        switch(suit) {
            case SUIT_HEARTS: {  // Heart
                // Two circles at the top
                graph_fill_circle(g, cx - r, cy - r, r + 1, color);
                graph_fill_circle(g, cx + r, cy - r, r + 1, color);
                // Bottom inverted triangle from wide to narrow
                int triH = halfH + r;
                for(int i = 0; i < triH; i++) {
                    int lineW = (triH - i) * halfW / triH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy + i - r,
                               cx + lineW, cy + i - r, color);
                }
                break;
            }
            case SUIT_DIAMONDS: { // Diamond
                for(int i = -halfH + 1; i < halfH; i++) {
                    int lineW = halfW - abs(i) * halfW / halfH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy + i, cx + lineW, cy + i, color);
                }
                break;
            }
            case SUIT_CLUBS: {   // Club
                graph_fill_circle(g, cx, cy - r, r, color);
                graph_fill_circle(g, cx - r - 1, cy, r, color);
                graph_fill_circle(g, cx + r + 1, cy, r, color);
                // Bottom stem
                graph_fill_rect(g, cx - r/2, cy + r - 1, r, r + 2, color);
                break;
            }
            case SUIT_SPADES: {  // Spade
                // Two circles at the bottom
                graph_fill_circle(g, cx - r, cy + r, r + 1, color);
                graph_fill_circle(g, cx + r, cy + r, r + 1, color);
                // Top upright triangle from wide to narrow
                int triH = halfH + r;
                for(int i = 0; i < triH; i++) {
                    int lineW = (triH - i) * halfW / triH;
                    if(lineW <= 0) continue;
                    graph_line(g, cx - lineW, cy - i + r,
                               cx + lineW, cy - i + r, color);
                }
                // Small bottom stem
                graph_fill_rect(g, cx - r/3, cy + r + r - 1, r * 2 / 3, r + 2, color);
                break;
            }
        }
    }

    // Draw a single card
    void drawCard(graph_t* g, int x, int y, Card c, XTheme* theme,
                  bool faceUp, const LayoutMetrics& layout) {
        (void)theme;
        int radius = scaleMetric(4, (float)layout.cardH / (float)BASE_CARD_H, 2);
        if(faceUp) {
            // Card face background
            graph_fill_round(g, x, y, layout.cardW, layout.cardH, radius, COLOR_CARD_BG);
            graph_round(g, x, y, layout.cardW, layout.cardH, radius, 1, COLOR_CARD_BORDER);

            // Card color based on suit
            uint32_t col = isRed(c.suit) ? COLOR_RED : COLOR_BLACK;
            const char* vs = getValueChar(c.value);

            // Top-left corner: value + suit icon
            int textPadX = scaleMetric(3, (float)layout.cardW / (float)BASE_CARD_W, 2);
            int textPadY = scaleMetric(2, (float)layout.cardH / (float)BASE_CARD_H, 1);
            int iconGapX = scaleMetric(5, (float)layout.cardW / (float)BASE_CARD_W, 3);
            int iconGapY = scaleMetric(6, (float)layout.cardH / (float)BASE_CARD_H, 3);
            graph_draw_text_font(g, x + textPadX, y + textPadY, vs, theme->getFont(),
                layout.cardFontSize, col);
            // Small suit icon
            drawSuit(g, x + textPadX + iconGapX,
                     y + textPadY + layout.cardFontSize + iconGapY,
                     layout.smallSuitSize, c.suit, col);

            // Large center suit icon
            drawSuit(g, x + layout.cardW / 2, y + layout.cardH / 2,
                     layout.centerSuitSize, c.suit, col);

            // Bottom-right corner: value + suit icon
            uint32_t vw, vh;
            font_text_size(vs, theme->getFont(), layout.cardFontSize, &vw, &vh);
            graph_draw_text_font(g, x + layout.cardW - vw - textPadX,
                y + layout.cardH - vh - textPadY,
                vs, theme->getFont(), layout.cardFontSize, col);
            drawSuit(g, x + layout.cardW - textPadX - iconGapX,
                     y + layout.cardH - vh - textPadY - iconGapY,
                     layout.smallSuitSize, c.suit, col);
        }
        else {
            // Card back with a classic blue decorative pattern
            // Outer frame
            graph_fill_round(g, x, y, layout.cardW, layout.cardH, radius, COLOR_CARD_BACK);
            // Inner frame
            graph_round(g, x, y, layout.cardW, layout.cardH, radius, 1, COLOR_CARD_BG);
            // Inner decoration made from small rectangles
            int inset = scaleMetric(4, (float)layout.cardW / (float)BASE_CARD_W, 2);
            graph_fill_round(g, x + inset, y + inset,
                layout.cardW - inset * 2, layout.cardH - inset * 2,
                scaleMetric(2, (float)layout.cardH / (float)BASE_CARD_H, 1), COLOR_CARD_BG);
            graph_round(g, x + inset, y + inset,
                layout.cardW - inset * 2, layout.cardH - inset * 2,
                scaleMetric(2, (float)layout.cardH / (float)BASE_CARD_H, 1), 1, COLOR_CARD_BG);
            // Red accent outline
            graph_round(g, x, y, layout.cardW, layout.cardH, radius, 1, COLOR_CARD_BORDER);
            // Center decoration
            // Small red center marker
            int cx = x + layout.cardW / 2;
            int cy = y + layout.cardH / 2;
            int mark = scaleMetric(12, (float)layout.cardW / (float)BASE_CARD_W, 4);
            int markOffset = scaleMetric(6, (float)layout.cardW / (float)BASE_CARD_W, 2);
            int markTop = scaleMetric(18, (float)layout.cardH / (float)BASE_CARD_H, 6);
            int markRadius = scaleMetric(2, (float)layout.cardH / (float)BASE_CARD_H, 1);
            graph_fill_round(g, cx - markOffset, cy - markTop, mark, mark, markRadius, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx - markTop, cy - markOffset, mark, mark, markRadius, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx - markOffset, cy + markOffset, mark, mark, markRadius, COLOR_CARD_BACK_P);
            graph_fill_round(g, cx + markOffset, cy - markOffset, mark, mark, markRadius, COLOR_CARD_BACK_P);
            graph_fill_circle(g, cx, cy, scaleMetric(4, (float)layout.cardW / (float)BASE_CARD_W, 2), COLOR_CARD_BACK_P);
        }
    }

    bool onMouse(xevent_t* ev) {
        applyLayout();
        LayoutMetrics layout = getLayoutMetrics();
        grect_t newGameBtn = getNewGameButtonRect(layout);
        gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
        int x = pos.x;
        int y = pos.y;

        if(autoFinishing || autoMoveAnim.active) {
            if(ev->state == MOUSE_STATE_DOWN && inRect(x, y, newGameBtn)) {
                newGame();
                return true;
            }
            return true;
        }

        if(ev->state == MOUSE_STATE_DOWN) {
            clearPendingPress();
            if(inRect(x, y, newGameBtn)) {
                clearPendingClick();
                newGame();
                return true;
            }

            // Ignore clicks in the top status area except for buttons
            if(y < layout.topPadding) {
                return true;
            }

            // Handle stock pile click
            if(x >= stock.x && x < stock.x + layout.cardW &&
               y >= stock.y && y < stock.y + layout.cardH) {
                clearPendingClick();
                drawFromStock();
                return true;
            }

            // Find the clicked card
            Pile* p;
            int idx;
            if(findCardAt(x, y, p, idx, layout) && idx >= 0) {
                if(canDragFrom(p, idx)) {
                    pressedPile = p;
                    pressedIndex = idx;
                    pressedMouseX = x;
                    pressedMouseY = y;
                    return true;
                }
            }
        }
        else if(ev->state == MOUSE_STATE_DRAG) {
            if(dragging) {
                drag.curX = x - drag.offsetX;
                drag.curY = y - drag.offsetY;
                update();
                return true;
            }
            if(pressedPile != NULL &&
               beginDrag(pressedPile, pressedIndex, pressedMouseX, pressedMouseY,
                   x, y, layout)) {
                clearPendingClick();
                clearPendingPress();
                return true;
            }
        }
        else if(ev->state == MOUSE_STATE_UP) {
            if(dragging) {
                // Find the nearest valid drop target
                Pile* dstPile = NULL;
                int bestDist = 0x7FFFFFFF;
                Card c = drag.cards[0];

                // Check foundation piles
                for(int i = 0; i < 4; i++) {
                    int dx = foundation[i].x + layout.cardW / 2 - (drag.curX + layout.cardW / 2);
                    int dy = foundation[i].y + layout.cardH / 2 - (drag.curY + layout.cardH / 2);
                    int dist = dx * dx + dy * dy;
                    if(drag.count == 1 && dist < bestDist && canPlaceOn(c, &foundation[i])) {
                        bestDist = dist;
                        dstPile = &foundation[i];
                    }
                }

                // Check tableau piles
                for(int i = 0; i < 7; i++) {
                    int tx = tableau[i].x;
                    int ty = tableau[i].y;
                    if(tableau[i].count > 0) {
                        int cx, cy;
                        getCardPos(&tableau[i], tableau[i].count - 1, cx, cy, layout);
                        tx = cx;
                        ty = cy;
                    }
                    int dx = tx + layout.cardW / 2 - (drag.curX + layout.cardW / 2);
                    int dy = ty + layout.cardH / 2 - (drag.curY + layout.cardH / 2);
                    int dist = dx * dx + dy * dy;
                    if(dist < bestDist && canPlaceOn(c, &tableau[i])) {
                        bestDist = dist;
                        dstPile = &tableau[i];
                    }
                }

                if(dstPile != NULL) {
                    // Foundation piles accept exactly one card; tableau piles
                    // may accept a full dragged run.
                    int moveCount = drag.count;
                    if(dstPile->type == PILE_FOUNDATION) {
                        moveCount = 1;
                    }
                    for(int i = 0; i < moveCount; i++) {
                        dstPile->cards[dstPile->count++] = drag.cards[i];
                    }
                    // Reveal the next card in the source tableau pile
                    if(drag.srcPile->type == PILE_TABLEAU && drag.srcPile->count > 0) {
                        drag.srcPile->cards[drag.srcPile->count - 1].faceUp = true;
                    }
                    if(dstPile->type == PILE_FOUNDATION) score += 10;
                    moves++;
                    checkWin();
                    if(!won) {
                        requestAutoFinishGame();
                    }
                }
                else {
                    // Return cards to their original pile
                    for(int i = 0; i < drag.count; i++) {
                        drag.srcPile->cards[drag.srcPile->count++] = drag.cards[i];
                    }
                }

                dragging = false;
                drag.count = 0;
                clearPendingPress();
                clearPendingClick();
                update();
                return true;
            }

            if(pressedPile != NULL) {
                Pile* releasedPile = NULL;
                int releasedIndex = -1;
                bool sameCard = findCardAt(x, y, releasedPile, releasedIndex, layout) &&
                    releasedPile == pressedPile && releasedIndex == pressedIndex;
                Pile* clickedPile = pressedPile;
                int clickedIndex = pressedIndex;
                clearPendingPress();

                if(sameCard) {
                    uint64_t now = kernel_tic_ms(0);
                    if(lastClickPile == clickedPile &&
                       lastClickIndex == clickedIndex &&
                       (now - lastClickMs) <= 300) {
                        clearPendingClick();
                        onDoubleClick(clickedPile, clickedIndex);
                        return true;
                    }

                    lastClickPile = clickedPile;
                    lastClickIndex = clickedIndex;
                    lastClickMs = now;
                    return true;
                }
            }
        }
        return false;
    }

    bool onIM(xevent_t* ev) {
        if(ev->state == XIM_STATE_PRESS) {
            int key = ev->value.im.key_code;
            // N key: start a new game
            if(key == 'n' || key == 'N') {
                newGame();
                return true;
            }
            if(autoFinishing || autoMoveAnim.active) {
                return true;
            }
            // S key: draw from stock
            if(key == 's' || key == 'S') {
                drawFromStock();
                return true;
            }
        }
        return false;
    }

    // Timer callback for auto-finish moves and victory animation
    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        (void)timerFPS;
        (void)timerSteps;
        if(autoMoveAnim.active) {
            autoMoveAnim.step++;
            if(autoMoveAnim.step >= autoMoveAnim.steps) {
                finishAutoFinishMove();
            }
            else {
                update();
            }
            return;
        }
        if(autoFinishing) {
            LayoutMetrics layout = getLayoutMetrics();
            if(!beginAutoFinishMove(layout)) {
                autoFinishing = false;
                checkWin();
            }
            return;
        }
        if(won) {
            winFrame++;
            update();
        }
    }

    void onResize() {
        applyLayout();
        if(autoMoveAnim.active) {
            finishAutoFinishMove();
        }
        autoFinishing = false;
        if(dragging) {
            dragging = false;
            drag.count = 0;
        }
    }

public:
    CardsWidget() : Widget() {
        srand((unsigned int)time(NULL));
        dragging = false;
        pressedPile = NULL;
        pressedIndex = -1;
        pressedMouseX = 0;
        pressedMouseY = 0;
        lastClickPile = NULL;
        lastClickIndex = -1;
        lastClickMs = 0;
        memset(&drag, 0, sizeof(drag));
        score = 0;
        moves = 0;
        won = false;
        winFrame = 0;
        autoFinishing = false;
        autoMoveAnim.active = false;
        autoMoveAnim.srcPile = NULL;
        autoMoveAnim.dstPile = NULL;
        autoMoveAnim.revealSrcTop = false;
        autoMoveAnim.countStats = false;
        autoMoveAnim.triggerAutoFinishAfterMove = false;
        autoMoveAnim.step = 0;
        autoMoveAnim.steps = 0;
        newGame();
    }
};

class Cards : public WidgetWin {
public:
    Cards() : WidgetWin() {
        RootWidget* root = new RootWidget();
        setRoot(root);
        root->setType(Container::VERTICAL);

        CardsWidget* game = new CardsWidget();
        root->add(game);

        // Higher timer FPS keeps the same auto-finish duration but allows
        // smoother motion with more intermediate frames.
        setTimer(AUTO_FINISH_TIMER_FPS);
    }
};

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    X x;
    Cards win;

    grect_t wr;
    wr.x = 100;
    wr.y = 60;
    wr.w = BASE_BOARD_W;
    wr.h = BASE_BOARD_H;

    win.open(&x, -1, wr, "Cards", XWIN_STYLE_NORMAL, true);
    win.max();
    widgetXRun(&x, &win);
    return 0;
}
