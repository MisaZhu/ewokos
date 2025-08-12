#include <Widget/Slider.h>
#include <Widget/Scrollable.h>
#include <x++/XTheme.h>

namespace Ewok {

void Slider::drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
    graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, true);
}

void Slider::drawPos(graph_t* g, XTheme* theme, const grect_t& r) {
    graph_fill_3d(g, r.x, r.y, r.w, r.h, theme->basic.bgColor, false);
    graph_fill_3d(g, r.x+4, r.y+4, r.w-8, r.h-8, theme->basic.bgColor, true);
}

void Slider::onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
    drawBG(g, theme, r);
    grect_t pos_r;
    if(horizontal) {
        pos_r.h = r.h-2;
        pos_r.y = r.y+1;

        pos_r.w = r.h;
        if(pos_r.w < 4)
            pos_r.w = 4;

        pos_r.x = r.x + pos;
        if(pos_r.x >= (r.x+r.w-pos_r.w))
            pos_r.x = r.x+r.w-pos_r.w-1;
        if(pos_r.x < 0)
            pos_r.x = 0;
    }
    else {
        pos_r.w = r.w-2;
        pos_r.x = r.x+1;

        pos_r.h = r.w;
        if(pos_r.h < 4)
            pos_r.h = 4;

        pos_r.y = r.y + pos;
        if(pos_r.y >= (r.y+r.h-pos_r.h))
            pos_r.y = r.y+r.h-pos_r.h-1;
        if(pos_r.y < 0)
            pos_r.y = 0;
    }

    drawPos(g, theme, pos_r);
}

Slider::Slider(bool h) {
    range = 100;
    pos = 0;
    horizontal = h;
    widget = NULL;
    isDragging = false;
    lastMousePos = 0;
}

void Slider::setScrollable(Scrollable* widget) {
    this->widget = widget;
}

void Slider::setRange(uint32_t range) {
    if(range == 0)
        range = 100;

    if(pos >= range)
        pos = range - 1;

    this->range = range;
    update();
}

uint32_t Slider::getValue() {
    uint32_t max = horizontal? area.w - area.h: area.h - area.w;
    if(max == 0)
        return 0;
    uint32_t v = (pos * range) / max;
    return v; // 计算当前值并返回
}

void Slider::setPos(uint32_t pos) {
    this->pos = pos;
    onPosChange();
    update();
}

void Slider::setValue(uint32_t value) {
    uint32_t max = horizontal? area.w - area.h: area.h - area.w;
    if(max == 0)
        return;   
    uint32_t newPos = (value * max) / range;
    if(newPos >= max)
        newPos = max - 1; 
    setPos(newPos);
}

bool Slider::onMouse(xevent_t* ev) {
    gpos_t pos = getInsidePos(ev->value.mouse.x, ev->value.mouse.y);
    grect_t handleRect;

    if(horizontal) {
        handleRect.h = area.h - 2;
        handleRect.y = 0;
        handleRect.w = area.h;
        if(handleRect.w < 4)
            handleRect.w = 4;
        handleRect.x = this->pos;
    } else {
        handleRect.w = area.w - 2;
        handleRect.x = 0;
        handleRect.h = area.w;
        if(handleRect.h < 4)
            handleRect.h = 4;
        handleRect.y = this->pos;
    }

    bool ret = false;
    switch (ev->state) {
        case MOUSE_STATE_DOWN:
            if (pos.x >= handleRect.x && pos.x < (handleRect.x + handleRect.w) &&
                pos.y >= handleRect.y && pos.y < (handleRect.y + handleRect.h)) {
                isDragging = true;
                lastMousePos = horizontal ? pos.x : pos.y;
                lastMouseOffset = horizontal ? pos.x - handleRect.x : pos.y - handleRect.y;
            }
            ret = true;
            break;
        case MOUSE_STATE_UP:
            isDragging = false;
            break;
        case MOUSE_STATE_DRAG:
            if (isDragging) {
                int delta = (horizontal ? pos.x : pos.y) - lastMousePos;
                int32_t newPos = lastMousePos + delta - lastMouseOffset;
                uint32_t max = horizontal? area.w - area.h: area.h - area.w;
                if(newPos < 0)
                    newPos = 0;
                if(newPos >= max)
                    newPos = max - 1;
                setPos(newPos);
            }
            ret = true;
            break;
    }
    return ret;
}
}